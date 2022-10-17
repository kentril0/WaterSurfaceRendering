/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#include "pch.h"
#include "core/Log.h"
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/stdout_color_sinks.h>


namespace vkp 
{
    std::shared_ptr<spdlog::logger> Log::s_Logger;
    std::shared_ptr<spdlog::logger> Log::s_AssertLogger;

    void Log::Init()
    {
        spdlog::set_level(spdlog::level::trace);

        s_Logger = spdlog::stdout_color_mt("VKP");
        // TODO expose level later
        s_Logger->set_level(spdlog::level::trace);
        s_Logger->set_pattern("%^[%T]: %v%$");

        // TODO to file also

        s_AssertLogger = spdlog::stdout_color_mt("VKPA");
        s_AssertLogger->set_level(spdlog::level::trace);
        s_AssertLogger->set_pattern("%^[%T]: %s:%#:%!() %v%$");

        //s_Logger = std::make_shared<spdlog::logger>("VKPP");
        //spdlog::register_logger(s_Logger);

        // TODOD when outputting to a file
        //s_ProfileLogger->flush_on(spdlog::level::trace);

    }

} // namespace vkp 
