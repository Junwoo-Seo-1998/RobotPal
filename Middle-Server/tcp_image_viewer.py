import socket
import struct
import cv2
import numpy as np


HOST = '0.0.0.0'
PORT = 9998  

def main():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # 포트 재사용 옵션 (서버 재실행 시 에러 방지)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((HOST, PORT))
    server.listen(1)
    print(f">>> [TCP] Native Image Server listening on port {PORT}")

    conn, addr = server.accept()
    print(f"[TCP] Connected by {addr}")

    try:
        data_buffer = b""
        payload_size = struct.calcsize(">I") # 4 bytes (Big Endian size header)

        while True:
            # 1. 패킷 길이 헤더(4바이트) 수신
            while len(data_buffer) < payload_size:
                packet = conn.recv(4096)
                if not packet: break
                data_buffer += packet
            
            if len(data_buffer) < payload_size: break

            packed_msg_size = data_buffer[:payload_size]
            data_buffer = data_buffer[payload_size:]
            msg_size = struct.unpack(">I", packed_msg_size)[0]

            # 2. 실제 이미지 데이터 수신
            while len(data_buffer) < msg_size:
                data_buffer += conn.recv(4096)

            frame_data = data_buffer[:msg_size]
            data_buffer = data_buffer[msg_size:]

            # 3. 디코딩 및 출력
            nparr = np.frombuffer(frame_data, np.uint8)
            frame = cv2.imdecode(nparr, cv2.IMREAD_COLOR)

            frame = cv2.flip(frame,0)
            if frame is not None:
                cv2.imshow('Native TCP Stream (Port 9998)', frame)
                if cv2.waitKey(1) & 0xFF == ord('q'):
                    break
    except Exception as e:
        print(f"Error: {e}")
    finally:
        conn.close()
        server.close()
        cv2.destroyAllWindows()

if __name__ == "__main__":
    main()