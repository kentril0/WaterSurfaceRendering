/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#ifndef WATER_SURFACE_RENDERING_WATER_SURFACE_H_
#define WATER_SURFACE_RENDERING_WATER_SURFACE_H_

#include "core/Application.h"

#include "vulkan/RenderPass.h"
#include "vulkan/Descriptors.h"
#include "vulkan/ShaderModule.h"
#include "vulkan/Pipeline.h"
#include "vulkan/Texture2D.h"

#include "scene/WSTessendorf.h"
#include "scene/WaterSurfaceMesh.h"
#include "scene/Camera.h"


class WaterSurface : public vkp::Application
{
public:
    WaterSurface(AppCmdLineArgs args);
    ~WaterSurface();

protected:
    /** @brief Called once in "Run" function, before the main loop */
    void Start() override;
    
    /** @brief Called each frame, before rendering */
    void Update(vkp::Timestep deltaTime) override;

    /** @brief Render call, called each frame */
    void Render(
        uint32_t frameIndex,
        vkp::Timestep dt,
        std::vector<VkPipelineStageFlags>& stagesToWait,
        std::vector<VkCommandBuffer>& buffersToSubmit) override;

    void OnFrameBufferResize(int width, int height) override;
    void OnMouseMove(double xpos, double ypos) override;
    void OnMousePressed(int button, int action, int mods) override;
    void OnKeyPressed(int key, int action, int mods) override;
    void OnCursorEntered(int entered) override;

private:
    void CreateRenderPass();
    void BeginRenderPass(VkCommandBuffer cmdBuffer,
                         VkFramebuffer framebuffer);

    void CreateDrawCommandPools(const uint32_t count);
    void CreateDrawCommandBuffers();
    void DestroyDrawCommandPools();

    void CreateUniformBuffers(const uint32_t kBufferCount);
    void UpdateUniformBuffer(const uint32_t imageIndex);

    void CreateDescriptorPool();

    void SetupWaterSurfaceMeshRendering();
        void CreateWSMeshDescriptorSetLayout();
        void SetupWSMeshPipeline();
        void CreateWSMeshPipeline();
        void CreateWSMeshDescriptorSets(const uint32_t kCount);
        // Once, and later only when dirty
        void UpdateWSMeshDescriptorSets();
        void UpdateWSMeshDescriptorSet(const uint32_t frameIndex);

    void SetupAssets();
        void SetupGUI();
        void CreateCamera();
        void CreateWaterSurfaces();

    void UpdateCamera(vkp::Timestep dt);
    void UpdateWaterSurfaceMesh();
    void RenderWaterSurfaceMesh(VkCommandBuffer cmdBuffer,
                                const uint32_t frameIndex);
    void CopyToWaterSurfaceMaps(VkCommandBuffer commandBuffer);

    // On properties change:

    void PrepareWaterSurfaceMesh(uint32_t size, float scale);
    void PrepareWaterSurfaceTextures(uint32_t size,
                                     const VkFormat kMapFormat);

    // GUI:

    void UpdateGui();
    void ShowStatusWindow() const;
    void ShowCameraSettings();
    void ShowWaterSurfaceSettings();

private:
    std::unique_ptr<vkp::RenderPass> m_RenderPass{ nullptr };  // TODO maybe into app

    // TODO maybe into App
    std::array<VkClearValue, 2> m_ClearValues{
        VkClearValue{ 0.118f, 0.235f, 0.314f, 1.0f }, // clear color
        VkClearValue{ 1.0f, 0.0f, 0.0f, 0.0f }  // clear depth, stencil
    };

    // TODO maybe into app
    std::vector<vkp::CommandPool> m_DrawCmdPools;

    // -------------------------------------------------------------------------
    // Resource Descriptors

    std::unique_ptr<vkp::DescriptorPool> m_DescriptorPool{ nullptr };

    // Uniform buffer for each swap chain image
    //   Cleanup after fininshed rendering or on swap chain recreation
    std::vector<vkp::Buffer> m_UniformBuffers;

