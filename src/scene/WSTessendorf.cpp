/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#include "pch.h"
#include "scene/WSTessendorf.h"


WSTessendorf::WSTessendorf(uint32_t tileSize, float tileLength,
                           const glm::vec2& windDir, float animationPeriod,
                           float phillipsConst)
{
    VKP_REGISTER_FUNCTION();

    SetTileSize(tileSize);
    SetTileLength(tileLength);
    SetWindDirection(windDir);
    SetAnimationPeriod(animationPeriod);
    SetPhillipsConst(phillipsConst);

    Prepare();
}

WSTessendorf::~WSTessendorf()
{
    VKP_REGISTER_FUNCTION();

    DestroyFFTW();
}

void WSTessendorf::Prepare()
{
    // TODO timer
    VKP_REGISTER_FUNCTION();

    VKP_LOG_INFO("Water surface resolution: {} x {}", m_TileSize, m_TileSize);

    PrecomputeBaseWaveHeightField();
    PrecomputeWaveVectors();

    const uint32_t kTileSize1 = m_TileSize +1;

    const Displacement kDefaultDisplacement{ 0.0 };
    m_Displacements.resize(kTileSize1 * kTileSize1, kDefaultDisplacement);

    const Normal kDefaultNormal{ 0.0, 1.0, 0.0, 0.0 };
    m_Normals.resize(kTileSize1 * kTileSize1, kDefaultNormal);
    
    DestroyFFTW();
    SetupFFTW();
}

void WSTessendorf::PrecomputeBaseWaveHeightField()
{
    VKP_REGISTER_FUNCTION();

    const auto kTileSize = m_TileSize;
    const auto kTileSize1 = m_TileSize+1;
    const auto kTileLength = m_TileLength;

    m_BaseWaves.resize(kTileSize1 * kTileSize1);

    for (uint32_t m = 0; m < kTileSize1; ++m)
    {
        for (uint32_t n = 0; n < kTileSize1; ++n)
        {
            const glm::vec2 kWaveVec(
                M_PI * (2.0f * n - kTileSize) / kTileLength,
                M_PI * (2.0f * m - kTileSize) / kTileLength
            );

            auto& baseWave = m_BaseWaves[m * kTileSize1 + n];
            // h_0^~(k)
            baseWave.h0_FT = BaseWaveHeightFT(kWaveVec);
            // *h_0^~(-k)
            baseWave.h0_FT_conj = std::conj(BaseWaveHeightFT(-kWaveVec));
        }
    }
}

void WSTessendorf::PrecomputeWaveVectors()
{
    VKP_REGISTER_FUNCTION();

    const auto kTileSize = m_TileSize;
    const auto kTileSize1 = m_TileSize+1;
    const auto kTileLength = m_TileLength;

    m_WaveVectors.resize(kTileSize * kTileSize);
    m_UnitWaveVectors.resize(kTileSize * kTileSize, glm::vec2(0.0f));
    m_Dispersion.resize(kTileSize * kTileSize);

    for (uint32_t m = 0; m < kTileSize; ++m)
    {
        for (uint32_t n = 0; n < kTileSize; ++n)
        {
            const uint32_t kIndex = m * kTileSize + n;

            const glm::vec2 kWaveVec(
                M_PI * (2.0f * n - kTileSize) / kTileLength,
                M_PI * (2.0f * m - kTileSize) / kTileLength
            );

            m_WaveVectors[kIndex] = kWaveVec;

            const float kWaveVecLen = glm::length(kWaveVec);

            if (kWaveVecLen >= 0.00001f)
                m_UnitWaveVectors[kIndex] = kWaveVec / kWaveVecLen;

            m_Dispersion[kIndex] = QDispersion(kWaveVecLen);
        }
    }
}

