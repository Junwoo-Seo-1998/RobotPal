#ifndef __SHADER_H__
#define __SHADER_H__

#include <string>
#include <unordered_map>
#include <memory>

// Forward declaration for GLM types. If you use GLM, include its headers here.
// For now, we'll use basic types and pointers.
namespace glm {
    struct vec3;
    struct vec4;
    struct mat4;
}

class Shader
{
public:
    Shader(const std::string& filepath);
    Shader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);
    ~Shader();

    // Creates a shader program from vertex and fragment shader files.
    static std::shared_ptr<Shader> Create(const std::string& filepath);
    // Creates a shader program from vertex and fragment shader source strings.
    static std::shared_ptr<Shader> CreateFromSource(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);

    void Bind() const;
    void UnBind() const;

    // Uniform setters
    void SetInt(const std::string& name, int value);
    void SetIntArray(const std::string& name, int* values, uint32_t count);
    void SetFloat(const std::string& name, float value);
    void SetFloat2(const std::string& name, float v0, float v1);
    void SetFloat3(const std::string& name, float v0, float v1, float v2);
    void SetFloat4(const std::string& name, float v0, float v1, float v2, float v3);
    
    // Use this for glm::vec3, glm::vec4 etc. by passing glm::value_ptr(your_matrix)
    void SetMat3(const std::string& name, const float* matrix);
    void SetMat4(const std::string& name, const float* matrix);

    const std::string& GetName() const { return m_Name; }

private:
    std::string ReadFile(const std::string& filepath);
    std::unordered_map<unsigned int, std::string> PreProcess(const std::string& source);
    void Compile(const std::unordered_map<unsigned int, std::string>& shaderSources);
    int GetUniformLocation(const std::string& name) const;

private:
    uint32_t m_RendererID;
    std::string m_Name;
    mutable std::unordered_map<std::string, int> m_UniformLocationCache;
};

#endif