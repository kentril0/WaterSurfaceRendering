/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#ifndef WATER_SURFACE_RENDERING_VULKAN_UTILS_H_
#define WATER_SURFACE_RENDERING_VULKAN_UTILS_H_

#include "core/Assert.h"


namespace vkp
{
    /**
     * @brief Reads contents of a file as binary
     * @param filename The file to read from
     * @return Vector of characters from read binary data
     */
    std::vector<char> ReadFile(std::string_view filename);

    // @return Whether, ideally, a depth format has a stencil component
    bool HasStencilComponent(VkFormat format);

} // namespace vkp

#endif // WATER_SURFACE_RENDERING_VULKAN_UTILS_H_