void WSTessendorf::SetupFFTW()
{
    VKP_REGISTER_FUNCTION();

    m_h_FT        = (Complex*)fftwf_malloc(sizeof(fftwf_complex) * m_TileSize * m_TileSize);
    m_h_FT_slopeX = (Complex*)fftwf_malloc(sizeof(fftwf_complex) * m_TileSize * m_TileSize);
    m_h_FT_slopeZ = (Complex*)fftwf_malloc(sizeof(fftwf_complex) * m_TileSize * m_TileSize);

    m_PlanHeight = fftwf_plan_dft_2d(
        m_TileSize, m_TileSize, 
        reinterpret_cast<fftwf_complex*>(m_h_FT),
        reinterpret_cast<fftwf_complex*>(m_h_FT),
        FFTW_BACKWARD, FFTW_MEASURE);
    m_PlanSlopeX = fftwf_plan_dft_2d(
        m_TileSize, m_TileSize, 
        reinterpret_cast<fftwf_complex*>(m_h_FT_slopeX),
        reinterpret_cast<fftwf_complex*>(m_h_FT_slopeX),
        FFTW_BACKWARD, FFTW_MEASURE);
    m_PlanSlopeZ = fftwf_plan_dft_2d(
        m_TileSize, m_TileSize, 
        reinterpret_cast<fftwf_complex*>(m_h_FT_slopeZ),
        reinterpret_cast<fftwf_complex*>(m_h_FT_slopeZ),
        FFTW_BACKWARD, FFTW_MEASURE);
#ifdef CHOPPY_WAVES
    m_PlanD = fftwf_plan_dft_2d(
        m_TileSize, m_TileSize, 
        reinterpret_cast<fftwf_complex*>(m_h_FT_D),
        reinterpret_cast<fftwf_complex*>(m_h_FT_D),
        FFTW_BACKWARD, FFTW_MEASURE);
#endif
}

void WSTessendorf::DestroyFFTW()
{
    VKP_REGISTER_FUNCTION();

    if (m_PlanHeight == nullptr)
        return;

    fftwf_destroy_plan(m_PlanHeight);
    m_PlanHeight = nullptr;
    fftwf_destroy_plan(m_PlanSlopeX);
    fftwf_destroy_plan(m_PlanSlopeZ);
#ifdef CHOPPY_WAVES
    fftwf_destroy_plan(m_PlanD);
#endif

    fftwf_free((fftwf_complex*)m_h_FT);
    fftwf_free((fftwf_complex*)m_h_FT_slopeX);
    fftwf_free((fftwf_complex*)m_h_FT_slopeZ);
}

