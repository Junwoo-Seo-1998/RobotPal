/**
 * @file NetworkManager.cpp
 * @author Hong Yoon Pyo (cgantro@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2025-11-27
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "RobotPal/NetworkManager.h" // 실제 파일명에 맞게 수정 (NetworkManager.h 권장)
#include <iostream>
#include <vector>

// ---------------------------------------------------------
// [Web] Emscripten WebSocket Callbacks
// ---------------------------------------------------------
#ifdef __EMSCRIPTEN__
#include <emscripten.h>

// C 스타일 콜백 함수들 (멤버 함수로 바로 못 씀)
EM_BOOL OnOpen(int eventType, const EmscriptenWebSocketOpenEvent* websocketEvent, void* userData) {
    std::cout << "[Web] WebSocket Connected!" << std::endl;
    NetworkManager* nm = (NetworkManager*)userData;
    // nm->SetConnected(true); // 친구 클래스나 public 설정 필요
    return EM_TRUE;
}

EM_BOOL OnMessage(int eventType, const EmscriptenWebSocketMessageEvent* websocketEvent, void* userData) {
    if (websocketEvent->isText) {
        std::string msg((const char*)websocketEvent->data);
        // nm->OnReceive(msg);
        std::cout << "[Web] Recv: " << msg << std::endl;
    }
    return EM_TRUE;
}

EM_BOOL OnClose(int eventType, const EmscriptenWebSocketCloseEvent* websocketEvent, void* userData) {
    std::cout << "[Web] WebSocket Disconnected." << std::endl;
    return EM_TRUE;
}
#endif

// ---------------------------------------------------------
// Class Implementation
// ---------------------------------------------------------

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
    if (m_ClientSocket != INVALID_SOCKET) {
    #ifdef _WIN32
        closesocket(m_ClientSocket);
    #else
        close(m_ClientSocket);
    #endif
    }
    if (m_ListenSocket != INVALID_SOCKET) {
    #ifdef _WIN32
        closesocket(m_ListenSocket);
    #else
        close(m_ListenSocket);
    #endif
    }
    m_ClientSocket = INVALID_SOCKET;
    m_ListenSocket = INVALID_SOCKET;
#endif
    m_IsConnected = false;
}

void NetworkManager::SetNonBlocking(SOCKET s) {
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(s, FIONBIO, &mode);
#else
    int flags = fcntl(s, F_GETFL, 0);
    fcntl(s, F_SETFL, flags | O_NONBLOCK);
#endif
}

bool NetworkManager::Init(int port) {
#ifdef __EMSCRIPTEN__
    // [Web] 클라이언트로 동작 -> 중계 서버(Bridge)에 접속
    // 주의: 로컬 테스트 시 'ws://localhost:포트' 사용
    // 실제 배포 시에는 로봇과 통신할 수 있는 Proxy Server 주소 필요
    if (!emscripten_websocket_is_supported()) {
        std::cout << "[Web] WebSocket not supported!" << std::endl;
        return false;
    }

    EmscriptenWebSocketCreateAttributes attr = {
        "ws://localhost:5555", // 접속할 주소 (Bridge Server)
        NULL, EM_TRUE
    };

    m_WebSocket = emscripten_websocket_new(&attr);
    if (m_WebSocket <= 0) return false;

    // 콜백 등록
    emscripten_websocket_set_onopen_callback(m_WebSocket, (void*)this, OnOpen);
    emscripten_websocket_set_onmessage_callback(m_WebSocket, (void*)this, OnMessage);
    emscripten_websocket_set_onclose_callback(m_WebSocket, (void*)this, OnClose);

    return true;

#else
    // [Desktop] TCP 서버로 동작
    m_ListenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_ListenSocket == INVALID_SOCKET) return false;

    // SO_REUSEADDR (재시작 시 포트 점유 에러 방지)
    int opt = 1;
    setsockopt(m_ListenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(m_ListenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "[TCP] Bind Failed\n";
        return false;
    }

    listen(m_ListenSocket, 1);
    SetNonBlocking(m_ListenSocket);
    
    std::cout << "[TCP] Server Started on port " << port << std::endl;
    return true;
#endif
}

void NetworkManager::Update() {
#ifdef __EMSCRIPTEN__
    // 웹은 콜백 방식이라 Polling 불필요 (혹은 상태 체크용)
#else
    // [Desktop] 기존 TCP 서버 로직 유지
    if (m_ClientSocket == INVALID_SOCKET) {
        sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        SOCKET newClient = accept(m_ListenSocket, (sockaddr*)&clientAddr, &clientLen);

        if (newClient != INVALID_SOCKET) {
            std::cout << "[TCP] Client Connected!\n";
            m_ClientSocket = newClient;
            SetNonBlocking(m_ClientSocket);
            m_IsConnected = true;
        }
    } else {
        char buf[1024];
        int len = recv(m_ClientSocket, buf, sizeof(buf) - 1, 0);
        if (len > 0) {
            buf[len] = '\0';
            m_ReceivedBuffer = std::string(buf);
        } else if (len == 0) {
            Close(); // 연결 끊김
        }
    }
#endif
}

void NetworkManager::Send(const std::string& msg) {
    std::string finalMsg = msg + "\n";
#ifdef __EMSCRIPTEN__
    if (m_WebSocket > 0) {
        emscripten_websocket_send_utf8_text(m_WebSocket, finalMsg.c_str());
    }
#else
    if (m_ClientSocket != INVALID_SOCKET) {
        send(m_ClientSocket, finalMsg.c_str(), (int)finalMsg.size(), 0);
    }
#endif
}

bool NetworkManager::IsConnected() const {
#ifdef __EMSCRIPTEN__
    // 웹소켓 상태 확인 (1 = OPEN)
    unsigned short state = 0;
    emscripten_websocket_get_ready_state(m_WebSocket, &state);
    return state == 1; 
#else
    return m_IsConnected;
#endif
}

std::string NetworkManager::GetLastReceivedData() {
    std::string temp = m_ReceivedBuffer;
    m_ReceivedBuffer.clear();
    return temp;
}