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

    CreateDrawCommandPools();
    CreateDrawCommandBuffers();

    SetupGUI();
}

void WaterSurface::Update(vkp::Timestep dt)
{
    gui::NewFrame();
    {
        if (!ImGui::Begin("Application Configuration"))
        {
            ImGui::End();
            return;
        }

        ImGui::ColorEdit3("clear color", &(m_ClearValues[0].color.float32[0]));
        ImGui::End();
    }
}

void WaterSurface::Render(
    uint32_t frameIndex,
    vkp::Timestep dt,
    std::vector<VkPipelineStageFlags>& stagesToWait,
    std::vector<VkCommandBuffer>& buffersToSubmit)
{
    auto& drawCmdPool = m_DrawCmdPools[frameIndex];
    drawCmdPool->Reset();
    
    vkp::CommandBuffer& commandBuffer = drawCmdPool->Front();
    commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    {
        BeginRenderPass(
            commandBuffer,
            m_SwapChain->GetFramebuffer(frameIndex)
        );

        gui::Render(commandBuffer);

        vkCmdEndRenderPass(commandBuffer);
    }
    commandBuffer.End();

    stagesToWait.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    buffersToSubmit.push_back(commandBuffer);
}

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

void WaterSurface::OnFrameBufferResize(int width, int height)
{
    m_RenderPass->Destroy();
    DestroyDrawCommandPools();

    CreateRenderPass();
    m_SwapChain->CreateFramebuffers(*m_RenderPass);
    CreateDrawCommandPools();
    CreateDrawCommandBuffers();

    gui::OnFramebufferResized(m_SwapChain->GetMinImageCount());
}

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

void WaterSurface::CreateDrawCommandPools()
{
    VKP_REGISTER_FUNCTION();
    // A command pool for each frame that is going to reset its buffers 
    //  every time
    // TODO maybe on recreate
    const uint32_t kImageCount = m_SwapChain->GetImageCount();

    m_DrawCmdPools.reserve(kImageCount);

    for (uint32_t i = 0; i < kImageCount; ++i)
    {
        m_DrawCmdPools.emplace_back(
            std::make_unique<vkp::CommandPool>(
                *m_Device,
                vkp::QFamily::Graphics,
                VK_COMMAND_POOL_CREATE_TRANSIENT_BIT    // short-lived
            )
        );
    }
}

void WaterSurface::CreateDrawCommandBuffers()
{
    VKP_REGISTER_FUNCTION();
    const uint32_t kBuffersPerFrame = 1;

    for (auto& commandPool : m_DrawCmdPools)
    {
        commandPool->AllocateCommandBuffers(kBuffersPerFrame);
    }
}

void WaterSurface::DestroyDrawCommandPools()
{
    VKP_REGISTER_FUNCTION();
    m_DrawCmdPools.clear();
}

