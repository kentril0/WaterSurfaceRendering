/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#ifndef WATER_SURFACE_RENDERING_VULKAN_DESCRIPTORS_H_ 
#define WATER_SURFACE_RENDERING_VULKAN_DESCRIPTORS_H_

#include <memory>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>
#include "vulkan/Device.h"

/**
 Example: Working with vulkan descriptors using this abstraction
 ---------------------------------------------------------
 // 1. Create DescriptorSetLayout
 DescriptorSetLayout::Builder layoutBuilder(device);

 std::unique_ptr<DescriptorSetLayout> layout = 
    layoutBuilder.AddBinding({
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
        }).AddBinding({
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
        }).AddBinding({
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    }).Build();

// 2. Create descriptor pool
DescriptorPool::Builder poolBuilder(device);

std::unique_ptr<DescriptorPool> pool = poolBuilder
    .AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10)
    .AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5)
    .Build(maxSets = 1000);

// 3. Allocate descriptor sets
VkDescriptorSet descriptorSets[kImageCount];
...
for (uint32_t i = 0; i < kImageCount; ++i)
{
    auto err = pool->AllocateDescriptorSet(layout.GetLayout(), descriptorSets[i]);
    VKP_ASSERT_RESULT(err);
}

// 4. Update descriptor sets

DescriptorWriter writer(*layout, *pool)

writer.AddBufferDescriptor(0, bufferInfo)
      .AddImageDescriptor(1, imageInfo)
      .AddBufferDescriptor(2, bufferInfo)
.Build()
 
*/


namespace vkp
{
    struct DescriptorSetLayoutBinding
    {
        VkDescriptorSetLayoutBinding binding;

        DescriptorSetLayoutBinding() : binding{}  {}

        /**
         * @param binding Binding number corresponding to a resource of the same
         *  binding nubmer in the shader stages
         * @param descriptorType Type of resource descriptors used
         * @param stageFlags Mask of pipeline stages that can access a resource
         *  for this binding
         * @param count If it is an array, then number of descriptors accessible
         * @param pImmutableSamplers To initialize immutable samplers
         */
        DescriptorSetLayoutBinding(
            uint32_t           binding,
            VkDescriptorType   descriptorType,
            VkShaderStageFlags stageFlags,
            uint32_t           count = 1,
            const VkSampler*   pImmutableSamplers = nullptr)
            : binding{binding, descriptorType, count, stageFlags, 
                      pImmutableSamplers}  {}

        operator VkDescriptorSetLayoutBinding() const { return binding; }
    };

    class DescriptorSetLayout
    {
    public:
        using BindingMap = 
            std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding>;

        /**
         * @brief Used to create an instance of DescriptorSetLayout based on the
         *  supplied bindings.
         */
        class Builder
        {
        public:
            Builder(const Device& device) : m_Device(device) {}

            Builder& AddBinding(DescriptorSetLayoutBinding binding);

            /** 
             * @return Unique pointer to the built DescriptorSetLayout from the
             *  previously added layout bindings
             */
            std::unique_ptr<DescriptorSetLayout> Build() const;

        private:
            const Device& m_Device;

            BindingMap m_Bindings{};
        };

        /// 
        DescriptorSetLayout(const Device& device, 
                            BindingMap bindings);
        ~DescriptorSetLayout();

        operator VkDescriptorSetLayout() const { return m_Layout; }
        VkDescriptorSetLayout& GetLayout() {
            return m_Layout;
        }
        const VkDescriptorSetLayout& GetLayout() const {
            return m_Layout;
        }

        DescriptorSetLayout(const DescriptorSetLayout &) = delete;
        DescriptorSetLayout &operator=(const DescriptorSetLayout &) = delete;

    private:
        friend class DescriptorWriter;

        const Device& m_Device;

        VkDescriptorSetLayout m_Layout{ VK_NULL_HANDLE };
        BindingMap m_Bindings{};
    };

    class DescriptorPool
    {
    public:
        class Builder
        {
        public:
            Builder(Device& device) : m_Device(device) {}

            /**
             * @brief
             * @param descriptorType Type of the descriptor
             * @param count Number of descriptors of that type to allocate
             */
            Builder& AddPoolSize(VkDescriptorType descriptorType,
                                 uint32_t count);

            std::unique_ptr<DescriptorPool> Build(
                uint32_t maxSets = 1000,
                VkDescriptorPoolCreateFlags flags = 0) const;
        private:
            Device& m_Device;

