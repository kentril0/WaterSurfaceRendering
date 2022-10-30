/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#include "pch.h"
#include "scene/WSTessendorf.h"


WSTessendorf::WSTessendorf(uint32_t tileSize, float tileLength)
{
    VKP_REGISTER_FUNCTION();

    SetTileSize(tileSize);
    SetTileLength(tileLength);

    SetWindDirection(s_kDefaultWindDir);
    SetWindSpeed(s_kDefaultWindSpeed);
    SetAnimationPeriod(s_kDefaultAnimPeriod);

    SetPhillipsConst(s_kDefaultPhillipsConst);
    SetDamping(s_kDefaultPhillipsDamping);
}

WSTessendorf::~WSTessendorf()
{
    VKP_REGISTER_FUNCTION();
    DestroyFFTW();
}

void WSTessendorf::Prepare()
{
    vkp::Timer timer;
    VKP_REGISTER_FUNCTION();

    VKP_LOG_INFO("Water surface resolution: {} x {}", m_TileSize, m_TileSize);

    m_WaveVectors = ComputeWaveVectors();
    m_BaseWaveHeights = ComputeBaseWaveHeightField();

    const uint32_t kSize = m_TileSize;

    const Displacement kDefaultDisplacement{ 0.0 };
    m_Displacements.resize(kSize * kSize, kDefaultDisplacement);

    const Normal kDefaultNormal{ 0.0, 1.0, 0.0, 0.0 };
    m_Normals.resize(kSize * kSize, kDefaultNormal);

    DestroyFFTW();
    SetupFFTW();

    VKP_LOG_INFO("PREPARED in: {:f} ms", timer.ElapsedMicro() * 0.001);
}

std::vector<WSTessendorf::WaveVector> WSTessendorf::ComputeWaveVectors() const
{
    VKP_REGISTER_FUNCTION();

    const int32_t kSize = m_TileSize;
    const float kLength = m_TileLength;

    std::vector<WaveVector> waveVecs;
    waveVecs.reserve(kSize * kSize);
    
    for (int32_t m = 0; m < kSize; ++m)
    {
        for (int32_t n = 0; n < kSize; ++n)
        {
            waveVecs.emplace_back(
                glm::vec2(
                    M_PI * (2.0f * n - kSize) / kLength,
                    M_PI * (2.0f * m - kSize) / kLength
                )
            );
        }
    }

    return waveVecs;
}

std::vector<WSTessendorf::BaseWaveHeight>
    WSTessendorf::ComputeBaseWaveHeightField() const
{
    VKP_REGISTER_FUNCTION();

    const int32_t kSize = m_TileSize;
    const float kLength = m_TileLength;

    std::vector<WSTessendorf::BaseWaveHeight> baseWaveHeights(kSize * kSize);
    VKP_ASSERT(m_WaveVectors.size() == baseWaveHeights.size());

    for (int32_t m = 0; m < kSize; ++m)
    {
        for (int32_t n = 0; n < kSize; ++n)
        {
            auto& h0 = baseWaveHeights[m * kSize + n];

            const auto& kWaveVec = m_WaveVectors[m * kSize + n].vec;
            h0.heightAmp = BaseWaveHeightFT(kWaveVec);
            h0.heightAmp_conj = BaseWaveHeightFT( - kWaveVec);
            h0.dispersion = QDispersion( glm::length(kWaveVec) );
        }
    }

    return baseWaveHeights;
}

