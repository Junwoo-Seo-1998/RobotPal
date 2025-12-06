#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include <glad/gles2.h> // 또는 사용하는 OpenGL 로더
#include <vector>
#include <cstdint>      // uint64_t
#include <memory>
#include <string>

// 텍스처 포맷 정의
enum class TextureFormat {
    RGB8,
    RGBA8,
    RGBA16F,
    DEPTH24_STENCIL8 // 섀도우 맵, 뎁스 버퍼용
};

// 텍스처 타입 정의
enum class TextureType {
    Texture2D,
    TextureCube // 스카이박스, 옴니 섀도우용
};

class Texture {
public:
    // [생성자 1] 빈 텍스처 (FBO 연결용)
    Texture(int width, int height, TextureFormat format = TextureFormat::RGB8, TextureType type = TextureType::Texture2D);

    // [생성자 2] 데이터가 있는 텍스처 (이미지 로드용)
    Texture(int width, int height, const void* data, TextureFormat format = TextureFormat::RGBA8);

    ~Texture();

    // 텍스처 바인딩 (slot: 0 ~ 31)
    void Bind(int slot = 0) const;
    void Unbind() const;

    // 크기 변경 (FBO 리사이즈 시 사용)
    void Resize(int width, int height);

    // [CPU -> GPU] 데이터 업로드 (일반 2D)
    void SetData(const void* data, int size);

    // [CPU -> GPU] 큐브맵 데이터 업로드
    void SetCubeMapData(const std::vector<void*>& faces);

    // ---------------------------------------------------------
    // [GPU -> CPU] 비동기 데이터 읽기 (PBO 사용)
    // currentFrameIndex: 현재 프레임 번호 (중복 호출 방지용)
    // ---------------------------------------------------------
    std::vector<uint8_t> GetAsyncData(uint64_t currentFrameIndex);

    // Getters
    unsigned int GetID() const { return m_RendererID; }
    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }
    TextureFormat GetFormat() const { return m_Format; }
    TextureType GetType() const { return m_Type; }

    // 앱 종료 시 정적 리소스(공유 FBO) 정리
    static void CleanUpStaticResources();

private:
    void CreateInternal();
    void InitPBOs(); // PBO 초기화 함수

    unsigned int m_RendererID = 0;
    int m_Width, m_Height;
    TextureFormat m_Format;
    TextureType m_Type;

    // --- PBO(Pixel Buffer Object) 관련 ---
    unsigned int m_PBOs[2] = {0, 0}; 
    int m_PBOIndex = 0;              
    bool m_UsePBO = false;           

    // --- 프레임 캐싱 (중복 호출 및 Stall 방지) ---
    std::vector<uint8_t> m_CachedData;
    uint64_t m_LastUpdateFrame = 0;

    // --- Readback FBO 관련 ---
    static unsigned int s_ReadFBO; 
};

#endif