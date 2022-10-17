/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#ifndef WATER_SURFACE_RENDERING_CORE_TIMER_H_
#define WATER_SURFACE_RENDERING_CORE_TIMER_H_

#include <chrono>

#define MILLIS_TO_SECONDS 0.001f


namespace vkp
{
    class Timer
    {
    public:
        Timer()
        {
            Start();
        }

        inline void Start()
        {
            m_Start = std::chrono::high_resolution_clock::now();
        }

        inline float ElapsedMillis() const
        {
            return std::chrono::duration_cast<std::chrono::milliseconds>
                (std::chrono::high_resolution_clock::now() - m_Start).count();
        }

        inline float ElapsedMicro() const
        {
            return std::chrono::duration_cast<std::chrono::microseconds>
                (std::chrono::high_resolution_clock::now() - m_Start).count();
        }

        // @return Time elapsed in seconds
        inline float Elapsed() const
        {
            return ElapsedMillis() * MILLIS_TO_SECONDS; 
        }

    private:
        std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
    };

} // namespace vkp

#endif // WATER_SURFACE_RENDERING_CORE_TIMER_H_