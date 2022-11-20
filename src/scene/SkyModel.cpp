
/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#include "pch.h"
#include "scene/SkyModel.h"

#include <imgui/imgui.h>


SkyModel::SkyModel(
    const vkp::Device& device,
    const vkp::DescriptorPool& descriptorPool,
    const glm::vec3& sunDir
)
    : m_kDevice(device),
      m_kDescriptorPool(descriptorPool)
{
    VKP_REGISTER_FUNCTION();

    CreateDescriptorSetLayout();
    SetupPipeline();

    m_Sky.reset( new SkyPreetham(sunDir) );
    UpdateSkyUBO();
}

SkyModel::~SkyModel()
{
    VKP_REGISTER_FUNCTION();
}

void SkyModel::CreateRenderData(
    VkRenderPass renderPass,
    const uint32_t kImageCount,
    const VkExtent2D kFramebufferExtent,
    const bool kFramebufferHasDepthAttachment
)
{
    VKP_REGISTER_FUNCTION();

    CreatePipeline(kFramebufferExtent,
                   renderPass,
                   kFramebufferHasDepthAttachment);

    const bool kImageCountChanged = m_UniformBuffers.size() != kImageCount;
    if (kImageCountChanged)
    {
        VKP_LOG_WARN("Image Count Changed!");

        m_UniformBuffers.clear();
        CreateUniformBuffers(kImageCount);

        m_DescriptorSets.clear();
        CreateDescriptorSets(kImageCount);

        UpdateDescriptorSets();
    }
}

void SkyModel::Update(float dt)
{

}

void SkyModel::PrepareRender(
    const uint32_t frameIndex,
    glm::uvec2 screenResolution,
    const glm::vec3& camPos,
    const glm::mat3& camView,
    float camFOV
)
{
    m_SkyUBO.resolution = screenResolution;
    m_SkyUBO.camPos = camPos;
    m_SkyUBO.camViewRow0 = camView[0];
    m_SkyUBO.camViewRow1 = camView[1];
    m_SkyUBO.camViewRow2 = camView[2];
    m_SkyUBO.camFOV = camFOV;

    UpdateUniformBuffer(m_UniformBuffers[frameIndex]);

    // TODO not needed
    //UpdateDescriptorSet(
    //    m_DescriptorSets[frameIndex],
    //    m_UniformBuffers[frameIndex]
    //);
}

void SkyModel::Render(
    const uint32_t frameIndex,
    VkCommandBuffer cmdBuffer
)
{
   vkCmdBindPipeline(
       cmdBuffer,
       VK_PIPELINE_BIND_POINT_GRAPHICS, 
       *m_Pipeline
   );

   const uint32_t kFirstSet = 0, kDescriptorSetCount = 1;
   const uint32_t kDynamicOffsetCount = 0;
   const uint32_t* kDynamicOffsets = nullptr;

   vkCmdBindDescriptorSets(
       cmdBuffer,
       VK_PIPELINE_BIND_POINT_GRAPHICS,
       *m_Pipeline,
       kFirstSet,
       kDescriptorSetCount,
       &m_DescriptorSets[frameIndex].set,
       kDynamicOffsetCount,
       kDynamicOffsets
   );

   // Draw fullscreen triangle
   const uint32_t kVertexCount = 3, kInstanceCount = 1;
   const uint32_t kFirstVertex = 0, kFirstInstance = 0;
   vkCmdDraw(cmdBuffer, kVertexCount, kInstanceCount, 
                        kFirstVertex, kFirstInstance);
}

// -----------------------------------------------------------------------------
// Update data functions

void SkyModel::UpdateUniformBuffer(const vkp::Buffer& buffer)
{
    buffer.CopyToMapped(&m_SkyUBO, sizeof(m_SkyUBO));
    buffer.FlushMappedRange();
}

void SkyModel::UpdateSkyUBO()
{
    m_SkyUBO.params.props = m_Sky->GetProperties();
}

void SkyModel::UpdateDescriptorSets()
{
    VKP_ASSERT(m_DescriptorSets.size() == m_UniformBuffers.size());

    for (uint32_t i = 0; i < m_DescriptorSets.size(); ++i)
    {
        UpdateDescriptorSet(
            m_DescriptorSets[i],
            m_UniformBuffers[i]
        );
    }
}

void SkyModel::UpdateDescriptorSet(
    DescriptorSet& descriptor,
    const vkp::Buffer& buffer
)
{
    if (descriptor.isDirty == false)
        return;

    VKP_REGISTER_FUNCTION();

    VkDescriptorBufferInfo bufferInfos[1] = {};
    bufferInfos[0].buffer = buffer;
    bufferInfos[0].offset = 0;
    bufferInfos[0].range = sizeof(SkyUBO);

    vkp::DescriptorWriter descriptorWriter(*m_DescriptorSetLayout,
                                            m_kDescriptorPool);
    uint32_t binding = 0;

    descriptorWriter.AddBufferDescriptor(binding++, &bufferInfos[0]);

    descriptorWriter.UpdateSet(descriptor.set);
    descriptor.isDirty = false;
}

// -----------------------------------------------------------------------------
// Creation functions

void SkyModel::CreateDescriptorSetLayout()
{
    VKP_REGISTER_FUNCTION();

    uint32_t bindingPoint = 0;

    m_DescriptorSetLayout = vkp::DescriptorSetLayout::Builder(m_kDevice)
        // SkyUBO
        .AddBinding({
            .binding = bindingPoint++,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
        })
        .Build();
}