float WSTessendorf::ComputeWaves(float t)
{
    vkp::Timer timer;

    const auto kTileSize = m_TileSize;
    const auto kTileSize1 = m_TileSize+1;

    for (uint32_t m = 0; m < kTileSize; ++m)
        for (uint32_t n = 0; n < kTileSize; ++n)
        {
            const uint32_t kIndex = m * kTileSize + n;

            m_h_FT[kIndex] = WaveHeightFT(m_Dispersion[kIndex], t,
                                          m * kTileSize1 + n);
        }

    {
        // Slopes for normals computation
        for (uint32_t m = 0; m < kTileSize; ++m)
            for (uint32_t n = 0; n < kTileSize; ++n)
            {
                const uint32_t kIndex = m * kTileSize + n;
                const glm::vec2& kWaveVec = m_WaveVectors[kIndex];

                m_h_FT_slopeX[kIndex] = Complex(0, kWaveVec.x) * m_h_FT[kIndex];
                m_h_FT_slopeZ[kIndex] = Complex(0, kWaveVec.y) * m_h_FT[kIndex];
            }

    #ifdef CHOPPY_WAVES
        // Displacement vectors
        for (uint32_t m = 0; m < kTileSize; ++m)
            for (uint32_t n = 0; n < kTileSize; ++n)
            {
                const uint32_t kIndex = m * kTileSize + n;
                const glm::vec2& kUWaveVec = m_UnitWaveVectors[kIndex];

                m_h_FT_D[kIndex] = Complex(-kUWaveVec.x * m_h_FT[kIndex].imag(),
                                           -kUWaveVec.y * m_h_FT[kIndex].imag());
            }
    #endif
    }

    fftwf_execute(m_PlanHeight);
    fftwf_execute(m_PlanSlopeX);
    fftwf_execute(m_PlanSlopeZ);
#ifdef CHOPPY_WAVES
    fftwf_execute(m_PlanD);
#endif

    float maxHeight = std::numeric_limits<float>::min();
    float minHeight = std::numeric_limits<float>::max();

    // Conversion of the grid back to interval
    //  [-m_TileSize/2, ..., 0, ..., m_TileSize/2]
    const float kSigns[] = { 1.0f, -1.0f };

    for (uint32_t m = 0; m < kTileSize; ++m)
    {
        for (uint32_t n = 0; n < kTileSize; ++n)
        {
            const uint32_t kIndex  = m * kTileSize + n;
            const uint32_t kIndex1 = m * kTileSize1 + n;
            const int sign = kSigns[(n + m) & 1];

            const auto& h_FT = m_h_FT[kIndex] * static_cast<float>(sign);
            maxHeight = h_FT.real() > maxHeight ? h_FT.real() : maxHeight;
            minHeight = h_FT.real() < minHeight ? h_FT.real() : minHeight;

        #ifdef CHOPPY_WAVES
            // Horizontal displacement, height
            auto D_x = Complex(m_h_FT_D[kIndex].real(), 0.0);
            D_x *= static_cast<float>(sign);

            auto D_z = Complex(m_h_FT_D[kIndex].imag(), 0.0);
            D_z *= static_cast<float>(sign);

            m_Displacements[kIndex1] = glm::vec4(m_Lambda * D_x.real(),
                                                 h_FT.real(),
                                                 m_Lambda * D_z.real(),
                                                 0.0);
        #else
            m_Displacements[kIndex1].y = h_FT.real();
        #endif
        }
    }

    for (uint32_t m = 0; m < kTileSize; ++m)
    {
        for (uint32_t n = 0; n < kTileSize; ++n)
        {
            const uint32_t kIndex  = m * kTileSize + n;
            const uint32_t kIndex1 = m * kTileSize1 + n;
            const int sign = kSigns[(n + m) & 1];

            // Estimate normal from slope
            m_Normals[kIndex1] = glm::vec4(
                glm::normalize(
                    glm::vec3(
                        -(m_h_FT_slopeX[kIndex] * static_cast<float>(sign)).real(),
                        1.0f,
                        -(m_h_FT_slopeZ[kIndex] * static_cast<float>(sign)).real()
                    )
                ),
                0.0f
            );
        }
    }

    // Seamless tiling
    {
        const uint32_t kIndex1 = 0 * kTileSize1 + 0;

        m_Displacements[kIndex1 + kTileSize + kTileSize1 * kTileSize] = m_Displacements[kIndex1];
        m_Normals[kIndex1 + kTileSize + kTileSize1 * kTileSize] = m_Normals[kIndex1];

        m_Displacements[kIndex1 + kTileSize] = m_Displacements[kIndex1];
        m_Normals[kIndex1 + kTileSize] = m_Normals[kIndex1];
    }

    {
        for (uint32_t n = 1; n < kTileSize; ++n)
        {
            const uint32_t kIndex1 = 0 * kTileSize1 + n;

            m_Displacements[kIndex1 + kTileSize1 * kTileSize] = m_Displacements[kIndex1];
            m_Normals[kIndex1 + kTileSize1 * kTileSize] = m_Normals[kIndex1];
        }

        for (uint32_t m = 1; m < kTileSize; ++m)
        {
            const uint32_t kIndex1 = m * kTileSize1 + 0;
            m_Displacements[kIndex1 + kTileSize] = m_Displacements[kIndex1];
            m_Normals[kIndex1 + kTileSize] = m_Normals[kIndex1];
        }
    }

    VKP_LOG_INFO("Compute waves took: {} ms", timer.ElapsedMillis());
    return NormalizeHeights(minHeight, maxHeight);
}

float WSTessendorf::NormalizeHeights(float minHeight, float maxHeight)
{
    const float A = glm::max( glm::abs(minHeight), glm::abs(maxHeight) );
    const float OneOverA = 1.f / A;

    for (size_t i = 0; i < m_Displacements.size(); ++i)
    {
        m_Displacements[i].y *= OneOverA;
    }

    return A;
}

// =============================================================================

void WSTessendorf::SetTileSize(uint32_t size)
{
    const bool kSizeIsPowerOfTwo = ( size & (size-1) ) == 0;
    VKP_ASSERT_MSG(size > 0 && kSizeIsPowerOfTwo,
                   "Tile size must be power of two");

    m_TileSize = size;
}

void WSTessendorf::SetTileLength(float length)
{
    VKP_ASSERT(length > 0.0);
    m_TileLength = length;
}

void WSTessendorf::SetWindDirection(const glm::vec2& w)
{
    m_WindDir = w;
    m_WindDirUnit = glm::normalize(m_WindDir);
    m_WindDirLen = glm::length(m_WindDir);
}

void WSTessendorf::SetAnimationPeriod(float T)
{
    m_AnimationPeriod = T;
    m_BaseFreq = 2.0f * M_PI / m_AnimationPeriod;
}

void WSTessendorf::SetPhillipsConst(float A)
{
    m_A = A;
}

void WSTessendorf::SetLambda(float lambda)
{
    m_Lambda = lambda;
}

void WSTessendorf::SetDamping(float damping)
{
    m_Damping = damping;
}

