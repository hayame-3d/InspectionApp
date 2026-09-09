#include "Application.h"
#include "Logger.h"
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#include <cstdio>
#include <thread>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <cstring> // para std::memcpy en export de imagen

// ImGui backends (solo en el .cpp)
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// stb_image_write para exportar PNG
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../include/stb_image_write.h"
#define NOMINMAX // evita que windows.h defina macros min/max que rompen std::max
#include "../include/portable-file-dialogs.h"

namespace InspectionApp {
    Application::Application() { Init(); }
    Application::~Application() { Shutdown(); }

    void Application::Init() {
        Logger::Instance().Init("InspectionApp.log");
        LOG_INFO("InspectionApp v0.4.2 - Tags Overlay + PBO Pick");

        m_window = std::make_unique<Window>(WindowProps{ "InspectionApp", 1600, 900 });
        m_window->SetEventCallback([this](int key, int action) { OnEvent(key, action); });
        m_window->SetScrollCallback([this](float yoffset) { OnScroll(yoffset); });
        m_window->SetDropCallback([this](int count, const char** paths) { OnDrop(count, paths); });

        m_renderer = std::make_unique<Renderer>();
        m_renderer->Init();

        float aspect = m_window->GetAspectRatio();
        float orthoSize = 50.0f;
        m_camera = std::make_unique<Camera>(
            -orthoSize * aspect, orthoSize * aspect,
            -orthoSize, orthoSize,
            -1000.0f, 1000.0f
        );
        m_camera->SetView(OrthoView::Top);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(static_cast<GLFWwindow*>(m_window->GetNativeWindow()), true);
        ImGui_ImplOpenGL3_Init("#version 460");

        // -- PBO para lectura asincrona de depth buffer --
        glGenBuffers(1, &m_pickPBO);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pickPBO);
        glBufferData(GL_PIXEL_PACK_BUFFER, 9 * sizeof(float), nullptr, GL_STREAM_READ);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

        LOG_INFO("Controls: MiddleDrag=pan | Alt+WASD/QE=cloud | 1-4=step | H=mode | X=axis | V=edge | T=tag-axis");
        LOG_INFO("          Shift+Q/E=Offset | Shift+W/S=Range | Shift+F=Fit | LClick=tag | C=clear");
        LOG_INFO("          Shift++/- = tag font size | NavCube drag=orbit");

