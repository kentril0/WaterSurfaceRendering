/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#ifndef WATER_SURFACE_RENDERING_SCENE_WS_TESSENDORF_H_
#define WATER_SURFACE_RENDERING_SCENE_WS_TESSENDORF_H_

#include <complex>
#include <vector>
#include <random>
#include <memory>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <fftw3.h>


/**
 * @brief Generates data used for rendering the water surface
 *  - displacements
 *      horizontal displacements along with vertical displacement = height,
 *  - normals 
 *  Based on: Jerry Tessendorf. Simulating Ocean Water. 1999.
 * 
 * Assumptions:
 *  1) Size of a tile (patch) of the surface is a power of two:
 *      size = size_X = size_Z
 *  2) A tile is a uniform grid of points and waves, number of points
 *      on the grid is same as the number of waves, and it is
 *      a power of two. Commonly used sizes: 256 or 512
 */
class WSTessendorf
{
public:
    static constexpr uint32_t     s_kDefaultTileSize  { 256 };
    static constexpr float        s_kDefaultTileLength{ 32.0f };
    static constexpr float        s_kDefaultAnimPeriod{ 200.0f };
    static constexpr float        s_kDefaultPhillipsConst  { 0.00005f };
    static constexpr float        s_kDefaultPhillipsDamping{ 0.001f };
    static inline const glm::vec2 s_kDefaultWindDir{ 0.0f, 4.0f };

    // Both vec4 due to GPU memory alignment requirements
    using Displacement = glm::vec4;
    using Normal       = glm::vec4;

public:
    /**
     * @brief Sets up the surface's properties and prepares it for
     *  subsequent computation of waves using "ComputeWaves()" fnc.
     * @param tileSize Size of the tile, is the number of points and waves,
     *  must be power of two
     * @param tileLength Length of tile, or wave length
     * @param windDir Horizontal direction of wind, its magnitude is its speed
     * @param phillipsConst Numeric constant in Phillips spectrum
     * @param animationPeriod Duration of animation
     */
    WSTessendorf(
        uint32_t tileSize           = WSTessendorf::s_kDefaultTileSize,
        float tileLength            = WSTessendorf::s_kDefaultTileLength,
        const glm::vec2& windDir    = WSTessendorf::s_kDefaultWindDir,
        float animationPeriod       = WSTessendorf::s_kDefaultAnimPeriod,
        float phillipsConst         = WSTessendorf::s_kDefaultPhillipsConst);

    ~WSTessendorf();

    /**
     * @brief Computes the wave height, horizontal displacement,
     *  and normal for each vertex. "Prepare()" must be called once before.
     * @param time Elapsed time in seconds
     * @return Amplitude of normalized heights (in <-1, 1> )
     */
    float ComputeWaves(float time);

    /**
     * @brief Recreates needed structures to accomodate set properties,
     *  Use to see changes after setting properties.
     */
    void Prepare();

    // ---------------------------------------------------------------------
    // Getters

    auto GetTileSize() const { return m_TileSize; }
    auto GetTileLength() const { return m_TileLength; }
    auto GetWindDir() const { return m_WindDir; }
    auto GetAnimationPeriod() const { return m_AnimationPeriod; }
    auto GetPhillipsConst() const { return m_A; }
    auto GetDamping() const { return m_Damping; }
    auto GetDisplacementLambda() const { return m_Lambda; }

    // Data

    size_t GetDisplacementCount() const { return m_Displacements.size(); }
    const Displacement* GetDisplacementData() const {
        return m_Displacements.data();
    }
    const std::vector<Displacement>& GetDisplacements() const {
        return m_Displacements;
    }

    size_t GetNormalsCount() const { return m_Normals.size(); }
    const std::vector<Normal>& GetNormals() const { return m_Normals; }
    const Normal* GetNormalsData() const { return m_Normals.data(); }

    // ---------------------------------------------------------------------
    // Setters

    void SetTileSize(uint32_t size);
    void SetTileLength(float length);
    void SetWindDirection(const glm::vec2& w);
    void SetAnimationPeriod(float T);
    void SetPhillipsConst(float A);

    /** @param lambda Importance of displacement vector */
    void SetLambda(float lambda);

    /** @param Damping Suppresses wave lengths smaller that its value */
    void SetDamping(float damping);

private:

    void PrecomputeBaseWaveHeightField();
    void PrecomputeWaveVectors();

    /**
     * @brief Normalizes heights to interval <-1, 1>
     * @return Ampltiude, to reconstruct the orignal signal
     */
    float NormalizeHeights(float minHeight, float maxHeight);

    void SetupFFTW();
    void DestroyFFTW();

private:
    // ---------------------------------------------------------------------
    // Properties

    uint32_t m_TileSize  { 0 };
    float    m_TileLength{ 1.0f };

    glm::vec2 m_WindDir{ 1.0f };

    // Phillips spectrum
    float m_A      { 1.0f };
    float m_Damping{ 0.01f };

    float m_AnimationPeriod{ 1.0f };

    float m_Lambda{ -1.0f };  ///< Displacement vector importance

