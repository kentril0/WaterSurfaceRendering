/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#ifndef WATER_SURFACE_RENDERING_SCENE_WATER_SURFACE_MESH_H_
#define WATER_SURFACE_RENDERING_SCENE_WATER_SURFACE_MESH_H_

#include <vector>
#include <memory>

#include "vulkan/Device.h"
#include "vulkan/Descriptors.h"
#include "vulkan/ShaderModule.h"
#include "vulkan/Buffer.h"
#include "vulkan/Pipeline.h"
#include "vulkan/Texture2D.h"

#include "scene/Mesh.h"
#include "scene/WSTessendorf.h"

#include "Gui.h"


/**
 * @brief Enable for double buffered textures: two sets of textures, one used for
 *  copying to, the other for rendering to, at each frame they are swapped.
 * For the current use-case, the performance might be slightly worse.
 */
//#define DOUBLE_BUFFERED

/**
 * @brief Represents a water surface rendered as a mesh.
 *
 */
class WaterSurfaceMesh
{
public:
    struct Vertex;

    static const uint32_t s_kMinTileSize{ 16 };
    static const uint32_t s_kMaxTileSize{ 1024 };

public:
    /**
     * @brief Creates vertex and index buffers to accomodate maximum size of
     *  vertices and indices.
     *  To render the mesh, fnc "Prepare()" must be called with the size of tile
    */
    WaterSurfaceMesh(const vkp::Device& device,
                     const vkp::DescriptorPool& descriptorPool);
    ~WaterSurfaceMesh();

    void CreateRenderData(
        VkRenderPass renderPass,
        const uint32_t kImageCount,
        const VkExtent2D kFramebufferExtent,
        const bool kFramebufferHasDepthAttachment
    );

    void Prepare(VkCommandBuffer cmdBuffer);

    void Update(float dt);

    void PrepareRender(
        const uint32_t frameIndex,
        VkCommandBuffer cmdBuffer,
        const glm::mat4& viewMat,
        const glm::mat4& projMat,
        const glm::vec3& camPos
    );
 
    void Render(
        const uint32_t frameIndex,
        VkCommandBuffer cmdBuffer
    );

    // @pre Called inside ImGui Window scope
    void ShowGUISettings();

private:
    // TODO batch 

    void PrepareMesh(VkCommandBuffer cmdBuffer);
    void GenerateMeshVerticesIndices();
    void UpdateMeshBuffers(VkCommandBuffer cmdBuffer);

    void PrepareModelTess(VkCommandBuffer cmdBuffer);

    void ShowWaterSurfaceSettings();
    void ShowLightingSettings();
    void ShowMeshSettings();

    void UpdateUniformBuffer(const uint32_t imageIndex);
    void UpdateDescriptorSets();
    void UpdateDescriptorSet(const uint32_t frameIndex);
    void CreateDescriptorSetLayout();
    void CreateUniformBuffers(const uint32_t kBufferCount);
    void SetupPipeline();
    void CreateDescriptorSets(const uint32_t kCount);

    std::vector<
        std::shared_ptr<vkp::ShaderModule>
    > CreateShadersFromShaderInfos(const vkp::ShaderInfo* kShaderInfos,
                                   const uint32_t kShaderInfoCount) const;
    void CreatePipeline(
        const VkExtent2D& framebufferExtent,
        VkRenderPass renderPass,
        bool framebufferHasDepthAttachment);

    void CreateTessendorfModel();

    void CreateMesh();
    std::vector<Vertex> CreateGridVertices(const uint32_t kTileSize,
                                           const float kScale);
    std::vector<uint32_t> CreateGridIndices(const uint32_t kTileSize);

    void CreateStagingBuffer();
    void CreateFrameMaps(VkCommandBuffer cmdBuffer);
    std::unique_ptr<vkp::Texture2D> CreateMap(
        VkCommandBuffer cmdBuffer,
        const uint32_t kSize,
        const VkFormat kMapFormat,
        const bool kUseMipMapping);

    struct FrameMapData
    {
        std::unique_ptr<vkp::Texture2D> displacementMap{ nullptr };
        std::unique_ptr<vkp::Texture2D> normalMap{ nullptr };
    };

    void UpdateFrameMaps(
        VkCommandBuffer cmdBuffer,
        FrameMapData& frame);
    void CopyModelTessDataToStagingBuffer();

    uint32_t GetTotalVertexCount(const uint32_t kTileSize) const {
        return (kTileSize+1) * (kTileSize+1);
    }

    uint32_t GetTotalIndexCount(const uint32_t kTotalVertexCount) const
    {
        const uint32_t kIndicesPerTriangle = 3, kTrianglesPerQuad = 2;
        return kTotalVertexCount * kIndicesPerTriangle * kTrianglesPerQuad;
    }

