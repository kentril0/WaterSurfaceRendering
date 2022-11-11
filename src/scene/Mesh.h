/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#ifndef WATER_SURFACE_RENDERING_SCENE_MESH_H_
#define WATER_SURFACE_RENDERING_SCENE_MESH_H_

#include <vector>
#include <memory>

#include "vulkan/Device.h"
#include "vulkan/Buffer.h"


/**
 * @brief TODO use case
 */
template<class T>
class Mesh
{
public:
    Mesh();

    Mesh(const vkp::Device& device,
         const std::vector<T>& vertices,
         const std::vector<uint32_t>& indices) 
        : m_Vertices(vertices), m_Indices(indices)
    {
        CreateBuffers(device, GetVerticesSize(), GetIndicesSize());
    }

    Mesh(const vkp::Device& device,
         std::vector<T>&& vertices,
         std::vector<uint32_t>&& indices)
         : m_Vertices(vertices), m_Indices(indices)
    {
        CreateBuffers(device, GetVerticesSize(), GetIndicesSize());
    }

    Mesh(const vkp::Device& device,
         VkDeviceSize maxVerticesSize,
         VkDeviceSize maxIndicesSize)
    {
        CreateBuffers(device, maxVerticesSize, maxIndicesSize);
    }

    ~Mesh();

    void SetVertices(const std::vector<T>& kVertices) {
        m_Vertices = kVertices;
        m_LatestBuffersOnDevice = false;
    }
    void SetVertices(std::vector<T>&& vertices) {
        m_Vertices = vertices;
        m_LatestBuffersOnDevice = false;
    }

    void SetIndices(const std::vector<uint32_t>& kIndices) {
        m_Indices = kIndices;
        m_LatestBuffersOnDevice = false;
    }
    void SetIndices(std::vector<uint32_t>&& indices) {
        m_Indices = indices;
        m_LatestBuffersOnDevice = false;
    }

    /**
     * @pre Set vertices and indices
     * @brief Stages a copy of the set vertices and indices to the device buffers.
     * @param stagingBuffer Reference to a new buffer object, will be used
     *  to transfer indices to index buffer, must be mapped and big enough
     *  to store set vertices and indices sequentially in memory.
     * @param cmdBuffer Command buffer in recording state to record copy cmd to
     */
    bool UpdateDeviceBuffers(vkp::Buffer& stagingBuffer,
                             VkCommandBuffer cmdBuffer)
    {
        if (m_LatestBuffersOnDevice)
            return false;

        StageCopyVerticesToVertexBuffer(stagingBuffer, cmdBuffer);
        StageCopyIndicesToIndexBuffer(stagingBuffer, cmdBuffer);

        m_LatestBuffersOnDevice = true;
        return true;
    }

    /**
     * @pre Uploaded data to vertex and index buffers
     */
    void Render(VkCommandBuffer cmdBuffer)
    {
        BindBuffers(cmdBuffer);
        DrawIndexed(cmdBuffer, GetIndexCount());
    }

    /**
     * @brief Creates vertex and index buffers on the device with
     *  reserved size according to the set vertices and indices.
     * @param verticesSize Allocated size for vertex buffer
     * @param indicesSize Allocated size for index buffer
     */
    void CreateBuffers(const vkp::Device& device,
                       VkDeviceSize verticesSize,
                       VkDeviceSize indicesSize);

    void UpdateDeviceVertexBuffer(vkp::Buffer& stagingBuffer,
                                  VkCommandBuffer cmdBuffer)
    {
        StageCopyVerticesToVertexBuffer(stagingBuffer, cmdBuffer);
    }
    void UpdateDeviceIndexBuffer(vkp::Buffer& stagingBuffer,
                                 VkCommandBuffer cmdBuffer)
    {
        StageCopyVerticesToVertexBuffer(stagingBuffer, cmdBuffer);
    }

    const std::vector<T>& GetVertices() const { return m_Vertices; }
    std::vector<T>& GetVertices() { return m_Vertices; }

    uint32_t GetIndexCount() const { return m_Indices.size(); }
    uint32_t GetVertexCount() const { return m_Vertices.size(); }

    VkDeviceSize GetVerticesSize() const { return sizeof(T) * GetVertexCount(); }
    VkDeviceSize GetIndicesSize() const {
        return sizeof(uint32_t) * GetIndexCount();
    }

private:
    void InitVertices();
    void InitIndices();

