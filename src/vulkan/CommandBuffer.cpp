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

        auto err = vkBeginCommandBuffer(buffer, &beginInfo);
        VKP_ASSERT_RESULT(err);
    }

    void CommandBuffer::End()
    {
        auto err = vkEndCommandBuffer(buffer);
        VKP_ASSERT_RESULT(err);
    }

    void CommandBuffer::Reset()
    {
        //vkResetCommandBuffer();
    }

} // namespace vkp
