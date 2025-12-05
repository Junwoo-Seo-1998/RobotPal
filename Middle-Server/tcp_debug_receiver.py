import socket
import struct

HOST = '0.0.0.0'
PORT = 9998 # Native 클라이언트 포트

def main():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind((HOST, PORT))
    server.listen(1)
    print(f"Waiting for connection on {PORT}...")
    
    conn, addr = server.accept()
    print(f"Connected: {addr}")
    
    try:
        # 1. 처음 4바이트(헤더) 확인
        header = conn.recv(4)
        if len(header) < 4:
            print("Error: 헤더조차 오지 않음")
            return
            
        # Big Endian Unsigned Int로 디코딩
        msg_len = struct.unpack(">I", header)[0]
        print(f"1. Header Received: {header.hex()} -> Length: {msg_len} bytes")
        
        if msg_len > 10000000: # 10MB 이상이면 헤더 파싱 오류일 가능성 높음
            print("Warning: 데이터 길이가 비정상적으로 큽니다. 엔디안 문제거나 잘못된 프로토콜입니다.")
            
        # 2. 데이터 일부만 읽어서 JPEG 헤더 확인
        first_chunk = conn.recv(10)
        print(f"2. Data Start (Hex): {first_chunk.hex()}")
        
        # JPEG 매직 넘버 확인 (FF D8 ...)
        if first_chunk.startswith(b'\xff\xd8'):
            print(">> SUCCESS: JPEG 헤더가 확인되었습니다. (데이터 정상)")
        else:
            print(">> FAIL: JPEG 헤더가 아닙니다. (다른 데이터가 섞였거나 정렬 문제)")

    finally:
        conn.close()
        server.close()

if __name__ == "__main__":
    main()