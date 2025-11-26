#pragma once

// 로봇 제어 인터페이스
// 역할: 구체적인 제어 방식(Sim vs Real)을 몰라도 로봇을 조작할 수 있게 함
class IRobotController
{
public:
    virtual ~IRobotController() = default;

    // 초기화
    virtual bool Init() = 0;

    // 이동 명령 (v: 선속도, w: 각속도)
    virtual void Move(float v, float w) = 0; // Drive -> Move로 변경 (더 일반적)

    // 상태 업데이트 (매 프레임 Entity 데이터 수정)
    virtual void Update(float dt) = 0;
};