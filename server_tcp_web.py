import asyncio
import websockets
import cv2
import numpy as np
import struct

PORT_WEB = 9999
PORT_TCP = 9998

queue_web = asyncio.Queue(maxsize=1)
queue_tcp = asyncio.Queue(maxsize=1)


async def handle_websocket_client(websocket):
    print(f"[WebSocket] 클라이언트 연결됨 ({websocket.remote_address})")
    try:
        async for message in websocket:
            if isinstance(message, bytes):
                nparr = np.frombuffer(message, np.uint8)
                frame = cv2.imdecode(nparr, cv2.IMREAD_COLOR)

                if frame is not None:
                    
                    flipped_frame = cv2.flip(frame, 0)
                    
                    if queue_web.full():
                        try: queue_web.get_nowait()
                        except: pass
                    await queue_web.put(flipped_frame)
                else:
                    print("[WebSocket] 이미지 디코딩 실패")
            else:
                print(f"[WebSocket] 텍스트 수신: {message}")
                await websocket.send(f"Server received: {message}")

    except websockets.exceptions.ConnectionClosed:
        print("[WebSocket] 연결 종료")
    except Exception as e:
        print(f"[WebSocket] 에러: {e}")

async def handle_tcp_client(reader, writer):
    addr = writer.get_extra_info('peername')
    print(f"[TCP] 클라이언트 연결됨 ({addr})")

    try:
        while True:

            size_data = await reader.read(4)
            if not size_data:
                break 

            msg_size = struct.unpack('<L', size_data)[0]

            try:
                data = await reader.readexactly(msg_size)
            except asyncio.IncompleteReadError:
                break

            nparr = np.frombuffer(data, np.uint8)
            frame = cv2.imdecode(nparr, cv2.IMREAD_COLOR)

            

            if frame is not None:

                flipped_frame = cv2.flip(frame, 0)

                if queue_tcp.full():
                    try: queue_tcp.get_nowait()
                    except: pass
                await queue_tcp.put(flipped_frame)
            else:
                print("[TCP] 이미지 디코딩 실패")

    except Exception as e:
        print(f"[TCP] 에러: {e}")
    finally:
        print(f"[TCP] 연결 종료 ({addr})")
        writer.close()
        await writer.wait_closed()

async def display_manager():
    print(">> 디스플레이 매니저 시작 (OpenCV 창 관리)")
    
    while True:
        if not queue_web.empty():
            frame_web = await queue_web.get()
            cv2.imshow("Stream from ImGui Client (WebSocket)", frame_web)


        if not queue_tcp.empty():
            frame_tcp = await queue_tcp.get()
            cv2.imshow("Stream from TCP Client", frame_tcp)

        if cv2.waitKey(1) & 0xFF == ord('q'):
            print(">> 'q' 키 입력됨. 종료합니다.")

            cv2.destroyAllWindows()
            break

        await asyncio.sleep(0.01)

async def main():
    print(f"=== 통합 스트리밍 서버 시작 ===")
    print(f"1. WebSocket 서버 대기 중: ws://0.0.0.0:{PORT_WEB}")
    print(f"2. TCP 서버 대기 중: 0.0.0.0:{PORT_TCP}")
    print("=================================")

    # 1. WebSocket 서버 시작
    server_ws = await websockets.serve(handle_websocket_client, "0.0.0.0", PORT_WEB)
    
    # 2. TCP 서버 시작
    server_tcp = await asyncio.start_server(handle_tcp_client, '0.0.0.0', PORT_TCP)

    # 3. 디스플레이 매니저 실행 (Concurrent Task)
    display_task = asyncio.create_task(display_manager())

    async with server_tcp:
        # gather를 통해 WS, TCP(forever), Display가 동시에 돌게 함
        await asyncio.gather(
            server_tcp.serve_forever(),
            server_ws.wait_closed(), 
            display_task
        )

if __name__ == "__main__":
    try:
        
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n서버를 종료합니다.")
    finally:
        cv2.destroyAllWindows()