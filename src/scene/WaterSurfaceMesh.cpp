
/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#include "pch.h"
#include "scene/WaterSurfaceMesh.h"


WaterSurfaceMesh::WaterSurfaceMesh(const vkp::Device& device)
{
    VKP_REGISTER_FUNCTION();
    CreateBuffers(device);
}

WaterSurfaceMesh::~WaterSurfaceMesh()
{
    VKP_REGISTER_FUNCTION();

}

void WaterSurfaceMesh::CreateBuffers(const vkp::Device& device)
{
    VKP_REGISTER_FUNCTION();

    m_VertexBuffer.reset( new vkp::Buffer(device) );

    // Create Vertex buffer on host
    const VkDeviceSize kVerticesSize = sizeof(Vertex) * GetMaxVertexCount();
    m_VertexBuffer->Create(kVerticesSize,
                           VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // Create index buffer on device
    m_IndexBuffer.reset( new vkp::Buffer(device) );

    const VkDeviceSize kIndicesSize = sizeof(uint32_t) * GetMaxIndexCount();
    m_IndexBuffer->Create(kIndicesSize,
                          VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                          VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void WaterSurfaceMesh::Prepare(uint32_t size,
                               float scale,
                               vkp::Buffer& stagingBuffer,
                               VkCommandBuffer cmdBuffer)
{
    if (size == m_Size)
        return;

    m_Size = glm::clamp(size, s_kMinSize, s_kMaxSize);
    m_Scale = glm::max(0.001f, scale);

    InitVertices();
    InitIndices();

    MapVertexBufferToCurrentSize();
    UpdateVertexBuffer();
    CopyIndicesToIndexBuffer(stagingBuffer, cmdBuffer);
}

void WaterSurfaceMesh::InitVertices()
{
    VKP_REGISTER_FUNCTION();

    m_Vertices.resize( GetVertexCount() );

    const glm::vec4 kDefaultNormal(0.0f, 1.0f, 0.0f, 0.0f);

    const uint32_t kSize = m_Size;
    const uint32_t kSize1 = kSize + 1;
    // TODO benchmark
    // const float kScaleDivSize = m_Scale / m_Size;
    // const float kHalfSize = m_Size / 2.0f;
    for (uint32_t y = 0; y < kSize1; ++y)
    {
        for (uint32_t x = 0; x < kSize1; ++x)
        {
            auto& vertex = m_Vertices[y * kSize1 + x];

            vertex.pos = glm::vec4(
                (x - kSize / 2.0f) * m_Scale / kSize,
                0.0f,   // Must be zero
                (y - kSize / 2.0f) * m_Scale / kSize,
                0.0f
            );

            vertex.normal = kDefaultNormal;
        }
    }
}
    
void WaterSurfaceMesh::InitIndices()
{
    VKP_REGISTER_FUNCTION();

    m_Indices.resize( GetTotalIndexCount() );

    // TODO benchmark with member and local
    const uint32_t kSize = m_Size;
    const uint32_t kSize1 = kSize+1;

    uint32_t index = 0;
    for (uint32_t y = 0; y < kSize; ++y)
    {
        for (uint32_t x = 0; x < kSize; ++x)
        {
            const uint32_t kVertexIndex = y * kSize1 + x;
            
            // Top triangle
            m_Indices[index++] = kVertexIndex;
            m_Indices[index++] = kVertexIndex + kSize1 + 1;
            m_Indices[index++] = kVertexIndex + 1;
            // Bottom triangle
            m_Indices[index++] = kVertexIndex;
            m_Indices[index++] = kVertexIndex + kSize1;
            m_Indices[index++] = kVertexIndex + kSize1 + 1;
        }
    }

    m_IndicesCount = index;
}

void WaterSurfaceMesh::UpdateVertexBuffer()
{
    m_VertexBuffer->CopyToMapped(m_Vertices.data(),
                                 sizeof(Vertex) * GetVertexCount() );
}

void WaterSurfaceMesh::MapVertexBufferToCurrentSize()
{
    m_VertexBuffer->Unmap();

    auto err = m_VertexBuffer->Map( sizeof(Vertex) * GetVertexCount() );
    VKP_ASSERT_RESULT(err);
}

void WaterSurfaceMesh::CopyIndicesToIndexBuffer(vkp::Buffer& stagingBuffer,
                                                VkCommandBuffer cmdBuffer)
{
    VKP_REGISTER_FUNCTION();

    VKP_ASSERT(m_IndicesCount > 0);
    const VkDeviceSize kIndicesSize = sizeof(uint32_t) * m_IndicesCount;

    stagingBuffer.Create(kIndicesSize,
                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    stagingBuffer.Fill(m_Indices.data(), kIndicesSize);

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = kIndicesSize;

    m_IndexBuffer->StageCopy(stagingBuffer, &copyRegion, cmdBuffer);
}

void WaterSurfaceMesh::Render(VkCommandBuffer cmdBuffer)
{
    BindBuffers(cmdBuffer);
    DrawIndexed(cmdBuffer, m_IndicesCount);
}

void WaterSurfaceMesh::BindBuffers(VkCommandBuffer cmdBuffer) const
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

void WaterSurfaceMesh::DrawIndexed(VkCommandBuffer cmdBuffer,
                                   const uint32_t kIndexCount) const
{
    const uint32_t kInstanceCount = 1; 
    const uint32_t kFirstIndex = 0, kFirstInstance = 0;
    const uint32_t kVertexOffset = 0;

    vkCmdDrawIndexed(cmdBuffer, kIndexCount, kInstanceCount, kFirstIndex,
                     kVertexOffset, kFirstInstance);
}