    static uint32_t GetMaxVertexCount() {
        return (s_kMaxTileSize+1) * (s_kMaxTileSize+1);
    }
    static uint32_t GetMaxIndexCount()
    {
        const uint32_t kIndicesPerTriangle = 3, kTrianglesPerQuad = 2;
        return GetMaxVertexCount() * kIndicesPerTriangle * kTrianglesPerQuad;
    }

private:

    // =========================================================================
    // Vulkan

    const vkp::Device& m_Device;

    // TODO static GetDescriptorPoolSizes
    const vkp::DescriptorPool& m_DescriptorPool;

    static const inline std::array<vkp::ShaderInfo, 2> s_kShaderInfos {
        vkp::ShaderInfo{
            .paths = { "shaders/WaterSurfaceMesh.vert" },
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .isSPV = false
        },
        vkp::ShaderInfo{
            .paths = { "shaders/WaterSurfaceMesh.frag" },
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .isSPV = false
        }
    };

    std::unique_ptr<vkp::DescriptorSetLayout> m_DescriptorSetLayout{ nullptr };
    std::vector<VkDescriptorSet>              m_DescriptorSets;

    // Uniform buffer for each swap chain image
    //   Cleanup after fininshed rendering or on swap chain recreation
    std::vector<vkp::Buffer> m_UniformBuffers;

    std::unique_ptr<vkp::Pipeline> m_Pipeline{ nullptr };

    // =========================================================================

    // Mesh properties
    std::unique_ptr< Mesh<Vertex> > m_Mesh{ nullptr };

    uint32_t m_TileSize  { WSTessendorf::s_kDefaultTileSize };
    float m_VertexDistance{ WSTessendorf::s_kDefaultTileLength /
                            static_cast<float>(WSTessendorf::s_kDefaultTileSize) };
    // Model properties
    std::unique_ptr<WSTessendorf> m_ModelTess{ nullptr };

    bool m_PlayAnimation{ true };
    float m_TimeCtr  { 0.0 };
    float m_AnimSpeed{ 3.0 };

    // -------------------------------------------------------------------------
    // Water Surface textures
    //  both displacementMap and normalMap are generated on the CPU, then 
    //  transferred to the GPU, per frame

    static constexpr VkFormat s_kMapFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
    static constexpr bool s_kUseMipMapping = false;

    std::unique_ptr<vkp::Buffer> m_StagingBuffer{ nullptr };

    struct FrameMapPair
    {
    #ifndef DOUBLE_BUFFERED
        std::array<FrameMapData, 1> data;
    #else
        std::array<FrameMapData, 2> data;
    #endif
    };

    // Map pair is preallocated for each size of the model's data
    std::vector<FrameMapPair> m_FrameMaps;
    // Bound pair for the current model's size
    FrameMapPair* m_CurFrameMap{ nullptr };

    uint32_t m_FrameMapIndex{ 0 };      ///< Swap index

    // -------------------------------------------------------------------------
    // Uniform buffers data

    struct VertexUBO
    {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
        float WSHeightAmp;
    };

    struct WaterSurfaceUBO
    {
        alignas(16) glm::vec3 camPos;
        alignas(16) glm::vec3 sunDir{ 0.0, 1.0, 0.4 };
        //                    vec4(vec3(suncolor), sunIntensity)
        alignas(16) glm::vec4 sunColor   { 7.0, 4.5, 3.0, 0.1 };
        float terrainDepth{ -200.0};
        float skyIntensity{ 1.0 };
        float specularIntensity{ 0.6 };
        float specularHighlights{ 32.0 };
    };

    VertexUBO m_VertexUBO{};
    WaterSurfaceUBO m_WaterSurfaceUBO{};

    // -------------------------------------------------------------------------
    // GUI stuff

    // Should correspond to s_kMaxTileSize and s_kMinTileSize range
    static constexpr gui::ValueStringArray<uint32_t, 7> s_kWSResolutions{
        { 16, 32, 64, 128, 256, 512, 1024 },
        { "16", "32", "64", "128", "256", "512", "1024" }
    };
};

struct WaterSurfaceMesh::Vertex
{
    glm::vec3 pos;
    glm::vec2 uv;

    Vertex(const glm::vec3& position, const glm::vec2& texCoord)
        : pos(position), uv(texCoord) {}

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
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex, pos)
            },
            {
                .location = 1,
                .binding = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(Vertex, uv)
            }
        };
    }

    static const inline std::vector<VkVertexInputBindingDescription>
        s_BindingDescriptions{ GetBindingDescription() };

    static const inline std::vector<VkVertexInputAttributeDescription>
        s_AttribDescriptions{ GetAttributeDescriptions() };
};


#endif // WATER_SURFACE_RENDERING_SCENE_WATER_SURFACE_MESH_H_
