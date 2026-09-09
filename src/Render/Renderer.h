#pragma once
#include "Shader.h"
#include "Camera.h"
#include "Types.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace InspectionApp {

    struct GridConfig {
        bool visible = true;
        bool locked = false;
        float spacing = 1.0f;
        int lines = 40;
        glm::vec3 offset = glm::vec3(0.0f);
        glm::vec3 rotation = glm::vec3(0.0f); // grados
    };

    class Renderer {
    public:
        Renderer();
        ~Renderer();

        void Init();
        void Shutdown();
        void BeginFrame(const Camera& camera);
        void EndFrame();
        void SetClearColor(float r, float g, float b, float a);
        void DrawGrid(const Camera& camera);
        void GenerateGrid();
        GridConfig& GetGridConfig() { return m_grid; }

        void DrawPointCloudVAO(u32 vao, u32 pointCount, const Camera& camera,
            const glm::vec3& cloudOffset, int heatmapMode,
            int planeAxis, int planeEdgeMode,
            float planeOffset, float planeRange,
            float minIntensity, float maxIntensity,
            int pointDensity = 1,
            float pointSize = 0.5f);
        void DrawAnnotations(const std::vector<glm::vec3>& positions, const Camera& camera);

    private:
        std::unique_ptr<Shader> m_gridShader;
        std::unique_ptr<Shader> m_pointShader;
        GridConfig m_grid;
        u32 m_gridVBO = 0, m_gridVAO = 0;
        u32 m_gridVertexCount = 0;
    };

} // namespace InspectionApp