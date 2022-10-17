/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#ifndef WATER_SURFACE_RENDERING_VULKAN_SHADER_MODULE_H_
#define WATER_SURFACE_RENDERING_VULKAN_SHADER_MODULE_H_

#include <string_view>
#include <vector>
#include <memory>

#include <vulkan/vulkan.h>
#include "vulkan/Device.h"


namespace vkp
{
    /**
     * @brief Contains information for Shader module creation
     */
    struct ShaderInfo
    {
        std::vector<std::string_view> paths;    ///< Path(s) to shader file(s)
        VkShaderStageFlagBits         stage;    ///< Stage of the shader
        bool                          isSPV;    ///< if SPIR-V, else GLSL

        ShaderInfo(const std::vector<std::string_view>& ps,
                   VkShaderStageFlagBits               s,
                   bool                                SPV = true)
            : paths(ps), stage(s), isSPV(SPV) {}
    };

    /**
     * @brief VkShaderModule abstraction
     *  Shader code is loaded from a file, if more files are input then:
     *      if the files are SPV binary, then only the first one is loaded
     *      if the files are in GLSL, then their contents are concatenated
     *      together and compiled into one an SPV binary
     */
    class ShaderModule
    {
    public:
        ShaderModule(VkDevice device); 

        /**
         * @brief Creates a shader module from a file appropriate code.
         * @param device Created logical device abstraction
         * @param shaderInfo Information about the shader file
         */
        ShaderModule(VkDevice device,
                     const ShaderInfo& shaderInfo);
        /**
         * @brief Creates a shader module from a file with appropriate code
         * @param device Created logical device abstraction
         * @param files Path to files with shader code
         * @param stage Stage of the shader module, to be used in pipeline
         * @param isSPV Whether the file is SPIRV-V binary or GLSL code
         */
        ShaderModule(VkDevice device, 
                     const std::vector<std::string_view>& files,
                     VkShaderStageFlagBits stage,
                     bool isSPV = true);

        ~ShaderModule();

        operator VkShaderModule() const { return m_ShaderModule; }

        /**
         * @brief Creates a shader module from a file with SPIR-V code
         *  Can be called consecutively.
         * @param filename Path to file with shader code
         * @param stage Stage of the shader module, to be used in pipeline
         * @return True if succeded
         */
        bool FromSPV(std::string_view filename,
                     VkShaderStageFlagBits stage);

        /**
         * @brief Creates a shader module from files with GLSL code
         *  Reads the GLSL files, concatenates the contents in the given order,
         *  tries to compile it, if succeeds, then creates a shader module. 
         *  Can be called consecutively to reload the shader code.
         * @param files Path to files with shader code
         * @param stage Stage of the shader module, to be used in pipeline
         * @return True if succeded
         */
        bool FromGLSL(const std::vector<std::string_view>& files, 
                      VkShaderStageFlagBits stage);

        /**
         * @brief Tries to recreate the shader module, whether it originates
         *  from SPV or GLSL code
         *  a) if SPV, then reloads the file
         *  b) if GLSL, then reloads the file, and tries to compile it
         * @return True if succeeds, else false, and the original shader
         *  module persists, otherwise it is replaced by a new one.
         */
        bool ReCreate();

        VkShaderModule        GetModule() const { return m_ShaderModule; }
        VkShaderStageFlagBits GetStage() const { return m_ShaderStage; }

    private:

        /**
         * @brief Creates a vulkan shader module 
         * @param code Contents of a SPIR-V shader
         * @return Vulkan shader module if creation was successful
         */
        VkShaderModule CreateShaderModule(const std::vector<char>& code) const;
        VkShaderModule CreateShaderModule(
            const std::vector<uint32_t>& code) const;

        void Destroy();

    private:
        VkDevice              m_Device      { VK_NULL_HANDLE };
        VkShaderModule        m_ShaderModule{ VK_NULL_HANDLE };

        VkShaderStageFlagBits m_ShaderStage{};

        std::vector<std::string_view> m_Files;
        bool                          m_IsFromGLSL{false};
    };

} // namespace vkp


#endif // WATER_SURFACE_RENDERING_VULKAN_SHADER_MODULE_H_