            std::vector<VkDescriptorPoolSize> m_PoolSizes{};
        };

        DescriptorPool(Device&                                  device,
                       uint32_t                                 maxSets,
                       const std::vector<VkDescriptorPoolSize>& poolSizes, 
                       VkDescriptorPoolCreateFlags              flags = 0);
        
        /** 
         * @brief All allocated descriptor sets are implictly freed and 
         *  become invalid. Descriptor sets do not need to be freed beforehand.
         */
        ~DescriptorPool();

        /**
         * @brief
         * @param
         * @return Result of the descriptor set allocation call. May fail on
         *  allocation if total number of descriptor sets allocated exceed 
         *  the value maxSets used to create the pool, or due to the lack of 
         *  space.
         */
        VkResult AllocateDescriptorSet(
            const VkDescriptorSetLayout  descriptorSetLayout,
            VkDescriptorSet&             descriptor) const;

        /**
         * @brief Frees all the supplied descriptor sets allocated from the pool
         *  making them invalid.
         * @warn Descriptor pool must have been created with the 
         *  VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT flag.
         */
        void FreeDescriptorSets(
            const std::vector<VkDescriptorSet>& descriptorSets) const;

        /**
         * @brief Returns all allocated descriptor sets to the pool, rather than
         *  freeing them individually - recycles all of the resources back to
         *  the pool, the descriptor sets are implicitly freed
         */
        void Reset(VkDescriptorPoolResetFlags flags = 0);
        
        DescriptorPool(const DescriptorPool&) = delete;
        DescriptorPool& operator&=(const DescriptorPool&) = delete;

    private:
        friend class DescriptorWriter;

        Device& m_Device;

        VkDescriptorPool m_Pool{ VK_NULL_HANDLE };
    };

    class DescriptorWriter
    { 
    public:
        /**
         * @brief Create writer for descriptor sets of certain layout that are
         *  to be allocated from a certain pool
         * @param layout Layout for the descriptor sets to be written to
         * @param pool Descriptor pool for the descriptor sets to allocate from
         */
        DescriptorWriter(const DescriptorSetLayout& layout,
                         const DescriptorPool&      pool);

        /**
         * @brief Add VkDescriptorBufferInfo to descriptor set writes
         * @param binding Binding number of the descriptor set in the layout to
         *  write to
         * @param bufferInfo Buffer descriptor for this descriptor set
         */
        DescriptorWriter& AddBufferDescriptor(
            uint32_t                binding,
            VkDescriptorBufferInfo* bufferInfo,
            uint32_t                dstArrayElement = 0);

        DescriptorWriter& AddImageDescriptor(
            uint32_t binding,
            VkDescriptorImageInfo* imageInfo,
            uint32_t dstArrayElement = 0);

        DescriptorWriter& AddBufferViewDescriptor(
            uint32_t      binding,
            VkBufferView* texelBufferView,
            uint32_t      dstArrayElement = 0);

        /**
         * @param dstSet Descriptor set to update with all the previously added
         *  writes
         * @warn Descriptor bindings updated by this command which were created
         *  without the VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT or
         *  VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT bits set must
         *  not be used by any command that was recorded to a command buffer
         *  which is in the pending state
         * @warn If descriptorType is VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER or
         *  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, the offset member of
         *  each element of pBufferInfo must be a multiple of
         *  VkPhysicalDeviceLimits::minUniformBufferOffsetAlignment
         * @warn If descriptorType is VK_DESCRIPTOR_TYPE_STORAGE_BUFFER or 
         *  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, the offset member of each
         *  element of pBufferInfo must be a multiple of
         *  VkPhysicalDeviceLimits::minStorageBufferOffsetAlignment
         */
        void UpdateSet(VkDescriptorSet& dstSet);

        /**
         * @brief Allocates the descriptor set from the assigned pool and 
         *  writes all the previously added descriptors to the descriptor set.
         * @param dstSet The destination descriptor set 
         * @return Result of allocation
         */
        VkResult BuildSet(VkDescriptorSet& dstSet);
        
    private:
        const DescriptorSetLayout& m_Layout;
        const DescriptorPool&      m_Pool;

        std::vector<VkWriteDescriptorSet> m_Writes{};
    };

} // namespace vkp


#endif // WATER_SURFACE_RENDERING_VULKAN_DESCRIPTORS_H_
