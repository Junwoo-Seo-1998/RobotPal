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
                
                nparr = np.frombuffer(message, np.uint8)
                frame = cv2.imdecode(nparr, cv2.IMREAD_COLOR)

                if frame is not None:
                    # Flip the frame only horizontally (around Y-axis)
                    flipped_frame = cv2.flip(frame, 0)
                    
                    cv2.imshow("Stream from ImGui Client", flipped_frame)

                    if cv2.waitKey(1) & 0xFF == ord('q'):
                        cv2.destroyAllWindows()
                else:
                    print(" 이미지 디코딩 실패")

           
            else:
                
                print(f" 텍스트 수신: {message}")
                await websocket.send(f"Server received: {message}")

    except websockets.exceptions.ConnectionClosed:
        print(" 클라이언트 연결이 종료되었습니다.")
    except Exception as e:
        print(f" 에러 발생: {e}")
    finally:
        cv2.destroyAllWindows()

async def main():
    print(f" 서버 시작됨 (ws://localhost:{PORT})")
    print("   - 클라이언트 접속 대기 중...")

    async with websockets.serve(handle_client, "0.0.0.0", PORT):
        await asyncio.Future() 

if __name__ == "__main__":

    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n 서버를 종료합니다.")