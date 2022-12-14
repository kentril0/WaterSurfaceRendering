/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#include "pch.h"
#include "WaterSurface.h"

#include <imgui/imgui.h>

#include "Gui.h"
#include "core/Profile.h"


WaterSurface::WaterSurface(AppCmdLineArgs args)
    : vkp::Application("Water Surface Rendering", args)
{
    VKP_REGISTER_FUNCTION();

    m_Requirements.deviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
    m_Requirements.deviceFeatures.fillModeNonSolid = VK_TRUE;
    m_Requirements.deviceFeatures.samplerAnisotropy = VK_TRUE;
    m_Requirements.queueFamilies = { VK_QUEUE_GRAPHICS_BIT };
    m_Requirements.presentationSupport = true;

    m_DepthTestingEnabled = true;
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

    CreateDescriptorPool();

    SetupAssets();
}

void WaterSurface::SetupAssets()
{
    SetupGUI();

    CreateCamera();
    CreateWaterSurfaceMesh();
    CreateSkyModel();
}

void WaterSurface::CreateWaterSurfaceMesh()
{
    m_WaterSurfaceMesh.reset(
        new WaterSurfaceMesh(*m_Device, *m_DescriptorPool)
    );

    m_WaterSurfaceMesh->CreateRenderData(
        *m_RenderPass,
        m_SwapChain->GetImageCount(),
        m_SwapChain->GetExtent(),
        m_SwapChain->HasDepthAttachment()
    );

    auto& cmdBuffer = BeginOneTimeCommands();

        m_WaterSurfaceMesh->Prepare(cmdBuffer);

    EndOneTimeCommands(cmdBuffer);
}

void WaterSurface::CreateSkyModel()
{
    m_Sky.reset(
        new SkyModel(*m_Device, *m_DescriptorPool, s_kStartSunDir)
    );

    m_Sky->CreateRenderData(
        *m_RenderPass,
        m_SwapChain->GetImageCount(),
        m_SwapChain->GetExtent(),
        m_SwapChain->HasDepthAttachment()
    );
}

void WaterSurface::OnFrameBufferResize(int width, int height)
{
    // Application

    m_RenderPass->Destroy();
    CreateRenderPass();
    m_SwapChain->CreateFramebuffers(*m_RenderPass);

    // Also destroyes all the draw command buffers
    DestroyDrawCommandPools();

    const uint32_t kImageCount = m_SwapChain->GetImageCount();

    CreateDrawCommandPools(kImageCount);
    CreateDrawCommandBuffers();

    // -----------------------------------------------------
    // Assets

    m_WaterSurfaceMesh->CreateRenderData(
        *m_RenderPass,
        m_SwapChain->GetImageCount(),
        m_SwapChain->GetExtent(),
        m_SwapChain->HasDepthAttachment()
    );

    m_Sky->CreateRenderData(
        *m_RenderPass,
        m_SwapChain->GetImageCount(),
        m_SwapChain->GetExtent(),
        m_SwapChain->HasDepthAttachment()
    );

    gui::OnFramebufferResized(m_SwapChain->GetMinImageCount());

    m_Camera->SetAspectRatio(width / static_cast<float>(height));
}

void WaterSurface::Update(vkp::Timestep dt)
{
    UpdateGui();    // TODO wrong priority??

    UpdateCamera(dt);

    m_Sky->Update(dt);
    m_WaterSurfaceMesh->Update(dt);
}

