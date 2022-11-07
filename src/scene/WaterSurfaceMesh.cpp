
/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#include "pch.h"
#include "scene/WaterSurfaceMesh.h"

#include <imgui/imgui.h>


/**
 * @pre size > 0
 * @return Size 'size' aligned to 'alignment'
 */
static size_t AlignSizeTo(size_t size, size_t alignment)
{
    return (size + alignment - 1) & ~(alignment -1);
}

// =============================================================================

WaterSurfaceMesh::WaterSurfaceMesh(
    const vkp::Device& device,
    const vkp::DescriptorPool& descriptorPool
)
    : m_Device(device),
      m_DescriptorPool(descriptorPool)
{
    VKP_REGISTER_FUNCTION();

    CreateDescriptorSetLayout();
    SetupPipeline();

    CreateStagingBuffer();

    CreateTessendorfModel();
    CreateMesh();
}

WaterSurfaceMesh::~WaterSurfaceMesh()
{
    VKP_REGISTER_FUNCTION();
}

void WaterSurfaceMesh::CreateRenderData(
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
        // TODO must update !!
        ///UpdateDescriptorSets();
    }
}

void WaterSurfaceMesh::Prepare(VkCommandBuffer cmdBuffer)
{
    CreateFrameMaps(cmdBuffer);
    const uint32_t kIndex = s_kWSResolutions.GetIndex(m_ModelTess->GetTileSize());
    m_CurFrameMap = &m_FrameMaps[kIndex];

    PrepareModelTess(cmdBuffer);
    PrepareMesh(cmdBuffer);

    // TODO temp
    UpdateDescriptorSets();
}

void WaterSurfaceMesh::PrepareMesh(VkCommandBuffer cmdBuffer)
{
    GenerateMeshVerticesIndices();
    UpdateMeshBuffers(cmdBuffer);
}

void WaterSurfaceMesh::GenerateMeshVerticesIndices()
{
    VKP_REGISTER_FUNCTION();

    std::vector<Vertex> vertices = CreateGridVertices(m_TileSize,
                                                      m_VertexDistance);
    m_Mesh->SetVertices(std::move(vertices));

    std::vector<uint32_t> indices = CreateGridIndices(m_TileSize);
    m_Mesh->SetIndices(std::move(indices));
}

void WaterSurfaceMesh::UpdateMeshBuffers(VkCommandBuffer cmdBuffer)
{
    bool updated = m_Mesh->UpdateDeviceBuffers(*m_StagingBuffer, cmdBuffer);
    if (!updated)
        return;

    m_StagingBuffer->FlushMappedRange(
        m_Device.GetNonCoherentAtomSizeAlignment(
            AlignSizeTo(m_Mesh->GetVerticesSize() + m_Mesh->GetIndicesSize(),
                        vkp::Texture2D::FormatToBytes(s_kMapFormat) )
        )
    );
}

void WaterSurfaceMesh::PrepareModelTess(VkCommandBuffer cmdBuffer)
{
    VKP_REGISTER_FUNCTION();

    m_ModelTess->Prepare();

    // Do one pass to initialize the maps

    m_VertexUBO.WSHeightAmp = m_ModelTess->ComputeWaves(m_TimeCtr);

    CopyModelTessDataToStagingBuffer();

    // Frame for upcomming render
    // TODO
    m_FrameMapIndex = 0;

    UpdateFrameMaps(
        cmdBuffer,
        m_CurFrameMap->data[m_FrameMapIndex]
    );
}

void WaterSurfaceMesh::Update(float dt)
{
    if (m_PlayAnimation)
    {
        m_TimeCtr += dt * m_AnimSpeed;

        m_VertexUBO.WSHeightAmp = m_ModelTess->ComputeWaves(m_TimeCtr);

        CopyModelTessDataToStagingBuffer();
    }
}

