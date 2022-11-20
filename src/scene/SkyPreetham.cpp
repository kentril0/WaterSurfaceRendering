/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#include "pch.h"
#include "scene/SkyPreetham.h"

#include <glm/glm.hpp>

#define M_PI 3.14159265358979323846


SkyPreetham::SkyPreetham(const glm::vec3& sunDir, float turbidity)
{
    VKP_REGISTER_FUNCTION();

    SetSunDirection(sunDir);
    SetTurbidity(turbidity);

    Update();
}

void SkyPreetham::Update()
{
    ComputePerezDistribution();

    const glm::vec3 NORMAL_UP(0.0, 1.0, 0.0);
    const float thetaSun = glm::acos(
        glm::max( glm::dot(m_Props.sunDir, NORMAL_UP), 0.0f )
    );

    ComputeZenithLuminanceYxy(thetaSun);
    ComputePerezLuminanceYxy(thetaSun);
}

void SkyPreetham::ComputePerezDistribution()
{
    const float t = m_Props.turbidity;

    m_Props.A = glm::vec3(  0.1787 * t - 1.4630,
                           -0.0193 * t - 0.2592,
                           -0.0167 * t - 0.2608 );
    m_Props.B = glm::vec3( -0.3554 * t + 0.4275,
                           -0.0665 * t + 0.0008,
                           -0.0950 * t + 0.0092 );
    m_Props.C = glm::vec3( -0.0227 * t + 5.3251,
                           -0.0004 * t + 0.2125,
                           -0.0079 * t + 0.2102 );
    m_Props.D = glm::vec3(  0.1206 * t - 2.5771,
                           -0.0641 * t - 0.8989,
                           -0.0441 * t - 1.6537 );
    m_Props.E = glm::vec3( -0.0670 * t + 0.3703,
                           -0.0033 * t + 0.0452,
                           -0.0109 * t + 0.0529 );
}    

void SkyPreetham::ComputeZenithLuminanceYxy(const float thetaSun)
{
    const float t = m_Props.turbidity;
    const float chi = (4.0 / 9.0 - t / 120.0) * (M_PI - 2.0*thetaSun);
    const float Yz  = (4.0453*t - 4.9710) * glm::tan(chi) - 0.2155*t + 2.4192;

    const float theta2 = thetaSun * thetaSun;
    const float theta3 = theta2 * thetaSun;
    const float t2 = t * t;

    const float xz = 
        ( 0.00165*theta3 - 0.00375*theta2 + 0.00209*thetaSun) * t2 +
        (-0.02903*theta3 + 0.06377*theta2 - 0.03202*thetaSun + 0.00394) * t +
        ( 0.11693*theta3 - 0.21196*theta2 + 0.06052*thetaSun + 0.25886);

    const float yz =
      ( 0.00275*theta3 - 0.00610*theta2 + 0.00317*thetaSun + 0.0) * t2 +
      (-0.04214*theta3 + 0.08970*theta2 - 0.04153*thetaSun + 0.00516) * t +
      ( 0.15346*theta3 - 0.26756*theta2 + 0.06670*thetaSun + 0.26688);

    m_Props.ZenithLum = glm::vec3(Yz, xz, yz);
}

void SkyPreetham::ComputePerezLuminanceYxy(const float gamma)
{
    const float cosGamma = glm::cos(gamma);

    m_Props.ZeroThetaSun = (
        1.0f + m_Props.A * glm::exp( m_Props.B / glm::cos(0.0f) )
    ) * (
        1.0f + m_Props.C * glm::exp(m_Props.D * gamma) +
              m_Props.E * cosGamma * cosGamma
    );
}
