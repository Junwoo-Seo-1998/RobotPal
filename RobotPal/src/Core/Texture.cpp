#include "RobotPal/Core/Texture.h"

#include <cstring> // memcpy

// 정적 멤버 초기화
unsigned int Texture::s_ReadFBO = 0;

Texture::Texture(int width, int height, TextureFormat format, TextureType type)
    : m_Width(width), m_Height(height), m_Format(format), m_Type(type)
{
    CreateInternal();
}

Texture::Texture(int width, int height, const void* data, TextureFormat format)
    : m_Width(width), m_Height(height), m_Format(format), m_Type(TextureType::Texture2D)
{
    CreateInternal();
    if (data) {
        // 포맷에 따른 사이즈 계산
        int channels = (format == TextureFormat::RGBA8) ? 4 : 3;
        SetData(data, width * height * channels);
    }
}

Texture::~Texture() {
    if (m_RendererID) glDeleteTextures(1, &m_RendererID);
    
    // PBO가 생성되었다면 정리
    if (m_UsePBO) glDeleteBuffers(2, m_PBOs);
}

void Texture::Bind(int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(m_Type == TextureType::TextureCube ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, m_RendererID);
}

void Texture::Unbind() const {
    glBindTexture(m_Type == TextureType::TextureCube ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, 0);
}

void Texture::Resize(int width, int height) {
    if (width == m_Width && height == m_Height) return;
    if (width <= 0 || height <= 0) return;

    m_Width = width;
    m_Height = height;

    // 텍스처 재생성
    if (m_RendererID) glDeleteTextures(1, &m_RendererID);
    CreateInternal();

    // 중요: PBO도 사이즈가 바뀌었으므로 재생성 필요
    if (m_UsePBO) {
        glDeleteBuffers(2, m_PBOs);
        m_UsePBO = false; 
        // InitPBOs()는 다음번 GetAsyncData 호출 시 자동으로 실행됨
    }
}

void Texture::SetData(const void* data, int size) {
    if (m_Type != TextureType::Texture2D) return;

    GLenum format = (m_Format == TextureFormat::RGBA8) ? GL_RGBA : GL_RGB;
    glBindTexture(GL_TEXTURE_2D, m_RendererID);
    
    // 데이터 정렬 (RGB 텍스처 등 4바이트 정렬이 아닐 경우 대비)
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_Width, m_Height, format, GL_UNSIGNED_BYTE, data);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}

void Texture::SetCubeMapData(const std::vector<void*>& faces) {
    if (m_Type != TextureType::TextureCube || faces.size() != 6) return;

    glBindTexture(GL_TEXTURE_CUBE_MAP, m_RendererID);
    GLenum format = (m_Format == TextureFormat::RGBA8) ? GL_RGBA : GL_RGB;

    for (int i = 0; i < 6; i++) {
        if (faces[i]) {
            glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                0, 0, 0, m_Width, m_Height, format, GL_UNSIGNED_BYTE, faces[i]);
        }
    }
}

