#include "RobotPal/Core/Texture.h"
#include "RobotPal/Util/Benchmark.h"
#include <cstring> // memcpy
#include <algorithm> // std::max (혹시 모를 상황 대비)
// 정적 멤버 초기화
unsigned int Texture::s_ReadFBO = 0;

std::vector<uint8_t> Texture::GetSyncData(uint64_t currentFrameIndex) {
    if (m_Type != TextureType::Texture2D) return {};
    const int pixelCount = m_Width * m_Height;
    const int srcChannels = 4;
    const int dstChannels = (m_Format == TextureFormat::RGBA8) ? 4 : 3;
    const size_t rgbaSize = static_cast<size_t>(pixelCount) * srcChannels;
    if (dstChannels == 4 && m_CachedData.size() != rgbaSize) m_CachedData.resize(rgbaSize);
    std::vector<uint8_t> rgbaData;
    if (dstChannels != 4) rgbaData.resize(rgbaSize);
    uint8_t* readDestination = dstChannels == 4 ? m_CachedData.data() : rgbaData.data();

    if (s_ReadFBO == 0) glGenFramebuffers(1, &s_ReadFBO);
    GLint lastFBO;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &lastFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, s_ReadFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_RendererID, 0);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    const auto readStart = robotpal::benchmark::SteadyNowNs();
    glReadPixels(0, 0, m_Width, m_Height, GL_RGBA, GL_UNSIGNED_BYTE, readDestination);
    robotpal::benchmark::Event("readback_sync_glreadpixels", currentFrameIndex,
        robotpal::benchmark::SteadyNowNs() - readStart, rgbaSize);

    glBindFramebuffer(GL_FRAMEBUFFER, lastFBO);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    if (dstChannels == 4) return m_CachedData;

    std::vector<uint8_t> rgbData(static_cast<size_t>(pixelCount) * dstChannels);
    for (int i = 0; i < pixelCount; ++i) {
        rgbData[i * 3 + 0] = rgbaData[i * 4 + 0];
        rgbData[i * 3 + 1] = rgbaData[i * 4 + 1];
        rgbData[i * 3 + 2] = rgbaData[i * 4 + 2];
    }
    return rgbData;
}

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
        m_CachedData.clear(); // 사이즈 변경 시 캐시 초기화
        // InitPBOs()는 다음번 GetAsyncData 호출 시 자동으로 실행됨
    }
}

