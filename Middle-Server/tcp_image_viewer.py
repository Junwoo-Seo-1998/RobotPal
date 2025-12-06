import socket
import struct
import cv2
import numpy as np
import time
import os

HOST = '0.0.0.0'
PORT = 9998  

def main():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((HOST, PORT))
    server.listen(1)
    print(f">>> [TCP] Native Image Server listening on port {PORT}")

    conn, addr = server.accept()
    print(f"[TCP] Connected by {addr}")

    try:
        data_buffer = b""
        payload_size = struct.calcsize("<I")   # Little-endian 4바이트
        frame_index = 0

        while True:
            # 1) 패킷 길이 먼저 받기
            while len(data_buffer) < payload_size:
                packet = conn.recv(4096)
                if not packet:
                    break
                data_buffer += packet

            if len(data_buffer) < payload_size:
                break

            packed_msg_size = data_buffer[:payload_size]
            data_buffer = data_buffer[payload_size:]
            msg_size = struct.unpack("<I", packed_msg_size)[0]

            # 2) JPEG 데이터 수신
            while len(data_buffer) < msg_size:
                packet = conn.recv(4096)
                if not packet:
                    break
                data_buffer += packet

            frame_data = data_buffer[:msg_size]
            data_buffer = data_buffer[msg_size:]

            print(f"[DEBUG] Recv JPEG size: {len(frame_data)} bytes")

            # 4) OpenCV 디코딩
            npbuf = np.frombuffer(frame_data, np.uint8)
            frame = cv2.imdecode(npbuf, cv2.IMREAD_COLOR)
            # 뒤집기
            frame = cv2.flip(frame, 0)

            print(frame is None)
            if frame is None:
                print(f"[ERROR] OpenCV failed to decode JPEG (Frame {frame_index}) ❌")
            else:
                print(f"[DEBUG] Decode OK - Shape: {frame.shape}, Mean pixel: {frame.mean():.2f}")

                # 검은 화면 검사
                if frame.mean() < 3:
                    print("[WARN] Frame is almost black → 데이터가 깨졌거나 RGBA → RGB mismatch 가능")

                cv2.imshow("TCP Stream Debug", frame)
                if cv2.waitKey(1) & 0xFF == ord("q"):
                    break

            frame_index += 1

    except Exception as e:
        print(f"[EXCEPTION] {e}")

    finally:
        conn.close()
        server.close()
        cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