// ---------------------------------------------------------
// [핵심] PBO를 이용한 비동기 데이터 읽기 (Double Buffering)
// ---------------------------------------------------------
std::vector<uint8_t> Texture::GetAsyncData() {
    if (m_Type != TextureType::Texture2D) return {}; // 큐브맵 등은 미지원

    // 1. PBO 지연 초기화 (필요할 때만 메모리 할당)
    if (!m_UsePBO) InitPBOs();

    int channels = (m_Format == TextureFormat::RGBA8) ? 4 : 3;
    int dataSize = m_Width * m_Height * channels;
    std::vector<uint8_t> result(dataSize);

    // 인덱스 스위칭 (Ping-Pong)
    // index: 이번에 GPU에 "담아놔"라고 명령할 버퍼
    // nextIndex: 이번에 CPU가 "내놔"라고 열어볼 버퍼 (이전 프레임 데이터)
    int writeIndex = m_PBOIndex;
    int readIndex = (m_PBOIndex + 1) % 2;
    m_PBOIndex = (m_PBOIndex + 1) % 2; // 다음을 위해 인덱스 변경

    // --- STEP 1: GPU에게 캡처 명령 (비동기 Write) ---
    
    // 정적 FBO 생성 및 바인딩
    if (s_ReadFBO == 0) glGenFramebuffers(1, &s_ReadFBO);
    
    // 현재 바인딩 상태 백업
    GLint lastFBO;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &lastFBO);

    glBindFramebuffer(GL_FRAMEBUFFER, s_ReadFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_RendererID, 0);

    // 쓰기용 PBO 바인딩
    glBindBuffer(GL_PIXEL_PACK_BUFFER, m_PBOs[writeIndex]);
    
    // 팩 정렬 설정 (중요)
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    // ReadPixels 호출 (PBO가 바인딩되어 있으므로 즉시 리턴됨)
    GLenum format = (m_Format == TextureFormat::RGBA8) ? GL_RGBA : GL_RGB;
    glReadPixels(0, 0, m_Width, m_Height, format, GL_UNSIGNED_BYTE, 0);

    // FBO 상태 복구
    glBindFramebuffer(GL_FRAMEBUFFER, lastFBO);


    // --- STEP 2: 이전 프레임 데이터 가져오기 (CPU Read) ---
    
    glBindBuffer(GL_PIXEL_PACK_BUFFER, m_PBOs[readIndex]);
    
    // GLES 3.0 / GL 3.0 방식 맵핑
    void* ptr = glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, dataSize, GL_MAP_READ_BIT);
    
    if (ptr) {
        memcpy(result.data(), ptr, dataSize);
        glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    } else {
        // 첫 프레임이거나 에러 시: 데이터가 없으므로 그냥 빈 상태 혹은 0 리턴
        // (보통 첫 1~2 프레임은 검은색이 나옴)
    }

    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0); // PBO 언바인딩
    glPixelStorei(GL_PACK_ALIGNMENT, 4);   // 정렬 복구

    return result;
}

void Texture::InitPBOs() {
    int channels = (m_Format == TextureFormat::RGBA8) ? 4 : 3;
    int dataSize = m_Width * m_Height * channels;

    glGenBuffers(2, m_PBOs);
    for (int i = 0; i < 2; i++) {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, m_PBOs[i]);
        // GL_STREAM_READ: 데이터를 한번 쓰고, 몇 번 읽는 패턴 (스트리밍 최적화)
        glBufferData(GL_PIXEL_PACK_BUFFER, dataSize, nullptr, GL_STREAM_READ);
    }
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    m_UsePBO = true;
}

void Texture::CreateInternal() {
    glGenTextures(1, &m_RendererID);
    
    GLenum internalFormat, dataFormat, dataType;

    if (m_Format == TextureFormat::RGBA8) {
        internalFormat = GL_RGBA8; dataFormat = GL_RGBA; dataType = GL_UNSIGNED_BYTE;
    } else if (m_Format == TextureFormat::RGB8) {
        internalFormat = GL_RGB8; dataFormat = GL_RGB; dataType = GL_UNSIGNED_BYTE;
    } else if (m_Format == TextureFormat::DEPTH24_STENCIL8) {
        internalFormat = GL_DEPTH24_STENCIL8; dataFormat = GL_DEPTH_STENCIL; dataType = GL_UNSIGNED_INT_24_8;
    }

    GLenum target = (m_Type == TextureType::TextureCube) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
    glBindTexture(target, m_RendererID);

    if (m_Type == TextureType::Texture2D) {
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Width, m_Height, 0, dataFormat, dataType, nullptr);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        // 뎁스 맵(그림자)은 Clamp 필수, 일반 텍스처는 Repeat
        if (m_Format == TextureFormat::DEPTH24_STENCIL8) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        } else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }
    } 
    else if (m_Type == TextureType::TextureCube) {
        for (unsigned int i = 0; i < 6; ++i) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                0, internalFormat, m_Width, m_Height, 0, dataFormat, dataType, nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }
}

void Texture::CleanUpStaticResources() {
    if (s_ReadFBO != 0) {
        glDeleteFramebuffers(1, &s_ReadFBO);
        s_ReadFBO = 0;
    }
}