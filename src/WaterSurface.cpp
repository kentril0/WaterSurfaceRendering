/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#include "pch.h"
#include "WaterSurface.h"

#include <imgui/imgui.h>

#include "Gui.h"


WaterSurface::WaterSurface(AppCmdLineArgs args)
    : vkp::Application("Water Surface Rendering", args)
{
    VKP_REGISTER_FUNCTION();

    m_Requirements.deviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
    m_Requirements.deviceFeatures.fillModeNonSolid = VK_TRUE;
    m_Requirements.deviceFeatures.samplerAnisotropy = VK_TRUE;
    m_Requirements.queueFamilies = { VK_QUEUE_GRAPHICS_BIT };
    m_Requirements.presentationSupport = true;
}

WaterSurface::~WaterSurface()
{
    VKP_REGISTER_FUNCTION();

    gui::DestroyImGui();
}

void WaterSurface::Start()
{
    VKP_REGISTER_FUNCTION();

    CreateRenderPass();
    m_SwapChain->CreateFramebuffers(*m_RenderPass);

    const uint32_t kImageCount = m_SwapChain->GetImageCount();

    CreateDrawCommandPools(kImageCount);
    CreateDrawCommandBuffers();

    CreateUniformBuffers(kImageCount);
    CreateDescriptorPool();

    SetupWaterSurfaceMeshRendering();

    SetupAssets();
}

void WaterSurface::SetupWaterSurfaceMeshRendering()
{
    CreateWSMeshDescriptorSetLayout();
    SetupWSMeshPipeline();
    CreateWSMeshPipeline();

    CreateWSMeshDescriptorSets(m_SwapChain->GetImageCount());
    UpdateWSMeshDescriptorSets();
}

void WaterSurface::SetupAssets()
{
    SetupGUI();
    CreateCamera();
    CreateWaterSurfaces();
}

void WaterSurface::OnFrameBufferResize(int width, int height)
{
    m_RenderPass->Destroy();
    CreateRenderPass();
    m_SwapChain->CreateFramebuffers(*m_RenderPass);

    CreateWSMeshPipeline();

    // Also destroyes all the draw command buffers
    DestroyDrawCommandPools();

    const uint32_t kImageCount = m_SwapChain->GetImageCount();

    CreateDrawCommandPools(kImageCount);
    CreateDrawCommandBuffers();

    const bool kImageCountChanged = m_DrawCmdPools.size() != kImageCount;
    if (kImageCountChanged)
    {
        VKP_LOG_WARN("Image Count Changed!");

        m_UniformBuffers.clear();
        CreateUniformBuffers(kImageCount);

        m_WSMeshDescriptorSets.clear();
        CreateWSMeshDescriptorSets(kImageCount);
        UpdateWSMeshDescriptorSets();
    }

    gui::OnFramebufferResized(m_SwapChain->GetMinImageCount());

    m_Camera->SetAspectRatio(width / static_cast<float>(height));
}

void WaterSurface::Update(vkp::Timestep dt)
{
    UpdateCamera(dt);
    UpdateWaterSurfaceMesh();
    UpdateGui();
}

void WaterSurface::Render(
    uint32_t frameIndex,
    vkp::Timestep dt,
    std::vector<VkPipelineStageFlags>& stagesToWait,
    std::vector<VkCommandBuffer>& buffersToSubmit)
{
    UpdateUniformBuffer(frameIndex);

    auto& drawCmdPool = m_DrawCmdPools[frameIndex];
    drawCmdPool.Reset();
    
    vkp::CommandBuffer& commandBuffer = drawCmdPool.Front();
    commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    {
        BeginRenderPass(
            commandBuffer,
            m_SwapChain->GetFramebuffer(frameIndex)
        );

        RenderWaterSurfaceMesh(commandBuffer, frameIndex);

        gui::Render(commandBuffer);

        vkCmdEndRenderPass(commandBuffer);
    }
    commandBuffer.End();

    stagesToWait.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    buffersToSubmit.push_back(commandBuffer);
}

