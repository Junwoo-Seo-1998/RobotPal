import socket
import time
import threading
import random

SERVER_IP = '127.0.0.1' # PC와 동일한 컴퓨터라면 localhost
SERVER_PORT = 5555

def run_fake_agv():
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
    try:
        print(f"PC 서버({SERVER_IP}:{SERVER_PORT})에 접속 시도 중...")
        client_socket.connect((SERVER_IP, SERVER_PORT))
        print(">>> [접속 성공] PC와 연결되었습니다.")

        # 1. 수신 스레드 (PC -> AGV 명령 받기)
        def receive_loop():
            while True:
                try:
                    data = client_socket.recv(1024)
                    if not data: break
                    msg = data.decode().strip()
                    print(f"\n[명령 수신] {msg}")
                    
                    # 명령 해석 (CMD:v,w)
                    if msg.startswith("CMD:"):
                        parts = msg.split(":")[1].split(",")
                        v, w = float(parts[0]), float(parts[1])
                        print(f"   -> 모터 제어: 속도={v} m/s, 회전={w} rad/s")
                        
                except Exception as e:
                    print(f"수신 에러: {e}")
                    break
        
        threading.Thread(target=receive_loop, daemon=True).start()

        # 2. 송신 루프 (AGV -> PC 상태 보고)
        x, y = 0.0, 0.0
        while True:
            # 가상의 위치 데이터 생성 (조금씩 움직이는 척)
            x += 0.01
            y += 0.02
            
            state_msg = f"STATE:x={x:.2f},y={y:.2f}"
            client_socket.sendall(state_msg.encode())
            # print(f"[상태 전송] {state_msg}")
            
            time.sleep(0.1) # 10Hz 주기로 전송

    except ConnectionRefusedError:
        print(">>> [연결 실패] C++ 시뮬레이터를 먼저 실행해주세요.")
    except KeyboardInterrupt:
        print("\n>>> 종료합니다.")
    finally:
        client_socket.close()

if __name__ == "__main__":
    run_fake_agv()