void Texture::SetData(const void* data, int size) {
    if (m_Type != TextureType::Texture2D) return;

    GLenum format = GL_RGB;
    GLenum type = GL_UNSIGNED_BYTE;

    switch (m_Format) {
        case TextureFormat::RGBA8:
        format = GL_RGBA; type = GL_UNSIGNED_BYTE; break;
        case TextureFormat::RGB8:
        format = GL_RGB; type = GL_UNSIGNED_BYTE; break;
        case TextureFormat::RGBA16F:
        format = GL_RGBA; type = GL_FLOAT; break;
        case TextureFormat::RED_INTEGER:
        format = GL_RED_INTEGER; type = GL_INT; break;
    }

    glBindTexture(GL_TEXTURE_2D, m_RendererID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_Width, m_Height, format, type, data);
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
// std::vector<uint8_t> Texture::GetAsyncData(uint64_t currentFrameIndex) {
//     if (m_Type != TextureType::Texture2D) return {};

//     // --- 프레임 캐싱 ---
//     if (m_LastUpdateFrame == currentFrameIndex && !m_CachedData.empty()) {
//         return m_CachedData;
//     }

//     int pixelCount = m_Width * m_Height;
//     int dstChannels = (m_Format == TextureFormat::RGBA8) ? 4 : 3;

//     // 버퍼 크기 확보
//     if (m_CachedData.size() != pixelCount * dstChannels) {
//         m_CachedData.resize(pixelCount * dstChannels);
//     }

//     // --- FBO 준비 ---
//     static GLuint s_ReadFBO = 0;
//     if (s_ReadFBO == 0) glGenFramebuffers(1, &s_ReadFBO);

//     GLint lastFBO;
//     glGetIntegerv(GL_FRAMEBUFFER_BINDING, &lastFBO);
//     glBindFramebuffer(GL_FRAMEBUFFER, s_ReadFBO);
//     glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_RendererID, 0);

// #ifdef __EMSCRIPTEN__
//     // Web: 항상 RGBA 읽기
//     std::vector<uint8_t> tempData(pixelCount * 4); // RGBA 임시 버퍼
//     glPixelStorei(GL_PACK_ALIGNMENT, 1);
//     glReadPixels(0, 0, m_Width, m_Height, GL_RGBA, GL_UNSIGNED_BYTE, tempData.data());
//     glPixelStorei(GL_PACK_ALIGNMENT, 4);

//     if (dstChannels == 4) {
//         m_CachedData.swap(tempData);
//     } else {
//         for (int i = 0; i < pixelCount; ++i) {
//             m_CachedData[i*3 + 0] = tempData[i*4 + 0];
//             m_CachedData[i*3 + 1] = tempData[i*4 + 1];
//             m_CachedData[i*3 + 2] = tempData[i*4 + 2];
//         }
//     }
// #else
//     // 데스크톱: PBO 비동기 처리
//     if (!m_UsePBO) InitPBOs();

//     int writeIndex = m_PBOIndex;
//     int readIndex  = (m_PBOIndex + 1) % 2;

//     // GPU 비동기 캡처
//     glBindBuffer(GL_PIXEL_PACK_BUFFER, m_PBOs[writeIndex]);
//     glPixelStorei(GL_PACK_ALIGNMENT, 1);
//     glReadPixels(0, 0, m_Width, m_Height, GL_RGBA, GL_UNSIGNED_BYTE, 0);

//     // 이전 프레임 CPU 읽기
//     glBindBuffer(GL_PIXEL_PACK_BUFFER, m_PBOs[readIndex]);
//     uint8_t* ptr = (uint8_t*)glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, pixelCount * 4, GL_MAP_READ_BIT);

//     if (ptr) {
//         if (dstChannels == 4) {
//             memcpy(m_CachedData.data(), ptr, pixelCount * 4);
//         } else {
//             for (int i = 0; i < pixelCount; ++i) {
//                 m_CachedData[i*3 + 0] = ptr[i*4 + 0];
//                 m_CachedData[i*3 + 1] = ptr[i*4 + 1];
//                 m_CachedData[i*3 + 2] = ptr[i*4 + 2];
//             }
//         }
//         glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
//     }
//     glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
//     glPixelStorei(GL_PACK_ALIGNMENT, 4);
//     m_PBOIndex = (m_PBOIndex + 1) % 2;
// #endif

//     glBindFramebuffer(GL_FRAMEBUFFER, lastFBO);
//     m_LastUpdateFrame = currentFrameIndex;

//     return m_CachedData;
// }
std::vector<uint8_t> Texture::GetAsyncData(uint64_t currentFrameIndex) {
#ifdef __EMSCRIPTEN__
    if (m_Type != TextureType::Texture2D) return {};

    // GLES/بعض 드라이버에서 glReadPixels는 GL_RGB를 지원하지 않을 수 있습니다.
    // 안전하게 항상 GL_RGBA로 읽고, 필요 시 CPU에서 변환합니다.
    const int read_channels = 4;
    const GLenum read_format = GL_RGBA;
    
    int dataSizeWithAlpha = m_Width * m_Height * read_channels;
    std::vector<uint8_t> rgba_data(dataSizeWithAlpha);

    GLint lastFBO;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &lastFBO);

    if (s_ReadFBO == 0) glGenFramebuffers(1, &s_ReadFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, s_ReadFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_RendererID, 0);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    
    // glFinish()는 동기화 문제를 해결하지 못했으므로 제거합니다.
    // glFinish(); 

    glReadPixels(0, 0, m_Width, m_Height, read_format, GL_UNSIGNED_BYTE, rgba_data.data());

    glBindFramebuffer(GL_FRAMEBUFFER, lastFBO);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);

    // 원본 포맷에 따라 최종 결과 벡터를 생성합니다.
    if (m_Format == TextureFormat::RGB8) {
        int finalDataSize = m_Width * m_Height * 3;
        std::vector<uint8_t> rgb_data(finalDataSize);
        for (int i = 0; i < m_Width * m_Height; ++i) {
            rgb_data[i * 3 + 0] = rgba_data[i * 4 + 0]; // R
            rgb_data[i * 3 + 1] = rgba_data[i * 4 + 1]; // G
            rgb_data[i * 3 + 2] = rgba_data[i * 4 + 2]; // B
        }
        return rgb_data;
    } else {
        // RGBA8 또는 기타 지원 포맷의 경우, 그대로 반환 (또는 추가 처리)
        return rgba_data;
    }
