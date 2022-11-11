##
#  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
# (http://opensource.org/licenses/MIT)
##

cmake_minimum_required(VERSION 3.16)

project(water VERSION 0.1 LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose Release or Debug" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# ------------------------------------------------------------------------------
# Setup directories
SET(BUILD_DIR "${CMAKE_BINARY_DIR}")
set(LIBS_DIR "${CMAKE_SOURCE_DIR}/libs")
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(SHADERS_DIR "${SRC_DIR}/shaders")
set(TEXTURES_DIR "${CMAKE_SOURCE_DIR}/textures")
set(MODELS_DIR "${CMAKE_SOURCE_DIR}/models")

# ==============================================================================
# Libraries

# OS Dependencies
find_package(Vulkan REQUIRED)
if(UNIX)
    find_package(X11 REQUIRED)
    find_package(Threads REQUIRED)
endif()

# ----------------------------------------------------------
# spdlog
# https://github.com/gabime/spdlog

set(SPDLOG_DIR "${LIBS_DIR}/spdlog")
set(SPDLOG_INCLUDE_DIR "${SPDLOG_DIR}/include")
set(SPDLOG_BINARY_DIR "${BUILD_DIR}/spdlog")

add_subdirectory(${SPDLOG_DIR} ${SPDLOG_BINARY_DIR})

# ----------------------------------------------------------
# GLFW
# https://www.glfw.org/

set(GLFW_DIR "${LIBS_DIR}/glfw")
set(GLFW_INCLUDE_DIR "${GLFW_DIR}/include")
set(GLFW_BINARY_DIR "${BUILD_DIR}/glfw")

option(GLFW_LIBRARY_TYPE "STATIC")
option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
option(GLFW_INSTALL "Generate installation target" OFF)
option(GLFW_DOCUMENT_INTERNALS "Include internals in documentation" OFF)

add_subdirectory(${GLFW_DIR} ${GLFW_BINARY_DIR} EXCLUDE_FROM_ALL)

set(GLFW_LIBRARIES glfw)
if(UNIX)
    set(GLFW_LIBRARIES ${GLFW_LIBRARIES} ${VULKAN_LIBRARIES} ${X11_LIBRARIES}
                        ${X11_Xrandr_LIB} ${X11_Xcursor_LIB} ${X11_Xinerama_LIB}
                        ${X11_Xxf86vm_LIB}
                        ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS}
    )
else()
    # TODO Windows and others
endif()

# ----------------------------------------------------------
# Dear ImGui
# https://github.com/ocornut/imgui

set(IMGUI_DIR "${LIBS_DIR}/imgui")
set(IMGUI_INCLUDE_DIR "${LIBS_DIR}")
set(IMGUI_BINARY_DIR "${BUILD_DIR}/imgui")

set(IMGUI_SOURCES
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_demo.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp"
)

add_library(imgui ${IMGUI_SOURCES})
add_dependencies(imgui glfw)
target_link_libraries(imgui ${GLFW_LIBRARIES})

target_include_directories(imgui
    PRIVATE ${GLFW_INCLUDE_DIR}
    PUBLIC ${IMGUI_DIR}
)

#target_compile_options(imgui PRIVATE "-DVK_NO_PROTOTYPES")

# ----------------------------------------------------------
# libshaderc
# https://github.com/google/shaderc
# linking info: https://github.com/google/shaderc/blob/main/libshaderc/README.md

set(SHADERC_DIR "${LIBS_DIR}/shaderc")
set(SHADERC_INCLUDE_DIR "${SHADERC_DIR}/libshaderc/include")
set(SHADERC_BINARY_DIR  "${BUILD_DIR}/shaderc")

set(SHADERC_SKIP_INSTALL            ON CACHE INTERNAL "" FORCE)
set(SHADERC_SKIP_TESTS              ON CACHE INTERNAL "" FORCE)
set(SHADERC_SKIP_EXAMPLES           ON CACHE INTERNAL "" FORCE)
set(SHADERC_SKIP_COPYRIGHT_CHECK    ON CACHE INTERNAL "" FORCE)

add_subdirectory(${SHADERC_DIR} ${SHADERC_BINARY_DIR})

# TODO run ./utils/git-sync-deps to pull SPIRV-Tools into third_party/
# https://github.com/google/shaderc/issues/1162

# ----------------------------------------------------------
# FFTW
# http://www.fftw.org/

set(FFTW_DIR         "${LIBS_DIR}/fftw")
set(FFTW_INCLUDE_DIR "${FFTW_DIR}/api")
set(FFTW_BINARY_DIR  "${BUILD_DIR}/fftw")

include(FetchContent)

FetchContent_Populate(fftw

    TMP_DIR "${BUILD_DIR}/temp"
    DOWNLOAD_DIR "${BUILD_DIR}/download"
    SOURCE_DIR "${FFTW_DIR}"

    URL "http://fftw.org/fftw-3.3.10.tar.gz"
    URL_MD5 "8ccbf6a5ea78a16dbc3e1306e234cc5c"
)

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "" FORCE)
set(BUILD_TESTS       OFF CACHE INTERNAL "" FORCE)
set(ENABLE_OPENMP     OFF CACHE INTERNAL "" FORCE)
set(ENABLE_THREADS    OFF CACHE INTERNAL "" FORCE)
set(ENABLE_FLOAT      ON  CACHE INTERNAL "" FORCE)
set(ENABLE_SSE        OFF CACHE INTERNAL "" FORCE)
set(ENABLE_SSE2       OFF CACHE INTERNAL "" FORCE)
set(ENABLE_AVX        ON  CACHE INTERNAL "" FORCE)
set(ENABLE_AVX2       OFF CACHE INTERNAL "" FORCE)

