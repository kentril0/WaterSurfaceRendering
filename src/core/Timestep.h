/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#ifndef WATER_SURFACE_RENDERING_CORE_TIMESTEP_H_
#define WATER_SURFACE_RENDERING_CORE_TIMESTEP_H_


namespace vkp
{
    class Timestep
    {
    public:
        Timestep(float t) : m_Step(t) {}

        inline constexpr operator float() const { return m_Step; }

        inline constexpr float Seconds() const { return m_Step; }
        inline constexpr float Millis() const { return m_Step * 1000.0f; }

    private:
        float m_Step;
    };
    
} // namespace vkp

#endif // WATER_SURFACE_RENDERING_CORE_TIMESTEP_H_
