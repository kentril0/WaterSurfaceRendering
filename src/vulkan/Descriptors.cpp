/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#include "pch.h"
#include "vulkan/Descriptors.h"


namespace vkp
{
    DescriptorSetLayout::Builder& DescriptorSetLayout::Builder::AddBinding(
        DescriptorSetLayoutBinding b)
    {
        VKP_REGISTER_FUNCTION();

        VkDescriptorSetLayoutBinding& binding = b.binding;
        m_Bindings[binding.binding] = binding;
        return *this;
    }

    std::unique_ptr<DescriptorSetLayout> DescriptorSetLayout::Builder::Build()
        const
    {
        VKP_REGISTER_FUNCTION();
        return std::make_unique<DescriptorSetLayout>(m_Device, m_Bindings);
    }

    DescriptorSetLayout::DescriptorSetLayout(Device& device, BindingMap bindings)
        : m_Device(device),
          m_Bindings(bindings)
    {
        VKP_REGISTER_FUNCTION();
        VKP_ASSERT(m_Device != VK_NULL_HANDLE);

        std::vector<VkDescriptorSetLayoutBinding> bindingsArr{};
        bindingsArr.reserve(bindings.size());

        for (const auto& item : bindings)
        {
            bindingsArr.push_back(item.second);
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindingsArr.size());
        layoutInfo.pBindings = bindingsArr.data();

        VKP_ASSERT_RESULT(
            vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr,
                                        &m_Layout));
    }

    DescriptorSetLayout::~DescriptorSetLayout()
    {
        VKP_REGISTER_FUNCTION();
        vkDestroyDescriptorSetLayout(m_Device, m_Layout, nullptr);
    }

    DescriptorPool::Builder& DescriptorPool::Builder::AddPoolSize(
        VkDescriptorType descriptorType, uint32_t count)
    {
        VKP_REGISTER_FUNCTION();
        VKP_LOG_INFO("Pool size: {} x {}", descriptorType, count);

        m_PoolSizes.push_back(VkDescriptorPoolSize{
            descriptorType,
            count
        });

        return *this;
    }
    
    std::unique_ptr<DescriptorPool> DescriptorPool::Builder::Build(
        uint32_t maxSets, VkDescriptorPoolCreateFlags flags) const
    {
        VKP_REGISTER_FUNCTION();
        return std::make_unique<DescriptorPool>(m_Device, maxSets, m_PoolSizes,
                                                flags);
    }

    DescriptorPool::DescriptorPool(Device& device, uint32_t maxSets,
        const std::vector<VkDescriptorPoolSize>& poolSizes,
        VkDescriptorPoolCreateFlags flags)
        : m_Device(device)
    {
        VKP_REGISTER_FUNCTION();
        VKP_ASSERT(m_Device != VK_NULL_HANDLE);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = flags;
        poolInfo.maxSets = maxSets;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();

        VKP_ASSERT_RESULT(
            vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, 
                                   &m_Pool));
    }

    DescriptorPool::~DescriptorPool()
    {
        VKP_REGISTER_FUNCTION();
        vkDestroyDescriptorPool(m_Device, m_Pool, nullptr);
    }

    VkResult DescriptorPool::AllocateDescriptorSet(
        const VkDescriptorSetLayout descriptorSetLayout,
        VkDescriptorSet& descriptorSet) const
    {
        VKP_REGISTER_FUNCTION();

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_Pool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &descriptorSetLayout;

        return vkAllocateDescriptorSets(m_Device, &allocInfo, &descriptorSet);
    }

    void DescriptorPool::FreeDescriptorSets(
        const std::vector<VkDescriptorSet>& descriptorSets) const
    {
        VKP_REGISTER_FUNCTION();

        vkFreeDescriptorSets(m_Device, m_Pool, 
                             static_cast<uint32_t>(descriptorSets.size()),
                             descriptorSets.data());
    }

    void DescriptorPool::Reset(VkDescriptorPoolResetFlags flags)
    {
        VKP_REGISTER_FUNCTION();
        vkResetDescriptorPool(m_Device, m_Pool, flags);
    }

    DescriptorWriter::DescriptorWriter(DescriptorSetLayout& layout,
                                       DescriptorPool& pool)
        : m_Layout(layout),
          m_Pool(pool)
    {
        VKP_REGISTER_FUNCTION();
    }

    DescriptorWriter& DescriptorWriter::AddBufferDescriptor(uint32_t binding,
        VkDescriptorBufferInfo* bufferInfo, uint32_t dstArrayElement)
    {
        VKP_REGISTER_FUNCTION();
        VKP_ASSERT_MSG(m_Layout.m_Bindings.count(binding) == 1,
                       "Wrong layout binding for descriptor writes");
        VKP_ASSERT_MSG(bufferInfo != nullptr, "Must be a valid buffer handle");
        VKP_ASSERT_MSG(bufferInfo->buffer != VK_NULL_HANDLE,
                        "Must be a valid buffer handle");

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstBinding = binding;
        write.pBufferInfo = bufferInfo;
        write.dstArrayElement = dstArrayElement;

        const VkDescriptorSetLayoutBinding& bindingSet = 
            m_Layout.m_Bindings.at(binding);

        write.descriptorType = bindingSet.descriptorType;
        write.descriptorCount = bindingSet.descriptorCount;

        m_Writes.push_back(write);
        return *this;
    }

    DescriptorWriter& DescriptorWriter::AddImageDescriptor(uint32_t binding,
        VkDescriptorImageInfo* imageInfo, uint32_t dstArrayElement)
    {
        VKP_REGISTER_FUNCTION();
        VKP_ASSERT_MSG(m_Layout.m_Bindings.count(binding) == 1,
                       "Wrong layout binding for descriptor writes");

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstBinding = binding;
        write.pImageInfo = imageInfo;
        write.dstArrayElement = dstArrayElement;

        const VkDescriptorSetLayoutBinding& bindingSet = 
            m_Layout.m_Bindings.at(binding);

        write.descriptorType = bindingSet.descriptorType;
        write.descriptorCount = bindingSet.descriptorCount;

        m_Writes.push_back(write);
        return *this;
    }

    DescriptorWriter& DescriptorWriter::AddBufferViewDescriptor(uint32_t binding,
        VkBufferView* bufferView, uint32_t dstArrayElement)
    {
        VKP_REGISTER_FUNCTION();
        VKP_ASSERT_MSG(m_Layout.m_Bindings.count(binding) == 1,
                       "Wrong layout binding for descriptor writes");
        VKP_ASSERT_MSG(bufferView != nullptr, "Must be a valid buffer view handle");

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstBinding = binding;
        write.pTexelBufferView = bufferView;
        write.dstArrayElement = dstArrayElement;

        const VkDescriptorSetLayoutBinding& bindingSet = 
            m_Layout.m_Bindings.at(binding);

        write.descriptorType = bindingSet.descriptorType;
        write.descriptorCount = bindingSet.descriptorCount;

        m_Writes.push_back(write);
        return *this;
    }

    void DescriptorWriter::UpdateSet(VkDescriptorSet& dstSet)
    {
        VKP_REGISTER_FUNCTION();

        for (auto& write : m_Writes)
        {
            write.dstSet = dstSet;
        }
        
        vkUpdateDescriptorSets(m_Pool.m_Device,
                               static_cast<uint32_t>(m_Writes.size()),
                               m_Writes.data(),
                               0,           // Copy count
                               nullptr);    // copies
    }

    VkResult DescriptorWriter::BuildSet(VkDescriptorSet& dstSet)
    {
        VKP_REGISTER_FUNCTION();

        VkResult allocRes = m_Pool.AllocateDescriptorSet(m_Layout, dstSet);
        if (allocRes != VK_SUCCESS)
        {
            return allocRes;
        }

        UpdateSet(dstSet);
        return allocRes;
    }

} // namespace vkp