void SkyModel::CreateUniformBuffers(const uint32_t kBufferCount)
{
    VKP_REGISTER_FUNCTION();

    const VkDeviceSize kBufferSize = 
        m_kDevice.GetUniformBufferAlignment(sizeof(SkyUBO));

    m_UniformBuffers.reserve(kBufferCount);

    for (uint32_t i = 0; i < kBufferCount; ++i)
    {
        auto& buffer = m_UniformBuffers.emplace_back(m_kDevice);

        buffer.Create(kBufferSize,
                      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

        auto err = buffer.Map();
        VKP_ASSERT_RESULT(err);
    }
}

// TODO move
std::vector<
    std::shared_ptr<vkp::ShaderModule>
> SkyModel::CreateShadersFromShaderInfos(
        const vkp::ShaderInfo* kShaderInfos,
        const uint32_t kShaderInfoCount) const
{
    std::vector<
        std::shared_ptr<vkp::ShaderModule>
    > shaders(kShaderInfoCount, nullptr);

    for (uint32_t i = 0; i < kShaderInfoCount; ++i)
    {
        shaders[i] = std::make_shared<vkp::ShaderModule>(
            m_kDevice, kShaderInfos[i]
        );
    }

    return shaders;
}

void SkyModel::SetupPipeline()
{
    VKP_REGISTER_FUNCTION();

    std::vector<
        std::shared_ptr<vkp::ShaderModule>
    > shaders = CreateShadersFromShaderInfos(s_kShaderInfos.data(),
                                             s_kShaderInfos.size());

    m_Pipeline = std::make_unique<vkp::Pipeline>(
        m_kDevice, shaders
    );

    VKP_ASSERT(m_DescriptorSetLayout != nullptr);
    VKP_ASSERT(m_DescriptorSetLayout->GetLayout() != VK_NULL_HANDLE);

    // Set descriptor set layout
    {
        const auto& descriptorSetLayout = m_DescriptorSetLayout->GetLayout();

        auto& pipelineLayoutInfo = m_Pipeline->GetPipelineLayoutInfo();
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    }

    // Fullscreen triangle
    m_Pipeline->SetVertexInputState( vkp::Pipeline::InitVertexInput() );

    // Set front face as counter clockwise
    auto rasterizationState = vkp::Pipeline::InitRasterization();
    rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
    rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    m_Pipeline->SetRasterizationState(rasterizationState);
}

void SkyModel::CreateDescriptorSets(const uint32_t kCount)
{
    VKP_REGISTER_FUNCTION();

    m_DescriptorSets.resize(kCount);

    VkResult err;
    for (auto& descriptor : m_DescriptorSets)
    {
        err = m_kDescriptorPool.AllocateDescriptorSet(
            *m_DescriptorSetLayout,
            descriptor.set
        );
        VKP_ASSERT_RESULT(err);
    }
}

void SkyModel::CreatePipeline(
    const VkExtent2D& framebufferExtent,
    VkRenderPass renderPass,
    bool framebufferHasDepthAttachment
)
{
    VKP_REGISTER_FUNCTION();
    VKP_ASSERT(m_Pipeline != nullptr);

    m_kDevice.QueueWaitIdle(vkp::QFamily::Graphics);

    // Disable depth test
    m_Pipeline->Create(framebufferExtent,
                       renderPass,
                       false);
}

void SkyModel::RecompileShaders(
    VkRenderPass renderPass,
    const VkExtent2D kFramebufferExtent,
    const bool kFramebufferHasDepthAttachment
)
{
    const bool kNeedsRecreation = m_Pipeline->RecompileShaders();
    if (kNeedsRecreation)
    {
        CreatePipeline(kFramebufferExtent,
                       renderPass,
                       kFramebufferHasDepthAttachment);
    }
}

// =============================================================================
// GUI

static glm::vec3 GetDirFromAngles(float inclination, float azimuth)
{
    return glm::normalize(
        glm::vec3( glm::sin(inclination) * glm::cos(azimuth),
                   glm::cos(inclination),
                   glm::sin(inclination) * glm::sin(azimuth) )
    );
}

void SkyModel::ShowGUISettings()
{
    if ( ImGui::CollapsingHeader("Lighting Settings") )
                                //, ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
            ShowLightingSettings();
        ImGui::PopItemWidth();
        ImGui::NewLine();
    }
}

void SkyModel::ShowLightingSettings()
{
    static float sunAzimuth = glm::radians(90.0f);        // [-pi, pi]
    static float sunInclination = glm::radians(60.0f);    // [-pi/2, pi/2]
    static glm::vec3 sunDir = m_Sky->GetSunDirection();

    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.29f);

    // TODO change to wider range
    bool paramsChanged = ImGui::SliderAngle("##Sun Inclination",
                                            &sunInclination, -90.f, 90.f);
    ImGui::SameLine();
    paramsChanged |= ImGui::SliderAngle("Sun angles##Sun Azimuth",
                                        &sunAzimuth, -180.f, 180.f);
    if (paramsChanged)
    {
        sunDir = GetDirFromAngles(sunInclination, sunAzimuth);
    }
    ImGui::PopItemWidth();

    paramsChanged |=
        ImGui::DragFloat3("Sun Direction",
                          glm::value_ptr(sunDir), 0.1f);

    ImGui::SliderFloat("Sun Intensity", &m_SkyUBO.params.sunIntensity, 0.f, 10.f);
    ImGui::ColorEdit3("Sun Color", glm::value_ptr(m_SkyUBO.params.sunColor));

    static float turbidity = m_Sky->GetTurbidity();
    paramsChanged |= ImGui::SliderFloat("Sky Turbidity", &turbidity, 2.0f, 6.f);

    if (paramsChanged)
    {
        m_Sky->SetSunDirection(sunDir);
        m_Sky->SetTurbidity(turbidity);
        m_Sky->Update();
        UpdateSkyUBO();
    }
}
