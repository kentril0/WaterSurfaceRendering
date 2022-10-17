/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#include "pch.h"
#include "vulkan/RenderPass.h"


namespace vkp
{
    RenderPass::RenderPass(VkDevice device, VkFormat imageFormat, 
        VkFormat depthFormat)
        : m_Device(device), 
          m_AttachmentFormat(imageFormat),
          m_DepthAttachmentFormat(depthFormat)
    {
        VKP_REGISTER_FUNCTION();
        VKP_ASSERT(device != VK_NULL_HANDLE && 
                   imageFormat != VK_FORMAT_UNDEFINED);

        m_Attachments.emplace_back(
            InitAttachmentDescription(m_AttachmentFormat));
        m_ColorAttachmentRefs.emplace_back(InitAttachmentReference());

        m_Subpasses.emplace_back(InitSubpassDescription(m_ColorAttachmentRefs));
        m_SubpassDependencies.emplace_back(InitSubpassDependency());

        const bool useDepthAttachment = 
            m_DepthAttachmentFormat != VK_FORMAT_UNDEFINED;
        if (useDepthAttachment)
        {
            AddDepthStencilAttachment();
        }
    }

    RenderPass::~RenderPass()
    {
        VKP_REGISTER_FUNCTION();
        Destroy();
    }

    void RenderPass::Destroy()
    {
        if (m_RenderPass != VK_NULL_HANDLE)
        {
            vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
            m_RenderPass = VK_NULL_HANDLE;
        }
    }

    void RenderPass::AddDepthStencilAttachment()
    {
        VKP_ASSERT(m_DepthAttachmentFormat != VK_FORMAT_UNDEFINED);

        auto& depthDesc = m_Attachments.emplace_back(
            InitAttachmentDescription(
                m_DepthAttachmentFormat,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
            )
        );
        
        depthDesc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        m_DepthAttachmentRef = InitAttachmentReference(
            m_ColorAttachmentRefs.size(),
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        );
    }

    void RenderPass::Create(bool useDepthAttachment)
    {
        VKP_REGISTER_FUNCTION();
        Destroy();

        if (useDepthAttachment)
        {
            // Copy subpass state before connecting depthstencil
            auto lastSubpass = m_Subpasses.back();
            auto lastDependency = m_SubpassDependencies.back();
        
            ConnectDepthStencilToSubpass(m_Subpasses.back(),
                                         m_SubpassDependencies.back());

            CreateRenderPass(static_cast<uint32_t>(m_Attachments.size()) );

            // Restore subpass state
            m_Subpasses.back() = lastSubpass;
            m_SubpassDependencies.back() = lastDependency;
        }
        else
        {
            uint32_t attachmentCount = static_cast<uint32_t>(
                                        m_Attachments.size());
            if (m_DepthAttachmentFormat != VK_FORMAT_UNDEFINED)
            {
                --attachmentCount;
            }
            // Without the last depthstencil attachment
            CreateRenderPass(attachmentCount);
        }
    }

    void RenderPass::CreateRenderPass()
    {
        CreateRenderPass(static_cast<uint32_t>(m_Attachments.size()) );
    }

    void RenderPass::CreateRenderPass(uint32_t attachmentCount)
    {
        VkRenderPassCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        info.attachmentCount = attachmentCount;
        info.pAttachments = m_Attachments.data();
        info.subpassCount = static_cast<uint32_t>(m_Subpasses.size());
        info.pSubpasses = m_Subpasses.data();
        info.dependencyCount = static_cast<uint32_t>(m_SubpassDependencies.size());
        info.pDependencies = m_SubpassDependencies.data();
    
        VKP_ASSERT_RESULT(
            vkCreateRenderPass(m_Device, &info, nullptr, &m_RenderPass));
    }

    void RenderPass::ConnectDepthStencilToSubpass(
        VkSubpassDescription& pSubpass, VkSubpassDependency& pDependency) const
    {
        pSubpass.pDepthStencilAttachment = &m_DepthAttachmentRef;

        pDependency.srcStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        pDependency.dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        pDependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    // -------------------------------------------------------------------------
    // -------------------------------------------------------------------------

    VkAttachmentDescription RenderPass::InitAttachmentDescription(
        VkFormat format, VkImageLayout finalLayout)
    {
        VkAttachmentDescription attachment{};
        attachment.format = format;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = finalLayout;
        return attachment;
    }

    VkAttachmentReference RenderPass::InitAttachmentReference(
        uint32_t attachment, VkImageLayout layout)
    {
        VkAttachmentReference attachmentRef{};
        attachmentRef.attachment = attachment;
        attachmentRef.layout = layout;
        return attachmentRef;
    }

    VkSubpassDescription RenderPass::InitSubpassDescription(
        const std::vector<VkAttachmentReference>& colorAttachmentRef,
        const VkAttachmentReference* pDepthStencilAttachment)
    {
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        subpass.colorAttachmentCount = 
            static_cast<uint32_t>(colorAttachmentRef.size());
        subpass.pColorAttachments = colorAttachmentRef.data();

        subpass.pDepthStencilAttachment = pDepthStencilAttachment;

        return subpass; 
    }

    VkSubpassDependency RenderPass::InitSubpassDependency()
    {
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        return dependency;
    }

} // namespace vkp