#else
    if (m_Type != TextureType::Texture2D) return {};

    // 1. 중복 호출 방지 (프레임 캐싱)
    if (m_LastUpdateFrame == currentFrameIndex && !m_CachedData.empty()) {
        return m_CachedData;
    }

    // 2. PBO 지연 초기화
    if (!m_UsePBO) InitPBOs();

    // 3. 포맷 및 사이즈 설정
    // 중요: PBO 읽기는 호환성을 위해 무조건 4채널(RGBA)로 수행
    int pixelCount = m_Width * m_Height;
    int srcChannels = 4; // PBO 저장용 (RGBA)
    int dstChannels = (m_Format == TextureFormat::RGBA8) ? 4 : 3; // 사용자 반환용
    
    // 결과 버퍼 크기 확보
    if (m_CachedData.size() != pixelCount * dstChannels) {
        m_CachedData.resize(pixelCount * dstChannels);
    }

    // 인덱스 설정 (Ping-Pong)
    int writeIndex = m_PBOIndex;           // 이번에 GPU가 쓸 버퍼
    int readIndex = (m_PBOIndex + 1) % 2;  // 이번에 CPU가 읽을 버퍼 (이전 프레임)

    // --- STEP 1: GPU에게 캡처 명령 (비동기 Write) ---
    
    if (s_ReadFBO == 0) glGenFramebuffers(1, &s_ReadFBO);
    
    GLint lastFBO;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &lastFBO);

    glBindFramebuffer(GL_FRAMEBUFFER, s_ReadFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_RendererID, 0);

    glBindBuffer(GL_PIXEL_PACK_BUFFER, m_PBOs[writeIndex]);
    
    // [중요] 1바이트 정렬 (데이터 밀림 방지)
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    // [중요] GL_RGBA 포맷으로 읽기 (대부분의 드라이버 호환성 확보)
    const auto submitStart = robotpal::benchmark::SteadyNowNs();
    glReadPixels(0, 0, m_Width, m_Height, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    robotpal::benchmark::Event("readback_pbo_submit", currentFrameIndex,
        robotpal::benchmark::SteadyNowNs() - submitStart, pixelCount * srcChannels);

    glBindFramebuffer(GL_FRAMEBUFFER, lastFBO);

    // --- STEP 2: 이전 프레임 데이터 가져오기 (CPU Read) ---
    
    glBindBuffer(GL_PIXEL_PACK_BUFFER, m_PBOs[readIndex]);
    
    // 맵핑 (이전 프레임 데이터라 대기 시간 거의 없음)
    const auto mapStart = robotpal::benchmark::SteadyNowNs();
    uint8_t* ptr = (uint8_t*)glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, pixelCount * srcChannels, GL_MAP_READ_BIT);
    robotpal::benchmark::Event("readback_pbo_map", currentFrameIndex,
        robotpal::benchmark::SteadyNowNs() - mapStart, pixelCount * srcChannels);
    
    if (ptr) {
        const auto copyStart = robotpal::benchmark::SteadyNowNs();
        if (dstChannels == 4) {
            // RGBA 그대로 복사
            memcpy(m_CachedData.data(), ptr, pixelCount * srcChannels);
        } 
        else {
            // RGBA -> RGB 변환 (알파 제거)
            uint8_t* dst = m_CachedData.data();
            for (int i = 0; i < pixelCount; ++i) {
                dst[i*3 + 0] = ptr[i*4 + 0]; // R
                dst[i*3 + 1] = ptr[i*4 + 1]; // G
                dst[i*3 + 2] = ptr[i*4 + 2]; // B
            }
        }
        robotpal::benchmark::Event("readback_pbo_copy", currentFrameIndex,
            robotpal::benchmark::SteadyNowNs() - copyStart, pixelCount * dstChannels);
        glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    } 
    else {
        // 첫 프레임 등 데이터 없을 때: 검은색 유지
    }

    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0); // PBO 언바인딩
    glPixelStorei(GL_PACK_ALIGNMENT, 4);   // 정렬 복구

    // 상태 업데이트
    m_PBOIndex = (m_PBOIndex + 1) % 2; // 다음 턴을 위해 스왑
    m_LastUpdateFrame = currentFrameIndex;

    return m_CachedData;
#endif
}


void Texture::InitPBOs() {
    // [중요] GetAsyncData에서 GL_RGBA(4채널)로 읽으므로 버퍼도 4채널 크기로 확보해야 함
    int channels = 4;
    int dataSize = m_Width * m_Height * channels;

    glGenBuffers(2, m_PBOs);
    for (int i = 0; i < 2; i++) {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, m_PBOs[i]);
        // GL_STREAM_READ: GPU가 쓰고 CPU가 읽는 스트리밍 패턴 최적화
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
    } else if (m_Format == TextureFormat::RGBA16F) {
        internalFormat = GL_RGBA16F; dataFormat = GL_RGBA; dataType = GL_FLOAT;
    }
    else if (m_Format == TextureFormat::RED_INTEGER) {
        // 호환성을 위해 RGBA 4채널 정수 포맷 사용
        internalFormat = GL_RGBA32I;        
        dataFormat = GL_RGBA_INTEGER;     
        dataType = GL_INT;               
    }
    else if (m_Format == TextureFormat::DEPTH24_STENCIL8) {
        internalFormat = GL_DEPTH24_STENCIL8; dataFormat = GL_DEPTH_STENCIL; dataType = GL_UNSIGNED_INT_24_8;
    }

    GLenum target = (m_Type == TextureType::TextureCube) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
    glBindTexture(target, m_RendererID);

    if (m_Type == TextureType::Texture2D) {
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Width, m_Height, 0, dataFormat, dataType, nullptr);
        
        if (m_Format == TextureFormat::RED_INTEGER) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        } else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
        
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
