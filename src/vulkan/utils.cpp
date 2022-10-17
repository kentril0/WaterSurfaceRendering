/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#include "pch.h"
#include <fstream>
#include "vulkan/utils.h"


namespace vkp
{

    std::vector<char> ReadFile(std::string_view filename)
    {
        // Open reading from the end, read as binary
        std::ifstream file(filename.data(), std::ios::ate | std::ios::binary);
        VKP_ASSERT_MSG(file.is_open(), "Failed to open a file '{}'", filename);
        if (!file.is_open())
        {
            std::cerr << "Failed to open a file " << filename << std::endl;
            exit(1);
        }

        // Use read position to determine the size of the file
        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer(fileSize);

        // Seek back to the beginning of the file and read all of the bytes
        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }

    bool HasStencilComponent(VkFormat format)
    {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
               format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

} // namespace vkp
