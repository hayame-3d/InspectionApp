#include "Renderer.h"
#include "Logger.h"
#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

namespace InspectionApp {

    Renderer::Renderer() = default;
    Renderer::~Renderer() { Shutdown(); }

    void Renderer::Init() {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_PROGRAM_POINT_SIZE);
        glPointSize(2.0f);
        SetClearColor(0.15f, 0.15f, 0.15f, 1.0f);
        m_gridShader = std::make_unique<Shader>("resources/shaders/grid.vert", "resources/shaders/grid.frag");
        m_pointShader = std::make_unique<Shader>("resources/shaders/point.vert", "resources/shaders/point.frag");
        m_grid.spacing = 1.0f;
        m_grid.lines = 40;
        GenerateGrid();
        LOG_INFO("Renderer initialized (OpenGL 4.6 Core)");
    }

    void Renderer::Shutdown() {
        if (m_gridVAO) { glDeleteVertexArrays(1, &m_gridVAO); m_gridVAO = 0; }
        if (m_gridVBO) { glDeleteBuffers(1, &m_gridVBO); m_gridVBO = 0; }
        m_gridShader.reset();
        m_pointShader.reset();
    }

    void Renderer::SetClearColor(float r, float g, float b, float a) { glClearColor(r, g, b, a); }
    void Renderer::BeginFrame(const Camera& camera) { glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); }
    void Renderer::EndFrame() {}

    void Renderer::DrawGrid(const Camera& camera) {
        if (!m_gridShader || !m_gridVAO || !m_grid.visible || m_gridVertexCount == 0) return;
        m_gridShader->Bind();
        m_gridShader->SetMat4("u_PV", glm::value_ptr(camera.GetPV()));
        glBindVertexArray(m_gridVAO);
        glDrawArrays(GL_LINES, 0, m_gridVertexCount);
        glBindVertexArray(0);
        m_gridShader->Unbind();
    }

    void Renderer::GenerateGrid() {
        if (m_gridVAO) { glDeleteVertexArrays(1, &m_gridVAO); m_gridVAO = 0; }
        if (m_gridVBO) { glDeleteBuffers(1, &m_gridVBO); m_gridVBO = 0; }

        std::vector<float> v;
        int half = m_grid.lines / 2;
        float extent = half * m_grid.spacing;

        glm::mat4 R = glm::rotate(glm::mat4(1.0f), glm::radians(m_grid.rotation.z), glm::vec3(0, 0, 1))
            * glm::rotate(glm::mat4(1.0f), glm::radians(m_grid.rotation.y), glm::vec3(0, 1, 0))
            * glm::rotate(glm::mat4(1.0f), glm::radians(m_grid.rotation.x), glm::vec3(1, 0, 0));
        glm::mat4 M = glm::translate(glm::mat4(1.0f), m_grid.offset) * R;

        auto pushLine = [&](const glm::vec3& a, const glm::vec3& b, float r, float g_, float b_) {
            glm::vec4 ta = M * glm::vec4(a, 1.0f);
            glm::vec4 tb = M * glm::vec4(b, 1.0f);
            v.push_back(ta.x); v.push_back(ta.y); v.push_back(ta.z);
            v.push_back(r);    v.push_back(g_);   v.push_back(b_);
            v.push_back(tb.x); v.push_back(tb.y); v.push_back(tb.z);
            v.push_back(r);    v.push_back(g_);   v.push_back(b_);
            };

        for (int i = -half; i <= half; ++i) {
            float p = i * m_grid.spacing;
            pushLine(glm::vec3(-extent, p, 0), glm::vec3(extent, p, 0), 0.25f, 0.25f, 0.25f);
            pushLine(glm::vec3(p, -extent, 0), glm::vec3(p, extent, 0), 0.25f, 0.25f, 0.25f);
        }

        m_gridVertexCount = static_cast<u32>(v.size() / 6);
        glGenVertexArrays(1, &m_gridVAO); glGenBuffers(1, &m_gridVBO);
        glBindVertexArray(m_gridVAO); glBindBuffer(GL_ARRAY_BUFFER, m_gridVBO);
        glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);
    }

    void Renderer::DrawPointCloudVAO(u32 vao, u32 pointCount, const Camera& camera,
        const glm::vec3& cloudOffset, int heatmapMode, int planeAxis, int planeEdgeMode,
        float planeOffset, float planeRange, float minIntensity, float maxIntensity,
        int pointDensity, float pointSize) {
        if (!m_pointShader || !vao || pointCount == 0) return;
        m_pointShader->Bind();
        m_pointShader->SetMat4("u_PV", glm::value_ptr(camera.GetPV()));
        m_pointShader->SetVec3("u_CloudOffset", cloudOffset);
        m_pointShader->SetInt("u_HeatmapMode", heatmapMode);
        m_pointShader->SetInt("u_PlaneAxis", planeAxis);
        m_pointShader->SetInt("u_PlaneEdgeMode", planeEdgeMode);
        m_pointShader->SetFloat("u_PlaneOffset", planeOffset);
        m_pointShader->SetFloat("u_PlaneRange", planeRange);
        m_pointShader->SetFloat("u_MinIntensity", minIntensity);
        m_pointShader->SetFloat("u_MaxIntensity", maxIntensity);
        m_pointShader->SetInt("u_PointDensity", pointDensity);
        m_pointShader->SetFloat("u_PointSize", pointSize);
        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, pointCount);
        glBindVertexArray(0);
        m_pointShader->Unbind();
    }

    void Renderer::DrawAnnotations(const std::vector<glm::vec3>& positions, const Camera& camera) {
        if (positions.empty()) return;
        struct AnnotVertex { glm::vec3 pos; glm::u8vec3 color{ 255, 50, 50 }; };
        std::vector<AnnotVertex> verts;
        verts.reserve(positions.size());
        for (const auto& p : positions) verts.push_back({ p, {255, 50, 50} });
        u32 vao, vbo;
        glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo);
        glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(AnnotVertex), verts.data(), GL_STREAM_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(AnnotVertex), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(AnnotVertex), (void*)offsetof(AnnotVertex, color));
        glEnableVertexAttribArray(1);
        m_pointShader->Bind();
        m_pointShader->SetMat4("u_PV", glm::value_ptr(camera.GetPV()));
        m_pointShader->SetVec3("u_CloudOffset", glm::vec3(0.0f));
        m_pointShader->SetInt("u_HeatmapMode", 0);
        m_pointShader->SetInt("u_PlaneAxis", 2);
        m_pointShader->SetInt("u_PlaneEdgeMode", 0);
        m_pointShader->SetFloat("u_PlaneOffset", 0.0f);
        m_pointShader->SetFloat("u_PlaneRange", 1.0f);
        m_pointShader->SetFloat("u_MinIntensity", 0.0f);
        m_pointShader->SetFloat("u_MaxIntensity", 1.0f);
        m_pointShader->SetInt("u_PointDensity", 1);
        m_pointShader->SetFloat("u_PointSize", 10.0f);
        glPointSize(10.0f);
        glDrawArrays(GL_POINTS, 0, static_cast<u32>(verts.size()));
        glPointSize(2.0f);
        glBindVertexArray(0);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
        m_pointShader->Unbind();
    }

}