import asyncio
import websockets
import socket
import threading

# 설정
TCP_PORT = 12345       # 로봇, C++ 연결용
WEBSOCKET_PORT = 5555  # 웹 연결용

# 연결된 클라이언트 관리 (단순 리스트)
tcp_clients = set()
ws_clients = set()

# ---------------------------------------------------------
# 브로드캐스트 헬퍼 (즉시 전송)
# ---------------------------------------------------------
async def broadcast_to_ws(message, sender_ws=None):
    if not ws_clients: return
    for ws in list(ws_clients):
        if ws != sender_ws:
            try:
                await ws.send(message)
            except:
                pass

def broadcast_to_tcp(message, sender_socket=None):
    if not tcp_clients: return
    encoded = (message + '\n').encode()
    for sock in list(tcp_clients):
        if sock != sender_socket:
            try:
                sock.sendall(encoded)
            except:
                pass

# ---------------------------------------------------------
# 핸들러
# ---------------------------------------------------------
async def ws_handler(websocket):
    print(f"[Web] 접속: {websocket.remote_address}")
    ws_clients.add(websocket)
    try:
        async for message in websocket:
            # 받은 즉시 다른 클라이언트들에게 전달
            await broadcast_to_ws(message, websocket)
            broadcast_to_tcp(message)
    except:
        pass
    finally:
        ws_clients.remove(websocket)
        print("[Web] 퇴장")

def tcp_server_thread(loop):
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(('0.0.0.0', TCP_PORT))
    server.listen(5)
    print(f">>> [TCP] 서버 시작 (Port: {TCP_PORT})")

    while True:
        conn, addr = server.accept()
        print(f"[TCP] 접속: {addr}")
        tcp_clients.add(conn)
        
        # 각 TCP 클라이언트를 처리할 스레드 생성 (Non-blocking 처리를 위해)
        threading.Thread(target=handle_tcp_client, args=(conn, loop), daemon=True).start()

def handle_tcp_client(conn, loop):
    try:
        buffer = ""
        while True:
            data = conn.recv(1024)
            if not data: break
            
            # 패킷 파싱 (개행 기준)
            buffer += data.decode()
            while '\n' in buffer:
                msg, buffer = buffer.split('\n', 1)
                if msg.strip():
                    # 받은 즉시 웹과 다른 TCP에게 전달
                    broadcast_to_tcp(msg, conn)
                    asyncio.run_coroutine_threadsafe(broadcast_to_ws(msg), loop)
    except:
        pass
    finally:
        if conn in tcp_clients: tcp_clients.remove(conn)
        conn.close()
        print("[TCP] 퇴장")

# ---------------------------------------------------------
# 메인
# ---------------------------------------------------------
if __name__ == "__main__":
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    
    # TCP 서버 스레드 시작
    t = threading.Thread(target=tcp_server_thread, args=(loop,), daemon=True)
    t.start()
    
    # 웹소켓 서버 시작
    print(f">>> [Web] 서버 시작 (Port: {WEBSOCKET_PORT})")
    start_server = websockets.serve(ws_handler, "0.0.0.0", WEBSOCKET_PORT)
    
    loop.run_until_complete(start_server)
    loop.run_forever()