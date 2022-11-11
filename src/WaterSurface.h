/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#ifndef WATER_SURFACE_RENDERING_WATER_SURFACE_H_
#define WATER_SURFACE_RENDERING_WATER_SURFACE_H_

#include "core/Application.h"

#include "vulkan/RenderPass.h"
#include "vulkan/Descriptors.h"
#include "vulkan/ShaderModule.h"
#include "vulkan/Pipeline.h"
#include "vulkan/Texture2D.h"

#include "scene/Camera.h"
#include "scene/WaterSurfaceMesh.h"


class WaterSurface : public vkp::Application
{
public:
    WaterSurface(AppCmdLineArgs args);
    ~WaterSurface();

protected:
    /** @brief Called once in "Run" function, before the main loop */
    void Start() override;
    
    /** @brief Called each frame, before rendering */
    void Update(vkp::Timestep deltaTime) override;

    /** @brief Render call, called each frame */
    void Render(
        uint32_t frameIndex,
        vkp::Timestep dt,
        std::vector<VkPipelineStageFlags>& stagesToWait,
        std::vector<VkCommandBuffer>& buffersToSubmit) override;

    void OnFrameBufferResize(int width, int height) override;
    void OnMouseMove(double xpos, double ypos) override;
    void OnMousePressed(int button, int action, int mods) override;
    void OnKeyPressed(int key, int action, int mods) override;
    void OnCursorEntered(int entered) override;

private:
    void CreateRenderPass();
    void BeginRenderPass(VkCommandBuffer cmdBuffer,
                         VkFramebuffer framebuffer);

    void CreateDrawCommandPools(const uint32_t count);
    void CreateDrawCommandBuffers();
    void DestroyDrawCommandPools();

    void CreateDescriptorPool();

    void SetupAssets();
        void SetupGUI();
        void CreateCamera();
        void CreateWaterSurfaceMesh();

    void UpdateCamera(vkp::Timestep dt);

    // GUI:
    void UpdateGui();
    void ShowStatusWindow() const;
    void ShowCameraSettings();
    void ShowControlsWindow(bool* p_open) const;

private:
    // TODO maybe into app
    std::unique_ptr<vkp::RenderPass> m_RenderPass{ nullptr };

    // TODO maybe into App
    std::array<VkClearValue, 2> m_ClearValues{
        VkClearValue{ 0.118f, 0.235f, 0.314f, 1.0f }, // clear color
        VkClearValue{ 1.0f, 0.0f, 0.0f, 0.0f }  // clear depth, stencil
    };

    // TODO maybe into app
    std::vector<vkp::CommandPool> m_DrawCmdPools;

    std::unique_ptr<vkp::DescriptorPool> m_DescriptorPool{ nullptr };

    // =========================================================================

    /// @brief Application states
    enum class States
    {
        GuiControls = 0,    ///< Gui is shown and captures mouse controls
        CameraControls,     ///< Gui is hidden and camera captures mouse controls

        TotalStates
    };

    States m_State{ States::GuiControls };  ///< Current application state

    enum Controls
    {
        KeyHideGui = GLFW_KEY_ESCAPE,
    };

    // -------------------------------------------------------------------------
    // Assets

    std::unique_ptr<vkp::Camera> m_Camera{ nullptr };
    static constexpr glm::vec3 s_kCamStartPos{ 0.f, 340.f, -420.f };

    // Water Surfaces
    std::unique_ptr<WaterSurfaceMesh> m_WaterSurfaceMesh{ nullptr };

};

#endif // WATER_SURFACE_RENDERING_WATER_SURFACE_H_
