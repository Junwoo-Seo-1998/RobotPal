// Texture.h
#ifndef __TEXTURE_H__
#define __TEXTURE_H__
#include <glad/gles2.h>
#include <vector>
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

    // [CPU -> GPU] 큐브맵 데이터 업로드 (순서: Right, Left, Top, Bottom, Front, Back)
    void SetCubeMapData(const std::vector<void*>& faces);

    // [GPU -> CPU] 비동기 데이터 읽기 (PBO 사용, Non-blocking)
    // 네트워크 전송 시 이 함수를 사용하세요. (1프레임 지연 있음)
    std::vector<uint8_t> GetAsyncData();

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
    void InitPBOs(); // PBO 지연 초기화

    unsigned int m_RendererID = 0;
    int m_Width, m_Height;
    TextureFormat m_Format;
    TextureType m_Type;

    // --- PBO(Pixel Buffer Object) 관련 ---
    // 비동기 전송을 위해 인스턴스별로 소유 (공유 불가)
    unsigned int m_PBOs[2] = {0, 0}; 
    int m_PBOIndex = 0;              
    bool m_UsePBO = false;           

    // --- Readback FBO 관련 ---
    // 읽기 작업을 위한 임시 FBO는 전역 공유 (메모리 절약)
    static unsigned int s_ReadFBO; 
};

#endif