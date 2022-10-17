/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#include "pch.h"
#include "vulkan/ShaderModule.h"

#include "vulkan/utils.h"
#include <shaderc/shaderc.hpp>

namespace vkp
{
    ShaderModule::ShaderModule(VkDevice device)
        : m_Device(device)
    {
        VKP_REGISTER_FUNCTION();
        VKP_ASSERT(m_Device != VK_NULL_HANDLE);
    }

    ShaderModule::ShaderModule(VkDevice device, const ShaderInfo& shaderInfo)
        : ShaderModule(device, shaderInfo.paths, shaderInfo.stage, 
                       shaderInfo.isSPV )
    {}

    ShaderModule::ShaderModule(VkDevice device, 
                               const std::vector<std::string_view>& files,
                               VkShaderStageFlagBits stage, bool isSPV)
        : m_Device(device),
          m_ShaderStage(stage),
          m_Files(files),
          m_IsFromGLSL(!isSPV)
    {
        VKP_REGISTER_FUNCTION();
        VKP_ASSERT(m_Device != VK_NULL_HANDLE &&
                   files.size() > 0);

        ReCreate();
    }

    ShaderModule::~ShaderModule()
    {
        VKP_REGISTER_FUNCTION();
        Destroy();
    }

    bool ShaderModule::FromSPV(std::string_view filename,
                               VkShaderStageFlagBits stage)
    {
        VKP_REGISTER_FUNCTION();
        std::vector<char> code = ReadFile(filename.data());

        if (m_ShaderModule != VK_NULL_HANDLE)
        {
            Destroy();
        }

        m_ShaderModule = CreateShaderModule(code);
        m_ShaderStage = stage;

        m_Files.clear();
        m_Files.push_back(filename);

        return m_ShaderModule != VK_NULL_HANDLE;
    }

    VkShaderModule ShaderModule::CreateShaderModule(
        const std::vector<char>& code) const
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        // Needs a pointer to uint32_t
        //  std::vector by default should satisfy alignment requirements
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        VKP_ASSERT_RESULT(
            vkCreateShaderModule(m_Device, &createInfo, nullptr, &shaderModule)
        );

        return shaderModule;
    }

    VkShaderModule ShaderModule::CreateShaderModule(
        const std::vector<uint32_t>& code) const
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size() * 4;
        createInfo.pCode = code.data();

        VkShaderModule shaderModule;
        VKP_ASSERT_RESULT(
            vkCreateShaderModule(m_Device, &createInfo, nullptr, &shaderModule)
        );

        return shaderModule;
    }

    void ShaderModule::Destroy()
    {
        vkDestroyShaderModule(m_Device, m_ShaderModule, nullptr);
        m_ShaderModule = VK_NULL_HANDLE;
    }

    bool ShaderModule::ReCreate()
    {
        VKP_REGISTER_FUNCTION();
        return m_IsFromGLSL ? FromGLSL(m_Files, m_ShaderStage)
                            : FromSPV(m_Files[0], m_ShaderStage);
    }

    // =========================================================================
    // =========================================================================

    // -------------------------------------------------------------------------
    // shaderc 

    shaderc_shader_kind FromShaderStageToKind(VkShaderStageFlagBits stage)
    {
        switch(stage)
        {
            case VK_SHADER_STAGE_FRAGMENT_BIT:
                return shaderc_glsl_fragment_shader;
            case VK_SHADER_STAGE_VERTEX_BIT:
                return shaderc_glsl_vertex_shader;
            case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
                return shaderc_glsl_tess_control_shader;
            case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
                return shaderc_glsl_tess_evaluation_shader;
            case VK_SHADER_STAGE_GEOMETRY_BIT:
                return shaderc_glsl_compute_shader;
            case VK_SHADER_STAGE_COMPUTE_BIT:
                return shaderc_glsl_compute_shader;
            default:
                VKP_ASSERT_MSG(false, "Unsupported Shader stage");
                return shaderc_glsl_infer_from_source;
        }
    }
    
    /**
     * @brief Compiles a GLSL code to a SPIR-V binary
     * @param source GLSL source code
     * @param sourceSize Size of source code in bytes
     * @param kind Force to compile the code for a certain type of shader
     * @param filename Name of the file, only for error printing purposes
     * @param optimize WHether to optimize the code
     * @return SPIR-V binary as a vector of 32-bit words
     */
    std::vector<uint32_t> CompileGLSL(
        const char* source, size_t sourceSize, shaderc_shader_kind kind,
        const char* filename, bool optimize = false)
    {
        shaderc::Compiler compiler;
        shaderc::CompileOptions options;

        // Like -DMY_DEFINE=1
        //options.AddMacroDefinition("MY_DEFINE", "1");

        if (optimize)
        {
            options.SetOptimizationLevel(shaderc_optimization_level_size);
        }

        shaderc::SpvCompilationResult module =
            compiler.CompileGlslToSpv(source, sourceSize, kind, filename, options);

        if (module.GetCompilationStatus() != shaderc_compilation_status_success)
        {
            std::string errStr = module.GetErrorMessage();

            size_t flen = strlen(filename);
            size_t endPos = errStr.find(':', flen+1);

            int numRow = 0, nn = 1;
            for (size_t i = endPos-1; i > flen; --i)
            {
                numRow += (errStr[i] - '0') * nn;
                nn *= 10;
            }
            std::cerr << ">>>" << numRow << '\n';

            const char* c = source;
            for (int i = 0; i < numRow; ++i)
            {
                while(*c != '\n' )
                    ++c;
                ++c; 
            }

            printf("%.20s\n", c);

            std::cerr << errStr;
            return std::vector<uint32_t>();
        }

        return { module.cbegin(), module.cend() };
    }

    // -------------------------------------------------------------------------

    bool ShaderModule::FromGLSL(const std::vector<std::string_view>& files,
                                VkShaderStageFlagBits stage)
    {
        std::vector<char> code;

        // Load all the files and concatenate their contents
        for (const auto& file : files)
        {
            VKP_LOG_INFO("Loading GLSL shader: {}", file);
            std::vector<char> contents = ReadFile(file.data());
            
            code.reserve( code.size() + contents.size() );
            code.insert( code.end(), contents.begin(), contents.end() );
        }
        VKP_ASSERT(code.size() > 0);

        // Compile the GLSL code in SPV binary
        std::vector<uint32_t> spvBinary = 
            CompileGLSL(code.data(), code.size(),
                                      FromShaderStageToKind(stage),
                                      files.back().data());
        if (spvBinary.empty())  // an error occured
        {
            return false;
        }

        if (m_ShaderModule != VK_NULL_HANDLE)
        {
            Destroy();
        }

        m_ShaderModule = CreateShaderModule(spvBinary);
        m_ShaderStage = stage;
        m_IsFromGLSL = true;

        VKP_LOG_INFO("same? {}", m_Files == files);
        if (m_Files != files)
        {
            m_Files.resize(files.size());
            m_Files.insert( m_Files.begin(), files.begin(), files.end());
        }
        VKP_ASSERT(m_Files.size() > 0);

        return m_ShaderModule != VK_NULL_HANDLE;
    }

} // namespace vkp