        // Fecha por defecto para export
        auto now = std::chrono::system_clock::now();
        auto tt = std::chrono::system_clock::to_time_t(now);
        std::tm localTm;
#ifdef _WIN32
        localtime_s(&localTm, &tt);
#else
        localtime_r(&tt, &localTm);
#endif
        std::snprintf(m_exportDate, sizeof(m_exportDate), "%04d-%02d-%02d",
            localTm.tm_year + 1900, localTm.tm_mon + 1, localTm.tm_mday);
    }

    void Application::Shutdown() {
        LOG_INFO("Shutting down...");
        if (m_pickPBO) { glDeleteBuffers(1, &m_pickPBO); m_pickPBO = 0; }

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        if (m_cloudVAO) { glDeleteVertexArrays(1, &m_cloudVAO); m_cloudVAO = 0; }
        if (m_cloudVBO) { glDeleteBuffers(1, &m_cloudVBO); m_cloudVBO = 0; }
        m_renderer->Shutdown();
    }

    void Application::Run() {
        auto lastTime = std::chrono::high_resolution_clock::now();
        while (m_running && !m_window->ShouldClose()) {
            auto frameStart = std::chrono::high_resolution_clock::now();
            auto now = std::chrono::high_resolution_clock::now();
            float dt = std::chrono::duration<float>(now - lastTime).count();
            lastTime = now;

            OnUpdate(dt);
            OnRender();
            m_window->OnUpdate();

            auto frameEnd = std::chrono::high_resolution_clock::now();
            auto elapsedMs = std::chrono::duration<float, std::milli>(frameEnd - frameStart).count();
            const float targetFrameTime = 16.666f;
            if (elapsedMs < targetFrameTime) {
                std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(targetFrameTime - elapsedMs)));
            }
        }
    }

    void Application::OnUpdate(float dt) {
        GLFWwindow* native = static_cast<GLFWwindow*>(m_window->GetNativeWindow());

        // -- Resize handling --
        static u32 s_lastW = 0, s_lastH = 0;
        u32 currW = m_window->GetWidth();
        u32 currH = m_window->GetHeight();
        if (currW != s_lastW || currH != s_lastH) {
            if (s_lastW != 0) {
                m_camera->SetAspectRatio(m_window->GetAspectRatio());
                LOG_INFO("Window resized: " + std::to_string(currW) + "x" + std::to_string(currH));
            }
            s_lastW = currW; s_lastH = currH;
        }

        m_fpsTimer += dt; ++m_frameCount;
        if (m_fpsTimer >= 0.5f) {
            m_currentFPS = static_cast<u32>(m_frameCount / m_fpsTimer);
            m_frameCount = 0; m_fpsTimer = 0.0f;
        }

        double mx, my;
        glfwGetCursorPos(native, &mx, &my);
        float currX = static_cast<float>(mx);
        float currY = static_cast<float>(my);

        // === BLOQUEAR MOUSE SI IMGUI LO ESTA USANDO ===
        bool imguiWantsMouse = ImGui::GetIO().WantCaptureMouse;

        bool overNavCube = m_navCube.IsOver(glm::vec2(currX, currY));

        if (!imguiWantsMouse) {
            if (glfwGetMouseButton(native, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
                if (m_mouseMiddlePressed) {
                    float dx = currX - m_lastMouseX;
                    float dy = currY - m_lastMouseY;
                    if (dx != 0.0f || dy != 0.0f) {
                        float worldPerPixel = (m_camera->GetFrustumHeight() / m_window->GetHeight()) / m_camera->GetZoom();
                        glm::vec3 right = m_camera->GetRightVector();
                        glm::vec3 up = m_camera->GetUpVector();
                        float moveX = dx * worldPerPixel;
                        float moveY = -dy * worldPerPixel;
                        m_camera->AddOffset(
                            right.x * moveX - up.x * moveY,
                            right.y * moveX - up.y * moveY,
                            right.z * moveX - up.z * moveY
                        );
                    }
                }
                else { m_mouseMiddlePressed = true; }
                m_lastMouseX = currX; m_lastMouseY = currY;
            }
            else { m_mouseMiddlePressed = false; }

            // --- LEFT BUTTON: NavCube solo orbita, no cambia vistas ---
            if (glfwGetMouseButton(native, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                if (!m_mouseLeftPressed) {
                    m_mouseLeftPressed = true;
                    if (overNavCube) {
                        m_navCubeActive = true;
                        m_navCube.BeginDrag(m_camera->GetYaw(), m_camera->GetPitch(), glm::vec2(currX, currY));
                    }
                    else {
                        m_pendingPick = true;
                        m_pickX = currX; m_pickY = currY;
                    }
                }
                else if (m_navCubeActive) {
                    float newYaw = m_camera->GetYaw();
                    float newPitch = m_camera->GetPitch();
                    if (m_navCube.UpdateDrag(glm::vec2(currX, currY), newYaw, newPitch)) {
                        m_camera->SetYaw(newYaw);
                        m_camera->SetPitch(newPitch);
                    }
                }
            }
            else {
                if (m_mouseLeftPressed && m_navCubeActive) {
                    m_navCube.EndDrag(); // solo termina el drag, no hay pick de caras
                    m_navCubeActive = false;
                }
                m_mouseLeftPressed = false;
            }
        }
        else {
            m_mouseMiddlePressed = false;
            m_mouseLeftPressed = false;
            m_pendingPick = false;
            m_navCubeActive = false;
            if (m_navCube.isDragging) {
                m_navCube.isDragging = false;
                m_navCube.wasDragged = false;
            }
        }


        bool shiftHeld = glfwGetKey(native, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(native, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
        bool altHeld = glfwGetKey(native, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(native, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;


        UpdateTitle();
    }

    void Application::AutoFitPlane() {
        if (!m_pointCloud || !m_pointCloud->GetBounds().IsValid()) return;
        const auto& pts = m_pointCloud->GetPoints();
        if (pts.empty()) return;
        double sum = 0.0;
        for (const auto& p : pts) {
            float v = (m_planeAxis == 0) ? p.position.x : ((m_planeAxis == 1) ? p.position.y : p.position.z);
            sum += v;
        }
        double mean = sum / pts.size();
        double sqSum = 0.0;
        for (const auto& p : pts) {
            float v = (m_planeAxis == 0) ? p.position.x : ((m_planeAxis == 1) ? p.position.y : p.position.z);
            double d = v - mean;
            sqSum += d * d;
        }
        double stddev = std::sqrt(sqSum / pts.size());
        float cloudComp = (m_planeAxis == 0) ? m_cloudOffset.x : ((m_planeAxis == 1) ? m_cloudOffset.y : m_cloudOffset.z);
        m_planeOffset = static_cast<float>(mean) + cloudComp;
        m_planeRange = static_cast<float>(stddev * 3.0);
        if (std::abs(m_planeRange) < 0.001f) m_planeRange = 0.001f;
        LOG_INFO("Auto-fit: Axis=" + std::string(m_planeAxis == 0 ? "X" : (m_planeAxis == 1 ? "Y" : "Z")) +
            " Offset=" + std::to_string(m_planeOffset) + " Range=" + std::to_string(m_planeRange));
    }

    void Application::PlaceAnnotation(float mouseX, float mouseY) {
        if (!m_pointCloud || m_pickPhase != 0) return;

        int ww = static_cast<int>(m_window->GetWidth());
        int wh = static_cast<int>(m_window->GetHeight());

        int cx = static_cast<int>(mouseX);
        int cy = wh - static_cast<int>(mouseY);

        if (cx < 1) cx = 1;
        if (cy < 1) cy = 1;
        if (cx >= ww - 1) cx = ww - 2;
        if (cy >= wh - 1) cy = wh - 2;

        glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pickPBO);
        glReadPixels(cx - 1, cy - 1, 3, 3, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

        m_pickPendingX = mouseX;
        m_pickPendingY = mouseY;
        m_pickPendingPV = m_camera->GetPV();
        m_pickPhase = 1;
    }

    void Application::OnRender() {
        if (m_pickPhase == 1) {
            glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pickPBO);
            float* ptr = (float*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
            if (ptr) {
                float depth = 1.0f;
                bool found = false;
                for (int i = 0; i < 9; ++i) {
                    if (ptr[i] < 0.999f) {
                        if (!found || ptr[i] < depth) {
                            depth = ptr[i];
                            found = true;
                        }
                    }
                }
                glUnmapBuffer(GL_PIXEL_PACK_BUFFER);

                float ndcX = (2.0f * m_pickPendingX) / m_window->GetWidth() - 1.0f;
                float ndcY = 1.0f - (2.0f * m_pickPendingY) / m_window->GetHeight();
                float ndcZ = 2.0f * depth - 1.0f;

                glm::mat4 invPV = glm::inverse(m_pickPendingPV);
                glm::vec4 world = invPV * glm::vec4(ndcX, ndcY, ndcZ, 1.0f);
                world /= world.w;
                glm::vec3 pos(world.x, world.y, world.z);

                if (!found && m_pointCloud && m_pointCloud->GetBounds().IsValid()) {
                    if (m_camera->GetCurrentView() == OrthoView::Top || m_camera->GetCurrentView() == OrthoView::Bottom)
                        pos.z = m_planeOffset;
                    else if (m_camera->GetCurrentView() == OrthoView::Front || m_camera->GetCurrentView() == OrthoView::Back)
                        pos.y = m_planeOffset;
                    else
                        pos.x = m_planeOffset;
                }

                float planeCoord = 0.0f;
                switch (m_planeAxis) { case 0: planeCoord = pos.x; break; case 1: planeCoord = pos.y; break; case 2: planeCoord = pos.z; break; }
                                             float deviation = (planeCoord - m_planeOffset) * 1000.0f;

                                             Annotation ann;
                                             ann.position = pos;
                                             ann.deviation = deviation;
                                             ann.tagAxis = m_tagAxis;

                                             float coord = 0.0f;
                                             switch (m_tagAxis) {
                                             case 0: coord = pos.x; break;
                                             case 1: coord = pos.y; break;
                                             case 2: coord = pos.z; break;
                                             }
                                             float scaled = coord * m_tagScaleFactor;
                                             char buf[64];
                                             std::snprintf(buf, sizeof(buf), "%.0f", scaled);
                                             ann.label = buf;

                                             m_annotations.push_back(ann);
                                             LOG_INFO("Tag creado en (%.3f, %.3f, %.3f) %s", pos.x, pos.y, pos.z, found ? "[pick]" : "[plano]");
            }
            glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
            m_pickPhase = 0;
        }

        m_renderer->BeginFrame(*m_camera);
        m_renderer->DrawGrid(*m_camera);

        if (m_cloudDirty && m_pointCloud) { UploadPointCloud(); m_cloudDirty = false; }

        if (m_cloudVAO && m_pointCloud && m_pointCloud->GetPointCount() > 0) {
            m_renderer->DrawPointCloudVAO(m_cloudVAO, static_cast<u32>(m_pointCloud->GetPointCount()), *m_camera,
                m_cloudOffset, m_heatmapMode, m_planeAxis, m_planeEdgeMode, m_planeOffset, m_planeRange,
                m_minIntensity, m_maxIntensity, m_pointDensity, m_pointSize);
        }

        // NOTA: Puntos rojos de anotaciones eliminados a pedido del usuario.
        // Solo se muestran los tags flotantes de texto.

        if (m_pendingPick) { PlaceAnnotation(m_pickX, m_pickY); m_pendingPick = false; }
        m_renderer->EndFrame();
        RenderTagsOverlay();

        // === EXPORTAR IMAGEN (offscreen, despues del frame visible) ===
        if (m_pendingExport) {
            CaptureImage(m_exportImageWidth, m_exportImageHeight, m_exportTransparent, std::string(m_exportImagePath));
            m_pendingExport = false;
        }
    }

    // === RAMPA (sincronizada con shader) ===
    static ImU32 SampleRampColor(float t) {
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;

        struct Stop { float r, g, b; };
        static const Stop stops[9] = {
            {0.00f, 0.00f, 1.00f},   // azul
            {0.00f, 1.00f, 1.00f},   // cyan
            {0.00f, 1.00f, 0.00f},   // verde
            {1.00f, 1.00f, 0.00f},   // amarillo
            {1.00f, 0.50f, 0.00f},   // naranja
            {1.00f, 0.00f, 0.00f},   // rojo
            {1.00f, 0.00f, 1.00f},   // magenta
            {0.50f, 0.00f, 1.00f},   // morado
            {0.00f, 0.00f, 1.00f}    // azul
        };

        float idx = t * 8.0f;
        int i = (int)floorf(idx);
        if (i < 0) i = 0;
        if (i > 7) i = 7;
        int j = i + 1;
        float f = idx - (float)i;

        float r = stops[i].r + (stops[j].r - stops[i].r) * f;
        float g = stops[i].g + (stops[j].g - stops[i].g) * f;
        float b = stops[i].b + (stops[j].b - stops[i].b) * f;

        return IM_COL32((int)(r * 255), (int)(g * 255), (int)(b * 255), 255);
    }

    void Application::RenderColorBar() {
        if (m_heatmapMode == 0) return;

        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        if (!dl) return;

        ImVec2 canvas = ImGui::GetIO().DisplaySize;
        float barW = 22.0f;
        float barH = 220.0f;
        float textW = 55.0f;
        float padT = 10.0f;

        ImVec2 pos(canvas.x - 90.0f - padT - barW - textW, canvas.y * 0.5f - barH * 0.5f);

        dl->AddRectFilled(
            ImVec2(pos.x - 4, pos.y - 24),
            ImVec2(pos.x + barW + textW + 4, pos.y + barH + 4),
            IM_COL32(20, 20, 20, 180),
            4.0f
        );

        // Arriba t=0 (azul), centro t=0.5 (rojo), abajo t=1 (azul)
        for (int i = 0; i < (int)barH; ++i) {
            float t = (float)i / barH;
            dl->AddLine(
                ImVec2(pos.x, pos.y + i),
                ImVec2(pos.x + barW, pos.y + i),
                SampleRampColor(t), 1.0f
            );
        }

        dl->AddRect(
            ImVec2(pos.x, pos.y),
            ImVec2(pos.x + barW, pos.y + barH),
            IM_COL32(255, 255, 255, 100),
            0.0f, 0, 1.0f
        );

        char buf[32];
        int ticks = 5;
        for (int i = 0; i <= ticks; ++i) {
            float fy = pos.y + barH * ((float)i / ticks);
            dl->AddLine(ImVec2(pos.x + barW, fy), ImVec2(pos.x + barW + 5, fy), IM_COL32(255, 255, 255, 200), 1.0f);

            float value;
            if (m_heatmapMode == 2) {
                float halfRange = m_planeRange * 0.5f;
                float center = m_planeOffset;
                float v = center + halfRange * (1.0f - 2.0f * (float)i / ticks);
                value = v * 1000.0f;
                std::snprintf(buf, sizeof(buf), "%+.1f", value);
            }
            else {
                float v = m_minIntensity + (m_maxIntensity - m_minIntensity) * (1.0f - (float)i / ticks);
                std::snprintf(buf, sizeof(buf), "%.0f", v);
            }

            ImVec2 ts = ImGui::CalcTextSize(buf);
            dl->AddText(ImVec2(pos.x + barW + 8, fy - ts.y * 0.5f), IM_COL32(255, 255, 255, 220), buf);
        }

        const char* title = (m_heatmapMode == 2) ? "mm" : "Intensity";
        ImVec2 ts = ImGui::CalcTextSize(title);
        dl->AddText(ImVec2(pos.x + (barW + textW) * 0.5f - ts.x * 0.5f, pos.y - 20), IM_COL32(255, 255, 255, 200), title);
    }

    void Application::RenderTagsOverlay() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // === NAVCUBE (esquina superior derecha) ===
        m_navCube.Draw(m_camera->GetYaw(), m_camera->GetPitch(), m_camera->GetRotationZ());

        // === BARRA DE COLORES ===
        RenderColorBar();

        // ============================================
        // PANEL: GRILLAS
        // ============================================
        RenderGridPanel();

        // ============================================
        // PANEL: TAGS
        // ============================================
        RenderTagsPanel();

        // ============================================
        // PANEL 2: ROTACION
        // ============================================
        ImGui::Begin("Rotacion");
        ImGui::InputFloat("Paso (grados)", &m_rotationStep, 0.1f, 1.0f, "%.2f");
        float currentRot = m_camera->GetRotationZ();
        if (ImGui::InputFloat("Angulo Z", &currentRot, m_rotationStep, 0.0f, "%.2f")) {
            m_camera->SetRotationZ(currentRot);
        }
        if (ImGui::Button("Rotar -Z")) {
            m_camera->SetRotationZ(m_camera->GetRotationZ() - m_rotationStep);
        }
        ImGui::SameLine();
        if (ImGui::Button("Rotar +Z")) {
            m_camera->SetRotationZ(m_camera->GetRotationZ() + m_rotationStep);
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset")) {
            m_camera->SetRotationZ(0.0f);
        }
        ImGui::End();

        // ============================================
        // PANEL: VISTAS ORTOGONALES
        // ============================================
        ImGui::Begin("Vistas");

        const char* viewNames[6] = { "Top", "Bottom", "Front", "Back", "Left", "Right" };
        OrthoView views[6] = {
            OrthoView::Top, OrthoView::Bottom, OrthoView::Front,
            OrthoView::Back, OrthoView::Left, OrthoView::Right
        };

        for (int i = 0; i < 6; ++i) {
            if (i > 0 && i % 3 != 0) ImGui::SameLine();
            if (ImGui::Button(viewNames[i], ImVec2(60, 0))) {
                m_camera->SetView(views[i]);
                m_camera->SetRotationZ(0.0f);
                LOG_INFO("View: " + std::string(viewNames[i]));
            }
        }

        ImGui::End();

        // ============================================
        // PANEL 3: POSICION DE LA NUBE
        // ============================================
        ImGui::Begin("Posicion");
        ImGui::InputFloat3("Offset nube (X,Y,Z)", &m_cloudOffset.x, "%.3f");
        if (ImGui::Button("Resetear offset")) {
            m_cloudOffset = glm::vec3(0.0f);
        }
        ImGui::End();

        // ============================================
        // PANEL 4: HEATMAP
        // ============================================
        ImGui::Begin("Heatmap");
        ImGui::InputFloat("Plano offset (m)", &m_planeOffset, m_cloudStep, m_cloudStep * 10.0f, "%.4f");
        ImGui::InputFloat("Rango total (m)", &m_planeRange, m_cloudStep, m_cloudStep * 10.0f, "%.4f");
        if (std::abs(m_planeRange) < 0.0001f) m_planeRange = 0.0001f; // evita cero, permite negativos
        ImGui::Text("= +/- %.1f mm", (std::abs(m_planeRange) * 1000.0f) / 2.0f);
        if (ImGui::Button("Auto Fit")) {
            AutoFitPlane();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset")) {
            m_planeOffset = 0.0f;
            m_planeRange = 0.050f;
        }
        ImGui::End();

        // ============================================
        // PANEL 5: VISUALIZACION
        // ============================================
        ImGui::Begin("Visualizacion");
        ImGui::SliderInt("Densidad (1=todo)", &m_pointDensity, 1, 20);
        if (m_pointDensity < 1) m_pointDensity = 1;
        ImGui::SliderFloat("Tamanio punto", &m_pointSize, 0.5f, 5.0f, "%.1f");
        if (ImGui::Button("Normal")) {
            m_pointDensity = 1;
            m_pointSize = 0.5f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Exportar")) {
            m_pointDensity = 1;
            m_pointSize = 2.5f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Ligero")) {
            m_pointDensity = 5;
            m_pointSize = 1.0f;
        }
        ImGui::Text("Puntos: ~%s",
            (m_pointCloud ? std::to_string(m_pointCloud->GetPointCount() / m_pointDensity).c_str() : "-"));
        ImGui::End();

        // ============================================
        // PANEL: GUARDAR COMO (EXPORTAR IMAGEN)
        // ============================================
        RenderExportPanel();

        // ============================================
        // TAGS FLOTANTES (ImDrawList directo)
        // ============================================
        if (!m_annotations.empty()) {
            int w = static_cast<int>(m_window->GetWidth());
            int h = static_cast<int>(m_window->GetHeight());
            RenderFloatingTags(ImGui::GetBackgroundDrawList(), w, h, m_camera->GetPV());
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    
    // === PANEL EXPORTAR IMAGEN ===
    void Application::RenderExportPanel() {
        ImGui::Begin("Guardar como");

        ImGui::Text("Resolucion");
        ImGui::InputInt("Ancho", &m_exportImageWidth, 100, 1000);
        ImGui::InputInt("Alto", &m_exportImageHeight, 100, 1000);
        if (m_exportImageWidth < 100) m_exportImageWidth = 100;
        if (m_exportImageHeight < 100) m_exportImageHeight = 100;

        ImGui::Checkbox("Fondo transparente", &m_exportTransparent);

        if (!m_exportTransparent) {
            float bgCol[3] = { m_exportBackgroundColor.x, m_exportBackgroundColor.y, m_exportBackgroundColor.z };
            ImGui::ColorEdit3("Color fondo", bgCol, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
            m_exportBackgroundColor = glm::vec3(bgCol[0], bgCol[1], bgCol[2]);
        }
        else {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(Alpha 0)");
        }

        if (ImGui::Button("Guardar como...", ImVec2(130, 0))) {
            auto dialog = pfd::save_file("Guardar imagen", "export.png",
                { "Imagen PNG", "*.png" }, pfd::opt::force_overwrite);
            std::string result = dialog.result();
            if (!result.empty()) {
                if (result.size() < 4 || result.substr(result.size() - 4) != ".png") {
                    result += ".png";
                }
                std::strncpy(m_exportImagePath, result.c_str(), sizeof(m_exportImagePath) - 1);
                m_exportImagePath[sizeof(m_exportImagePath) - 1] = '\0';
                m_pendingExport = true;
            }
        }

        if (m_exportImagePath[0] != '\0') {
            ImGui::TextWrapped("Archivo: %s", m_exportImagePath);
        }

        ImGui::End();
    }

    // === CAPTURA OFFSCREEN A PNG ===
    void Application::CaptureImage(int width, int height, bool transparent, const std::string& path) {
        // Crear directorio si no existe
        try {
            std::filesystem::path p(path);
            std::filesystem::create_directories(p.parent_path());
        }
        catch (...) {}

        // Crear FBO offscreen
        u32 fbo = 0, colorTex = 0, depthRb = 0;
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        glGenTextures(1, &colorTex);
        glBindTexture(GL_TEXTURE_2D, colorTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex, 0);

        glGenRenderbuffers(1, &depthRb);
        glBindRenderbuffer(GL_RENDERBUFFER, depthRb);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRb);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            LOG_ERROR("Export FBO incomplete");
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteTextures(1, &colorTex);
            glDeleteRenderbuffers(1, &depthRb);
            glDeleteFramebuffers(1, &fbo);
            return;
        }

        // Guardar estado actual
        GLint oldViewport[4];
        glGetIntegerv(GL_VIEWPORT, oldViewport);
        float oldAspect = m_window->GetAspectRatio();
        float newAspect = static_cast<float>(width) / static_cast<float>(height);

        // Ajustar camara al aspecto de la imagen
        m_camera->SetAspectRatio(newAspect);

        glViewport(0, 0, width, height);
        if (transparent) {
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        }
        else {
            glClearColor(m_exportBackgroundColor.r, m_exportBackgroundColor.g, m_exportBackgroundColor.b, 1.0f);
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Dibujar solo la nube (sin grilla, sin anotaciones rojas)
        if (m_cloudVAO && m_pointCloud && m_pointCloud->GetPointCount() > 0) {
            m_renderer->DrawPointCloudVAO(m_cloudVAO, static_cast<u32>(m_pointCloud->GetPointCount()), *m_camera,
                m_cloudOffset, m_heatmapMode, m_planeAxis, m_planeEdgeMode, m_planeOffset, m_planeRange,
                m_minIntensity, m_maxIntensity, 1, m_pointSize);
        }

        // Dibujar tags flotantes via mini-frame ImGui (ventana invisible para draw list valido)
        ImGuiIO& io = ImGui::GetIO();
        ImVec2 oldDisplaySize = io.DisplaySize;
        io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));

        ImGui_ImplOpenGL3_NewFrame();
        io.DeltaTime = 1.0f / 60.0f;
        ImGui::NewFrame();

        // Ventana invisible fullscreen: garantiza mismo comportamiento que interfaz normal
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("ExportOverlay", nullptr,
            ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav |
            ImGuiWindowFlags_NoBringToFrontOnFocus);
        ImDrawList* dl = ImGui::GetWindowDrawList();

        // === ESCALADO PROPORCIONAL DEL TAG EN EXPORT ===
        float fontScale = (float)width / (float)m_window->GetWidth();
        RenderFloatingTags(dl, width, height, m_camera->GetPV(), fontScale);

        ImGui::End();
        ImGui::PopStyleVar();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        io.DisplaySize = oldDisplaySize;

        // Leer pixeles
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        std::vector<u8> pixels(width * height * 4);
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

        // Flip vertical (OpenGL entrega bottom-left, PNG espera top-left)
        std::vector<u8> flipped(width * height * 4);
        for (int y = 0; y < height; ++y) {
            std::memcpy(flipped.data() + y * width * 4,
                pixels.data() + (height - 1 - y) * width * 4,
                width * 4);
        }

        // Guardar PNG
        int ok = stbi_write_png(path.c_str(), width, height, 4, flipped.data(), width * 4);
        if (ok) {
            LOG_INFO("Imagen exportada: " + path + " (" + std::to_string(width) + "x" + std::to_string(height) + ")");
        }
        else {
            LOG_ERROR("No se pudo guardar: " + path);
        }

        // Restaurar estado
        m_camera->SetAspectRatio(oldAspect);
        glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glDeleteTextures(1, &colorTex);
        glDeleteRenderbuffers(1, &depthRb);
        glDeleteFramebuffers(1, &fbo);
    }

    void Application::UploadPointCloud() {
        if (!m_pointCloud || m_pointCloud->GetPoints().empty()) return;
        if (m_cloudVAO) { glDeleteVertexArrays(1, &m_cloudVAO); m_cloudVAO = 0; }
        if (m_cloudVBO) { glDeleteBuffers(1, &m_cloudVBO); m_cloudVBO = 0; }
        glGenVertexArrays(1, &m_cloudVAO); glGenBuffers(1, &m_cloudVBO);
        glBindVertexArray(m_cloudVAO); glBindBuffer(GL_ARRAY_BUFFER, m_cloudVBO);
        const auto& points = m_pointCloud->GetPoints();
        glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(Point), points.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)offsetof(Point, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Point), (void*)offsetof(Point, color));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)offsetof(Point, intensity));
        glBindVertexArray(0); glBindBuffer(GL_ARRAY_BUFFER, 0);
        LOG_INFO("Point cloud uploaded to GPU: " + std::to_string(points.size()) + " points");
    }

    void Application::CenterCameraOnCloud() {
        if (!m_pointCloud || !m_pointCloud->GetBounds().IsValid()) return;
        glm::vec3 center = m_pointCloud->GetBounds().GetCenter();
        glm::vec3 current = m_camera->GetOffset();
        m_camera->AddOffset(center.x - current.x, center.y - current.y, center.z - current.z);
        m_planeOffset = center.z;
        LOG_INFO("Camera centered. Auto Offset=" + std::to_string(m_planeOffset));
    }

    void Application::UpdateTitle() {
        std::string viewStr;
        switch (m_camera->GetCurrentView()) {
        case OrthoView::Top: viewStr = "Top"; break; case OrthoView::Bottom: viewStr = "Bottom"; break;
        case OrthoView::Front: viewStr = "Front"; break; case OrthoView::Back: viewStr = "Back"; break;
        case OrthoView::Left: viewStr = "Left"; break; case OrthoView::Right: viewStr = "Right"; break;
        }
        const char* modeStr = "RGB"; if (m_heatmapMode == 1) modeStr = "INT"; if (m_heatmapMode == 2) modeStr = "PLANE";
        const char* axisStr = (m_planeAxis == 0) ? "X" : ((m_planeAxis == 1) ? "Y" : "Z");
        const char* edgeStr = (m_planeEdgeMode == 1) ? "RPT" : "CLP";
        char buf[256];
        std::snprintf(buf, sizeof(buf), "InspectionApp | %s | %u FPS | Zoom: %.2fx | %s | Axis:%s Off:%.4f Rng:%.4f Edge:%s | Step:%.3f",
            viewStr.c_str(), m_currentFPS, m_camera->GetZoom(), modeStr, axisStr, m_planeOffset, m_planeRange, edgeStr, m_cloudStep);
        std::string newTitle(buf);
        if (newTitle != m_lastTitle) { m_window->SetTitle(newTitle); m_lastTitle = newTitle; }
    }

    void Application::OnScroll(float yoffset) {
        if (yoffset > 0.0f) m_camera->SetZoom(m_camera->GetZoom() * m_zoomSpeed);
        else if (yoffset < 0.0f) m_camera->SetZoom(m_camera->GetZoom() / m_zoomSpeed);
    }

    void Application::OnDrop(int count, const char** paths) {
        if (count == 0 || paths == nullptr) return;
        std::string filepath(paths[0]);
        LOG_INFO("File dropped: " + filepath);
        size_t dotPos = filepath.find_last_of('.');
        std::string ext = (dotPos != std::string::npos) ? filepath.substr(dotPos) : "";
        bool isPts = (ext == ".pts" || ext == ".PTS");
        if (!isPts) { LOG_WARN("Unsupported file type: " + ext); return; }
        PointCloud cloud; PTSImportStats stats;
        LOG_INFO("Importing PTS file...");
        bool success = PTSImporter::Import(filepath, cloud, stats, nullptr, 0);
        if (!success) { LOG_ERROR("Failed to import PTS file: " + filepath); return; }
        LOG_INFO("PTS imported: " + std::to_string(stats.pointsRead) + " points read");
        if (stats.pointsSkipped > 0) LOG_INFO("Points skipped: " + std::to_string(stats.pointsSkipped));
        m_minIntensity = static_cast<float>(stats.minIntensity);
        m_maxIntensity = static_cast<float>(stats.maxIntensity);
        std::string cloudPath = filepath + ".cloud";
        if (CloudSerializer::Save(cloud, cloudPath)) LOG_INFO("Auto-saved: " + cloudPath);
        else LOG_WARN("Failed to auto-save cloud file");
        m_pointCloud = std::make_unique<PointCloud>();
        *m_pointCloud = std::move(cloud);
        m_pointCloud->SetSourceFile(filepath);
        m_cloudDirty = true; m_cloudOffset = glm::vec3(0.0f); m_annotations.clear();
        CenterCameraOnCloud();
    }

    void Application::OnEvent(int key, int action) {
        if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
        GLFWwindow* native = static_cast<GLFWwindow*>(m_window->GetNativeWindow());
        bool ctrl = glfwGetKey(native, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(native, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
        bool alt = glfwGetKey(native, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(native, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;
        bool shift = glfwGetKey(native, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(native, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
        switch (key) {
        case GLFW_KEY_1: m_cloudStep = 0.001f; LOG_INFO("Step: 0.001 (1mm)"); return;
        case GLFW_KEY_2: m_cloudStep = 0.01f;  LOG_INFO("Step: 0.01 (10mm)"); return;
        case GLFW_KEY_3: m_cloudStep = 0.1f;   LOG_INFO("Step: 0.1 (100mm)"); return;
        case GLFW_KEY_4: m_cloudStep = 1.0f;   LOG_INFO("Step: 1.0 (1m)"); return;
        }
        if (shift && key == GLFW_KEY_F) { AutoFitPlane(); return; }
        if (shift) {
            switch (key) {
            case GLFW_KEY_Q: m_planeOffset += m_cloudStep; LOG_INFO("Offset: " + std::to_string(m_planeOffset)); return;
            case GLFW_KEY_E: m_planeOffset -= m_cloudStep; LOG_INFO("Offset: " + std::to_string(m_planeOffset)); return;
            case GLFW_KEY_W: m_planeRange += m_cloudStep;  LOG_INFO("Range: " + std::to_string(m_planeRange)); return;
            case GLFW_KEY_S: m_planeRange -= m_cloudStep;  LOG_INFO("Range: " + std::to_string(m_planeRange)); return;
            }
        }
        if (alt) {
            switch (key) {
            case GLFW_KEY_W: m_cloudOffset.y += m_cloudStep; LOG_INFO("Cloud Y: " + std::to_string(m_cloudOffset.y)); return;
            case GLFW_KEY_S: m_cloudOffset.y -= m_cloudStep; LOG_INFO("Cloud Y: " + std::to_string(m_cloudOffset.y)); return;
            case GLFW_KEY_A: m_cloudOffset.x -= m_cloudStep; LOG_INFO("Cloud X: " + std::to_string(m_cloudOffset.x)); return;
            case GLFW_KEY_D: m_cloudOffset.x += m_cloudStep; LOG_INFO("Cloud X: " + std::to_string(m_cloudOffset.x)); return;
            case GLFW_KEY_Q: m_cloudOffset.z += m_cloudStep; LOG_INFO("Cloud Z: " + std::to_string(m_cloudOffset.z)); return;
            case GLFW_KEY_E: m_cloudOffset.z -= m_cloudStep; LOG_INFO("Cloud Z: " + std::to_string(m_cloudOffset.z)); return;
            }
        }
        if (ctrl) {
            switch (key) {
            case GLFW_KEY_F1: m_camera->LockRotation(!m_camera->IsRotationLocked()); LOG_INFO(std::string("Rotation ") + (m_camera->IsRotationLocked() ? "LOCKED" : "UNLOCKED")); return;
            case GLFW_KEY_F2: m_camera->LockOffsetX(!m_camera->IsOffsetXLocked()); LOG_INFO(std::string("Offset X ") + (m_camera->IsOffsetXLocked() ? "LOCKED" : "UNLOCKED")); return;
            case GLFW_KEY_F3: m_camera->LockOffsetY(!m_camera->IsOffsetYLocked()); LOG_INFO(std::string("Offset Y ") + (m_camera->IsOffsetYLocked() ? "LOCKED" : "UNLOCKED")); return;
            case GLFW_KEY_F4: m_camera->LockOffsetZ(!m_camera->IsOffsetZLocked()); LOG_INFO(std::string("Offset Z ") + (m_camera->IsOffsetZLocked() ? "LOCKED" : "UNLOCKED")); return;
            }
            return;
        }
        switch (key) {
        case GLFW_KEY_F1: m_camera->SetView(OrthoView::Top);    LOG_INFO("View: Top"); break;
        case GLFW_KEY_F2: m_camera->SetView(OrthoView::Bottom); LOG_INFO("View: Bottom"); break;
        case GLFW_KEY_F3: m_camera->SetView(OrthoView::Front);  LOG_INFO("View: Front"); break;
        case GLFW_KEY_F4: m_camera->SetView(OrthoView::Back);   LOG_INFO("View: Back"); break;
        case GLFW_KEY_F5: m_camera->SetView(OrthoView::Left);   LOG_INFO("View: Left"); break;
        case GLFW_KEY_F6: m_camera->SetView(OrthoView::Right);  LOG_INFO("View: Right"); break;
        case GLFW_KEY_H:
            m_heatmapMode = (m_heatmapMode + 1) % 3;
            if (m_heatmapMode == 0) LOG_INFO("Mode: RGB");
            else if (m_heatmapMode == 1) LOG_INFO("Mode: INTENSITY");
            else LOG_INFO("Mode: PLANE (Offset=" + std::to_string(m_planeOffset) + " Range=" + std::to_string(m_planeRange) + ")");
            break;
        case GLFW_KEY_X: m_planeAxis = (m_planeAxis + 1) % 3; LOG_INFO("Plane Axis: " + std::string(m_planeAxis == 0 ? "X" : (m_planeAxis == 1 ? "Y" : "Z"))); break;
        case GLFW_KEY_V: m_planeEdgeMode = (m_planeEdgeMode + 1) % 2; LOG_INFO(std::string("Plane Edge: ") + (m_planeEdgeMode == 1 ? "REPEAT" : "CLAMP")); break;
        case GLFW_KEY_T: m_tagAxis = (m_tagAxis + 1) % 3; LOG_INFO("Tag Axis: " + std::string(m_tagAxis == 0 ? "X" : (m_tagAxis == 1 ? "Y" : "Z"))); break;
        case GLFW_KEY_C: m_annotations.clear(); LOG_INFO("Annotations cleared"); break;
        case GLFW_KEY_KP_ADD:
        case GLFW_KEY_EQUAL:
            if (shift) { m_tagFontSize += 2.0f; LOG_INFO("Tag font size: " + std::to_string(m_tagFontSize)); }
            else m_camera->SetZoom(m_camera->GetZoom() * m_zoomSpeed);
            break;
        case GLFW_KEY_KP_SUBTRACT:
        case GLFW_KEY_MINUS:
            if (shift) { m_tagFontSize = (std::max)(6.0f, m_tagFontSize - 2.0f); LOG_INFO("Tag font size: " + std::to_string(m_tagFontSize)); }
            else m_camera->SetZoom(m_camera->GetZoom() / m_zoomSpeed);
            break;
        case GLFW_KEY_R: m_camera->SetRotationZ(m_camera->GetRotationZ() + m_rotationStep); break;
        }
    }

    void Application::RenderTagsPanel() {
        ImGui::Begin("Tags");

        // --- Mostrar/Ocultar todos ---
        ImGui::Checkbox("Mostrar tags", &m_tagsVisible);
        ImGui::Separator();

        // --- Prefijo global ---
        char prefixBuf[64];
        std::strncpy(prefixBuf, m_tagPrefix.c_str(), sizeof(prefixBuf) - 1);
        prefixBuf[sizeof(prefixBuf) - 1] = '\0';
        if (ImGui::InputText("Prefijo", prefixBuf, sizeof(prefixBuf))) {
            m_tagPrefix = prefixBuf;
        }

        // --- Tamano de letra (combo desplegable, 12 valores ordenados) ---
        static const int TAG_SIZES[] = { 8, 10, 12, 15, 17, 20, 23, 25, 27, 30, 35, 40 };
        static const int TAG_SIZE_COUNT = sizeof(TAG_SIZES) / sizeof(TAG_SIZES[0]);
        static const char* sizeLabels[] = { "8", "10", "12", "15", "17", "20", "23", "25", "27", "30", "35", "40" };

        int currentIdx = 0;
        for (int i = 0; i < TAG_SIZE_COUNT; ++i) {
            if ((int)m_tagFontSize == TAG_SIZES[i]) { currentIdx = i; break; }
        }
        if (ImGui::Combo("Tamano", &currentIdx, sizeLabels, TAG_SIZE_COUNT)) {
            m_tagFontSize = (float)TAG_SIZES[currentIdx];
        }

        // --- Fondo, Outline, Bold y Color ---
        ImGui::Checkbox("Fondo", &m_tagShowBackground);
        ImGui::SameLine();
        ImGui::Checkbox("Borde", &m_tagShowOutline);
        ImGui::SameLine();
        ImGui::Checkbox("Negrita", &m_tagBold);
        float col[4] = { m_tagTextColor.x, m_tagTextColor.y, m_tagTextColor.z, m_tagTextColor.w };
        ImGui::ColorEdit4("Color", col, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
        m_tagTextColor = glm::vec4(col[0], col[1], col[2], col[3]);

        ImGui::Separator();

        // --- Recuento ---
        int visibleCount = 0;
        for (const auto& a : m_annotations) if (a.visible) ++visibleCount;
        ImGui::Text("Total: %d  |  Visibles: %d", (int)m_annotations.size(), visibleCount);

        ImGui::Separator();

        // --- Tabla: Vis | Prefijo | Eje | Coord ---
        ImGuiTableFlags tblFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp;
        if (ImGui::BeginTable("TagsTable", 4, tblFlags, ImVec2(0, 180))) {
            ImGui::TableSetupColumn("Vis", ImGuiTableColumnFlags_WidthFixed, 35.0f);
            ImGui::TableSetupColumn("Prefijo", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Eje", ImGuiTableColumnFlags_WidthFixed, 30.0f);
            ImGui::TableSetupColumn("Coord", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            for (int i = 0; i < (int)m_annotations.size(); ++i) {
                auto& ann = m_annotations[i];
                ImGui::TableNextRow();

                // Highlight fila seleccionada
                if (m_selectedTagIndex == i) {
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(50, 70, 110, 80));
                }

                // Columna 0: Checkbox de visibilidad individual
                ImGui::TableSetColumnIndex(0);
                ImGui::Checkbox(("##vis" + std::to_string(i)).c_str(), &ann.visible);

                // Columna 1: Selectable invisible que spannea toda la fila
                ImGui::TableSetColumnIndex(1);
                bool isSelected = (m_selectedTagIndex == i);
                ImGui::PushID(i);
                if (ImGui::Selectable("", isSelected,
                    ImGuiSelectableFlags_SpanAllColumns)) {
                    m_selectedTagIndex = i;
                }
                ImGui::PopID();

                // Contenido de la columna 1 (dibujado encima del selectable)
                ImGui::SameLine();
                ImGui::TextUnformatted(m_tagPrefix.c_str());

                // Columna 2: Eje
                ImGui::TableSetColumnIndex(2);
                const char* axisName = (ann.tagAxis == 0) ? "X" : ((ann.tagAxis == 1) ? "Y" : "Z");
                ImGui::TextUnformatted(axisName);

                // Columna 3: Coordenada
                ImGui::TableSetColumnIndex(3);
                float coord = (ann.tagAxis == 0) ? ann.position.x :
                    ((ann.tagAxis == 1) ? ann.position.y : ann.position.z);
                ImGui::Text("%.3f", coord);
            }
            ImGui::EndTable();
        }

        // --- Botones ---
        if (ImGui::Button("Borrar seleccionado", ImVec2(130, 0))) {
            if (m_selectedTagIndex >= 0 && m_selectedTagIndex < (int)m_annotations.size()) {
                m_annotations.erase(m_annotations.begin() + m_selectedTagIndex);
                m_selectedTagIndex = -1;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Borrar todos", ImVec2(100, 0))) {
            m_annotations.clear();
            m_selectedTagIndex = -1;
        }

        ImGui::Separator();

        // --- Exportar a archivo ---
        ImGui::Text("Exportar:");
        ImGui::InputText("Fecha", m_exportDate, sizeof(m_exportDate));
        if (ImGui::Button("Guardar como...", ImVec2(130, 0))) {
            auto dialog = pfd::save_file("Guardar tags", "tags_export.txt",
                { "Archivo de texto", "*.txt" }, pfd::opt::force_overwrite);
            std::string result = dialog.result();
            if (!result.empty()) {
                if (result.size() < 4 || result.substr(result.size() - 4) != ".txt") {
                    result += ".txt";
                }
                ExportAnnotationsToTSV(result);
            }
        }

        ImGui::End();
    }

    // === DIBUJAR TAGS FLOTANTES (reutilizable para pantalla y export) ===
    void Application::RenderFloatingTags(ImDrawList* dl, int w, int h, const glm::mat4& pv, float fontScale) {
        if (m_annotations.empty() || !dl || !m_tagsVisible) return;
        ImFont* font = ImGui::GetFont();
        if (!font) return;

        float fontSize = m_tagFontSize * fontScale;

        for (size_t i = 0; i < m_annotations.size(); ++i) {
            const auto& ann = m_annotations[i];
            if (!ann.visible) continue;

            glm::vec4 clip = pv * glm::vec4(ann.position, 1.0f);
            if (clip.w <= 0.0f) continue;

            glm::vec3 ndc = glm::vec3(clip) / clip.w;
            if (ndc.x < -1.0f || ndc.x > 1.0f || ndc.y < -1.0f || ndc.y > 1.0f) continue;

            float sx = (ndc.x * 0.5f + 0.5f) * w;
            float sy = (-ndc.y * 0.5f + 0.5f) * h;

            const char* text = ann.label.c_str();
            ImVec2 ts = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, text, text + ann.label.size());

            // === CENTRADO EXACTO SOBRE EL PUNTO ===
            float padX = 7.0f;
            float padY = 4.0f;
            ImVec2 pos(sx - ts.x * 0.5f, sy - ts.y * 0.5f);

            ImVec2 bgMin(pos.x - padX, pos.y - padY);
            ImVec2 bgMax(pos.x + ts.x + padX, pos.y + ts.y + padY);

            // --- Leader line (pin) hacia abajo si el tag tapa el punto ---
            float leaderLen = 14.0f;
            ImVec2 leaderTop(sx, sy + 4.0f);
            ImVec2 leaderBot(sx, sy + leaderLen);

            // Solo dibujar línea si el tag no llega tan abajo
            if (leaderBot.y < bgMin.y) {
                dl->AddLine(leaderTop, leaderBot, IM_COL32(255, 255, 255, 140), 1.5f);
                // Punto de anclaje pequeño (visible por debajo del tag)
                dl->AddCircleFilled(leaderBot, 2.5f, IM_COL32(255, 255, 255, 180));
            }

            // --- Sombra suave ---
            if (m_tagShowBackground) {
                dl->AddRectFilled(
                    ImVec2(bgMin.x + 2.0f, bgMin.y + 2.0f),
                    ImVec2(bgMax.x + 2.0f, bgMax.y + 2.0f),
                    IM_COL32(0, 0, 0, 90), 6.0f
                );
            }

            // --- Fondo redondeado ---
            if (m_tagShowBackground) {
                dl->AddRectFilled(bgMin, bgMax, IM_COL32(12, 12, 12, 225), 6.0f);
                // Borde sutil
                dl->AddRect(bgMin, bgMax, IM_COL32(255, 255, 255, 50), 6.0f, 0, 1.0f);
            }

            ImU32 textCol = IM_COL32(
                (int)(m_tagTextColor.x * 255),
                (int)(m_tagTextColor.y * 255),
                (int)(m_tagTextColor.z * 255),
                (int)(m_tagTextColor.w * 255)
            );

            // --- Outline negro suave (4 direcciones, sin esquinas) ---
            if (m_tagShowOutline) {
                ImU32 outlineCol = IM_COL32(0, 0, 0, 200);
                dl->AddText(font, fontSize, ImVec2(pos.x - 1, pos.y), outlineCol, text);
                dl->AddText(font, fontSize, ImVec2(pos.x + 1, pos.y), outlineCol, text);
                dl->AddText(font, fontSize, ImVec2(pos.x, pos.y - 1), outlineCol, text);
                dl->AddText(font, fontSize, ImVec2(pos.x, pos.y + 1), outlineCol, text);
            }

            // --- Negrita simulada ---
            if (m_tagBold) {
                dl->AddText(font, fontSize, ImVec2(pos.x + 0.5f, pos.y), textCol, text);
            }

            dl->AddText(font, fontSize, pos, textCol, text);
        }
    }

    void Application::ExportAnnotationsToTSV(const std::string& filePath) {
        // Crear carpeta si no existe
        try {
            std::filesystem::path p(filePath);
            std::filesystem::create_directories(p.parent_path());
        }
        catch (const std::exception& e) {
            LOG_ERROR(std::string("Error creando carpeta: ") + e.what());
            return;
        }

        std::FILE* file = std::fopen(filePath.c_str(), "w");
        if (!file) {
            LOG_ERROR("No se pudo crear archivo: " + filePath);
            return;
        }

        std::fprintf(file, "# Inspection App 4.9\n");
        std::fprintf(file, "# Created %s\n", m_exportDate);
        std::fprintf(file, "# All distance values are in Metres\n");
        std::fprintf(file, "# All position values are in Metres\n");
        std::fprintf(file, "Tag\tPnt1\tX\tY\tZ\n");

        int exportedCount = 0;
        for (const auto& ann : m_annotations) {
            if (m_exportFilterAxis < 3 && ann.tagAxis != m_exportFilterAxis) continue;

            std::fprintf(file, "%s\t%.3f\t%.3f\t%.3f\n",
                m_tagPrefix.c_str(),
                ann.position.x,
                ann.position.y,
                ann.position.z
            );
            ++exportedCount;
        }

        std::fclose(file);
        LOG_INFO("Exportado: " + filePath + " (" + std::to_string(exportedCount) + " tags)");
    }

    void Application::RenderGridPanel() {
        ImGui::Begin("Grilla");

        auto& g = m_renderer->GetGridConfig();

        // Visible + Bloqueado
        ImGui::Checkbox("Visible", &g.visible);
        ImGui::SameLine();
        ImGui::Checkbox("Bloqueado", &g.locked);

        bool disabled = g.locked;
        if (disabled) ImGui::BeginDisabled();

        // Espaciado
        if (ImGui::InputFloat("Espaciado (m)", &g.spacing, 0.1f, 1.0f, "%.3f")) {
            if (g.spacing < 0.001f) g.spacing = 0.001f;
            m_renderer->GenerateGrid();
        }

        ImGui::Separator();
        ImGui::Text("Posicion (m)");

        // Offset XYZ con 3 decimales
        if (ImGui::InputFloat("X", &g.offset.x, 0.1f, 1.0f, "%.3f")) m_renderer->GenerateGrid();
        if (ImGui::InputFloat("Y", &g.offset.y, 0.1f, 1.0f, "%.3f")) m_renderer->GenerateGrid();
        if (ImGui::InputFloat("Z", &g.offset.z, 0.1f, 1.0f, "%.3f")) m_renderer->GenerateGrid();

        ImGui::Separator();
        ImGui::Text("Rotacion (grados)");

        // Rotacion XYZ
        if (ImGui::InputFloat("Rot X", &g.rotation.x, 1.0f, 10.0f, "%.1f")) m_renderer->GenerateGrid();
        if (ImGui::InputFloat("Rot Y", &g.rotation.y, 1.0f, 10.0f, "%.1f")) m_renderer->GenerateGrid();
        if (ImGui::InputFloat("Rot Z", &g.rotation.z, 1.0f, 10.0f, "%.1f")) m_renderer->GenerateGrid();

        if (disabled) ImGui::EndDisabled();

        ImGui::End();
    }

} // namespace InspectionApp