    // =========================================================================

    /// @brief Application states
    enum class States
    {
        GuiControls = 0,    ///< Gui is shown and captures mouse controls
        CameraControls,     ///< Gui is hidden and camera captures mouse controls

        TotalStates
    };

    States m_State{ States::GuiControls };  ///< Current application state

    enum Controls
    {
        KeyHideGui = GLFW_KEY_ESCAPE,
    };

    // Assets

    std::unique_ptr<vkp::Camera> m_Camera{ nullptr };
    static constexpr glm::vec3 s_kCamStartPos{ 0.f, 12.f, -14.f };

    // -------------------------------------------------------------------------
    // Water Surfaces

    struct VertexUBO
    {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

    struct WaterSurfaceUBO
    {
        /*
        float time;             ///< Elapsed time in seconds since the start
        glm::vec2 resolution;   ///< Viewport resolution in pixels, w x h
        alignas(16) glm::vec3 camPosition{ 0.0, 500.0, 0.0};
        alignas(16) glm::vec3 viewMatRow0;
        alignas(16) glm::vec3 viewMatRow1;
        alignas(16) glm::vec3 viewMatRow2;
        float camFOV          { 1.5 };   // Radians
        float camNear         { 0.1 };
        float camFar          { 2000. };
        alignas(16) glm::vec3 sunDir{ 0.0, 1.0, 0.4 };
        // vec4(vec3(suncolor), sunIntensity)
        alignas(16) glm::vec4 sunColor   { 7.0, 4.5, 3.0, 0.1 };
        alignas(16) glm::vec3 skyColor        { 0.4, 0.75, 1.0 };
        alignas(16) glm::vec3 skyHorizontColor{ 0.7, 0.75, 0.8 };
        alignas(16) glm::vec3 fillLight  { 0.5, 0.8, 0.9 };
        alignas(16) glm::vec3 bounceLight{ 0.7, 0.3, 0.2 };
        */
        float heightAmp{ 1.0 };  ///< Water surface height amplitude
    };

    VertexUBO m_VertexUBO{};
    WaterSurfaceUBO m_WaterSurfaceUBO{};

    std::unique_ptr<vkp::DescriptorSetLayout> m_WSMeshDescriptorSetLayout{ nullptr };
    std::vector<VkDescriptorSet> m_WSMeshDescriptorSets;

    std::unique_ptr<WSTessendorf> m_WSTess{ nullptr };
    std::unique_ptr<WaterSurfaceMesh> m_WSMesh{ nullptr };

    // -------------------------------------------------------------------------
    // Water Surface textures
    //  both heightmap and normalmap are generated on the CPU, then 
    //  transferred to the GPU, per frame

    std::unique_ptr<vkp::Buffer> m_StagingBuffer{ nullptr };

    std::unique_ptr<vkp::Texture2D> m_DisplacementMap{ nullptr };
    std::unique_ptr<vkp::Texture2D> m_NormalMap{ nullptr };

    // -------------------------------------------------------------------------
    // Rendering water surface as a mesh

    static const inline std::array<vkp::ShaderInfo, 2> s_kWSMeshShaderInfos {
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

    std::unique_ptr<vkp::Pipeline> m_WSMeshPipeline{ nullptr };

    // -------------------------------------------------------------------------
    // GUI stuff

    template<typename T, size_t S>
    struct ValueStringArray
    {
        const T types[S];
        const char* strings[S];
        constexpr T operator[](int i) const { return types[i]; }
        constexpr operator auto() const { return strings; }
        constexpr uint32_t size() const { return S; }
    };

    // Should correspond to WaterSurfaceMesh::s_kMaxSize and s_kMinSize range
    static constexpr ValueStringArray<uint32_t, 9> s_kWSResolutions{
        { 4, 8, 16, 32, 64, 128, 256, 512, 1024 },
        { "4", "8", "16", "32", "64", "128", "256", "512", "1024" }
    };

};

#endif // WATER_SURFACE_RENDERING_WATER_SURFACE_H_
