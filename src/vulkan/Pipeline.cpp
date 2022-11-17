/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#include "pch.h"
#include "vulkan/Pipeline.h"


namespace vkp
{
    Pipeline::Pipeline(VkDevice device, 
        const std::vector<std::shared_ptr<ShaderModule>>& modules)
        : m_Device(device),
          m_ShaderModules(modules)
    {
        VKP_REGISTER_FUNCTION();
        VKP_ASSERT(device != VK_NULL_HANDLE);

        InitShaderStages();

        // TODO types of pipelines: Graphics

        m_VertexInputInfo = Pipeline::InitVertexInput();
        m_InputAssembly = Pipeline::InitInputAssembly();
        m_RasterizationState = Pipeline::InitRasterization();
        m_Multisampling = Pipeline::InitMultisample();
        m_DepthStencil = Pipeline::InitDepthStencil();
        m_ColorBlendAttachment = Pipeline::InitColorBlendAttachment();
        m_ColorBlending = Pipeline::InitColorBlending(m_ColorBlendAttachment);
        m_PipelineLayoutInfo = Pipeline::InitPipelineLayout();

        m_PipelineCache = CreatePipelineCache();
    }

    Pipeline::~Pipeline()
    {
        VKP_REGISTER_FUNCTION();
        DestroyPipeline();
        DestroyPipelineLayout();
        DestroyPipelineCache();
    }

    void Pipeline::DestroyPipeline()
    {
        if (m_Pipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(m_Device, m_Pipeline, nullptr);
            m_Pipeline = VK_NULL_HANDLE;
        }
    }
    
    void Pipeline::DestroyPipelineLayout()
    {
        if (m_PipelineLayout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
            m_PipelineLayout = VK_NULL_HANDLE;
        }
    }

    void Pipeline::DestroyPipelineCache()
    {
        if (m_PipelineCache != VK_NULL_HANDLE)
        {
            vkDestroyPipelineCache(m_Device, m_PipelineCache, nullptr);
            m_PipelineCache = VK_NULL_HANDLE;
        }
    }

    void Pipeline::InitShaderStages()
    {
        m_ShaderStages.resize(m_ShaderModules.size());

        for (size_t i = 0; i < m_ShaderStages.size(); ++i)
        {
            ShaderModule& module = *(m_ShaderModules[i]);
            m_ShaderStages[i] = 
                vkp::Pipeline::InitShaderStage(module, module.GetStage());
        }
    }

    VkPipelineCache Pipeline::CreatePipelineCache()
    {
    	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	    pipelineCacheCreateInfo.sType = 
            VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

        VkPipelineCache pipelineCache;
        auto err = vkCreatePipelineCache(m_Device, &pipelineCacheCreateInfo,
                                         nullptr, &pipelineCache);
        VKP_ASSERT_RESULT(err);
        return pipelineCache;
    }

    void Pipeline::Create(const VkExtent2D kSurfaceExtent, 
                          VkRenderPass renderPass, 
                          bool enableDepthTesting)
    {
        VKP_REGISTER_FUNCTION();
        VKP_ASSERT(renderPass != VK_NULL_HANDLE);

        DestroyPipeline();

        // TODO what if pipelineLayout does not get destroyed?
        DestroyPipelineLayout();
        m_PipelineLayout = CreatePipelineLayout();

        const VkViewport viewport = Pipeline::InitViewport(kSurfaceExtent);
        const VkRect2D scissor = Pipeline::InitScissor(kSurfaceExtent);
        m_ViewportState = Pipeline::InitViewportScissor(viewport, scissor);

        m_DepthStencil = Pipeline::InitDepthStencil(enableDepthTesting);

        m_RenderPass = renderPass;
        m_Pipeline = CreatePipeline();
    }

    VkPipelineLayout Pipeline::CreatePipelineLayout()
    {
        VkPipelineLayout pipelineLayout;
        auto err = vkCreatePipelineLayout(m_Device, &m_PipelineLayoutInfo,
                                          nullptr, &pipelineLayout);
        VKP_ASSERT_RESULT(err);
        return pipelineLayout;
    }

