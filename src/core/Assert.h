/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#ifndef WATER_SURFACE_RENDERING_CORE_ASSERT_H_
#define WATER_SURFACE_RENDERING_CORE_ASSERT_H_

#include "core/Log.h"
#include <filesystem>

#ifdef VKP_ENABLE_ASSERTS
    // TODO other platforms, like windows: __debugbreak()
    #include <signal.h>
    #define VKP_DEXIT() raise(SIGTRAP)
    #define VKP_ASSERT_LOGGER() ::vkp::Log::GetAssertLogger()

    /**
     * @brief Assert macro. Prints default message
     *  when 'check' does NOT hold
     */
    #define VKP_ASSERT(check) { if(!(check)) {                                \
        SPDLOG_LOGGER_CRITICAL(VKP_ASSERT_LOGGER(), "Assertion '{0}' failed", \
                               VKP_STRINGIFY(check));                         \
        VKP_DEXIT(); } }

    /**
     * @brief Assert macro 
     * @param check If does NOT hold then exits the program
     * @param msg Log message printed if assert triggered
     */
    #define VKP_ASSERT_MSG(check, ...) { if(!(check)) {            \
        SPDLOG_LOGGER_CRITICAL(VKP_ASSERT_LOGGER(), __VA_ARGS__);  \
        VKP_DEXIT(); } }

    #define VKP_ASSERT_RESULT(st) VKP_ASSERT((st) == VK_SUCCESS)
    #define VKP_ASSERT_RESULT_MSG(st, msg) VKP_ASSERT_MSG((st) == VK_SUCCESS, msg)
#else
    #define VKP_DEXIT()
    #define VKP_ASSERT_LOGGER()
    #define VKP_ASSERT(...)
    #define VKP_ASSERT_MSG(...)
    #define VKP_ASSERT_RESULT(...)
    #define VKP_ASSERT_RESULT_MSG(...)
#endif // ifdef VKP_ENABLE_ASSERTS

#endif // WATER_SURFACE_RENDERING_CORE_ASSERT_H_
