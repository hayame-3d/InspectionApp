#pragma once
#include "Types.h"
#include <string>
#include <functional>

struct GLFWwindow;

namespace InspectionApp {

    struct WindowProps {
        std::string Title = "InspectionApp";
        u32 Width = 1600;
        u32 Height = 900;
    };

    class Window {
    public:
        using EventCallbackFn = std::function<void(int key, int action)>;
        using ScrollCallbackFn = std::function<void(float yoffset)>;
        using DropCallbackFn = std::function<void(int count, const char** paths)>;

        Window(const WindowProps& props = WindowProps{});
        ~Window();

        void OnUpdate();
        bool ShouldClose() const;
        void SetEventCallback(const EventCallbackFn& callback) { m_data.EventCallback = callback; }
        void SetScrollCallback(const ScrollCallbackFn& callback) { m_data.ScrollCallback = callback; }
        void SetDropCallback(const DropCallbackFn& callback) { m_data.DropCallback = callback; }
        void SetTitle(const std::string& title);
        void* GetNativeWindow() const { return m_window; }
        u32 GetWidth() const { return m_data.Width; }
        u32 GetHeight() const { return m_data.Height; }
        float GetAspectRatio() const { return static_cast<float>(m_data.Width) / static_cast<float>(m_data.Height); }

    private:
        void Init();
        void Shutdown();
        static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
        static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
        static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
        static void DropCallback(GLFWwindow* window, int count, const char** paths);

        GLFWwindow* m_window = nullptr;

        struct WindowData {
            std::string Title;
            u32 Width = 0;
            u32 Height = 0;
            EventCallbackFn EventCallback;
            ScrollCallbackFn ScrollCallback;
            DropCallbackFn DropCallback;
        };
        WindowData m_data;
    };

} // namespace InspectionApp