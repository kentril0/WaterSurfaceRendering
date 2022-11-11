/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#ifndef WATER_SURFACE_RENDERING_VULKAN_PIPELINE_H_
#define WATER_SURFACE_RENDERING_VULKAN_PIPELINE_H_

#include <array>
#include <memory>
#include <vulkan/vulkan.h>
#include "vulkan/ShaderModule.h"


namespace vkp
{
    
    class Pipeline 
    {
    public:
        /**
         * @brief Makes the pipeline ready for any subsequent creation
         * @param device Created logical device
         * @param shaderStages Vector of shader stages to be used for the
         *  pipeline creation
         */
        Pipeline(VkDevice device,
                 const std::vector<std::shared_ptr<ShaderModule>>& modules);
        ~Pipeline();

        operator VkPipeline() const { return m_Pipeline; }
        operator VkPipelineLayout() const { return m_PipelineLayout; }

        VkPipeline GetPipeline() const { return m_Pipeline; }
        VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }

        /**
         * @brief On any subsequent call creates a new pipeline and pipeline 
         *  layout from its own initialized data members and parameters.
         */
        // TODO fix needed params
        void Create(const VkExtent2D surfaceExtent,
                    VkRenderPass renderPass,
                    bool enableDepthTesting = false);
        
        // TODO divide into pipeline types
        void CreateCompute();

        /**
         * @brief Tries to recompile attached shaders
         * @return True if a shader was recompiled and pipeline needs recreation
         *  else false, any errors will be print
         */
        bool RecompileShaders();

        // ---------------------------------------------------------------------

        /**
         * @brief Sets the vertex input state structure to use for subsequent
         *  pipeline creation
         */
        void SetVertexInputState(
            const VkPipelineVertexInputStateCreateInfo& state);
        void SetVertexInputState(VkPipelineVertexInputStateCreateInfo&& state);

        void SetRasterizationState(
            const VkPipelineRasterizationStateCreateInfo& state);
        void SetRasterizationState(
            VkPipelineRasterizationStateCreateInfo&& state);

        VkPipelineRasterizationStateCreateInfo& GetRasterizationState()
        {
            return m_RasterizationState;
        }

        VkPipelineLayoutCreateInfo& GetPipelineLayoutInfo()
        {
            return m_PipelineLayoutInfo;
        }
    

        // ---------------------------------------------------------------------
        // Functions to create or initialize default stuctures to predefined
        //  "common" values

        static VkPipelineShaderStageCreateInfo InitShaderStage(
            VkShaderModule shaderModule,
            VkShaderStageFlagBits stage);


        /** @brief No vertex input bindings */
        static VkPipelineVertexInputStateCreateInfo InitVertexInput();

        /**
         * @brief Vertex input - describe the format of the vertex data 
         *  passed to the vertex shader
         */
        static VkPipelineVertexInputStateCreateInfo InitVertexInput(
            const std::vector<VkVertexInputBindingDescription>&
                bindingDescriptions,
            const std::vector<VkVertexInputAttributeDescription>&
                attributeDescriptions);

        static VkPipelineInputAssemblyStateCreateInfo InitInputAssembly(
            VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

        static VkViewport InitViewport(const VkExtent2D kSurfaceExtent);
        static VkRect2D InitScissor(const VkExtent2D kSurfaceExtent);

        // @brief Initializes viewport and scissor for the entire surface
        static VkPipelineViewportStateCreateInfo InitViewportScissor(
            const VkViewport& kViewport, const VkRect2D& kScissor);

        // @brief Initializes rasterizer state with predefined parameters
        static VkPipelineRasterizationStateCreateInfo InitRasterization();

        // @brief Initializes Multisampling state to predefined parameters
        // @warn enabling it requires a GPU feature
        static VkPipelineMultisampleStateCreateInfo InitMultisample();

        // @brief Initializes Depth and Stencil testing parameters
        static VkPipelineDepthStencilStateCreateInfo InitDepthStencil(
            bool depthTesting = false);

        // @brief Initilizes color blending to predefined state
        static VkPipelineColorBlendAttachmentState InitColorBlendAttachment();

        // @brief Initilizes color blending to predefined state
        static VkPipelineColorBlendStateCreateInfo InitColorBlending(
            const VkPipelineColorBlendAttachmentState& colorBlendAttachment);

        static VkPipelineLayoutCreateInfo InitPipelineLayout();

    private:
        void InitShaderStages();

        VkPipelineLayout CreatePipelineLayout();
        VkPipeline       CreatePipeline(uint32_t subpass = 0) const;
        VkPipelineCache  CreatePipelineCache();

        VkPipeline CreateComputePipeline() const;


        void DestroyPipelineLayout();
        void DestroyPipeline();
        void DestroyPipelineCache();

    private:
        VkDevice         m_Device    { VK_NULL_HANDLE };
        VkRenderPass     m_RenderPass{ VK_NULL_HANDLE };

        // ---------------------------------------------------------------------
        // Owned
        VkPipelineLayout m_PipelineLayout{ VK_NULL_HANDLE };
        VkPipeline       m_Pipeline      { VK_NULL_HANDLE };
        VkPipelineCache  m_PipelineCache { VK_NULL_HANDLE };

        std::vector<std::shared_ptr<ShaderModule>>   m_ShaderModules{};
        std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStages{};

        // Data members to setup the pipline

        VkPipelineVertexInputStateCreateInfo   m_VertexInputInfo     {};
        VkPipelineInputAssemblyStateCreateInfo m_InputAssembly       {};
        VkPipelineViewportStateCreateInfo      m_ViewportState       {};
        VkPipelineRasterizationStateCreateInfo m_RasterizationState  {};
        VkPipelineMultisampleStateCreateInfo   m_Multisampling       {};
        VkPipelineDepthStencilStateCreateInfo  m_DepthStencil        {};
        VkPipelineColorBlendAttachmentState    m_ColorBlendAttachment{};
        VkPipelineColorBlendStateCreateInfo    m_ColorBlending       {};

        VkPipelineLayoutCreateInfo             m_PipelineLayoutInfo  {};
    };

} // namespace vkp



#endif // WATER_SURFACE_RENDERING_VULKAN_PIPELINE_H_