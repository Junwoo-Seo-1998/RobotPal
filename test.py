import socket
import threading
import time
import sys
import tty
import termios

# [설정] PC(서버)의 IP 주소를 입력하세요.
SERVER_IP = '100.115.224.93' 
SERVER_PORT = 5555

# 로봇 상태 (위치, 회전)
current_x = 0.0
current_y = 0.0
current_yaw = 0.0

# 이동 속도 설정
MOVE_STEP = 0.1
ROT_STEP = 0.1

def getch():
    """터미널에서 문자 하나를 입력받는 함수 (Linux/Mac)"""
    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    try:
        tty.setraw(sys.stdin.fileno())
        ch = sys.stdin.read(1)
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
    return ch

def receive_loop(sock):
    """서버(PC)에서 오는 데이터를 수신하는 스레드"""
    while True:
        try:
            data = sock.recv(1024)
            if not data:
                break
            # PC가 보낸 명령 출력 (필요시)
            # print(f"\r[PC 명령]: {data.decode().strip()}")
        except:
            break

def run_teleop_client():
    global current_x, current_y, current_yaw
    
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
    try:
        print(f"Connecting to {SERVER_IP}:{SERVER_PORT}...")
        client_socket.connect((SERVER_IP, SERVER_PORT))
        print(">>> [연결 성공] 키보드 'W/A/S/D'를 눌러 로봇을 움직이세요. ('q' 종료)")

        # 수신 스레드 (PC 메시지 확인용)
        t = threading.Thread(target=receive_loop, args=(client_socket,), daemon=True)
        t.start()

        while True:
            # 1. 키보드 입력 받기 (Blocking)
            key = getch()
            
            if key == 'q':
                print("\r>>> 종료합니다.")
                break
            
            # 2. 좌표 업데이트 (전역 좌표계 기준 단순 이동)
            # W/S: X축 이동 (앞/뒤)
            # A/D: Y축 이동 (좌/우) - 시뮬레이션에서는 Z축으로 매핑됨
            # Q/E: 회전 (Yaw)
            
            if key == 'w':
                current_x += MOVE_STEP
            elif key == 's':
                current_x -= MOVE_STEP
            elif key == 'a':
                current_y += MOVE_STEP 
            elif key == 'd':
                current_y -= MOVE_STEP
            elif key == 'q': # 좌회전 (Q 키)
                current_yaw += ROT_STEP
            elif key == 'e': # 우회전 (E 키)
                current_yaw -= ROT_STEP

            # 3. 상태 패킷 전송 (프로토콜: "STATE:x=...,y=...,yaw=...")
            # C++의 RealDriver가 이 형식을 파싱합니다.
            msg = f"STATE:x={current_x:.2f},y={current_y:.2f},yaw={current_yaw:.2f}\n"
            client_socket.sendall(msg.encode())
            
            print(f"\r[전송] {msg.strip()}", end="")

    except Exception as e:
        print(f"\n>>> [오류] {e}")
    finally:
        # 터미널 설정 복구 (혹시 모를 상황 대비)
        # termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old_settings) # getch에서 처리됨
        client_socket.close()

if __name__ == "__main__":
    run_teleop_client()