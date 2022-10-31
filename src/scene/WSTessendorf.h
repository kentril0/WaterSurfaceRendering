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
    static constexpr float        s_kDefaultTileLength{ 1000.0f };

    static inline const glm::vec2 s_kDefaultWindDir{ 1.0f, 1.0f };
    static constexpr float        s_kDefaultWindSpeed{ 30.0f };
    static constexpr float        s_kDefaultAnimPeriod{ 200.0f };
    static constexpr float        s_kDefaultPhillipsConst{ 3e-7f };
    static constexpr float        s_kDefaultPhillipsDamping{ 0.1f };

    // Both vec4 due to GPU memory alignment requirements
    using Displacement = glm::vec4;
    using Normal       = glm::vec4;

public:
    /**
     * @brief Sets up the surface's properties.
     *  1. Call "Prepare()" to setup necessary structures for computation of waves
     *  2. Call "ComputeWaves()" fnc.
     * @param tileSize Size of the tile, is the number of points and waves,
     *  must be power of two
     * @param tileLength Length of tile, or wave length
     */
    WSTessendorf(uint32_t tileSize = WSTessendorf::s_kDefaultTileSize,
                 float tileLength  = WSTessendorf::s_kDefaultTileLength);
    ~WSTessendorf();

    /**
     * @brief (Re)Creates necessary structures according to the previously set
     *  properties:
     *  a) Computes wave vectors
     *  b) Computes base wave height field amplitudes
     *  c) Sets up FFTW memory and plans
     */
    void Prepare();

    /**
     * @brief Computes the wave height, horizontal displacement,
     *  and normal for each vertex. "Prepare()" must be called once before.
     * @param time Elapsed time in seconds
     * @return Amplitude of normalized heights (in <-1, 1> )
     */
    float ComputeWaves(float time);

    // ---------------------------------------------------------------------
    // Getters

    auto GetTileSize() const { return m_TileSize; }
    auto GetTileLength() const { return m_TileLength; }
    auto GetWindDir() const { return m_WindDir; }
    auto GetWindSpeed() const { return m_WindSpeed; }
    auto GetAnimationPeriod() const { return m_AnimationPeriod; }
    auto GetPhillipsConst() const { return m_A; }
    auto GetDamping() const { return m_Damping; }
    auto GetDisplacementLambda() const { return m_Lambda; }

    // Data

    size_t GetDisplacementCount() const { return m_Displacements.size(); }
    const std::vector<Displacement>& GetDisplacements() const {
        return m_Displacements;
    }

    size_t GetNormalCount() const { return m_Normals.size(); }
    const std::vector<Normal>& GetNormals() const { return m_Normals; }

    // ---------------------------------------------------------------------
    // Setters

    void SetTileSize(uint32_t size);
    void SetTileLength(float length);

    /** @param w Unit vector - direction of wind blowing */
    void SetWindDirection(const glm::vec2& w);

    /** @param v Positive floating point number - speed of the wind in its direction */
    void SetWindSpeed(float v);

    void SetAnimationPeriod(float T);
    void SetPhillipsConst(float A);

    /** @param lambda Importance of displacement vector */
    void SetLambda(float lambda);

    /** @param Damping Suppresses wave lengths smaller that its value */
    void SetDamping(float damping);

private:

    using Complex = std::complex<float>;

    struct WaveVector
    {
        glm::vec2 vec;
        glm::vec2 unit;
        WaveVector(const glm::vec2& v)
            : vec(v)
        {
            const float k = glm::length(v);
            unit = k < 0.00001f ? glm::vec2(0) : glm::normalize(v);
        }
    };

    struct BaseWaveHeight
    {
        Complex heightAmp;          ///< FT amplitude of wave height
        Complex heightAmp_conj;     ///< conjugate of wave height amplitude
        float dispersion;           ///< Descrete dispersion value
    };

    std::vector<WaveVector> ComputeWaveVectors() const;
    std::vector<BaseWaveHeight> ComputeBaseWaveHeightField() const;

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

    uint32_t m_TileSize;
    float    m_TileLength;

    glm::vec2 m_WindDir;        ///< Unit vector
    float m_WindSpeed;

    // Phillips spectrum
    float m_A;
    float m_Damping;

    float m_AnimationPeriod;
    float m_BaseFreq{ 1.0f };

    float m_Lambda{ -1.0f };  ///< Importance of displacement vector

    // -------------------------------------------------------------------------
    // Data

    // vec4(Displacement_X, height, Displacement_Z, padding)
    std::vector<Displacement> m_Displacements;
    // vec4(normal_X, normal_Y, normal_Z, padding)
    std::vector<Normal> m_Normals;

    // =========================================================================
    // Computation

    std::vector<WaveVector> m_WaveVectors;   ///< Precomputed Wave vectors

    // Base wave height field generated from the spectrum for each wave vector
    std::vector<BaseWaveHeight> m_BaseWaveHeights;

    // ---------------------------------------------------------------------
    // FT computation using FFTW

    Complex* m_Height{ nullptr };
    Complex* m_SlopeX{ nullptr };
    Complex* m_SlopeZ{ nullptr };
    Complex* m_DisplacementX{ nullptr };
    Complex* m_DisplacementZ{ nullptr };

    fftwf_plan m_PlanHeight{ nullptr };
    fftwf_plan m_PlanSlopeX{ nullptr };
    fftwf_plan m_PlanSlopeZ{ nullptr };
    fftwf_plan m_PlanDisplacementX{ nullptr };
    fftwf_plan m_PlanDisplacementZ{ nullptr };

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
            return 0.0f;

        const float k2 = k * k;
        const float k4 = k2 * k2;

        float cosFact = glm::dot(glm::normalize(waveVec), m_WindDir);
        cosFact = cosFact * cosFact;

        const float L = m_WindSpeed * m_WindSpeed / s_kG;
        const float L2 = L * L;

        return m_A * glm::exp(-1.0f / (k2 * L2)) / k4
                   * cosFact
                   * glm::exp(-k2 * m_Damping * m_Damping);
    }

    static Complex WaveHeightFT(const BaseWaveHeight& waveHeight, const float t)
    {
        const float omega_t = waveHeight.dispersion * t;

        // exp(ix) = cos(x) * i*sin(x)
        const float pcos = glm::cos(omega_t);
        const float psin = glm::sin(omega_t);

        return waveHeight.heightAmp * Complex(pcos, psin) +
               waveHeight.heightAmp_conj * Complex(pcos, -psin);
    }

    // --------------------------------------------------------------------

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
