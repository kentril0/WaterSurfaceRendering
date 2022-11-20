/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#ifndef WATER_SURFACE_RENDERING_SCENE_SKY_MODEL_H_
#define WATER_SURFACE_RENDERING_SCENE_SKY_MODEL_H_

#include <vector>
#include <memory>

#include "vulkan/Device.h"
#include "vulkan/Descriptors.h"
#include "vulkan/ShaderModule.h"
#include "vulkan/Buffer.h"
#include "vulkan/Pipeline.h"

#include "scene/SkyPreetham.h"


class SkyModel
{
public:
    struct Params
    {
        alignas(16) glm::vec3 sunColor    { 1.0 };
                    float     sunIntensity{ 1.0 };
        // -------------------------------------------------
        SkyPreetham::Props props;
    };

public:
    SkyModel(const vkp::Device& device,
             const vkp::DescriptorPool& descriptorPool,
             const glm::vec3& sunDir);
    ~SkyModel();

    void CreateRenderData(
        VkRenderPass renderPass,
        const uint32_t kImageCount,
        const VkExtent2D kFramebufferExtent,
        const bool kFramebufferHasDepthAttachment);

    void Update(float dt);

    void PrepareRender(
        const uint32_t frameIndex,
        glm::uvec2 screenResolution,
        const glm::vec3& camPos,
        const glm::mat3& camView,
        float camFOV
    );
 
    void Render(
        const uint32_t frameIndex,
        VkCommandBuffer cmdBuffer);

    // @pre Called inside ImGui Window scope
    void ShowGUISettings()
    {
        ShowLightingSettings();
    }

    void RecompileShaders(
        VkRenderPass renderPass,
        const VkExtent2D kFramebufferExtent,
        const bool kFramebufferHasDepthAttachment);
    
    const Params& GetParams() const { return m_SkyUBO.params; }

private:
    struct DescriptorSet;

    void CreateDescriptorSetLayout();
    void SetupPipeline();

    void CreateUniformBuffers(const uint32_t kBufferCount);
    void CreateDescriptorSets(const uint32_t kCount);
    void CreatePipeline(
        const VkExtent2D& framebufferExtent,
        VkRenderPass renderPass,
        bool framebufferHasDepthAttachment
    );

    void UpdateDescriptorSets();
    void UpdateDescriptorSet(
        DescriptorSet& descriptor,
        const vkp::Buffer& buffer
    );
    void UpdateUniformBuffer(const vkp::Buffer& buffer);
    void UpdateSkyUBO();

    std::vector<
        std::shared_ptr<vkp::ShaderModule>
    > CreateShadersFromShaderInfos(const vkp::ShaderInfo* kShaderInfos,
                                   const uint32_t kShaderInfoCount) const;

    void ShowLightingSettings();

private:
    const vkp::Device&         m_kDevice;
    // TODO static GetDescriptorPoolSizes
    const vkp::DescriptorPool& m_kDescriptorPool;

    // =========================================================================

    static const inline std::array<vkp::ShaderInfo, 2> s_kShaderInfos {
        vkp::ShaderInfo{
            .paths = { "shaders/FullScreenQuad.vert" },
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .isSPV = false
        },
        vkp::ShaderInfo{
            .paths = { "shaders/SkyPreetham.frag" },
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .isSPV = false
        }
    };

    std::unique_ptr<vkp::DescriptorSetLayout> m_DescriptorSetLayout{ nullptr };

    struct DescriptorSet
    {
        bool            isDirty{ true };
        VkDescriptorSet set    { VK_NULL_HANDLE };
    };
    std::vector<DescriptorSet> m_DescriptorSets;

    std::unique_ptr<vkp::Pipeline> m_Pipeline{ nullptr };

    std::vector<vkp::Buffer> m_UniformBuffers;

    // =========================================================================

    struct SkyUBO
    {
        alignas(16) glm::vec2 resolution;
        alignas(16) glm::vec3 camViewRow0;
        alignas(16) glm::vec3 camViewRow1;
        alignas(16) glm::vec3 camViewRow2;
        alignas(16) glm::vec3 camPos;
        float camFOV;
        Params params;
    };
    SkyUBO m_SkyUBO;

    std::unique_ptr<SkyPreetham> m_Sky{ nullptr };
};


#endif // WATER_SURFACE_RENDERING_SCENE_SKY_MODEL_H_