    VkPipeline Pipeline::CreatePipeline(uint32_t subpass) const
    {
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

        pipelineInfo.stageCount = m_ShaderStages.size();
        pipelineInfo.pStages = m_ShaderStages.data();
        pipelineInfo.pVertexInputState = &m_VertexInputInfo;
        pipelineInfo.pInputAssemblyState = &m_InputAssembly;
        pipelineInfo.pViewportState = &m_ViewportState;
        pipelineInfo.pRasterizationState = &m_RasterizationState;
        pipelineInfo.pMultisampleState = &m_Multisampling;
        pipelineInfo.pDepthStencilState = &m_DepthStencil;
        pipelineInfo.pColorBlendState = &m_ColorBlending;
        // TODO
        pipelineInfo.pDynamicState = nullptr;

        pipelineInfo.layout = m_PipelineLayout;
        // Specify the render pass and the index of the subpass where this 
        //  graphics pipeline will be used
        pipelineInfo.renderPass = m_RenderPass;
        pipelineInfo.subpass = subpass;
        // Is not created (derived) an from existing pipeline TODO
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        //  Create just 1 graphics pipeline
        //      No VkPipelineCache object TODO
        VkPipeline pipeline;
        auto err = vkCreateGraphicsPipelines(m_Device, m_PipelineCache, 1,
                                             &pipelineInfo, nullptr, &pipeline);
        VKP_ASSERT_RESULT(err);
        return pipeline;
    }

    void Pipeline::CreateCompute()
    {
        VKP_REGISTER_FUNCTION();

        DestroyPipeline();

        // TODO what if pipelineLayout does not get destroyed?
        DestroyPipelineLayout();
        m_PipelineLayout = CreatePipelineLayout();

        m_Pipeline = CreateComputePipeline();
    }

    VkPipeline Pipeline::CreateComputePipeline() const
    {
        VkComputePipelineCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        createInfo.stage = m_ShaderStages[0];
        createInfo.layout = m_PipelineLayout;
        createInfo.basePipelineHandle = VK_NULL_HANDLE;

        VkPipeline pipeline;
        auto err = vkCreateComputePipelines(m_Device, m_PipelineCache,
                                            1, &createInfo, nullptr,
                                            &pipeline);
        VKP_ASSERT_RESULT(err);
        return pipeline;
    }

    bool Pipeline::RecompileShaders()
    {
        bool needsRecreation = false;
        for (size_t i = 0; i < m_ShaderModules.size(); ++i)
        {
            auto m = m_ShaderModules[i];
            if (m->ReCreate())
            {
                m_ShaderStages[i] = 
                    vkp::Pipeline::InitShaderStage(m->GetModule(), m->GetStage());

                needsRecreation = true;
            }
        }
        return needsRecreation;
    }

    // =========================================================================
    // =========================================================================

    void Pipeline::SetVertexInputState(
        const VkPipelineVertexInputStateCreateInfo& state)
    {
        m_VertexInputInfo = state;
    }

    void Pipeline::SetVertexInputState(
        VkPipelineVertexInputStateCreateInfo&& state)
    {
        m_VertexInputInfo = state;
    }

    void Pipeline::SetRasterizationState(
        const VkPipelineRasterizationStateCreateInfo& state)
    {
        m_RasterizationState = state;
    }

    void Pipeline::SetRasterizationState(
            VkPipelineRasterizationStateCreateInfo&& state)
    {
        m_RasterizationState = state;
    }



    // =========================================================================
    // =========================================================================

    VkPipelineShaderStageCreateInfo Pipeline::InitShaderStage(
        VkShaderModule module, VkShaderStageFlagBits stage)
    {
        VkPipelineShaderStageCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        info.stage = stage;
        info.module = module;
        info.pName = "main";
        return info;
    }

    VkPipelineVertexInputStateCreateInfo Pipeline::InitVertexInput(
        const std::vector<VkVertexInputBindingDescription>& 
            bindingDescriptions,
        const std::vector<VkVertexInputAttributeDescription>&
            attributeDescriptions)
    {
        VkPipelineVertexInputStateCreateInfo info{};
        info.sType = 
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        info.vertexBindingDescriptionCount = bindingDescriptions.size();
        info.pVertexBindingDescriptions = bindingDescriptions.data();
        info.vertexAttributeDescriptionCount = attributeDescriptions.size();
        info.pVertexAttributeDescriptions = attributeDescriptions.data();
        return info;
    }

    VkPipelineVertexInputStateCreateInfo Pipeline::InitVertexInput()
    {
        VkPipelineVertexInputStateCreateInfo info{};
        info.sType = 
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        info.vertexBindingDescriptionCount = 0;
        info.pVertexBindingDescriptions = nullptr;
        info.vertexAttributeDescriptionCount = 0;
        info.pVertexAttributeDescriptions = nullptr;
        return info;
    }