void WaterSurface::RenderWaterSurfaceMesh(VkCommandBuffer cmdBuffer,
                                          const uint32_t frameIndex)
{
    vkCmdBindPipeline(
        cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS, 
        *m_WSMeshPipeline
    );

    const uint32_t kFirstSet = 0, kDescriptorSetCount = 1;
    const uint32_t kDynamicOffsetCount = 0;
    const uint32_t* kDynamicOffsets = nullptr;

    vkCmdBindDescriptorSets(
        cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        *m_WSMeshPipeline,
        kFirstSet,
        kDescriptorSetCount,
        &m_WSMeshDescriptorSets[frameIndex],
        kDynamicOffsetCount,
        kDynamicOffsets
    );

    m_WSMesh->Render(cmdBuffer);
}

// =============================================================================
// Create functions

void WaterSurface::CreateRenderPass()
{
    VKP_REGISTER_FUNCTION();

    m_RenderPass.reset(
        new vkp::RenderPass(
            *m_Device,
            m_SwapChain->GetImageFormat(),
            m_SwapChain->GetDepthAttachmentFormat())
    );

    m_RenderPass->Create(m_SwapChain->HasDepthAttachment());
}

void WaterSurface::CreateDrawCommandPools(const uint32_t kCount)
{
    VKP_REGISTER_FUNCTION();

    m_DrawCmdPools.reserve(kCount);

    for (uint32_t i = 0; i < kCount; ++i)
    {
        m_DrawCmdPools.emplace_back(
            *m_Device,
            vkp::QFamily::Graphics,
            VK_COMMAND_POOL_CREATE_TRANSIENT_BIT    // short-lived
        );
    }
}

void WaterSurface::CreateDrawCommandBuffers()
{
    VKP_REGISTER_FUNCTION();
    const uint32_t kBuffersPerFrame = 1;

    for (auto& pool : m_DrawCmdPools)
    {
        pool.AllocateCommandBuffers(kBuffersPerFrame);
    }
}

