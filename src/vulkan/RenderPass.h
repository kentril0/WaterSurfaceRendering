/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#ifndef WATER_SURFACE_RENDERING_VULKAN_RENDER_PASS_H_
#define WATER_SURFACE_RENDERING_VULKAN_RENDER_PASS_H_

#include <vulkan/vulkan.h>


namespace vkp
{
    /**
     * @brief VkRenderPass abstraction
     *  Allows default construction, 
     *      can change default parameters,
     *      recreation with already set parameters, 
     *      automatic destruction at the end of its lifetime
     */
    class RenderPass
    {
    public:
        // Initializers
        static VkAttachmentDescription InitAttachmentDescription(
            VkFormat format,
            VkImageLayout finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        static VkAttachmentReference InitAttachmentReference(
            uint32_t attachment = 0,
            VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        static VkSubpassDescription InitSubpassDescription(
            const std::vector<VkAttachmentReference>& attachmentRefs,
            const VkAttachmentReference* pDepthStencilAttachment = nullptr);

        static VkSubpassDependency InitSubpassDependency();

    public:
        /**
         * @brief Sets up its members for further creation of render pass
         * TODO in detail what is done, created, etc. 
         * @param device Created logical device
         * @param imageFormat Image format of used color attachment
         * @param depthFormat Image format of depth attachment, if used
         */
        RenderPass(VkDevice device, 
                   VkFormat imageFormat, 
                   VkFormat depthFormat = VK_FORMAT_UNDEFINED);
        ~RenderPass();

        operator VkRenderPass() const { return m_RenderPass; }

        /**
         * @brief Creates the render pass from its own initialized data members
         * @param useDepthAttachment Whether depth attachment should be 
         *  "attached", assumes that the depth format is defined
         */
        void Create(bool useDepthAttachment);
        void Destroy();

        void CreateRenderPass(uint32_t attachmentCount);
        void CreateRenderPass();

        void SetSubpassDependencies(
            const std::vector<VkSubpassDependency>& dependencies)
        {
            m_SubpassDependencies = dependencies;
        }
        
        std::vector<VkAttachmentDescription>& 
            GetAttachmentDescriptions() { return m_Attachments; }
        
    private:

        /** @brief DepthStencil attachment is added as the last attachment */
        void AddDepthStencilAttachment();

        void ConnectDepthStencilToSubpass(VkSubpassDescription& pSubpass,
                                          VkSubpassDependency& pDependency) const;

    private:
        VkDevice m_Device{ VK_NULL_HANDLE };

        // ---------------------------------------------------------------------
        VkFormat     m_AttachmentFormat     { VK_FORMAT_UNDEFINED };
        VkFormat     m_DepthAttachmentFormat{ VK_FORMAT_UNDEFINED };

        VkRenderPass m_RenderPass      { VK_NULL_HANDLE };

        // Data members used as a state for subsequent creation
        std::vector<VkAttachmentDescription> m_Attachments{};
        std::vector<VkAttachmentReference>   m_ColorAttachmentRefs{};
        VkAttachmentReference                m_DepthAttachmentRef{};

        std::vector<VkSubpassDescription>    m_Subpasses{};
        std::vector<VkSubpassDependency>     m_SubpassDependencies{};
    };

} // namespace vkp



#endif // WATER_SURFACE_RENDERING_VULKAN_RENDER_PASS_H_