    VkPipelineInputAssemblyStateCreateInfo Pipeline::InitInputAssembly(
        VkPrimitiveTopology topology)
    {
        VkPipelineInputAssemblyStateCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        info.topology = topology;
        info.primitiveRestartEnable = VK_FALSE;
        return info;
    }

    VkViewport Pipeline::InitViewport(const VkExtent2D kSurfaceExtent)
    {
        // TODO OpenGL compatibility: flip Y, requires Vulkan v. 1.1
        return VkViewport {
            .x = 0.0f,
            .y = 0.0f,
            //.y = static_cast<float>(kSurfaceExtent.height),
            .width = static_cast<float>(kSurfaceExtent.width),
            .height = static_cast<float>(kSurfaceExtent.height),
            //.height = -static_cast<float>(kSurfaceExtent.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f
        };
    }

    VkRect2D Pipeline::InitScissor(const VkExtent2D kSurfaceExtent)
    {
        return VkRect2D {
            .offset = {0, 0},
            .extent = kSurfaceExtent 
        };
    }

    VkPipelineViewportStateCreateInfo Pipeline::InitViewportScissor(
        const VkViewport& kViewport, const VkRect2D& kScissor)
    {
        return VkPipelineViewportStateCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .viewportCount = 1,
            .pViewports = &kViewport,
            .scissorCount = 1,
            .pScissors = &kScissor
        };
    }

    VkPipelineRasterizationStateCreateInfo Pipeline::InitRasterization()
    {
        //> m_RasterizationState
        //  Depth testing, face culling, scissor test
        VkPipelineRasterizationStateCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        info.depthClampEnable = VK_FALSE;
        info.rasterizerDiscardEnable = VK_FALSE;
        info.polygonMode = VK_POLYGON_MODE_FILL;
        info.lineWidth = 1.0f;
        info.cullMode = VK_CULL_MODE_NONE;
        info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        info.depthBiasEnable = VK_FALSE;
        info.depthBiasConstantFactor = 0.0f;
        info.depthBiasClamp = 0.0f;
        info.depthBiasSlopeFactor = 0.0f;

        return info;
    }

    VkPipelineMultisampleStateCreateInfo Pipeline::InitMultisample()
    {
        VkPipelineMultisampleStateCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        info.sampleShadingEnable = VK_FALSE;
        info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        info.minSampleShading = 1.0f; 
        info.pSampleMask = nullptr;
        info.alphaToCoverageEnable = VK_FALSE;
        info.alphaToOneEnable = VK_FALSE;

        return info;
    }
    
    VkPipelineDepthStencilStateCreateInfo Pipeline::InitDepthStencil(
        bool depthTesting)
    {
        return VkPipelineDepthStencilStateCreateInfo{
            .sType = 
                VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .depthTestEnable = depthTesting ? VK_TRUE : VK_FALSE,
            .depthWriteEnable = depthTesting ? VK_TRUE : VK_FALSE,
            .depthCompareOp = VK_COMPARE_OP_LESS,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = VK_FALSE,
            .front = {},
            .back = {},
            .minDepthBounds = 0.0f,
            .maxDepthBounds = 1.0f
        };
    }

    VkPipelineColorBlendAttachmentState Pipeline::InitColorBlendAttachment()
    {
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | 
                                              VK_COLOR_COMPONENT_G_BIT |
                                              VK_COLOR_COMPONENT_B_BIT |
                                              VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        return colorBlendAttachment;
    }

    VkPipelineColorBlendStateCreateInfo Pipeline::InitColorBlending(
        const VkPipelineColorBlendAttachmentState& state)
    {
        //  Global color blending settings for all framebuffers
        VkPipelineColorBlendStateCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        info.logicOpEnable = VK_FALSE;
        info.logicOp = VK_LOGIC_OP_COPY;
        info.attachmentCount = 1;
        info.pAttachments = &state;
        info.blendConstants[0] = 0.0f;
        info.blendConstants[1] = 0.0f;
        info.blendConstants[2] = 0.0f;
        info.blendConstants[3] = 0.0f;
        return info;
    }

    VkPipelineLayoutCreateInfo Pipeline::InitPipelineLayout()
    {
        VkPipelineLayoutCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        info.setLayoutCount = 0;
        info.pSetLayouts = nullptr;
        info.pushConstantRangeCount = 0;
        info.pPushConstantRanges = nullptr;
        return info;
    }


} // namespace vkp