void WaterSurfaceMesh::PrepareRender(
    const uint32_t frameIndex,
    VkCommandBuffer cmdBuffer,
    const glm::mat4& viewMat,
    const glm::mat4& projMat,
    const glm::vec3& camPos
)
{
    //m_VertexUBO.model = glm::scale(glm::mat4(1.0f), glm::vec3(5.f, 5.f, 5.f));
    m_VertexUBO.model = glm::mat4(1.0f);
    m_VertexUBO.view = viewMat;
    m_VertexUBO.proj = projMat;

    // Vulkan uses inverted Y coord in comparison to OpenGL (set by glm lib)
    // -> flip the sign on the scaling factor of the Y axis
    m_VertexUBO.proj[1][1] *= -1;
    
    //const VkExtent2D screenExt = m_SwapChain->GetExtent();
    //m_WaterSurfaceUBO.resolution.x = static_cast<float>(screenExt.width);
    //m_WaterSurfaceUBO.resolution.y = static_cast<float>(screenExt.height);

    m_WaterSurfaceUBO.camPos = camPos;
    //const glm::mat3 view = m_Camera->GetView();
    //m_WaterSurfaceUBO.viewMatRow0 = view[0];
    //m_WaterSurfaceUBO.viewMatRow1 = view[1];
    //m_WaterSurfaceUBO.viewMatRow2 = view[2];

    //m_WaterSurfaceUBO.camFOV = m_Camera->GetFov();
    //m_WaterSurfaceUBO.camNear = m_Camera->GetNear();
    //m_WaterSurfaceUBO.camFar = m_Camera->GetFar();

    m_WaterSurfaceUBO.terrainDepth = glm::min(m_WaterSurfaceUBO.terrainDepth,
                                              m_ModelTess->GetMinHeight());
    UpdateUniformBuffer(frameIndex);

    UpdateMeshBuffers(cmdBuffer);

#ifndef DOUBLE_BUFFERED
    uint32_t transferIndex = 0;

    UpdateFrameMaps(
        cmdBuffer,
        m_CurFrameMap->data[transferIndex]
    );
    // TODO if curMapChanged
    // TODO update only if dirty
    UpdateDescriptorSet(frameIndex);

#else
    UpdateDescriptorSet(frameIndex);

    // TODO more general to size
    uint32_t transferIndex = m_FrameMapIndex == 0 ? 1 : 0;

    UpdateFrameMaps(
        cmdBuffer,
        m_CurFrameMap->data[transferIndex]
    );

    // TODO more general to size
    //m_FrameMapIndex = m_FrameMapIndex == 0 ? 1 : 0;
#endif
}

void WaterSurfaceMesh::Render(
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
        &m_DescriptorSets[frameIndex],
        kDynamicOffsetCount,
        kDynamicOffsets
    );

    m_Mesh->Render(cmdBuffer);

#ifdef DOUBLE_BUFFERED
    m_FrameMapIndex = m_FrameMapIndex == 0 ? 1 : 0;
#endif
}

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
// Update data functions

void WaterSurfaceMesh::UpdateUniformBuffer(const uint32_t imageIndex)
{
    auto& buffer = m_UniformBuffers[imageIndex];

    void* dataAddr = buffer.GetMappedAddress();

    buffer.CopyToMapped(&m_VertexUBO, sizeof(m_VertexUBO));
    buffer.CopyToMapped(&m_WaterSurfaceUBO, sizeof(m_WaterSurfaceUBO),
        static_cast<void*>(
            static_cast<uint8_t*>(dataAddr) + 
            m_Device.GetUniformBufferAlignment(sizeof(VertexUBO))
        )
    );

    buffer.FlushMappedRange();
}

void WaterSurfaceMesh::UpdateDescriptorSets()
{
    VKP_ASSERT(m_DescriptorSets.size() == m_UniformBuffers.size());

    for (uint32_t i = 0; i < m_DescriptorSets.size(); ++i)
    {
        UpdateDescriptorSet(i);
    }
}

