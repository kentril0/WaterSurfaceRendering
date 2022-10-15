
#include "pch.h"
#include "Window.h"


namespace vkp 
{

    static void GLFWErrorCallback(int error, const char* description)
    {
        fprintf(stderr, "Glfw Error %d: %s\n", error, description);
    }

    Window::Window(const char* title, int width, int height)
    {
        VKP_REGISTER_FUNCTION();

        InitGLFW();
        CreateWindow(title, width, height);
    }

    Window::~Window()
    {
        VKP_REGISTER_FUNCTION();

        glfwDestroyWindow(m_Window);
        glfwTerminate();
    }

    void Window::InitGLFW()
    {
        VKP_REGISTER_FUNCTION();

        glfwSetErrorCallback(GLFWErrorCallback);

        int success = glfwInit();
        VKP_ASSERT_MSG(success, "Failed to initialize GLFW");

        // Do not create an OpenGL context
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }

    void Window::CreateWindow(const char* title, int width, int height)
    {
        // Create the GLFWwindow
        //  windowed mode, do not share resources
        m_Window = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if (m_Window == NULL)
        {
            glfwTerminate();
            VKP_ASSERT_MSG(false, "Failed to create a GLFWwindow");
        }

        auto success = glfwVulkanSupported();
        VKP_ASSERT_MSG(success, "GLFW: Vulkan not supported");
    }

    std::vector<const char*> Window::GetRequiredExtensions() const
    {
        // Need an extension to interface with the window system
        // Use GLFW to return the extension(s) it needs
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = 
            glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, 
                                            glfwExtensions + glfwExtensionCount);
        return extensions;
    }

    VkSurfaceKHR Window::CreateSurface(VkInstance instance) const
    {
        VkSurfaceKHR surface;
        auto err = glfwCreateWindowSurface(instance, m_Window, nullptr, &surface);
        VKP_ASSERT_RESULT(err);

        return surface;
    }

    int Window::GetWidth() const
    {
        int width;
        glfwGetWindowSize(m_Window, &width, nullptr);
        return width;
    }

    int Window::GetHeight() const
    {
        int height;
        glfwGetWindowSize(m_Window, nullptr, &height);
        return height;
    }
        
    void Window::SetVSync(bool enabled)
    {
        m_VSync = enabled;
        glfwSwapInterval(enabled ? 1 : 0);
    }

    void Window::SetWidth(int width)
    {
        glfwSetWindowSize(m_Window, width, this->GetHeight());
    }

    void Window::SetHeight(int height)
    {
        glfwSetWindowSize(m_Window, this->GetWidth(), height);
    }

    void Window::ShowCursor(bool b)
    {
        glfwSetInputMode(m_Window, GLFW_CURSOR, b ? GLFW_CURSOR_NORMAL :
                                                    GLFW_CURSOR_DISABLED);
    }

    void Window::SetUserPointer(void* userPointer)
    {
        glfwSetWindowUserPointer(m_Window, userPointer);
    }

    void Window::SetFramebufferSizeCallback(GLFWframebuffersizefun callback)
    {
        glfwSetFramebufferSizeCallback(m_Window, callback);
    }

    void Window::SetCursorPosCallback(GLFWcursorposfun callback)
    {
        glfwSetCursorPosCallback(m_Window, callback);
    }

    void Window::SetMouseButtonCallback(GLFWmousebuttonfun callback)
    {
        glfwSetMouseButtonCallback(m_Window, callback);
    }

    void Window::SetKeyCallback(GLFWkeyfun callback)
    {
        glfwSetKeyCallback(m_Window, callback);
    }

    void Window::SetCursorEnterCallback(GLFWcursorenterfun callback)
    {
        glfwSetCursorEnterCallback(m_Window, callback);
    }

} // namespace vkp 