void WaterSurface::Render(
    uint32_t frameIndex,
    vkp::Timestep dt,
    std::vector<VkPipelineStageFlags>& stagesToWait,
    std::vector<VkCommandBuffer>& buffersToSubmit)
{
    auto& drawCmdPool = m_DrawCmdPools[frameIndex];
    drawCmdPool.Reset();
    
    vkp::CommandBuffer& commandBuffer = drawCmdPool.Front();
    commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    {
        const VkExtent2D kSwapChainExtent = m_SwapChain->GetExtent();

        m_Sky->PrepareRender(
            frameIndex,
            glm::vec2(kSwapChainExtent.width, kSwapChainExtent.height),
            m_Camera->GetPosition(),
            m_Camera->GetView(),
            m_Camera->GetFov()
        );

        // Copy to water surface maps, update uniform buffers
        m_WaterSurfaceMesh->PrepareRender(
            frameIndex, commandBuffer,
            m_Camera->GetViewMat(),
            m_Camera->GetProjMat(),
            m_Camera->GetPosition(),
            m_Sky->GetParams()
        );

        BeginRenderPass(
            commandBuffer,
            m_SwapChain->GetFramebuffer(frameIndex)
        );

        // TODO optimize - render last
        // Render sky first - rendered in the background,
        //  does not write into depth attachment
        m_Sky->Render(frameIndex, commandBuffer);

        m_WaterSurfaceMesh->Render(frameIndex, commandBuffer);

        gui::Render(commandBuffer);

        vkCmdEndRenderPass(commandBuffer);
    }
    commandBuffer.End();

    stagesToWait.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    buffersToSubmit.push_back(commandBuffer);
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

// -----------------------------------------------------------------------------
// Descriptors 

void WaterSurface::CreateDescriptorPool()
{
    VKP_REGISTER_FUNCTION();

    m_DescriptorPool = vkp::DescriptorPool::Builder(*m_Device)
        .AddPoolSize(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            m_SwapChain->GetImageCount() * 10
        )
        .AddPoolSize(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            m_SwapChain->GetImageCount() * 10
        )
        .Build(m_SwapChain->GetImageCount() * 2);
}

// -----------------------------------------------------------------------------
// Assets

void WaterSurface::CreateCamera()
{
    const VkExtent2D kSwapChainExtent = m_SwapChain->GetExtent();

    m_Camera = std::make_unique<vkp::Camera>(
        kSwapChainExtent.width / static_cast<float>(kSwapChainExtent.height),
        s_kCamStartPos,
        s_kCamStartPitch,
        s_kCamStartYaw
    );
    m_Camera->SetFov(s_kCamStartFov);
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

void WaterSurface::UpdateCamera(vkp::Timestep dt)
{
    m_Camera->Update(dt);
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
        else if (key == KeyRecompileShaders)
        {
            m_WaterSurfaceMesh->RecompileShaders(
                *m_RenderPass,
                m_SwapChain->GetExtent(),
                m_SwapChain->HasDepthAttachment()
            );
            m_Sky->RecompileShaders(
                *m_RenderPass,
                m_SwapChain->GetExtent(),
                m_SwapChain->HasDepthAttachment()
            );
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

    ShowStatusWindow();

    static bool controlsOpened = true;
    if (controlsOpened)
        ShowControlsWindow(&controlsOpened);

    if (m_State != States::GuiControls)
        return;

    if (!ImGui::Begin("Application Configuration"))
    {
        ImGui::End();
        return;
    }

    ImGui::ColorEdit3("clear color", &(m_ClearValues[0].color.float32[0]));
    if ( ImGui::Button("Open Controls window") )
        controlsOpened = true;

    ShowCameraSettings();
    m_WaterSurfaceMesh->ShowGUISettings();
    m_Sky->ShowGUISettings();

    ImGui::End();
}

void WaterSurface::ShowControlsWindow(bool* p_open) const
{
    if ( !ImGui::Begin("Application Controls", p_open) )
    {
        ImGui::End();
        return;
    }

    ImGui::Text("Global:");
    ImGui::BulletText("ESC to show / hide Configuration menu");
    ImGui::Separator();

    ImGui::Text("Camera:");
    ImGui::BulletText("When menu is hidden: use Mouse to pan.");
    ImGui::BulletText("WASD keys to fly.");
    ImGui::BulletText("Left Shift to speed up.");
    ImGui::BulletText("Left Ctrl to slow down.");
    ImGui::Separator();
}

void WaterSurface::ShowStatusWindow() const
{
    const float kIsControlsHidden =
        static_cast<float>(m_State != States::GuiControls);
    const float kAlphaValue = 0.1 * kIsControlsHidden + 
                              1.0 * (1.0-kIsControlsHidden);
    ImGui::SetNextWindowBgAlpha(kAlphaValue);

    if (!ImGui::Begin("Application Metrics"))
    {
        ImGui::End();
        return;
    }

    // Show frametime and FPS
    const ImGuiIO& io = ImGui::GetIO();
    const float kFrameTime = 1000.0f / io.Framerate;
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 
                kFrameTime, io.Framerate);

    // Record frametimes
    static const uint32_t kMaxFrameTimeCount = 256;
    static std::vector<float> frameTimes(kMaxFrameTimeCount);
    frameTimes.back() = kFrameTime;
    std::copy(frameTimes.begin()+1, frameTimes.end(), frameTimes.begin());
    
    ImGui::PlotLines("Frame Times", frameTimes.data(), kMaxFrameTimeCount,
                     0, NULL, FLT_MAX, FLT_MAX, ImVec2(0,60));

    // Show profiling records
    #ifdef VKP_PROFILE
        ImGui::NewLine();
        ImGui::Text("Profiling data");
        if ( ImGui::BeginTable("Profiling data", 2, ImGuiTableFlags_Resizable | 
                                                    ImGuiTableFlags_BordersOuter |
                                                    ImGuiTableFlags_BordersV) )
        {
            const auto& kRecords = vkp::Profile::GetRecordsFromLatest();
            for (const auto& record : kRecords)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%.3f ms", record.duration);
                ImGui::TableNextColumn();
                ImGui::Text("%s", record.name);
            }
            ImGui::EndTable();
        }
    #endif

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

        if ( ImGui::DragFloat3("Position", glm::value_ptr(pos), 1.f) )
            m_Camera->SetPosition(pos);

        ImGui::InputFloat3("Front", glm::value_ptr(front), "%.3f",
                           ImGuiInputTextFlags_ReadOnly);

        bool viewChanged =
            ImGui::SliderAngle("Pitch angle", &pitch,
                                vkp::Camera::s_kMinPitchDeg,
                                vkp::Camera::s_kMaxPitchDeg);// "%.0f deg");

        viewChanged |=
            ImGui::SliderAngle("Yaw angle", &yaw, 0.f,
                                vkp::Camera::s_kMaxYawDeg);// "%.0f deg");

        bool projChanged =
            ImGui::SliderAngle("Field of view", &fov, 0.f, 120.f);
        projChanged |=
            ImGui::SliderFloat("Near plane", &camNear, 0.f, 10.f);
        projChanged |=
            ImGui::SliderFloat("Far plane", &camFar, 100.f, 10000.f);

        if (viewChanged)
        {
            m_Camera->SetYaw(yaw);
            m_Camera->SetPitch(pitch);
            m_Camera->UpdateVectors();
        }

        if (projChanged)
        {
            m_Camera->SetPerspective(fov, camNear, camFar);
        }

        ImGui::NewLine();
    }
}
