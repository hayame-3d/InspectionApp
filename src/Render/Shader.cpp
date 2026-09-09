#include "Shader.h"
#include "Logger.h"
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <fstream>
#include <sstream>

namespace InspectionApp {

    Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath) {
        std::string vertexCode = LoadFromFile(vertexPath);
        std::string fragmentCode = LoadFromFile(fragmentPath);

        u32 vertex = Compile(GL_VERTEX_SHADER, vertexCode);
        u32 fragment = Compile(GL_FRAGMENT_SHADER, fragmentCode);

        m_id = glCreateProgram();
        glAttachShader(m_id, vertex);
        glAttachShader(m_id, fragment);
        glLinkProgram(m_id);

        int success;
        glGetProgramiv(m_id, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetProgramInfoLog(m_id, 512, nullptr, infoLog);
            LOG_ERROR(std::string("Shader link error: ") + infoLog);
        }

        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    Shader::~Shader() {
        glDeleteProgram(m_id);
    }

    void Shader::Bind() { glUseProgram(m_id); }
    void Shader::Unbind() { glUseProgram(0); }

    void Shader::SetInt(const std::string& name, int value) {
        glUniform1i(glGetUniformLocation(m_id, name.c_str()), value);
    }

    void Shader::SetBool(const std::string& name, bool value) {
        glUniform1i(glGetUniformLocation(m_id, name.c_str()), value ? 1 : 0);
    }

    void Shader::SetFloat(const std::string& name, float value) {
        glUniform1f(glGetUniformLocation(m_id, name.c_str()), value);
    }

    void Shader::SetVec3(const std::string& name, float x, float y, float z) {
        glUniform3f(glGetUniformLocation(m_id, name.c_str()), x, y, z);
    }

    void Shader::SetVec3(const std::string& name, const glm::vec3& value) {
        glUniform3f(glGetUniformLocation(m_id, name.c_str()), value.x, value.y, value.z);
    }

    void Shader::SetMat4(const std::string& name, const float* mat) {
        glUniformMatrix4fv(glGetUniformLocation(m_id, name.c_str()), 1, GL_FALSE, mat);
    }

    std::string Shader::LoadFromFile(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            LOG_ERROR("Failed to open shader: " + path);
            return "";
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    u32 Shader::Compile(GLenum type, const std::string& source) {
        u32 id = glCreateShader(type);
        const char* src = source.c_str();
        glShaderSource(id, 1, &src, nullptr);
        glCompileShader(id);

        int success;
        glGetShaderiv(id, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(id, 512, nullptr, infoLog);
            LOG_ERROR(std::string("Shader compile error: ") + infoLog);
        }
        return id;
    }

} // namespace InspectionApp