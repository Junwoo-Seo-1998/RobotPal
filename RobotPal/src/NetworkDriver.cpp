/**
 * @file NetworkDriver.cpp
 * @author Hong Yoon Pyo (cgantro@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2025-11-26
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "RobotPal/RobotDriver.h"
#include "RobotPal/TcpServer.h"

class NetworkDriver : public IRobotDriver{
public:
    NetworkDriver(){
        // 자원관리는 객체에게
    }

    bool Init() override{
        return m_Server.Start(5555);
    }

    void Drive(float v, float w) override{
        // 패킷 형식 및 전송 구현은 추후에
    }

    void Update(float ts) override{
        m_Server.Update(); // 소켓 수신 처리
    }
private:
    TcpServer m_Server;
};