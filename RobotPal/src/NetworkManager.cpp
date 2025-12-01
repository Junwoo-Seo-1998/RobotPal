#include "RobotPal/NetworkManager.h"
#include <iostream>

// =========================================================
// [WEB] Emscripten Callbacks
// =========================================================
#ifdef __EMSCRIPTEN__
#include <emscripten.h>

EM_BOOL OnOpen(int eventType, const EmscriptenWebSocketOpenEvent* websocketEvent, void* userData) {
    std::cout << ">>> [Web] Connected to Bridge Server!" << std::endl;
    return EM_TRUE;
}

EM_BOOL OnMessage(int eventType, const EmscriptenWebSocketMessageEvent* websocketEvent, void* userData) {
    NetworkManager* nm = (NetworkManager*)userData;
    if (websocketEvent->isText) {
        std::string msg((const char*)websocketEvent->data);
        nm->PushToRecvQueue(msg); 
    }
    return EM_TRUE;
}

EM_BOOL OnClose(int eventType, const EmscriptenWebSocketCloseEvent* websocketEvent, void* userData) {
    std::cout << ">>> [Web] Disconnected." << std::endl;
    return EM_TRUE;
}
#endif

// =========================================================
// Class Implementation
// =========================================================

NetworkManager::NetworkManager() {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

NetworkManager::~NetworkManager() {
    Close();
#ifdef _WIN32
    WSACleanup();
#endif
}

void NetworkManager::Close() {
#ifdef __EMSCRIPTEN__
    if (m_WebSocket > 0) {
        emscripten_websocket_close(m_WebSocket, 1000, "End");
        emscripten_websocket_delete(m_WebSocket);
        m_WebSocket = 0;
    }
#else
    if (m_Socket != -1) {
    #ifdef _WIN32
        closesocket(m_Socket);
    #else
        close(m_Socket);
    #endif
        m_Socket = -1;
    }
#endif
    m_IsConnected = false;
    m_SendQueue.clear();
    m_RecvQueue.clear();
    m_AccumulatedBuffer.clear();
}

void NetworkManager::SetNonBlocking(int s) {
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(s, FIONBIO, &mode);
#else
    int flags = fcntl(s, F_GETFL, 0);
    fcntl(s, F_SETFL, flags | O_NONBLOCK);
#endif
}

// [변경] 클라이언트 모드로 접속 (IP, Port)
bool NetworkManager::Init(const std::string& ip, int port) {
#ifdef __EMSCRIPTEN__
    if (!emscripten_websocket_is_supported()) return false;

    // 웹은 ws:// 프로토콜 사용
    // (주의: 로컬 테스트 시 localhost, 배포 시 서버 IP 필요)
    std::string url = "ws://" + ip + ":" + std::to_string(5555); // 웹 포트는 보통 다름
    EmscriptenWebSocketCreateAttributes attr = { url.c_str(), NULL, EM_TRUE };
    
    m_WebSocket = emscripten_websocket_new(&attr);
    if (m_WebSocket <= 0) return false;

    emscripten_websocket_set_onopen_callback(m_WebSocket, (void*)this, OnOpen);
    emscripten_websocket_set_onmessage_callback(m_WebSocket, (void*)this, OnMessage);
    emscripten_websocket_set_onclose_callback(m_WebSocket, (void*)this, OnClose);
    
    m_IsConnected = true; 
    return true;
#else
    // [PC] TCP 클라이언트 모드
    m_Socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_Socket == -1) return false;

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr);

    // 접속 시도 (Blocking)
    std::cout << ">>> [Network] Connecting to " << ip << ":" << port << "..." << std::endl;
    if (connect(m_Socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cout << ">>> [Network] Connection Failed!" << std::endl;
        return false;
    }

    std::cout << ">>> [Network] Connected to Bridge Server!" << std::endl;
    SetNonBlocking(m_Socket); // 접속 후 Non-blocking 전환
    m_IsConnected = true;
    return true;
#endif
}

void NetworkManager::Send(const std::string& msg) {
    // [Write Queue] 바로 보내지 않고 큐에 저장
    if (m_IsConnected) {
        m_SendQueue.push_back(msg);
    }
}

void NetworkManager::Update() {
    // -----------------------------------------------------
    // 1. Write Queue 처리 (보낼 메시지 전송)
    // -----------------------------------------------------
    if (m_IsConnected && !m_SendQueue.empty()) {
        while (!m_SendQueue.empty()) {
            std::string msg = m_SendQueue.front();
            m_SendQueue.pop_front();
            
            std::string finalMsg = msg + "\n"; // 개행 문자 추가
            
            #ifdef __EMSCRIPTEN__
            if (m_WebSocket > 0) {
                unsigned short state;
                emscripten_websocket_get_ready_state(m_WebSocket, &state);
                if (state == 1) // OPEN
                    emscripten_websocket_send_utf8_text(m_WebSocket, finalMsg.c_str());
            }
            #else
            if (m_Socket != -1) {
                send(m_Socket, finalMsg.c_str(), (int)finalMsg.size(), 0);
            }
            #endif
        }
    }

    // -----------------------------------------------------
    // 2. Read Queue 처리 (수신 및 파싱)
    // -----------------------------------------------------
    #ifdef __EMSCRIPTEN__
        // 웹은 콜백에서 처리됨
        if (m_WebSocket > 0) {
            unsigned short state;
            emscripten_websocket_get_ready_state(m_WebSocket, &state);
            m_IsConnected = (state == 1);
        }
    #else
        // PC 소켓 수신
        if (m_Socket != -1 && m_IsConnected) {
            char buf[4096];
            int len = recv(m_Socket, buf, sizeof(buf) - 1, 0);
            
            if (len > 0) {
                buf[len] = '\0';
                m_AccumulatedBuffer += buf;
                
                // 패킷 파싱 (개행 기준)
                size_t pos = 0;
                while ((pos = m_AccumulatedBuffer.find('\n')) != std::string::npos) {
                    std::string packet = m_AccumulatedBuffer.substr(0, pos);
                    if (!packet.empty()) {
                        PushToRecvQueue(packet);
                    }
                    m_AccumulatedBuffer.erase(0, pos + 1);
                }
            } 
            else if (len == 0) {
                std::cout << ">>> [Network] Server Disconnected." << std::endl;
                Close();
            }
            else {
                #ifdef _WIN32
                int err = WSAGetLastError();
                if (err != WSAEWOULDBLOCK) Close();
                #endif
            }
        }
    #endif
}

bool NetworkManager::IsConnected() const { return m_IsConnected; }

void NetworkManager::PushToRecvQueue(const std::string& packet) {
    if (m_RecvQueue.size() > 100) m_RecvQueue.pop_front();
    m_RecvQueue.push_back(packet);
}

bool NetworkManager::HasMessage() {
    return !m_RecvQueue.empty();
}

std::string NetworkManager::PopMessage() {
    if (m_RecvQueue.empty()) return "";
    std::string msg = m_RecvQueue.front();
    m_RecvQueue.pop_front();
    return msg;
}