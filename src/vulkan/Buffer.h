/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#ifndef WATER_SURFACE_RENDERING_VULKAN_BUFFER_H_
#define WATER_SURFACE_RENDERING_VULKAN_BUFFER_H_

#include <memory>
#include <vulkan/vulkan.h>
#include "vulkan/Device.h"


namespace vkp
{
    // TODO derived classes for Vertex, Index, and Unifrm and others buffers
    // virtual bind buffer function
    class Buffer
    {
    public:
        /** @brief Initializes members for subsequent buffer creation */
        Buffer(const Device& device);

        /**
         * @brief Destroys the buffer handle, unmaps its memory (if mapped),
         *  and frees the memory.
         */
        ~Buffer();

        Buffer(Buffer&& other)
            : m_Device(other.m_Device),
              m_Buffer(other.m_Buffer),
              m_BufferMemory(other.m_BufferMemory),
              m_MapAddr(other.m_MapAddr),
              m_BufferView(other.m_BufferView)
        {
            other.m_Buffer = VK_NULL_HANDLE;
            other.m_BufferMemory = VK_NULL_HANDLE;
            other.m_MapAddr = nullptr;
            other.m_BufferView = VK_NULL_HANDLE;
        }

        operator VkBuffer() const { return m_Buffer; }
        VkBuffer GetBuffer() const { return m_Buffer; }

        VkDescriptorBufferInfo GetDescriptor(VkDeviceSize offset = 0,
                                             VkDeviceSize range = VK_WHOLE_SIZE
        ) const {
            VKP_ASSERT(m_Buffer != VK_NULL_HANDLE);
            return VkDescriptorBufferInfo{
                .buffer = m_Buffer,
                .offset = offset,
                .range = range
            };
        }
    
        /**
         * @brief Creates a buffer with required parameters, allocates its 
         *  required memory and binds it to the buffer.
         * @param size Required size of the buffer
         * @param usage Usage flags of the buffer, by default used as 
         *  a staging buffer
         * @param properties Required properties for the buffer memory, by 
         *  default in host visible coherent memory
         * @param sharingMode Accessibility among queue families, by default 
         *  accessible only to a single queue family at a time
         */
        void Create(VkDeviceSize size, 
                    VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VkMemoryPropertyFlags properties = 
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE);

        /**
         * @brief *Maps* the whole bound memory of the buffer, copies the 
         *  supplied data of certain size and *umaps* the memory.
         * @param data Pointer to the source data
         * @param size Size of the data to be copied
         * @return Result of the mapping call
         */
        VkResult Fill(const void* data, VkDeviceSize size);

        /** 
         * @brief Records a command to copy data from the source staging buffer 
         *  to this buffer based on supplied copy region.
         * Assumes that all parameters are valid in respect to the copy command.
         * @param commandBuffer Command buffer in recording stage
         */ 
        void StageCopy(VkBuffer stagingBuffer, 
                       const VkBufferCopy *pRegion,
                       VkCommandBuffer commandBuffer);

        /**
         * @brief Maps a memory region of the buffer. (Memory of the buffer
         *  must have the host visible property to be considered mappable).
         *  Keeps the mapped address saved internally as a member
         * @param size Size of the memory to map
         * @param offset Starting offset of the mapped memory
         * @return Result of the mapping call
         */
        VkResult Map(VkDeviceSize size = VK_WHOLE_SIZE,
                     VkDeviceSize offset = 0);

        /**
         * @brief Maps a memory region of the buffer. (Memory of the buffer
         *  must have the host visible property to be considered mappable).
         * @param ppData Address of the mapped memory region, if successful
         * @param size Size of the memory to map
         * @param offset Starting offset of the mapped memory
         * @return Result of the mapping call
         */
        VkResult Map(void** ppData,
                     VkDeviceSize size = VK_WHOLE_SIZE, 
                     VkDeviceSize offset = 0);

        void* GetMappedAddress() const { return m_MapAddr; }

        /** @brief Unmaps the mapped memory region */
        void Unmap();

        /** 
         * @brief Copies the data 'srcData' of size 'size' to the mapped
         *  memory region of the buffer
         * @param srcData Pointer to data the be copied
         * @param size Size of the data to be copied, in bytes
         * @param destAddr Optional custom address in the buffer's mapped region
         *  to start copying to, if nullptr then the mapped address is used
         */
        void CopyToMapped(const void* srcData, VkDeviceSize size,
                          void* destAddr = nullptr);

        /**
         * @brief Flushes mapped memory range
         *      (for case when memory is *NOT* HOST_COHERENT)
         * @param size Size of range in bytes
         * @param offset Zero-based byte offset from the beginning of the memory
         *   object, MUST be a multiple of VkPhysicalDeviceLimits::nonCoherentAtomSize
         * @pre If size is not equal to VK_WHOLE_SIZE, size must either be
         *  a multiple of VkPhysicalDeviceLimits::nonCoherentAtomSize, or offset
         *  plus size must equal the size of memory.
         */
        void FlushMappedRange(VkDeviceSize size = VK_WHOLE_SIZE,
                              VkDeviceSize offset = 0) const;
        
        /**
         * @brief In order to create a buffer view, the buffer must have been
         *  created with usage flags: VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT,
         *  or VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT
         * @param format The format of the data elements in the buffer
         *  FORMAT MUST BE SUPPORTED FOR that certain BUFFER
         * @param reqFeatures Required format features to check whether a device
         *  supports them on that format
         * @param offset OFfset in bytes from the base address of the buffer.
         *  Accesses to the buffer view from shaders use addressing that is
         *  relative to this starting offset.
         * @param range Range is a size in bytes of the buffer view.
         */
        void CreateBufferView(VkFormat format,
                              VkFormatFeatureFlags reqFeatures,
                              VkDeviceSize offset = 0,
                              VkDeviceSize range = VK_WHOLE_SIZE);

        void DestoryBufferView();

        VkBufferView GetBufferView() const { return m_BufferView; };

    private:
        void CreateBuffer(VkDeviceSize size, 
                          VkBufferUsageFlags usage, 
                          VkSharingMode sharingMode);

        void AllocateMemory(VkMemoryPropertyFlags properties);

        /** 
         * @brief Associate the memory with the buffer at offset 'offset'
         */
        void Bind(VkDeviceSize offset = 0) const;

        void Destroy();

    private:
        const Device& m_Device;

        VkBuffer       m_Buffer       { VK_NULL_HANDLE };
        VkDeviceMemory m_BufferMemory { VK_NULL_HANDLE };

        // Address of the mapped memory region
        void*          m_MapAddr       { nullptr }; 

        // Buffer view - for texel buffers
        VkBufferView m_BufferView{ VK_NULL_HANDLE };
    };

} // namespace vkp


#endif // WATER_SURFACE_RENDERING_VULKAN_BUFFER_H_
