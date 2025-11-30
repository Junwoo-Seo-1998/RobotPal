#ifndef __FRAMEBUFFER_H__
#define __FRAMEBUFFER_H__
#include "RobotPal/Core/Texture.h"
#include <memory>
#include <vector>
#include <iostream>


class Framebuffer {
public:
    static std::shared_ptr<Framebuffer> Create(int width, int height, TextureFormat colorFormat = TextureFormat::RGB8, bool useDepthTexture = false);
    static std::shared_ptr<Framebuffer> Create(std::shared_ptr<Texture> colorTex, std::shared_ptr<Texture> depthTex = nullptr);

    // [생성자 1: 일반 모드]
    // FBO가 스스로 텍스처를 생성하고 관리합니다. (가장 많이 사용)
    // useDepthTexture: true면 뎁스 텍스처 생성(그림자/포스트프로세싱), false면 RBO 생성(일반 렌더링)
    Framebuffer(int width, int height, TextureFormat colorFormat = TextureFormat::RGB8, bool useDepthTexture = false);

    // [생성자 2: 주입 모드]
    // 외부에서 만든 텍스처를 가져와서 FBO를 구성합니다. (핑퐁 버퍼, 그림자 맵, 멀티 타겟 등)
    // colorTex가 nullptr이면 'Depth Only' 모드로 작동합니다.
    Framebuffer(std::shared_ptr<Texture> colorTex, std::shared_ptr<Texture> depthTex = nullptr);

    ~Framebuffer();

    // 렌더링 시작
    void Bind();
    
    // 렌더링 종료 (기본 화면으로 복귀)
    void Unbind();

    // 윈도우 크기 변경 시 호출 (가지고 있는 텍스처도 같이 리사이즈 됨)
    void Resize(int width, int height);

    // 텍스처 가져오기 (이걸로 Texture::GetAsyncData 등을 호출)
    std::shared_ptr<Texture> GetColorAttachment() const { return m_ColorAttachment; }
    std::shared_ptr<Texture> GetDepthAttachment() const { return m_DepthAttachment; }

    // FBO ID 확인
    unsigned int GetID() const { return m_RendererID; }

    // 현재 설정 정보 확인
    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }

private:
    // FBO 생성 및 연결 로직 (초기화 및 리사이즈 시 호출)
    void Invalidate();
    
    // 자원 해제
    void CleanUp();

    unsigned int m_RendererID = 0;
    unsigned int m_DepthRBO = 0; // 뎁스 텍스처를 안 쓸 때 사용하는 Renderbuffer

    int m_Width, m_Height;
    TextureFormat m_ColorFormat;
    bool m_UseDepthTexture;

    std::shared_ptr<Texture> m_ColorAttachment;
    std::shared_ptr<Texture> m_DepthAttachment;
};

#endif