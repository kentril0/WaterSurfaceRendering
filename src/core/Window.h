#ifndef WATER_SURFACE_RENDERING_WINDOW_H_
#define WATER_SURFACE_RENDERING_WINDOW_H_

#include "core/Base.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>


namespace vkp
{
    /** 
     * @brief Abstraction over GLFW and GLFWwindow
     */
    class Window
    {
    public:
        Window(const char* title, 
               int width = VKP_DEFAULT_WINDOW_WIDTH, 
               int height = VKP_DEFAULT_WINDOW_HEIGHT);
        ~Window();

        operator GLFWwindow*() const { return m_Window; }

        /**
         * @pre Initialized GLFW
         * @return Extension(s) GLFW needs to interface with the Windows Sys.
         */
        std::vector<const char*> GetRequiredExtensions() const;

        /** @return Created window surface for the vulkan instance */
        VkSurfaceKHR CreateSurface(VkInstance instance) const;


        inline int IsOpen() const {
            return !glfwWindowShouldClose(m_Window);
        }

        bool IsVSync() const { return m_VSync; }

        int GetWidth() const;
        int GetHeight() const;

        inline void GetFramebufferSize(int* width, int* height) const
        {
            glfwGetFramebufferSize(m_Window, width, height);
        }

        void SetVSync(bool enabled);
        void SetWidth(int width);
        void SetHeight(int height);
        void ShowCursor(bool show);

        // Callbacks

        void SetUserPointer(void* userPointer);
        void SetFramebufferSizeCallback(GLFWframebuffersizefun callback);
        void SetCursorPosCallback(GLFWcursorposfun callback);
        void SetMouseButtonCallback(GLFWmousebuttonfun callback);
        void SetKeyCallback(GLFWkeyfun callback);
        void SetCursorEnterCallback(GLFWcursorenterfun callback);

    private:
        void InitGLFW();
        void CreateWindow(const char* title, int width, int height);

    private:
        GLFWwindow* m_Window;
        bool m_VSync;
    };

} // namespace vkp

#endif // WATER_SURFACE_RENDERING_WINDOW_H_