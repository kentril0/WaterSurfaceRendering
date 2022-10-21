
/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#ifndef WATER_SURFACE_RENDERING_SCENE_WATER_SURFACE_MESH_H_
#define WATER_SURFACE_RENDERING_SCENE_WATER_SURFACE_MESH_H_

#include <vector>
#include <memory>

#include "vulkan/Device.h"
#include "vulkan/Buffer.h"


/**
 * @brief Water surface mesh represents a grid or a tile of a certain tile,
 *  made of vertices of type "WaterSurfaceMesh::Vertex".
 */
class WaterSurfaceMesh
{
public:
    struct Vertex;

public:
    /**
     * @brief Creates vertex and index buffers to accomodate maximum size of
     *  vertices and indices.
     *  To render the mesh, fnc "Prepare()" must be called with the size of tile
    */
    WaterSurfaceMesh(const vkp::Device& device);
    ~WaterSurfaceMesh();

    /**
     * @brief Generates vertices and indices of a tile of a certain size
     * @param size Size of a tile, must be in range [s_kMinSize, s_kMaxSize]
     * @param scale Tile scale or length, must be positive
     * @param stagingBuffer Reference to a new buffer object, will be used
     *  to transfer indices to index buffer 
     * @param cmdBuffer Command buffer in recording state to record copy cmd to
     */
    void Prepare(uint32_t size,
                 float scale,
                 vkp::Buffer& stagingBuffer,
                 VkCommandBuffer cmdBuffer);

    void Render(VkCommandBuffer cmdBuffer);

    const std::vector<Vertex>& GetVertices() const { return m_Vertices; }
    std::vector<Vertex>& GetVertices() { return m_Vertices; }

    /** @brief Copies current vertices to vertex buffer and flushes
     *  Use to see changes after modification of vertices.
     */
    void UpdateVertexBuffer();

private:

    void CreateBuffers(const vkp::Device& device);

    void InitVertices();
    void InitIndices();

    //                                  m_Size*(m_Size+2)+1
    uint32_t GetVertexCount() const { return (m_Size+1) * (m_Size+1); }

    uint32_t GetTotalIndexCount() const
    {
        const uint32_t kTrianglesPerQuad = 2, kIndicesPerTriangle = 3;
        return m_Size * m_Size * kTrianglesPerQuad * kIndicesPerTriangle;
    }

    uint32_t GetMaxVertexCount() const { return (s_kMaxSize+1) * (s_kMaxSize+1); }
    uint32_t GetMaxIndexCount() const
    {
        const uint32_t kTrianglesPerQuad = 2, kIndicesPerTriangle = 3;
        return s_kMaxSize*s_kMaxSize * kTrianglesPerQuad * kIndicesPerTriangle;
    }

    void MapVertexBufferToCurrentSize();
    void CopyIndicesToIndexBuffer(vkp::Buffer& stagingBuffer,
                                  VkCommandBuffer cmdBuffer);

    void BindBuffers(VkCommandBuffer cmdBuffer) const;
    void DrawIndexed(VkCommandBuffer cmdBuffer,
                     const uint32_t kIndexCount) const;

private:
    static const uint32_t s_kMinSize{ 4 };
    static const uint32_t s_kMaxSize{ 1024 };

    uint32_t m_Size{ 0 };
    float m_Scale{ 0.0 };

    std::vector<Vertex> m_Vertices;
    std::vector<uint32_t> m_Indices;

    uint32_t m_IndicesCount{ 0 };

    std::unique_ptr<vkp::Buffer> m_VertexBuffer;
    std::unique_ptr<vkp::Buffer> m_IndexBuffer;
};

struct WaterSurfaceMesh::Vertex
{
    glm::vec4 pos;
    glm::vec4 normal;

    constexpr static VkVertexInputBindingDescription GetBindingDescription()
    {
        return VkVertexInputBindingDescription {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        };
    }

    static std::vector<VkVertexInputAttributeDescription>
        GetAttributeDescriptions()
    {
        return std::vector<VkVertexInputAttributeDescription> {
            {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                .offset = offsetof(Vertex, pos)
            },
            {
                .location = 1,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                .offset = offsetof(Vertex, normal)
            }
        };
    }

    static const inline std::vector<VkVertexInputBindingDescription>
        s_BindingDescriptions{ GetBindingDescription() };

    static const inline std::vector<VkVertexInputAttributeDescription>
        s_AttribDescriptions{ GetAttributeDescriptions() };
};


#endif // WATER_SURFACE_RENDERING_SCENE_WATER_SURFACE_MESH_H_
