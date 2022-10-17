/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#ifndef WATER_SURFACE_RENDERING_CORE_LOG_H_
#define WATER_SURFACE_RENDERING_CORE_LOG_H_

#include <memory>

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
// Ignore any warnings raised inside external headers
#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#pragma warning(pop)

#include "core/Base.h"


namespace vkp
{
    /**
     * @brief Singleton logging class
     */
    class Log
    {
    public:
        static void Init();

        static std::shared_ptr<spdlog::logger>& GetLogger() { return s_Logger; }
        static std::shared_ptr<spdlog::logger>& GetAssertLogger() {
             return s_AssertLogger;
        }

    private:
        static std::shared_ptr<spdlog::logger> s_Logger;
        static std::shared_ptr<spdlog::logger> s_AssertLogger;
    };

} // namespace vkp

#define VKP_LOG_TRACE(...) ::vkp::Log::GetLogger()->trace(__VA_ARGS__)
#define VKP_LOG_INFO(...)  ::vkp::Log::GetLogger()->info(__VA_ARGS__)
#define VKP_LOG_WARN(...)  ::vkp::Log::GetLogger()->warn(__VA_ARGS__)
#define VKP_LOG_ERR(...)   ::vkp::Log::GetLogger()->error(__VA_ARGS__)
#define VKP_LOG_CRIT(...)  ::vkp::Log::GetLogger()->critical(__VA_ARGS__)

#define VKP_ASSERT_LOGGER() ::vkp::Log::GetAssertLogger()

// @brief Prints that the function has been called, and when
#define VKP_REGISTER_FUNCTION() { \
    SPDLOG_LOGGER_TRACE(::vkp::Log::GetAssertLogger(), ""); \
}

// TODO - scoped timer
#define VKP_PROFILE_FUNCTION() { \
    SPDLOG_LOGGER_TRACE(::vkp::Log::GetAssertLogger(), ""); \
}

#endif // WATER_SURFACE_RENDERING_CORE_LOG_H_
