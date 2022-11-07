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
            h0.heightAmp_conj = std::conj(BaseWaveHeightFT(-kWaveVec));
            h0.dispersion = QDispersion( glm::length(kWaveVec) );
        }
    }

    return baseWaveHeights;
}

void WSTessendorf::SetupFFTW()
{
    VKP_REGISTER_FUNCTION();

    const uint32_t kSize = m_TileSize;

    m_Height = (Complex*)fftwf_malloc(sizeof(fftwf_complex) * kSize * kSize);
    m_SlopeX = (Complex*)fftwf_malloc(sizeof(fftwf_complex) * kSize * kSize);
    m_SlopeZ = (Complex*)fftwf_malloc(sizeof(fftwf_complex) * kSize * kSize);
    m_DisplacementX = (Complex*)fftwf_malloc(sizeof(fftwf_complex) * kSize * kSize);
    m_DisplacementZ = (Complex*)fftwf_malloc(sizeof(fftwf_complex) * kSize * kSize);

    m_PlanHeight = fftwf_plan_dft_2d(
        kSize, kSize, 
        reinterpret_cast<fftwf_complex*>(m_Height),
        reinterpret_cast<fftwf_complex*>(m_Height),
        FFTW_BACKWARD, FFTW_MEASURE);
    m_PlanSlopeX = fftwf_plan_dft_2d(
        kSize, kSize, 
        reinterpret_cast<fftwf_complex*>(m_SlopeX),
        reinterpret_cast<fftwf_complex*>(m_SlopeX),
        FFTW_BACKWARD, FFTW_MEASURE);
    m_PlanSlopeZ = fftwf_plan_dft_2d(
        kSize, kSize, 
        reinterpret_cast<fftwf_complex*>(m_SlopeZ),
        reinterpret_cast<fftwf_complex*>(m_SlopeZ),
        FFTW_BACKWARD, FFTW_MEASURE);
    m_PlanDisplacementX = fftwf_plan_dft_2d(
        kSize, kSize, 
        reinterpret_cast<fftwf_complex*>(m_DisplacementX),
        reinterpret_cast<fftwf_complex*>(m_DisplacementX),
        FFTW_BACKWARD, FFTW_MEASURE);
    m_PlanDisplacementZ = fftwf_plan_dft_2d(
        kSize, kSize, 
        reinterpret_cast<fftwf_complex*>(m_DisplacementZ),
        reinterpret_cast<fftwf_complex*>(m_DisplacementZ),
        FFTW_BACKWARD, FFTW_MEASURE);
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
    fftwf_destroy_plan(m_PlanDisplacementX);
    fftwf_destroy_plan(m_PlanDisplacementZ);

    fftwf_free((fftwf_complex*)m_Height);
    fftwf_free((fftwf_complex*)m_SlopeX);
    fftwf_free((fftwf_complex*)m_SlopeZ);
    fftwf_free((fftwf_complex*)m_DisplacementX);
    fftwf_free((fftwf_complex*)m_DisplacementZ);
}

float WSTessendorf::ComputeWaves(float t)
{
    const auto kTileSize = m_TileSize;
    const auto kTileSize1 = m_TileSize+1;

    for (uint32_t m = 0; m < kTileSize; ++m)
        for (uint32_t n = 0; n < kTileSize; ++n)
        {
            m_Height[m * kTileSize + n] =
                WaveHeightFT(m_BaseWaveHeights[m * kTileSize + n], t);
        }

    {
        // Slopes for normals computation
        for (uint32_t m = 0; m < kTileSize; ++m)
            for (uint32_t n = 0; n < kTileSize; ++n)
            {
                const uint32_t kIndex = m * kTileSize + n;
                
                const auto& kWaveVec = m_WaveVectors[kIndex].vec;
                m_SlopeX[kIndex] = Complex(0, kWaveVec.x) * m_Height[kIndex];
                m_SlopeZ[kIndex] = Complex(0, kWaveVec.y) * m_Height[kIndex];
            }

        // Displacement vectors
        for (uint32_t m = 0; m < kTileSize; ++m)
            for (uint32_t n = 0; n < kTileSize; ++n)
            {
                const uint32_t kIndex = m * kTileSize + n;
                
                const auto& kUnitWaveVec = m_WaveVectors[kIndex].unit;
                m_DisplacementX[kIndex] = Complex(0, -kUnitWaveVec.x) * m_Height[kIndex];
                m_DisplacementZ[kIndex] = Complex(0, -kUnitWaveVec.y) * m_Height[kIndex];
            }
    }

    fftwf_execute(m_PlanHeight);
    fftwf_execute(m_PlanSlopeX);
    fftwf_execute(m_PlanSlopeZ);
    fftwf_execute(m_PlanDisplacementX);
    fftwf_execute(m_PlanDisplacementZ);

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

            const auto h_FT = m_Height[kIndex].real() * static_cast<float>(sign);
            maxHeight = glm::max(h_FT, maxHeight);
            minHeight = glm::min(h_FT, minHeight);
        
            auto& displacement = m_Displacements[kIndex];
            displacement.y = h_FT;
            displacement.x =
                static_cast<float>(sign) * m_Lambda * m_DisplacementX[kIndex].real();
            displacement.z =
                static_cast<float>(sign) * m_Lambda * m_DisplacementZ[kIndex].real();

            m_Normals[kIndex] = glm::vec4(glm::normalize(glm::vec3(
                -( sign * m_SlopeX[kIndex].real() ),
                1.0f,
                -( sign * m_SlopeZ[kIndex].real() )
            )
            ),
            0.0f);
        }
    }

    return NormalizeHeights(minHeight, maxHeight);
}

float WSTessendorf::NormalizeHeights(float minHeight, float maxHeight)
{
    m_MinHeight = minHeight;
    m_MaxHeight = maxHeight;

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

