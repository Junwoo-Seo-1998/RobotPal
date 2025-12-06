import asyncio
import websockets
import socket
import threading

# ---------------------------------------------------------
# [설정] 스트리밍 전용 포트
# Control Bridge(12345/5555)와 겹치지 않게 설정
# ---------------------------------------------------------
TCP_PORT = 12346       # C++(Robot) -> 여기로 이미지 전송
WEBSOCKET_PORT = 5556  # Web Browser/Viewer -> 여기서 이미지 수신

# 연결된 클라이언트 관리
tcp_clients = set()
ws_clients = set()

# ---------------------------------------------------------
# 브로드캐스트 (바이너리 모드)
# ---------------------------------------------------------
async def broadcast_to_ws(data, sender_ws=None):
    if not ws_clients: return
    for ws in list(ws_clients):
        if ws != sender_ws:
            try:
                # bytes를 보내면 자동으로 바이너리 프레임 전송
                await ws.send(data) 
            except:
                pass

def broadcast_to_tcp(data, sender_socket=None):
    if not tcp_clients: return
    for sock in list(tcp_clients):
        if sock != sender_socket:
            try:
                sock.sendall(data) 
            except:
                pass

# ---------------------------------------------------------
# 웹소켓 핸들러
# ---------------------------------------------------------
async def ws_handler(websocket):
    print(f"[Stream-Web] 접속: {websocket.remote_address}")
    ws_clients.add(websocket)
    try:
        async for message in websocket:
            await broadcast_to_ws(message, websocket)
            broadcast_to_tcp(message)
    except Exception:
        pass
    finally:
        ws_clients.remove(websocket)
        print("[Stream-Web] 퇴장")

# ---------------------------------------------------------
# TCP 핸들러
# ---------------------------------------------------------
def tcp_server_thread(loop):
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(('0.0.0.0', TCP_PORT))
    server.listen(5)
    print(f">>> [Stream-TCP] 영상 중계 서버 시작 (Port: {TCP_PORT})")

    while True:
        try:
            conn, addr = server.accept()
            print(f"[Stream-TCP] 로봇 접속: {addr}")
            tcp_clients.add(conn)
            
            # TCP 클라이언트 처리 스레드 시작
            threading.Thread(target=handle_tcp_client, args=(conn, loop), daemon=True).start()
        except Exception as e:
            print(f"TCP Accept Error: {e}")

def handle_tcp_client(conn, loop):
    try:
        while True:
            # 8KB 단위로 읽어서 즉시 중계 (디코딩 없음)
            data = conn.recv(8192)
            if not data: break
            
            # 1. 다른 TCP에게 전송
            broadcast_to_tcp(data, conn)
            
            # 2. 웹소켓에게 전송 (메인 루프의 스레드에서 실행)
            asyncio.run_coroutine_threadsafe(broadcast_to_ws(data), loop)
    except:
        pass
    finally:
        if conn in tcp_clients: tcp_clients.remove(conn)
        conn.close()
        print("[Stream-TCP] 로봇 퇴장")

# ---------------------------------------------------------
# 메인 (수정됨: asyncio.run 사용)
# ---------------------------------------------------------
async def main():
    # 현재 실행 중인 루프 가져오기 (asyncio.run이 만들어준 루프)
    loop = asyncio.get_running_loop()

    # TCP 서버 스레드 시작
    # (TCP 스레드가 웹소켓으로 메시지를 보낼 때 이 loop를 사용함)
    t = threading.Thread(target=tcp_server_thread, args=(loop,), daemon=True)
    t.start()
    
    print(f">>> [Stream-Web] 웹 중계 서버 시작 (Port: {WEBSOCKET_PORT})")
    
    # 웹소켓 서버 시작 (비동기 컨텍스트 매니저 사용)
    async with websockets.serve(ws_handler, "0.0.0.0", WEBSOCKET_PORT):
        # 서버가 꺼지지 않도록 무한 대기
        await asyncio.Future()

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("서버 종료")