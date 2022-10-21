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

#include "scene/WSTessendorf.h"
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

private:
    void CreateRenderPass();
    void BeginRenderPass(VkCommandBuffer cmdBuffer,
                         VkFramebuffer framebuffer);

    void CreateDrawCommandPools(const uint32_t count);
    void CreateDrawCommandBuffers();
    void DestroyDrawCommandPools();

    void CreateUniformBuffers(const uint32_t kBufferCount);
    void UpdateUniformBuffer(const uint32_t imageIndex);

    void CreateDescriptorPool();

    void SetupWaterSurfaceMeshRendering();
        void CreateWSMeshDescriptorSetLayout();
        void SetupWSMeshPipeline();
        void CreateWSMeshPipeline();
        void CreateWSMeshDescriptorSets(const uint32_t kCount);
        // Once, and later only when dirty
        void UpdateWSMeshDescriptorSets();
        void UpdateWSMeshDescriptorSet(const uint32_t frameIndex);

    void SetupAssets();
        void SetupGUI();
        void CreateWaterSurfaces();

    void UpdateWaterSurfaceMesh();
    void RenderWaterSurfaceMesh(VkCommandBuffer cmdBuffer,
                                const uint32_t frameIndex);

    // On properties change:

    void PrepareWaterSurfaceMesh(uint32_t size, float scale);

private:
    std::unique_ptr<vkp::RenderPass> m_RenderPass;  // TODO maybe into app

    // TODO maybe into App
    std::array<VkClearValue, 2> m_ClearValues{
        VkClearValue{ 0.118f, 0.235f, 0.314f, 1.0f }, // clear color
        VkClearValue{ 1.0f, 0.0f, 0.0f, 0.0f }  // clear depth, stencil
    };

    // TODO maybe into app
    std::vector< std::unique_ptr<vkp::CommandPool> > m_DrawCmdPools;

    // -------------------------------------------------------------------------
    // Resource Descriptors

    std::unique_ptr<vkp::DescriptorPool> m_DescriptorPool;

    // Uniform buffer for each swap chain image
    //   Cleanup after fininshed rendering or on swap chain recreation
    std::vector<vkp::Buffer> m_UniformBuffers;

    // =========================================================================
    // Assets

    // -------------------------------------------------------------------------
    // Water Surfaces

    struct VertexUBO
    {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

    struct WaterSurfaceUBO
    {
        float heightAmp{ 1.0 };  ///< Water surface height amplitude
    };

    VertexUBO m_VertexUBO{
        .model = glm::mat4(1.0f),
        .view = glm::mat4(1.0f),
        .proj = glm::mat4(1.0f)
    };
    WaterSurfaceUBO m_WaterSurfaceUBO{};

    std::unique_ptr<vkp::DescriptorSetLayout> m_WSMeshDescriptorSetLayout;
    std::vector<VkDescriptorSet> m_WSMeshDescriptorSets;

    std::unique_ptr<WSTessendorf> m_WSTess;
    std::unique_ptr<WaterSurfaceMesh> m_WSMesh;


    // -------------------------------------------------------------------------
    // Rendering water surface as a mesh

    static const inline std::array<vkp::ShaderInfo, 2> s_kWSMeshShaderInfos {
        vkp::ShaderInfo{
            .paths = { "shaders/WaterSurfaceMesh.vert" },
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .isSPV = false
        },
        vkp::ShaderInfo{
            .paths = { "shaders/WaterSurfaceMesh.frag" },
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .isSPV = false
        }
    };

    std::unique_ptr<vkp::Pipeline> m_WSMeshPipeline;
};

#endif // WATER_SURFACE_RENDERING_WATER_SURFACE_H_