void WaterSurface::CreateUniformBuffers(const uint32_t kBufferCount)
{
    VKP_REGISTER_FUNCTION();

    const VkDeviceSize kBufferSize = 
        m_Device->GetUniformBufferAlignment(sizeof(VertexUBO)) +
        m_Device->GetUniformBufferAlignment(sizeof(WaterSurfaceUBO));

    m_UniformBuffers.reserve(kBufferCount);

    for (uint32_t i = 0; i < kBufferCount; ++i)
    {
        m_UniformBuffers.emplace_back(*m_Device);

        m_UniformBuffers[i].Create(kBufferSize,
                                   VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

        VKP_ASSERT_RESULT(m_UniformBuffers[i].Map());
    }
}

// -----------------------------------------------------------------------------
// Descriptors 

void WaterSurface::CreateDescriptorPool()
{
    VKP_REGISTER_FUNCTION();

    m_DescriptorPool = vkp::DescriptorPool::Builder(*m_Device)
        .AddPoolSize(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            m_SwapChain->GetImageCount() * 2
        )
        .Build(m_SwapChain->GetImageCount());
}

void WaterSurface::CreateWSMeshDescriptorSetLayout()
{
    VKP_REGISTER_FUNCTION();

    uint32_t bindingPoint = 0;

    m_WSMeshDescriptorSetLayout = vkp::DescriptorSetLayout::Builder(*m_Device)
        .AddBinding({
            .binding = bindingPoint++,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
        })
        .AddBinding({
            .binding = bindingPoint++,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
        })
        .Build();
}

void WaterSurface::CreateWSMeshDescriptorSets(const uint32_t kCount)
{
    VKP_REGISTER_FUNCTION();

    m_WSMeshDescriptorSets.resize(kCount, VK_NULL_HANDLE);

    VkResult err;
    for (auto& set : m_WSMeshDescriptorSets)
    {
        err = m_DescriptorPool->AllocateDescriptorSet(
            *m_WSMeshDescriptorSetLayout,
            set
        );
        VKP_ASSERT_RESULT(err);
    }
}

void WaterSurface::SetupWSMeshPipeline()
{
    VKP_REGISTER_FUNCTION();

    std::vector<
        std::shared_ptr<vkp::ShaderModule>
    > shaders = CreateShadersFromShaderInfos(s_kWSMeshShaderInfos.data(),
                                             s_kWSMeshShaderInfos.size());

    m_WSMeshPipeline = std::make_unique<vkp::Pipeline>(
        *m_Device, shaders 
    );

    VKP_ASSERT(m_WSMeshDescriptorSetLayout != nullptr);
        VKP_ASSERT(m_WSMeshDescriptorSetLayout->GetLayout() != VK_NULL_HANDLE);

    auto& pipelineLayoutInfo = m_WSMeshPipeline->GetPipelineLayoutInfo();
    pipelineLayoutInfo.setLayoutCount = 1;
    const auto& descriptorSetLayout = m_WSMeshDescriptorSetLayout->GetLayout();
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

    VKP_LOG_INFO( "VertexSize: {}", sizeof(WaterSurfaceMesh::Vertex) );

    const auto& kAttribDescs = WaterSurfaceMesh::Vertex::s_AttribDescriptions;
    m_WSMeshPipeline->SetVertexInputState(
        vkp::Pipeline::InitVertexInput(
            WaterSurfaceMesh::Vertex::s_BindingDescriptions,
            WaterSurfaceMesh::Vertex::s_AttribDescriptions
        )
    );
}

void WaterSurface::CreateWSMeshPipeline()
{
    VKP_REGISTER_FUNCTION();
    VKP_ASSERT(m_WSMeshPipeline != nullptr);

    m_Device->QueueWaitIdle(vkp::QFamily::Graphics);

    m_WSMeshPipeline->Create(m_SwapChain->GetExtent(),
                             *m_RenderPass,
                             m_SwapChain->HasDepthAttachment());
}

// -----------------------------------------------------------------------------
// Assets

void WaterSurface::CreateCamera()
{
    const VkExtent2D kSwapChainExtent = m_SwapChain->GetExtent();

    m_Camera = std::make_unique<vkp::Camera>(
        kSwapChainExtent.width / static_cast<float>(kSwapChainExtent.height)
    );
    m_Camera->SetPitch(-45.0f);
    m_Camera->SetYaw(90.0f);
    m_Camera->SetFov(1.5);
    m_Camera->SetNear(0.1f);
    m_Camera->SetFar(1000.f);
}

void WaterSurface::SetupGUI()
{
    VKP_REGISTER_FUNCTION();

    VkDescriptorPool descriptorPool;
    {
        const auto kPoolSizes = gui::GetImGuiDescriptorPoolSizes();

        descriptorPool = m_Device->CreateDescriptorPool(
            1000 * kPoolSizes.size(),
            kPoolSizes,
            VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
    }

    const auto& physDevice = m_Device->GetPhysicalDevice();

    gui::InitImGui(
        *m_Instance,
        physDevice,
        *m_Device,
        physDevice.GetQueueFamilyIndices().Graphics(),
        m_Device->GetQueue(vkp::QFamily::Graphics),
        descriptorPool,
        m_SwapChain->GetMinImageCount(),
        m_SwapChain->GetImageCount(),
        [](VkResult err){ 
            VKP_ASSERT_RESULT(err);
        },
        *m_Window,
        *m_RenderPass
    );

    vkp::CommandBuffer& cmdBuffer = BeginOneTimeCommands();

        gui::UploadFonts(cmdBuffer);

    EndOneTimeCommands(cmdBuffer);

    gui::DestroyFontUploadObjects();
}

void WaterSurface::CreateWaterSurfaces()
{
    VKP_REGISTER_FUNCTION();

    m_WSMesh.reset( new WaterSurfaceMesh(*m_Device) );

    m_WSTess.reset( new WSTessendorf() );

    PrepareWaterSurfaceMesh(m_WSTess->GetTileSize(), m_WSTess->GetTileLength());
}

void WaterSurface::PrepareWaterSurfaceMesh(uint32_t size, float scale)
{
    VKP_REGISTER_FUNCTION();

    // TODO batch cmdBuffers
    // TODO host fence instead
    vkp::CommandBuffer& cmdBuffer = BeginOneTimeCommands();

        vkp::Buffer stagingBuffer(*m_Device);
        m_WSMesh->Prepare(size, scale, stagingBuffer, cmdBuffer);

    EndOneTimeCommands(cmdBuffer);
}

// =============================================================================
// Update functions

void WaterSurface::BeginRenderPass(
    VkCommandBuffer commandBuffer,
    VkFramebuffer framebuffer)
{
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = *m_RenderPass;
    renderPassInfo.framebuffer = framebuffer;

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_SwapChain->GetExtent();

    renderPassInfo.clearValueCount = 
        static_cast<uint32_t>(m_ClearValues.size());
    renderPassInfo.pClearValues = m_ClearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, 
                         VK_SUBPASS_CONTENTS_INLINE);
}

void WaterSurface::UpdateUniformBuffer(const uint32_t imageIndex)
{
    auto& buffer = m_UniformBuffers[imageIndex];

    void* dataAddr = buffer.GetMappedAddress();

    buffer.CopyToMapped(&m_VertexUBO, sizeof(m_VertexUBO));
    buffer.CopyToMapped(&m_WaterSurfaceUBO, sizeof(m_WaterSurfaceUBO),
        static_cast<void*>(
            static_cast<uint8_t*>(dataAddr) + 
            m_Device->GetUniformBufferAlignment(sizeof(VertexUBO))
        )
    );

    buffer.FlushMappedRange();
}

void WaterSurface::UpdateWSMeshDescriptorSets()
{
    VKP_ASSERT(m_WSMeshDescriptorSets.size() == m_UniformBuffers.size());

    for (uint32_t i = 0; i < m_WSMeshDescriptorSets.size(); ++i)
    {
        UpdateWSMeshDescriptorSet(i);
    }
}

void WaterSurface::UpdateWSMeshDescriptorSet(const uint32_t frameIndex)
{
    VKP_REGISTER_FUNCTION();
    VKP_ASSERT(frameIndex < m_WSMeshDescriptorSets.size());

    vkp::DescriptorWriter descriptorWriter(*m_WSMeshDescriptorSetLayout,
                                           *m_DescriptorPool);

    VkDescriptorBufferInfo bufferInfos[2] = {};
    bufferInfos[0].buffer = m_UniformBuffers[frameIndex];
    bufferInfos[0].offset = 0;
    bufferInfos[0].range = sizeof(VertexUBO);

    bufferInfos[1].buffer = m_UniformBuffers[frameIndex];
    bufferInfos[1].offset = 
        m_Device->GetUniformBufferAlignment(sizeof(VertexUBO));
    bufferInfos[1].range = sizeof(WaterSurfaceUBO);

    descriptorWriter
        .AddBufferDescriptor(0, &bufferInfos[0])
        .AddBufferDescriptor(1, &bufferInfos[1]);

    descriptorWriter.UpdateSet(m_WSMeshDescriptorSets[frameIndex]);
}

void WaterSurface::UpdateCamera(vkp::Timestep dt)
{
    m_Camera->Update(dt);

    //m_VertexUBO.model = glm::scale(glm::mat4(1.0f), glm::vec3(5.f, 5.f, 5.f));
    m_VertexUBO.model = glm::mat4(1.0f);
    m_VertexUBO.view = m_Camera->GetViewMat();
    m_VertexUBO.proj = m_Camera->GetProjMat();

    // Vulkan uses inverted Y coord in comparison to OpenGL (set by glm lib)
    // -> flip the sign on the scaling factor of the Y axis
    m_VertexUBO.proj[1][1] *= -1;
}

void WaterSurface::UpdateWaterSurfaceMesh()
{
    m_WaterSurfaceUBO.heightAmp =
        m_WSTess->ComputeWaves(m_LastFrameTime);

    const auto& displacements = m_WSTess->GetDisplacements();
    const auto& normals = m_WSTess->GetNormals();

    auto& vertices = m_WSMesh->GetVertices();
    static std::vector<glm::vec4> orgPositions;
    static bool once = true;
    if (once)
    {
        orgPositions.reserve(vertices.size());
        std::transform(vertices.begin(), vertices.end(),
                       std::back_inserter(orgPositions),
                       [](const WaterSurfaceMesh::Vertex& v) { return v.pos; });
        once = false;
    }

    // TODO benchmark loop
    VKP_ASSERT(vertices.size() == displacements.size() &&
               vertices.size() == normals.size());
    for (uint32_t i = 0; i < vertices.size(); ++i)
    {
        vertices[i].pos.y = orgPositions[i].y +
                          m_WaterSurfaceUBO.heightAmp * displacements[i].y;
        vertices[i].normal = normals[i];
    }

    m_WSMesh->UpdateVertexBuffer();
}

// =============================================================================
// Destroy functions

void WaterSurface::DestroyDrawCommandPools()
{
    VKP_REGISTER_FUNCTION();
    m_DrawCmdPools.clear();
}

// =============================================================================
// Callbacks

void WaterSurface::OnMouseMove(double xpos, double ypos)
{
    if (m_State == States::CameraControls)
    {
        m_Camera->OnMouseMove(xpos, ypos);
    }
}

void WaterSurface::OnMousePressed(int button, int action, int mods)
{
    if (m_State == States::CameraControls)
    {
        m_Camera->OnMouseButton(button, action, mods);
    }
}

void WaterSurface::OnKeyPressed(int key, int action, int mods)
{
    m_Camera->OnKeyPressed(key, action);

    if (action == GLFW_RELEASE)
    {
        if (key == KeyHideGui)
        {
            m_State = m_State == States::GuiControls ? States::CameraControls
                                                     : States::GuiControls;
            m_Window->ShowCursor(m_State == States::GuiControls);
        }
    }
}

void WaterSurface::OnCursorEntered(int entered)
{
    m_Camera->OnCursorEntered(entered);
}

// =============================================================================
// Gui functions

void WaterSurface::UpdateGui()
{
    gui::NewFrame();

    if (m_State != States::GuiControls)
        return;

    if (!ImGui::Begin("Application Configuration"))
    {
        ImGui::End();
        return;
    }

    ImGui::ColorEdit3("clear color", &(m_ClearValues[0].color.float32[0]));

    ShowCameraSettings();

    ImGui::End();
}

void WaterSurface::ShowCameraSettings()
{
    if (ImGui::CollapsingHeader("Camera Settings"))
    {
        // Must be automatic variables, camera is controlled also using user input
        glm::vec3 pos = m_Camera->GetPosition();
        glm::vec3 front = m_Camera->GetFront();
        float pitch = m_Camera->GetPitch();
        float yaw = m_Camera->GetYaw();
        float fov = m_Camera->GetFov();
        float camNear = m_Camera->GetNear();
        float camFar = m_Camera->GetFar();

        if (ImGui::DragFloat3("Position", glm::value_ptr(pos), 0.1f))
            m_Camera->SetPosition(pos);

        ImGui::InputFloat3("Front", glm::value_ptr(front), "%.3f",
                           ImGuiInputTextFlags_ReadOnly);

        if (ImGui::SliderFloat("Pitch angle", &pitch, -89.f, 89.f, "%.0f deg"))
            m_Camera->SetPitch(pitch);

        if (ImGui::SliderFloat("Yaw angle", &yaw, 0.f, 360.f, "%.0f deg"))
            m_Camera->SetYaw(yaw);

        if (ImGui::SliderAngle("Field of view", &fov, 0.f, 120.f))
            m_Camera->SetFov(fov);

        if (ImGui::SliderFloat("Near plane", &camNear, 0.f, 10.f))
            m_Camera->SetNear(camNear);

        if (ImGui::SliderFloat("Far plane", &camFar, 1000.f, 10000.f))
            m_Camera->SetFar(camFar);

        ImGui::NewLine();
    }
}