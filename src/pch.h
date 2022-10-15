/**
 *  Copyright (c) 2022 WaterSurfaceRendering authors Distributed under MIT License
 * (http://opensource.org/licenses/MIT)
 */

#ifndef WATER_SURFACE_RENDERING_PCH_H_
#define WATER_SURFACE_RENDERING_PCH_H_

#include <iostream>
#include <memory>

#include <string>
#include <vector>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <optional>
#include <algorithm>
#include <array>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

// TODO benchmark later
//#define GLM_FORCE_AVX
//#define GLM_FORCE_INLINE
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "core/Base.h"
#include "core/Log.h"
#include "core/Assert.h"
#include "core/Timestep.h"
#include "core/Timer.h"

#include "vulkan/utils.h"

#endif // WATER_SURFACE_RENDERING_PCH_H_
