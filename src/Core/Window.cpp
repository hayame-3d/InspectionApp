#include "Window.h"
#include "Logger.h"
#include <glad/gl.h>
#include <GLFW/glfw3.h>

namespace InspectionApp {

    Window::Window(const WindowProps& props) {
        m_data.Title = props.Title;
        m_data.Width = props.Width;
        m_data.Height = props.Height;
        Init();
    }

    Window::~Window() {
        Shutdown();
    }

    void Window::Init() {
        if (!glfwInit()) {
            LOG_FATAL("Failed to initialize GLFW");
            return;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

        m_window = glfwCreateWindow(static_cast<int>(m_data.Width), static_cast<int>(m_data.Height),
            m_data.Title.c_str(), nullptr, nullptr);
        if (!m_window) {
            LOG_FATAL("Failed to create GLFW window");
            glfwTerminate();
            return;
        }

        glfwMakeContextCurrent(m_window);
        glfwSetWindowUserPointer(m_window, &m_data);
        glfwSetFramebufferSizeCallback(m_window, FramebufferSizeCallback);
        glfwSetKeyCallback(m_window, KeyCallback);
        glfwSetScrollCallback(m_window, ScrollCallback);
        glfwSetDropCallback(m_window, DropCallback);

        if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
            LOG_FATAL("Failed to initialize GLAD");
            return;
        }

        glfwSwapInterval(1);
        LOG_INFO("VSync enabled (glfwSwapInterval=1)");

        LOG_INFO("Window created: " + std::to_string(m_data.Width) + "x" + std::to_string(m_data.Height));
        LOG_INFO("OpenGL Version: " + std::string((const char*)glGetString(GL_VERSION)));
    }

    void Window::Shutdown() {
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }

    void Window::OnUpdate() {
        glfwPollEvents();
        glfwSwapBuffers(m_window);
    }

    bool Window::ShouldClose() const {
        return glfwWindowShouldClose(m_window);
    }

    void Window::SetTitle(const std::string& title) {
        glfwSetWindowTitle(m_window, title.c_str());
    }

    void Window::FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
        auto* data = static_cast<WindowData*>(glfwGetWindowUserPointer(window));
        data->Width = static_cast<u32>(width);
        data->Height = static_cast<u32>(height);
        glViewport(0, 0, width, height);
    }

    void Window::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        auto* data = static_cast<WindowData*>(glfwGetWindowUserPointer(window));
        if (data->EventCallback) {
            data->EventCallback(key, action);
        }
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }
    }

    void Window::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
        auto* data = static_cast<WindowData*>(glfwGetWindowUserPointer(window));
        if (data->ScrollCallback) {
            data->ScrollCallback(static_cast<float>(yoffset));
        }
    }

    void Window::DropCallback(GLFWwindow* window, int count, const char** paths) {
        auto* data = static_cast<WindowData*>(glfwGetWindowUserPointer(window));
        if (data->DropCallback) {
            data->DropCallback(count, paths);
        }
    }

} // namespace InspectionApp