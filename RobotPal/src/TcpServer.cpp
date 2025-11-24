#include "RobotPal/TcpServer.h"
#include <iostream>

TcpServer::TcpServer(){
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2),&wsaData);
#endif
}

TcpServer::~TcpServer(){
    Close();
#ifdef _WIN32
    WSACleanup();
#endif
}

void TcpServer::Close()
{
    if (m_ClientSocket != INVALID_SOCKET) closesocket(m_ClientSocket);
    if (m_ListenSocket != INVALID_SOCKET) closesocket(m_ListenSocket);
    m_ClientSocket = INVALID_SOCKET;
    m_ListenSocket = INVALID_SOCKET;
}

void TcpServer::SetNonBlocking(SOCKET s){
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(s, FIONBIO,&mode);
#endif
}

bool TcpServer::Start(int port){
    // 1. 소켓 생성
    m_ListenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(m_ListenSocket == INVALID_SOCKET) return false;

    // 2. 주소 바인딩
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if(bind(m_ListenSocket,(sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR){
        std::cerr << "[TCP] Bind Failed\n";
        return false;
    }

    listen(m_ListenSocket,1);

    SetNonBlocking(m_ListenSocket);
    std::cout << "[TCP] Server Started on port" << port << std::endl;
    return true;
}

void TcpServer::Update()
{
    // Case A: 아직 손님이 없을 때 -> 손님 기다리기 (Accept)
    if (m_ClientSocket == INVALID_SOCKET)
    {
        sockaddr_in clientAddr;
        int clientLen = sizeof(clientAddr);
        SOCKET newClient = accept(m_ListenSocket, (sockaddr*)&clientAddr, &clientLen);

        if (newClient != INVALID_SOCKET) {
            std::cout << "[TCP] Client Connected!\n";
            m_ClientSocket = newClient;
            SetNonBlocking(m_ClientSocket); // 손님 소켓도 논블로킹으로!
        }
    }
    // Case B: 손님이 있을 때 -> 데이터 읽기 (Recv)
    else
    {
        char buf[1024];
        int bytesReceived = recv(m_ClientSocket, buf, sizeof(buf) - 1, 0);

        if (bytesReceived > 0) {
            buf[bytesReceived] = '\0'; // 문자열 끝 처리
            m_ReceivedBuffer = std::string(buf); // 버퍼에 저장
            std::cout << "[TCP] Recv: " << m_ReceivedBuffer << std::endl;
        }
        else if (bytesReceived == 0) {
            // 연결 끊김
            std::cout << "[TCP] Client Disconnected\n";
            closesocket(m_ClientSocket);
            m_ClientSocket = INVALID_SOCKET;
        }
        else {
            // 에러 발생 (WSAEWOULDBLOCK은 "아직 데이터 없음"이므로 에러 아님)
            int err = WSAGetLastError();
            if (err != WSAEWOULDBLOCK) {
                // 진짜 에러
                closesocket(m_ClientSocket);
                m_ClientSocket = INVALID_SOCKET;
            }
        }
    }
}

void TcpServer::Send(const std::string& msg)
{
    if (m_ClientSocket != INVALID_SOCKET) {
        // TCP는 패킷이 붙을 수 있으니 끝에 개행(\n)을 넣어주는 게 국룰
        std::string finalMsg = msg + "\n";
        send(m_ClientSocket, finalMsg.c_str(), (int)finalMsg.size(), 0);
    }
}

std::string TcpServer::GetLastReceivedData()
{
    std::string temp = m_ReceivedBuffer;
    m_ReceivedBuffer.clear(); // 읽었으면 비움
    return temp;
}