    // -------------------------------------------------------------------------
    // Data

    // vec4(Displacement_X, height, Displacement_Z, padding)
    std::vector<Displacement> m_Displacements;

    // vec4(normal_X, normal_Y, normal_Z, padding)
    std::vector<Normal> m_Normals;

    // =========================================================================
    // Computation

    using Complex = std::complex<float>;

    struct BaseWave
    {
        Complex h0_FT;      ///< FT amplitude of wave height
        Complex h0_FT_conj; ///< conjugate of "h0_FT"
    };

    glm::vec2 m_WindDirUnit{ 1.0f };
    float     m_WindDirLen { 1.0f };

    // Base wave height field generated from the spectrum
    std::vector<BaseWave> m_BaseWaves{};

    // Precomputed values of dispersion relation
    float m_BaseFreq{ 1.0f };
    std::vector<float> m_Dispersion;

    // Precomputed wave vectors
    std::vector<glm::vec2> m_WaveVectors;
    std::vector<glm::vec2> m_UnitWaveVectors;

    // ---------------------------------------------------------------------
    // FT computation using FFTW

    std::vector<Complex> m_h_FT{};          ///< FT wave height
    std::vector<Complex> m_h_FT_slopeX{};
    std::vector<Complex> m_h_FT_slopeZ{};
#ifdef CHOPPY_WAVES
    std::vector<Complex> m_h_FT_D{};  ///< Displacement vectors
#endif

    fftwf_plan m_PlanHeight{ nullptr };
    fftwf_plan m_PlanSlopeX{ nullptr };
    fftwf_plan m_PlanSlopeZ{ nullptr };
#ifdef CHOPPY_WAVES
    fftwf_plan m_PlanDispl{ nullptr };
#endif

private:
    static constexpr float s_kG{ 9.81 };   ///< Gravitational constant
    static constexpr float s_kOneOver2sqrt{ 1.0f / std::sqrt(2.0f) };

    /**
     * @brief Realization of water wave height field in fourier domain
     * @return Fourier amplitudes of a wave height field
     */
    Complex BaseWaveHeightFT(const glm::vec2 waveVec) const
    {
        return s_kOneOver2sqrt * Complex(glm::gaussRand(0.0f, 1.0f),
                                         glm::gaussRand(0.0f, 1.0f))
                               * glm::sqrt( PhillipsSpectrum(waveVec) );
    }

   /**
     * @brief Phillips spectrum - wave spectrum, 
     *  a model for wind-driven waves larger than capillary waves
     */
    float PhillipsSpectrum(const glm::vec2 waveVec) const
    {
        const float k  = glm::length(waveVec);
        if (k < 0.000001f)      // zero div
        {
            return 0.0f;
        }

        const float k2 = k * k;
        const float k4 = k2 * k2;

        float cosFact = glm::dot(glm::normalize(waveVec), m_WindDirUnit);
        cosFact = cosFact * cosFact;

        const float L = m_WindDirLen * m_WindDirLen / s_kG;
        const float L2 = L * L;

        const float l2 = L2 * m_Damping;

        return m_A * exp(-1.0f / (k2 * L2)) / k4
                   * cosFact
                   * exp(-k2 * l2);
    }

    Complex WaveHeightFT(float dispersion, float t, int index) const
    {
        const float omega_t = dispersion * t;

        // exp(ix) = cos(x) * i*sin(x)
        const float pcos = glm::cos(omega_t);
        const float psin = glm::sin(omega_t);

        const auto& baseWave = m_BaseWaves[index];
        return baseWave.h0_FT * Complex(pcos, psin) +
               baseWave.h0_FT_conj * Complex(pcos, -psin);
    }

    /** 
     * @brief Computes quantization of the dispersion surface
     *  - of the frequencies of dispersion relation
     * @param k Magnitude of wave vector
     */
    inline constexpr float QDispersion(float k) const
    {
        return glm::floor(DispersionDeepWaves(k) / m_BaseFreq) * m_BaseFreq;
    }

    /**
     * @brief Dispersion relation for deep water, where the bottom may be
     *  ignored
     * @param k Magnitude of wave vector
     */
    inline constexpr float DispersionDeepWaves(float k) const
    {
        return glm::sqrt(s_kG * k);
    }

    /**
     * @brief Dispersion relation for waves where the bottom is relatively
     *  shallow compared to the length of the waves
     * @param k Magnitude of wave vector
     * @param D Depth below the mean water level
     */
    inline constexpr float DispersionTransWaves(float k, float D) const
    {
        return glm::sqrt(s_kG * k * glm::tanh(k * D));
    }

    /**
     * @brief Dispersion relation for very small waves, i.e., with
     *  wavelength of about >1 cm
     * @param k Magnitude of wave vector
     * @param L wavelength
     */
    inline constexpr float DispersionSmallWaves(float k, float L) const
    {
        return glm::sqrt(s_kG * k * (1 + k*k * L*L));
    }

};


#endif // WATER_SURFACE_RENDERING_SCENE_WS_TESSENDORF_H_