import asyncio
import cv2
import numpy as np
import struct # 데이터 길이를 해석하기 위해 필요

PORT = 9999

async def handle_client(reader, writer):
    # 접속한 클라이언트 주소 확인
    addr = writer.get_extra_info('peername')
    print(f"클라이언트가 연결되었습니다. ({addr})")

    try:
        while True:
            # 1. [헤더 수신] 데이터 길이 (4바이트 int) 읽기
            # TCP는 경계가 없으므로, C++ 클라이언트가 먼저 '보낼 데이터 크기'를 4바이트로 보내줘야 합니다.
            size_data = await reader.read(4)
            if not size_data:
                break # 연결이 끊김

            # 받은 4바이트를 정수(int)로 변환 (Little Endian 기준: '<L')
            # C++에서 보낼 때의 엔디안 설정과 맞춰야 합니다.
            msg_size = struct.unpack('<L', size_data)[0]

            # 2. [바디 수신] 실제 이미지 데이터 읽기
            # msg_size만큼의 데이터를 확실하게 다 읽을 때까지 기다립니다.
            try:
                data = await reader.readexactly(msg_size)
            except asyncio.IncompleteReadError:
                break # 데이터를 다 읽기 전에 연결이 끊김

            # 3. 이미지 디코딩 및 출력 (기존 로직과 동일)
            nparr = np.frombuffer(data, np.uint8)
            frame = cv2.imdecode(nparr, cv2.IMREAD_COLOR)

            if frame is not None:
                # Flip the frame (상하 반전 유지)
                flipped_frame = cv2.flip(frame, 0)
                
                cv2.imshow("Stream from TCP Client", flipped_frame)

                if cv2.waitKey(1) & 0xFF == ord('q'):
                    break
            else:
                print("이미지 디코딩 실패")

    except Exception as e:
        print(f"에러 발생: {e}")
    finally:
        print("클라이언트 연결이 종료되었습니다.")
        writer.close()
        await writer.wait_closed()
        cv2.destroyAllWindows()

async def main():
    # websockets.serve 대신 asyncio.start_server 사용
    server = await asyncio.start_server(handle_client, '0.0.0.0', PORT)
    
    addr = server.sockets[0].getsockname()
    print(f"TCP 서버 시작됨 ({addr})")
    print(" - 클라이언트 접속 대기 중...")

    async with server:
        await server.serve_forever()

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n서버를 종료합니다.")