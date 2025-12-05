import asyncio
import websockets
import cv2
import numpy as np

PORT = 9999

async def handle_client(websocket):
    print(f"클라이언트가 연결되었습니다. ({websocket.remote_address})")
    try:
        async for message in websocket:
            if isinstance(message, bytes):
                # [수정] 앞 4바이트(길이 정보)를 건너뛰고 이미지 디코딩
                # StreamingManager가 보낼 때 길이를 포함하기 때문입니다.
                if len(message) > 4:
                    jpg_data = message[4:] 
                    nparr = np.frombuffer(jpg_data, np.uint8)
                    frame = cv2.imdecode(nparr, cv2.IMREAD_COLOR)

                    if frame is not None:
                        flipped_frame = cv2.flip(frame, 0)
                        cv2.imshow("Web WebSocket Stream (Port 9999)", flipped_frame)
                        if cv2.waitKey(1) & 0xFF == ord('q'):
                            break
            else:
                print(f" 텍스트 수신: {message}")

    except Exception as e:
        print(f" 연결 종료 또는 에러: {e}")
    finally:
        cv2.destroyAllWindows()

async def main():
    print(f" 서버 시작됨 (ws://0.0.0.0:{PORT})")
    async with websockets.serve(handle_client, "0.0.0.0", PORT):
        await asyncio.Future()

if __name__ == "__main__":
    asyncio.run(main())