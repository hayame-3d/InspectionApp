#pragma once

#include "Window.h"
#include "Renderer.h"
#include "Camera.h"
#include "PointCloud.h"
#include "PTSImporter.h"
#include "CloudSerializer.h"
#include "NavCube.h"

#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>

struct ImDrawList; // forward declaration for tag rendering helper

namespace InspectionApp {

    struct Annotation {
        glm::vec3 position;
        std::string label;
        float deviation = 0.0f;
        int tagAxis = 2;  // 0=X, 1=Y, 2=Z (eje que se mostro en el tag flotante)
        bool visible = true; // <--- NUEVO: visibilidad individual
    };

    class Application {
    public:
        Application();
        ~Application();
        void Run();
        void OnEvent(int key, int action);
        void OnScroll(float yoffset);
        void OnDrop(int count, const char** paths);

    private:
        void Init();
        void Shutdown();
        void OnUpdate(float dt);
        void OnRender();
        void UpdateTitle();
        void UploadPointCloud();
        void CenterCameraOnCloud();
        void AutoFitPlane();
        void PlaceAnnotation(float mouseX, float mouseY);
        void RenderTagsOverlay();
        void RenderColorBar();
        void RenderTagsPanel();
        void ExportAnnotationsToTSV(const std::string& folderPath);
        void RenderGridPanel();

        // === EXPORTAR IMAGEN ===
        void RenderExportPanel();
        void CaptureImage(int width, int height, bool transparent, const std::string& path);
        void RenderFloatingTags(ImDrawList* dl, int w, int h, const glm::mat4& pv, float fontScale = 1.0f); // <--- MODIFICADO

        std::unique_ptr<Window> m_window;
        std::unique_ptr<Renderer> m_renderer;
        std::unique_ptr<Camera> m_camera;
        std::unique_ptr<PointCloud> m_pointCloud;

        bool m_running = true;
        float m_cameraSpeed = 5.0f;
        float m_zoomSpeed = 1.1f;
        float m_lastMouseX = 0.0f;
        float m_lastMouseY = 0.0f;
        bool m_mouseMiddlePressed = false;
        bool m_mouseLeftPressed = false;
        float m_fpsTimer = 0.0f;
        u32 m_frameCount = 0;
        u32 m_currentFPS = 0;
        std::string m_lastTitle;
        u32 m_cloudVAO = 0;
        u32 m_cloudVBO = 0;
        bool m_cloudDirty = false;
        glm::vec3 m_cloudOffset{ 0.0f, 0.0f, 0.0f };
        float m_cloudStep = 0.001f;

        int m_heatmapMode = 0;
        int m_planeAxis = 2;
        int m_planeEdgeMode = 1;
        float m_planeOffset = 0.0f;
        float m_planeRange = 0.050f;
        float m_minIntensity = 0.0f;
        float m_maxIntensity = 1.0f;

        // === PANELES DE CONTROL ===
        float m_tagScaleFactor = 1000.0f;
        float m_rotationStep = 5.0f;
        int m_pointDensity = 1;
        float m_pointSize = 0.5f;

        bool m_pendingPick = false;
        float m_pickX = 0.0f;
        float m_pickY = 0.0f;

        u32 m_pickPBO = 0;
        int m_pickPhase = 0;
        float m_pickPendingX = 0.0f;
        float m_pickPendingY = 0.0f;
        glm::mat4 m_pickPendingPV = glm::mat4(1.0f);

        std::vector<Annotation> m_annotations;
        int m_tagAxis = 2;
        float m_tagFontSize = 14.0f;

        // === NAVCUBE ===
        NavCube m_navCube;
        bool m_navCubeActive = false;

        // === TAG PANEL SETTINGS ===
        std::string m_tagPrefix = "dpto. 01";
        bool m_tagShowBackground = true;
        bool m_tagShowOutline = true;
        bool m_tagBold = false;
        glm::vec4 m_tagTextColor = glm::vec4(1.0f, 1.0f, 0.15f, 1.0f);
        int m_selectedTagIndex = -1;
        int m_exportFilterAxis = 3; // 0=X, 1=Y, 2=Z, 3=Todos
        char m_exportFolderPath[256] = "./exports";
        char m_exportDate[32] = "2026-08-15";

        // === EXPORTAR IMAGEN ===
        bool m_pendingExport = false;
        int m_exportImageWidth = 1920;
        int m_exportImageHeight = 1080;
        bool m_exportTransparent = false;
        char m_exportImagePath[256] = "./export.png";
        glm::vec3 m_exportBackgroundColor = glm::vec3(1.0f, 1.0f, 1.0f);

        bool m_tagsVisible = true; // <--- NUEVO: visibilidad global
    };

} // namespace InspectionApp