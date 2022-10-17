/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#include "pch.h"
#include "vulkan/CommandBuffer.h"


namespace vkp
{
    void CommandBuffer::Begin(VkCommandBufferUsageFlags flags, 
                              VkCommandBufferInheritanceInfo* inheritInfo)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = flags;
        beginInfo.pInheritanceInfo = inheritInfo;

        VKP_ASSERT_RESULT(vkBeginCommandBuffer(buffer, &beginInfo));
    }

    void CommandBuffer::End()
    {
        VKP_ASSERT_RESULT(vkEndCommandBuffer(buffer));
    }

    void CommandBuffer::Reset()
    {
        //vkResetCommandBuffer();
    }

} // namespace vkp
