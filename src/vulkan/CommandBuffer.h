/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#ifndef WATER_SURFACE_RENDERING_VULKAN_COMMAND_BUFFER_H_
#define WATER_SURFACE_RENDERING_VULKAN_COMMAND_BUFFER_H_

#include <vulkan/vulkan.h>


namespace vkp
{
    /**
     * @brief Thin abstraction of VkCommandBuffer 
     *  Is always in one of the following states:
     *   - initial: after allocation or reset, can only be moved to recording
     *      state, or freed,
     *   - recording: commands can be recorded in it,
     *   - executable: after the recording, can be submitted, reset, or recorded
     *      to another command buffer,
     *   - pending: when submitteed to a queue, MUST NOT be modified, once 
     *      completed, then reverts back to the executable state, or if is 
     *      used only one time, then moves to invalid state,
     *   - invalid: after modifying or deleting a resource recorded in the buffer,
     *      can only be reset or freed.
     */
    struct CommandBuffer
    {
        VkCommandBuffer buffer;

        CommandBuffer() : buffer(VK_NULL_HANDLE) {}
        CommandBuffer(VkCommandBuffer buf) : buffer(buf) {}

        static inline auto& fromVk(VkCommandBuffer& buf) {
            return reinterpret_cast<CommandBuffer&>(buf);
        }

        operator VkCommandBuffer() const { return buffer; }

        /**
         * @brief Starts recording to the command buffer on valid usage
         * @param inheritInfo For secondary buffers only. The state to inherit
         *  from the calling primary buffer
         * @param flags Bitmask of zero or more VkCommandBufferUsageFlagBits
         */
        void Begin(VkCommandBufferUsageFlags flags = 0, 
                   VkCommandBufferInheritanceInfo* inheritInfo = VK_NULL_HANDLE);

        /**
         * @brief Completes the recording on valid usage, was in recording state,
         *  is moved to executable state.
         */
        void End();

        /**
         * @brief Discards any previously recorded commnds, goes back in the initial state
         *  Can only be reset if the command pool that created this buffer has flags: 
         *  VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
         */
        void Reset();
    };

} // namespace vkp

#endif // WATER_SURFACE_RENDERING_VULKAN_COMMAND_BUFFER_H_