void WaterSurfaceMesh::UpdateDescriptorSet(const uint32_t frameIndex)
{
    VKP_REGISTER_FUNCTION();
    VKP_ASSERT(frameIndex < m_DescriptorSets.size());

    vkp::DescriptorWriter descriptorWriter(*m_DescriptorSetLayout,
                                           m_DescriptorPool);

    // Add water surface uniform buffers
    VkDescriptorBufferInfo bufferInfos[2] = {};
    bufferInfos[0].buffer = m_UniformBuffers[frameIndex];
    bufferInfos[0].offset = 0;
    bufferInfos[0].range = sizeof(VertexUBO);

    bufferInfos[1].buffer = m_UniformBuffers[frameIndex];
    bufferInfos[1].offset = 
        m_Device.GetUniformBufferAlignment(sizeof(VertexUBO));
    bufferInfos[1].range = sizeof(WaterSurfaceUBO);

    uint32_t binding = 0;

    descriptorWriter
        .AddBufferDescriptor(binding++, &bufferInfos[0])
        .AddBufferDescriptor(binding++, &bufferInfos[1]);
    
    // Add Water Surface textures
    
    const auto& kFrameMaps = m_CurFrameMap->data[m_FrameMapIndex];

    VKP_ASSERT(kFrameMaps.displacementMap != nullptr);
    VKP_ASSERT(kFrameMaps.normalMap != nullptr);
    
    VkDescriptorImageInfo imageInfos[2] = {};
    imageInfos[0] = kFrameMaps.displacementMap->GetDescriptor();
    // TODO force future image layout
    imageInfos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfos[1] = kFrameMaps.normalMap->GetDescriptor();
    // TODO force future image layout
    imageInfos[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    descriptorWriter
        .AddImageDescriptor(binding++, &imageInfos[0])
        .AddImageDescriptor(binding++, &imageInfos[1]);

    // Update the set
    descriptorWriter.UpdateSet(m_DescriptorSets[frameIndex]);
}

// -----------------------------------------------------------------------------
// Creation functions

void WaterSurfaceMesh::CreateDescriptorSetLayout()
{
    VKP_REGISTER_FUNCTION();

    uint32_t bindingPoint = 0;

    m_DescriptorSetLayout = vkp::DescriptorSetLayout::Builder(m_Device)
        // VertexUBO
        .AddBinding({
            .binding = bindingPoint++,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
        })
        // WaterSurfaceUBO
        .AddBinding({
            .binding = bindingPoint++,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
        })
        // Displacement map
        .AddBinding({
            .binding = bindingPoint++,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
        })
        // Normal map
        .AddBinding({
            .binding = bindingPoint++,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
        })
        .Build();
}

void WaterSurfaceMesh::CreateUniformBuffers(const uint32_t kBufferCount)
{
    VKP_REGISTER_FUNCTION();

    const VkDeviceSize kBufferSize = 
        m_Device.GetUniformBufferAlignment(sizeof(VertexUBO)) +
        m_Device.GetUniformBufferAlignment(sizeof(WaterSurfaceUBO));

    m_UniformBuffers.reserve(kBufferCount);

    for (uint32_t i = 0; i < kBufferCount; ++i)
    {
        m_UniformBuffers.emplace_back(m_Device);

        m_UniformBuffers[i].Create(kBufferSize,
                                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

        // Keep each buffer mapped
        VKP_ASSERT_RESULT(m_UniformBuffers[i].Map());
    }
}

std::vector<
    std::shared_ptr<vkp::ShaderModule>
> WaterSurfaceMesh::CreateShadersFromShaderInfos(
        const vkp::ShaderInfo* kShaderInfos,
        const uint32_t kShaderInfoCount) const
{
    std::vector<
        std::shared_ptr<vkp::ShaderModule>
    > shaders(kShaderInfoCount, nullptr);

    for (uint32_t i = 0; i < kShaderInfoCount; ++i)
    {
        shaders[i] = std::make_shared<vkp::ShaderModule>(
            m_Device, kShaderInfos[i]
        );
    }

    return shaders;
}

void WaterSurfaceMesh::SetupPipeline()
{
    VKP_REGISTER_FUNCTION();

    std::vector<
        std::shared_ptr<vkp::ShaderModule>
    > shaders = CreateShadersFromShaderInfos(s_kShaderInfos.data(),
                                             s_kShaderInfos.size());

    m_Pipeline = std::make_unique<vkp::Pipeline>(
        m_Device, shaders
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

    m_Pipeline->SetVertexInputState(
        vkp::Pipeline::InitVertexInput(Vertex::s_BindingDescriptions,
                                       Vertex::s_AttribDescriptions)
    );
}

void WaterSurfaceMesh::CreateDescriptorSets(const uint32_t kCount)
{
    VKP_REGISTER_FUNCTION();

    m_DescriptorSets.resize(kCount, VK_NULL_HANDLE);

    VkResult err;
    for (auto& set : m_DescriptorSets)
    {
        err = m_DescriptorPool.AllocateDescriptorSet(
            *m_DescriptorSetLayout,
            set
        );
        VKP_ASSERT_RESULT(err);
    }
}

void WaterSurfaceMesh::CreatePipeline(
    const VkExtent2D& framebufferExtent,
    VkRenderPass renderPass,
    bool framebufferHasDepthAttachment
)
{
    VKP_REGISTER_FUNCTION();
    VKP_ASSERT(m_Pipeline != nullptr);

    m_Device.QueueWaitIdle(vkp::QFamily::Graphics);

    m_Pipeline->Create(framebufferExtent,
                       renderPass,
                       framebufferHasDepthAttachment);
}

void WaterSurfaceMesh::CreateTessendorfModel()
{
    VKP_REGISTER_FUNCTION();

    const auto kSampleCount = m_ModelTess->s_kDefaultTileSize;
    const auto kWaveLength = m_ModelTess->s_kDefaultTileLength;

    m_ModelTess.reset( new WSTessendorf(kSampleCount, kWaveLength) );
}

void WaterSurfaceMesh::CreateMesh()
{
    VKP_REGISTER_FUNCTION();

    const VkDeviceSize kMaxVerticesSize = sizeof(Vertex) * GetMaxVertexCount();
    const VkDeviceSize kMaxIndicesSize = sizeof(uint32_t) * GetMaxIndexCount();

    m_Mesh.reset(
        new Mesh<Vertex>(m_Device, kMaxVerticesSize, kMaxIndicesSize)
    );
}

std::vector<WaterSurfaceMesh::Vertex> WaterSurfaceMesh::CreateGridVertices(
    const uint32_t kTileSize,
    const float kScale
)
{
    VKP_REGISTER_FUNCTION();

    std::vector<Vertex> vertices;

    vertices.reserve( GetTotalVertexCount(kTileSize) );

    const int32_t kHalfSize = kTileSize / 2;

    for (int32_t y = -kHalfSize; y <= kHalfSize; ++y)
    {
        for (int32_t x = -kHalfSize; x <= kHalfSize; ++x)
        {
            vertices.emplace_back(
                // Position
                glm::vec3(
                    static_cast<float>(x),      // x
                    0.0f,                       // y
                    static_cast<float>(y)       // z
                ) * kScale
                ,
                // Texcoords
                glm::vec2(
                    static_cast<float>(x + kHalfSize),  // u
                    static_cast<float>(y + kHalfSize)   // v
                ) / static_cast<float>(kTileSize)
            );
        }
    }

    return vertices;
}

std::vector<uint32_t> WaterSurfaceMesh::CreateGridIndices(
    const uint32_t kTileSize
)
{
    VKP_REGISTER_FUNCTION();

    const uint32_t kVertexCount = kTileSize+1;

    std::vector<uint32_t> indices;
    indices.reserve( GetTotalIndexCount(kVertexCount * kVertexCount) );

    for (uint32_t y = 0; y < kTileSize; ++y)
    {
        for (uint32_t x = 0; x < kTileSize; ++x)
        {
            const uint32_t kVertexIndex = y * kVertexCount + x;

            // Top triangle
            indices.emplace_back(kVertexIndex);
            indices.emplace_back(kVertexIndex + kVertexCount);
            indices.emplace_back(kVertexIndex + 1);

            // Bottom triangle
            indices.emplace_back(kVertexIndex + 1);
            indices.emplace_back(kVertexIndex + kVertexCount);
            indices.emplace_back(kVertexIndex + kVertexCount + 1);
        }
    }

    return indices;
}

void WaterSurfaceMesh::CreateStagingBuffer()
{
    m_StagingBuffer.reset( new vkp::Buffer(m_Device) );

    const VkDeviceSize kVerticesSize = sizeof(Vertex) * GetMaxVertexCount();
    const VkDeviceSize kIndicesSize = sizeof(uint32_t) * GetMaxIndexCount();

    const VkDeviceSize kMapSize = vkp::Texture2D::FormatToBytes(s_kMapFormat) *
                                  s_kMaxTileSize * s_kMaxTileSize;

    const VkDeviceSize kTotalSize = 
        AlignSizeTo(kVerticesSize + kIndicesSize,
                    vkp::Texture2D::FormatToBytes(s_kMapFormat)) +
        (kMapSize * 2) // * m_FrameMaps.size()
    ;

    m_StagingBuffer->Create(kTotalSize,
                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    // TODO can map to lower size after each resize
    auto err = m_StagingBuffer->Map();
    VKP_ASSERT_RESULT(err);
}

void WaterSurfaceMesh::CreateFrameMaps(VkCommandBuffer cmdBuffer)
{
    VKP_REGISTER_FUNCTION();

    m_Device.QueueWaitIdle(vkp::QFamily::Graphics);

    // Create frame map pair for each water surface resolution
    m_FrameMaps.resize(s_kWSResolutions.size());

    for (uint32_t i = 0; i < s_kWSResolutions.size(); ++i)
    {
        const uint32_t kSize = s_kWSResolutions[i];

        for (auto& frame : m_FrameMaps[i].data)
        {
            frame.displacementMap = CreateMap(cmdBuffer,
                                              kSize,
                                              s_kMapFormat,
                                              s_kUseMipMapping);
            frame.normalMap = CreateMap(cmdBuffer,
                                        kSize,
                                        s_kMapFormat,
                                        s_kUseMipMapping);
        }
    }

    m_FrameMapIndex = 0;
}

std::unique_ptr<vkp::Texture2D> WaterSurfaceMesh::CreateMap(
    VkCommandBuffer cmdBuffer,
    const uint32_t kSize,
    const VkFormat kMapFormat,
    const bool kUseMipMapping
)
{
    VKP_REGISTER_FUNCTION();

    auto map = std::make_unique<vkp::Texture2D>(m_Device);

    map->Create(cmdBuffer, kSize, kSize, kMapFormat,
                kUseMipMapping,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                VK_IMAGE_USAGE_SAMPLED_BIT);

    return map;
}

void WaterSurfaceMesh::UpdateFrameMaps(
    VkCommandBuffer cmdBuffer,
    FrameMapData& frame
)
{
    VkDeviceSize stagingBufferOffset =
        AlignSizeTo(m_Mesh->GetVerticesSize() + m_Mesh->GetIndicesSize(),
                    vkp::Texture2D::FormatToBytes(s_kMapFormat) );

#ifndef DOUBLE_BUFFERED
    frame.displacementMap->CopyFromBuffer(
        cmdBuffer,
        *m_StagingBuffer,
        s_kUseMipMapping,
        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
        stagingBufferOffset
    );
#else
    frame.displacementMap->CopyFromBuffer(
        cmdBuffer,
        *m_StagingBuffer,
        s_kUseMipMapping,
        //VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
        //VK_PIPELINE_STAGE_NONE,
        //VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        stagingBufferOffset,
        0   // dstaccess
    );
#endif

    const VkDeviceSize kMapSize = vkp::Texture2D::FormatToBytes(s_kMapFormat) *
                                  frame.displacementMap->GetWidth() *
                                  frame.displacementMap->GetHeight();
    stagingBufferOffset += kMapSize;

#ifndef DOUBLE_BUFFERED
    frame.normalMap->CopyFromBuffer(
        cmdBuffer,
        *m_StagingBuffer,
        s_kUseMipMapping,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        stagingBufferOffset
    );
#else
    frame.normalMap->CopyFromBuffer(
        cmdBuffer,
        *m_StagingBuffer,
        s_kUseMipMapping,
        //VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        //VK_PIPELINE_STAGE_NONE,
        //VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        stagingBufferOffset,
        0   // dstaccess
    );
#endif
}

void WaterSurfaceMesh::CopyModelTessDataToStagingBuffer()
{
    VKP_ASSERT(m_StagingBuffer != nullptr);

    const VkDeviceSize kDisplacementsSize = sizeof(WSTessendorf::Displacement) *
                                            m_ModelTess->GetDisplacementCount();
    const VkDeviceSize kNormalsSize = sizeof(WSTessendorf::Normal) *
                                      m_ModelTess->GetNormalCount();

    // StagingBuffer layout: 
    //  ----------------------------------------------
    // | Vertices | Indices | Displacements | Normals |
    //  ----------------------------------------------
    // mapped

    void* stagingData = m_StagingBuffer->GetMappedAddress();
    VKP_ASSERT(stagingData != nullptr);

    // Copy displacements
    {
        stagingData = static_cast<void*>(
            static_cast<uint8_t*>(stagingData) +
                AlignSizeTo(m_Mesh->GetVerticesSize() + m_Mesh->GetIndicesSize(),
                            vkp::Texture2D::FormatToBytes(s_kMapFormat) )
        );

        m_StagingBuffer->CopyToMapped(
            m_ModelTess->GetDisplacements().data(),
            kDisplacementsSize,
            stagingData
        );
    }

    // Copy normals
    {
        stagingData = static_cast<void*>(
            static_cast<uint8_t*>(stagingData) + kDisplacementsSize
        );
        
        m_StagingBuffer->CopyToMapped(
            m_ModelTess->GetNormals().data(),
            kNormalsSize,
            stagingData
        );
    }

    m_StagingBuffer->FlushMappedRange(
        m_Device.GetNonCoherentAtomSizeAlignment(
            AlignSizeTo(m_Mesh->GetVerticesSize() + m_Mesh->GetIndicesSize(),
                        vkp::Texture2D::FormatToBytes(s_kMapFormat) ) +
            kDisplacementsSize +
            kNormalsSize
        )
    );
}

// -----------------------------------------------------------------------------
// GUI

void WaterSurfaceMesh::ShowGUISettings()
{
    ShowWaterSurfaceSettings();
    ShowMeshSettings();
    ShowLightingSettings();
}

void WaterSurfaceMesh::ShowWaterSurfaceSettings()
{
    if (ImGui::CollapsingHeader("Water Surface settings",
                                ImGuiTreeNodeFlags_DefaultOpen))
    {
        static int tileRes = s_kWSResolutions.GetIndex(m_TileSize);
        const char* resName =
            (tileRes >= 0 && tileRes < s_kWSResolutions.size())
            ? s_kWSResolutions.strings[tileRes]
            : "Unknown";

        static float tileLen = m_ModelTess->GetTileLength();
        static glm::vec2 windDir = m_ModelTess->GetWindDir();
        static float windSpeed = m_ModelTess->GetWindSpeed();
        static float animPeriod = m_ModelTess->GetAnimationPeriod();
        static float phillipsA = m_ModelTess->GetPhillipsConst();
        static float damping = m_ModelTess->GetDamping();
        static float lambda = m_ModelTess->GetDisplacementLambda();

        ImGui::SliderInt("Sample Resolution", &tileRes, 0,
                         s_kWSResolutions.size() -1, resName);
        ImGui::DragFloat("Tile Length", &tileLen, 2.0f, 0.0f, 1024.0f, "%.0f");
        // TODO unit
        // TODO 2 angles
        ImGui::DragFloat2("Wind Direction", glm::value_ptr(windDir),
                          0.1f, 0.0f, 0.0f, "%.1f");
        ImGui::DragFloat("Wind Speed", &windSpeed, 0.1f);
        ImGui::DragFloat("Lambda", &lambda,
                              0.1f, -8.0f, 8.0f, "%.1f");
        ImGui::DragFloat("Animation Period", &animPeriod, 1.0f, 1.0f, 0.0f, "%.0f");
        ImGui::DragFloat("Animation speed", &m_AnimSpeed, 0.1f, 0.1f, 8.0f);
        ImGui::Checkbox(" Play Animation ", &m_PlayAnimation);

        if (ImGui::TreeNodeEx("Phillips Spectrum", 
                              ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::DragFloat("A constant", &phillipsA, 0.00001f, 0.0f, 1.0f,
                             "%.5f");
            ImGui::DragFloat("Damping factor", &damping, 0.0001f, 0.0f, 1.0f,
                             "%.4f");
            ImGui::TreePop();
        }

        if (ImGui::Button("Apply"))
        {
            const uint32_t kNewSize = s_kWSResolutions[tileRes];
            const bool kTileSizeChanged = kNewSize != m_ModelTess->GetTileSize();

            const bool kTileLengthChanged =
                glm::epsilonNotEqual(tileLen, m_ModelTess->GetTileLength(),
                                     0.001f);
            const bool kWindDirChanged =
                glm::any( glm::epsilonNotEqual(windDir, m_ModelTess->GetWindDir(),
                                     glm::vec2(0.001f) ) );
            const bool kWindSpeedChanged =
                glm::epsilonNotEqual(windSpeed, m_ModelTess->GetWindSpeed(),
                                     0.001f);
            const bool kAnimationPeriodChanged =
                glm::epsilonNotEqual(animPeriod,
                                     m_ModelTess->GetAnimationPeriod(), 0.001f);
            const bool kPhillipsConstChanged =
                glm::epsilonNotEqual(phillipsA,
                                     m_ModelTess->GetPhillipsConst(),
                                     WSTessendorf::s_kDefaultPhillipsConst / 10.f);
            const bool kDampingChanged =
                glm::epsilonNotEqual(damping,
                                     m_ModelTess->GetDamping(),
                                     0.001f);

            const bool kNeedsPrepare =
                kTileSizeChanged ||
                kTileLengthChanged ||
                kWindDirChanged ||
                kWindSpeedChanged ||
                kAnimationPeriodChanged ||
                kPhillipsConstChanged ||
                kDampingChanged;

            if (kTileSizeChanged)
            {
                m_ModelTess->SetTileSize(kNewSize);

                const uint32_t kIndex =
                    s_kWSResolutions.GetIndex(m_ModelTess->GetTileSize());
                m_CurFrameMap = &m_FrameMaps[kIndex];
            }

            if (kNeedsPrepare)
            {
                m_ModelTess->SetTileSize(s_kWSResolutions[tileRes]);
                m_ModelTess->SetTileLength(tileLen);
                m_ModelTess->SetWindDirection(windDir);
                m_ModelTess->SetWindSpeed(windSpeed);
                m_ModelTess->SetAnimationPeriod(animPeriod);
                m_ModelTess->SetPhillipsConst(phillipsA);
                m_ModelTess->SetDamping(damping);

                m_ModelTess->Prepare();
            }

            m_ModelTess->SetLambda(lambda);
        }

        ImGui::NewLine();
    }
}

void WaterSurfaceMesh::ShowMeshSettings()
{
    if (ImGui::CollapsingHeader("Mesh settings",
                                ImGuiTreeNodeFlags_DefaultOpen))
    {
        static int tileSize = m_TileSize;
        static float tileLength = WSTessendorf::s_kDefaultTileLength;
        static float vertexDist = tileLength / static_cast<float>(tileSize);

        ImGui::SliderInt("Tile Resolution", &tileSize, s_kMinTileSize,
                         s_kMaxTileSize);
        ImGui::DragFloat("Tile Length", &tileLength, 10.f, 10.f, 10000.0f);
        vertexDist = tileLength / static_cast<float>(tileSize);

        ImGui::DragFloat("Vertex Distance", &vertexDist, 0.1f, 0.1f, 100.0f);
        tileLength = vertexDist * tileSize;

        // TODO auto
        if (ImGui::Button("Apply##1"))
        {
            const bool kNeedsRegeneration = 
                tileSize != static_cast<int>(m_TileSize) ||
                glm::epsilonNotEqual(vertexDist, m_VertexDistance, 0.0001f);

            if (kNeedsRegeneration)
            {
                m_TileSize = tileSize;
                m_VertexDistance = vertexDist;

                GenerateMeshVerticesIndices();
            }
        }

        ImGui::NewLine();
    }
}

void WaterSurfaceMesh::ShowLightingSettings()
{
    if (ImGui::CollapsingHeader("Lighting settings",
                                ImGuiTreeNodeFlags_DefaultOpen))
    {
        // TODO zenith, azimuth
        ImGui::DragFloat3("Sun direction",
                          glm::value_ptr(m_WaterSurfaceUBO.sunDir), 0.1f);
        ImGui::SliderFloat("Sun intensity", &m_WaterSurfaceUBO.sunColor.a, 0.f, 10.f);
        ImGui::ColorEdit3("Sun color", glm::value_ptr(m_WaterSurfaceUBO.sunColor));

        ImGui::SliderFloat("Sky Intensity",
                           &m_WaterSurfaceUBO.skyIntensity, 0.f, 10.f);
        ImGui::SliderFloat("Specular Intensity",
                           &m_WaterSurfaceUBO.specularIntensity, 0.f, 10.f);
        ImGui::SliderFloat("Specular Highlights",
                           &m_WaterSurfaceUBO.specularHighlights, 1.f, 64.f);

        // Terrain
        ImGui::DragFloat("Terrain plane depth", &m_WaterSurfaceUBO.terrainDepth,
                         1.0f, -999.0f, 0.0f);

        ImGui::NewLine();
    }
}