    void StageCopyVerticesToVertexBuffer(vkp::Buffer& stagingBuffer,
                                         VkCommandBuffer cmdBuffer);
    void StageCopyIndicesToIndexBuffer(vkp::Buffer& stagingBuffer,
                                       VkCommandBuffer cmdBuffer);

    void BindBuffers(VkCommandBuffer cmdBuffer) const;
    void DrawIndexed(VkCommandBuffer cmdBuffer,
                     const uint32_t kIndexCount) const;

private:
    std::vector<T> m_Vertices;
    std::vector<uint32_t> m_Indices;

    // On device buffers
    std::unique_ptr<vkp::Buffer> m_VertexBuffer{ nullptr };
    std::unique_ptr<vkp::Buffer> m_IndexBuffer{ nullptr };

    bool m_LatestBuffersOnDevice{ false };
};


// TODO into implementation header file

template <class T>
Mesh<T>::Mesh()
{
    VKP_REGISTER_FUNCTION();
}

template <class T>
Mesh<T>::~Mesh()
{
    VKP_REGISTER_FUNCTION();
}

template <class T>
void Mesh<T>::CreateBuffers(
    const vkp::Device& device,
    VkDeviceSize verticesSize,
    VkDeviceSize indicesSize
)
{
    VKP_REGISTER_FUNCTION();

    m_VertexBuffer.reset( new vkp::Buffer(device) );
    m_IndexBuffer.reset( new vkp::Buffer(device) );

    m_VertexBuffer->Create(verticesSize,
                           VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                           VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    m_IndexBuffer->Create(indicesSize,
                          VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                          VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

template <class T>
void Mesh<T>::StageCopyVerticesToVertexBuffer(vkp::Buffer& stagingBuffer,
                                           VkCommandBuffer cmdBuffer)
{
    VKP_REGISTER_FUNCTION();
    VKP_ASSERT(GetVertexCount() > 0);

    const VkDeviceSize kVerticesSize = GetVerticesSize();
    stagingBuffer.CopyToMapped(m_Vertices.data(), kVerticesSize);

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = kVerticesSize;

    m_VertexBuffer->StageCopy(stagingBuffer, &copyRegion, cmdBuffer);
}

template <class T>
void Mesh<T>::StageCopyIndicesToIndexBuffer(vkp::Buffer& stagingBuffer,
                                         VkCommandBuffer cmdBuffer)
{
    VKP_REGISTER_FUNCTION();
    VKP_ASSERT(GetIndexCount() > 0);

    const VkDeviceSize kVerticesSize = GetVerticesSize();
    const VkDeviceSize kIndicesSize = GetIndicesSize();

    void* dataAddr = stagingBuffer.GetMappedAddress();
    stagingBuffer.CopyToMapped(
        m_Indices.data(),
        kIndicesSize,
        static_cast<void*>(
            static_cast<uint8_t*>(dataAddr) + kVerticesSize
        )
    );

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = kVerticesSize;
    copyRegion.dstOffset = 0;
    copyRegion.size = kIndicesSize;

    m_IndexBuffer->StageCopy(stagingBuffer, &copyRegion, cmdBuffer);
}

template <class T>
void Mesh<T>::BindBuffers(VkCommandBuffer cmdBuffer) const
{
    const VkBuffer kVertexBuffers[] = { m_VertexBuffer->GetBuffer() };
    const VkDeviceSize kOffsets[] = { 0 };
    const uint32_t kFirstBinding = 0, kBindingCount = 1;

    vkCmdBindVertexBuffers(cmdBuffer, kFirstBinding, kBindingCount, 
                           kVertexBuffers, kOffsets);

    const VkDeviceSize kIndexOffset = 0;
    vkCmdBindIndexBuffer(cmdBuffer, m_IndexBuffer->GetBuffer(),
                         kIndexOffset, VK_INDEX_TYPE_UINT32);
}

template <class T>
void Mesh<T>::DrawIndexed(VkCommandBuffer cmdBuffer,
                                   const uint32_t kIndexCount) const
{
    const uint32_t kInstanceCount = 1; 
    const uint32_t kFirstIndex = 0, kFirstInstance = 0;
    const uint32_t kVertexOffset = 0;

    vkCmdDrawIndexed(cmdBuffer, kIndexCount, kInstanceCount, kFirstIndex,
                     kVertexOffset, kFirstInstance);
}


#endif // WATER_SURFACE_RENDERING_SCENE_MESH_H_