add_subdirectory(${FFTW_DIR} ${FFTW_BINARY_DIR})

#===============================================================================
# OpenMP

#find_package(OpenMP REQUIRED)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")

# ==============================================================================
# Header-only
#   Should be able to access them relative to variable LIBS_DIR
#   LIBS_DIR variable should already be included in include path

# ----------------------------------------------------------
# GLM
# https://github.com/g-truc/glm

set(GLM_DIR "${LIBS_DIR}/glm")
set(GLM_INCLUDE_DIR "${GLM_DIR}")

# ----------------------------------------------------------
# STB: stb_image
# https://github.com/nothings/stb

set(STB_DIR "${LIBS_DIR}/stb")
set(STB_INCLUDE_DIR "${LIBS_DIR}")

# ==============================================================================
# ==============================================================================
# ==============================================================================

set(MAIN_DIR "${SRC_DIR}")
set(MAIN_INCLUDE_DIR "${SRC_DIR}")
set(MAIN_CORE_DIR "${MAIN_DIR}/core")
set(MAIN_VULKAN_DIR "${MAIN_DIR}/vulkan")
set(MAIN_SCENE_DIR "${MAIN_DIR}/scene")

# TODO file(GLOB sources *.cpp)
set(MAIN_SOURCES 
    "${MAIN_CORE_DIR}/Log.cpp"
    "${MAIN_CORE_DIR}/Profile.cpp"
    "${MAIN_VULKAN_DIR}/utils.cpp"
    "${MAIN_VULKAN_DIR}/Instance.cpp"
    "${MAIN_VULKAN_DIR}/PhysicalDevice.cpp"
    "${MAIN_VULKAN_DIR}/Device.cpp"
    "${MAIN_VULKAN_DIR}/Surface.cpp"
    "${MAIN_VULKAN_DIR}/Image.cpp"
    "${MAIN_VULKAN_DIR}/ImageView.cpp"
    "${MAIN_VULKAN_DIR}/SwapChain.cpp"
    "${MAIN_VULKAN_DIR}/Buffer.cpp"
    "${MAIN_VULKAN_DIR}/CommandBuffer.cpp"
    "${MAIN_VULKAN_DIR}/CommandPool.cpp"
    "${MAIN_VULKAN_DIR}/ShaderModule.cpp"
    "${MAIN_VULKAN_DIR}/RenderPass.cpp"
    "${MAIN_VULKAN_DIR}/Pipeline.cpp"
    "${MAIN_VULKAN_DIR}/Sampler.cpp"
    "${MAIN_VULKAN_DIR}/Texture2D.cpp"
    "${MAIN_VULKAN_DIR}/Descriptors.cpp"
    "${MAIN_VULKAN_DIR}/Texture3D.cpp"
    "${MAIN_VULKAN_DIR}/QueueTypes.cpp"
    "${MAIN_CORE_DIR}/Window.cpp"
    "${MAIN_DIR}/Gui.cpp"
    "${MAIN_CORE_DIR}/Application.cpp"
    "${MAIN_SCENE_DIR}/Camera.cpp"
    "${MAIN_SCENE_DIR}/WaterSurfaceMesh.cpp"
    "${MAIN_SCENE_DIR}/WSTessendorf.cpp"
    "${MAIN_DIR}/WaterSurface.cpp"
    "${MAIN_DIR}/main.cpp"
)

add_executable(${PROJECT_NAME} 
    ${MAIN_SOURCES}
)

target_include_directories(${PROJECT_NAME}
    PRIVATE ${MAIN_INCLUDE_DIR}
            ${SPDLOG_INCLUDE_DIR}
            ${GLFW_INCLUDE_DIR}
            ${GLM_INCLUDE_DIR}
            ${STB_INCLUDE_DIR}
            ${IMGUI_INCLUDE_DIR}
            ${SHADERC_INCLUDE_DIR}
            ${FFTW_INCLUDE_DIR}
)

add_dependencies(${PROJECT_NAME}
    Vulkan::Vulkan
    glfw
    spdlog::spdlog
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE ${GLFW_LIBRARIES}
    PRIVATE Vulkan::Vulkan
    PRIVATE spdlog::spdlog
    PRIVATE imgui
    PRIVATE shaderc
    PRIVATE fftw3f
    #PRIVATE OpenMP::OpenMP_CXX
)

target_precompile_headers(${PROJECT_NAME}
    PRIVATE "${SRC_DIR}/pch.h"
)

target_compile_options(${PROJECT_NAME}
    PRIVATE "-DVKP_DEBUG"
)

#--------------------------------------------------------------------------------
# Copy assets to build folder
#--------------------------------------------------------------------------------

set(SHADERS_BIN_DIR "${PROJECT_BINARY_DIR}/shaders")

add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${SHADERS_DIR}
    ${SHADERS_BIN_DIR}
    COMMENT "Copying shaders to build tree")
