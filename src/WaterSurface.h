/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#ifndef WATER_SURFACE_RENDERING_WATER_SURFACE_H_
#define WATER_SURFACE_RENDERING_WATER_SURFACE_H_

#include "core/Application.h"


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

private:
    void SetupGUI();
    void CreateImGuiContext();

    void CreateRenderPass();
    void CreateDrawCommandPools();
    void CreateDrawCommandBuffers();

    void BeginRenderPass(VkCommandBuffer cmdBuffer,
                         VkFramebuffer framebuffer);
    void DestroyDrawCommandPools();

private:
    std::unique_ptr<vkp::RenderPass> m_RenderPass;

    std::vector< std::unique_ptr<vkp::CommandPool> > m_DrawCmdPools{ };
    std::unique_ptr<vkp::Gui> m_Gui{ nullptr };

    std::array<VkClearValue, 2> m_ClearValues{
        VkClearValue{ 0.118f, 0.235f, 0.314f, 1.0f }, // clear color
        VkClearValue{ 1.0f, 0.0f, 0.0f, 0.0f }  // clear depth, stencil
    };
};

#endif // WATER_SURFACE_RENDERING_WATER_SURFACE_H_