void WSTessendorf::SetupFFTW()
{
    VKP_REGISTER_FUNCTION();

    const uint32_t kSize = m_TileSize;

    m_h_FT        = (Complex*)fftwf_malloc(sizeof(fftwf_complex) * kSize * kSize);
    m_h_FT_slopeX = (Complex*)fftwf_malloc(sizeof(fftwf_complex) * kSize * kSize);
    m_h_FT_slopeZ = (Complex*)fftwf_malloc(sizeof(fftwf_complex) * kSize * kSize);

    m_PlanHeight = fftwf_plan_dft_2d(
        kSize, kSize, 
        reinterpret_cast<fftwf_complex*>(m_h_FT),
        reinterpret_cast<fftwf_complex*>(m_h_FT),
        FFTW_BACKWARD, FFTW_MEASURE);
    m_PlanSlopeX = fftwf_plan_dft_2d(
        kSize, kSize, 
        reinterpret_cast<fftwf_complex*>(m_h_FT_slopeX),
        reinterpret_cast<fftwf_complex*>(m_h_FT_slopeX),
        FFTW_BACKWARD, FFTW_MEASURE);
    m_PlanSlopeZ = fftwf_plan_dft_2d(
        kSize, kSize, 
        reinterpret_cast<fftwf_complex*>(m_h_FT_slopeZ),
        reinterpret_cast<fftwf_complex*>(m_h_FT_slopeZ),
        FFTW_BACKWARD, FFTW_MEASURE);
#ifdef CHOPPY_WAVES
    m_PlanD = fftwf_plan_dft_2d(
        kSize, kSize, 
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
            m_h_FT[m * kTileSize + n] = WaveHeightFT(m_BaseWaveHeights[m * kTileSize + n], t);
        }

    {
        // Slopes for normals computation
        for (uint32_t m = 0; m < kTileSize; ++m)
            for (uint32_t n = 0; n < kTileSize; ++n)
            {
                const uint32_t kIndex = m * kTileSize + n;
                
                const auto& kWaveVec = m_WaveVectors[kIndex].vec;
                m_h_FT_slopeX[kIndex] = Complex(0, kWaveVec.x) * m_h_FT[kIndex];
                m_h_FT_slopeZ[kIndex] = Complex(0, kWaveVec.y) * m_h_FT[kIndex];
            }

    #ifdef CHOPPY_WAVES
        // Displacement vectors
        for (uint32_t m = 0; m < kTileSize; ++m)
        {
            for (uint32_t n = 0; n < kTileSize; ++n)
            {
                const uint32_t kIndex = m * kTileSize + n;
                const auto& kUnitWaveVec = m_WaveVectors[kIndex].unit;

                m_h_FT_D[kIndex] = Complex(-kUnitWaveVec.x * m_h_FT[kIndex].imag(),
                                           -kUnitWaveVec.y * m_h_FT[kIndex].imag());
            }
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
            const uint32_t kIndex = m * kTileSize + n;
            const int sign = kSigns[(n + m) & 1];

            const auto h_FT = m_h_FT[kIndex].real() * static_cast<float>(sign);
            maxHeight = glm::max(h_FT, maxHeight);
            minHeight = glm::min(h_FT, minHeight);
            m_Displacements[kIndex].y = h_FT;

            m_Normals[kIndex] = glm::vec4(glm::normalize(glm::vec3(
                -( sign * m_h_FT_slopeX[kIndex].real() ),
                1.0f,
                -( sign * m_h_FT_slopeZ[kIndex].real() )
            )
            ),
            0.0f);
        }
    }

    VKP_LOG_INFO("COMPUTE_WAVES: post: {:f} ms", timer.ElapsedMicro() * 0.001);
    return NormalizeHeights(minHeight, maxHeight);
}

float WSTessendorf::NormalizeHeights(float minHeight, float maxHeight)
{
    vkp::Timer timer;
    const float A = glm::max( glm::abs(minHeight), glm::abs(maxHeight) );
    const float OneOverA = 1.f / A;

    std::for_each(m_Displacements.begin(), m_Displacements.end(),
                  [OneOverA](auto& d){ d.y *= OneOverA; });

    return A;
}

// =============================================================================

void WSTessendorf::SetTileSize(uint32_t size)
{
    const bool kSizeIsPowerOfTwo = ( size & (size-1) ) == 0;
    VKP_ASSERT_MSG(size > 0 && kSizeIsPowerOfTwo,
                   "Tile size must be power of two");
    if (!kSizeIsPowerOfTwo)
        return;

    m_TileSize = size;
}

void WSTessendorf::SetTileLength(float length)
{
    VKP_ASSERT(length > 0.0);
    m_TileLength = length;
}

void WSTessendorf::SetWindDirection(const glm::vec2& w)
{
    m_WindDir = glm::normalize(w);
}

void WSTessendorf::SetWindSpeed(float v)
{
    m_WindSpeed = glm::max(0.0001f, v);
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

