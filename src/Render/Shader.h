#pragma once
#include "Types.h"
#include <glad/gl.h>
#include <string>
#include <glm/glm.hpp>

namespace InspectionApp {

    class Shader {
    public:
        Shader(const std::string& vertexPath, const std::string& fragmentPath);
        ~Shader();

        void Bind();
        void Unbind();

        void SetInt(const std::string& name, int value);
        void SetBool(const std::string& name, bool value);
        void SetFloat(const std::string& name, float value);
        void SetVec3(const std::string& name, float x, float y, float z);
        void SetVec3(const std::string& name, const glm::vec3& value);
        void SetMat4(const std::string& name, const float* mat);

    private:
        u32 m_id;
        std::string LoadFromFile(const std::string& path);
        u32 Compile(GLenum type, const std::string& source);
    };

} // namespace InspectionApp