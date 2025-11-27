#include "RobotPal/Core/Framebuffer.h"
#include <glad/gles2.h>

// [생성자 1: 일반 모드]
std::shared_ptr<Framebuffer> Framebuffer::Create(int width, int height, TextureFormat colorFormat, bool useDepthTexture)
{
    return std::make_shared<Framebuffer>(width, height, colorFormat, useDepthTexture);
}


std::shared_ptr<Framebuffer> Framebuffer::Create(std::shared_ptr<Texture> colorTex, std::shared_ptr<Texture> depthTex)
{
    return std::make_shared<Framebuffer>(colorTex, depthTex);
}


Framebuffer::Framebuffer(int width, int height, TextureFormat colorFormat, bool useDepthTexture)
    : m_Width(width), m_Height(height), m_ColorFormat(colorFormat), m_UseDepthTexture(useDepthTexture)
{
    // 1. 컬러 텍스처 생성
    m_ColorAttachment = std::make_shared<Texture>(width, height, colorFormat);

    // 2. 뎁스 텍스처 생성 (옵션)
    if (m_UseDepthTexture) {
        m_DepthAttachment = std::make_shared<Texture>(width, height, TextureFormat::DEPTH24_STENCIL8);
    }

    // 3. FBO 조립
    Invalidate();
}

// [생성자 2: 주입 모드]
Framebuffer::Framebuffer(std::shared_ptr<Texture> colorTex, std::shared_ptr<Texture> depthTex)
    : m_ColorAttachment(colorTex), m_DepthAttachment(depthTex)
{
    // 스펙 역추적
    if (colorTex) {
        m_Width = colorTex->GetWidth();
        m_Height = colorTex->GetHeight();
        m_ColorFormat = colorTex->GetFormat();
    } else if (depthTex) {
        m_Width = depthTex->GetWidth();
        m_Height = depthTex->GetHeight();
        m_ColorFormat = TextureFormat::RGB8; // 더미 값
    }
    
    m_UseDepthTexture = (depthTex != nullptr);

    Invalidate();
}

Framebuffer::~Framebuffer() {
    CleanUp();
}

void Framebuffer::Bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
    glViewport(0, 0, m_Width, m_Height);
}

void Framebuffer::Unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::Resize(int width, int height) {
    if (width <= 0 || height <= 0) return;
    if (width == m_Width && height == m_Height) return;

    m_Width = width;
    m_Height = height;

    // 소유한 텍스처들도 리사이즈 (Texture 클래스 내부에서 PBO 등도 알아서 처리됨)
    if (m_ColorAttachment) m_ColorAttachment->Resize(width, height);
    if (m_DepthAttachment) m_DepthAttachment->Resize(width, height);

    // FBO 재생성
    Invalidate();
}

void Framebuffer::CleanUp() {
    if (m_RendererID) {
        glDeleteFramebuffers(1, &m_RendererID);
        m_RendererID = 0;
    }
    if (m_DepthRBO) {
        glDeleteRenderbuffers(1, &m_DepthRBO);
        m_DepthRBO = 0;
    }
}

void Framebuffer::Invalidate() {
    if (m_RendererID) CleanUp();

    glGenFramebuffers(1, &m_RendererID);
    glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);

    // --- 1. Color Attachment 연결 ---
    if (m_ColorAttachment) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorAttachment->GetID(), 0);
        
        // [수정] 컬러가 있을 때는 0번 어태치먼트에 그린다 명시 (GLES 호환)
        GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
        glDrawBuffers(1, drawBuffers);
    } 
    else {
        // [수정] 컬러가 없는 경우 (Shadow Map 전용 등)
        // GLES 3.0에서는 glDrawBuffer(GL_NONE) 대신 이렇게 써야 함
        GLenum drawBuffers[] = { GL_NONE };
        glDrawBuffers(1, drawBuffers);
        
        glReadBuffer(GL_NONE);
    }

    // --- 2. Depth/Stencil Attachment 연결 ---
    if (m_DepthAttachment) {
        // [Case A] 뎁스 '텍스처' 사용
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_DepthAttachment->GetID(), 0);
    } 
    else {
        // [Case B] 렌더버퍼(RBO) 사용
        glGenRenderbuffers(1, &m_DepthRBO);
        glBindRenderbuffer(GL_RENDERBUFFER, m_DepthRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_Width, m_Height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_DepthRBO);
    }

    // --- 3. 상태 확인 ---
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}