// File: Source/Filament/Internal/Auxiliary/FilamentTypes.h
#pragma once

/*====================================================================================================================================
                                                      FILAMENTTYPES.H
====================================================================================================================================*/
// 🧩 Common types, Vulkan forward declarations, and utility macros for the Filament renderer.

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <cstdint>
#include <cstddef>
#include <memory>
#include <vector>
#include <array>
#include <string>
#include <stdexcept>
#include <functional>

//------------------------------------------------------------------------------------------------------------------------
//                                                    CONSTANTS
//------------------------------------------------------------------------------------------------------------------------

static constexpr uint32_t FILAMENT_MAX_FRAMES_IN_FLIGHT = 2;   // [-] - Double-buffered frame resources
static constexpr uint32_t FILAMENT_MAX_SCENE_OBJECTS    = 64;  // [-] - Maximum objects in the scene
static constexpr uint32_t FILAMENT_GBUFFER_RT_COUNT     = 3;   // [-] - GBuffer render target count


//------------------------------------------------------------------------------------------------------------------------
//                                                    RESULT CHECK
//------------------------------------------------------------------------------------------------------------------------

// ⚙️ Vulkan result checking macro — throws on failure
#define FILAMENT_VK_VERIFY(expr)                                        \
    do {                                                                \
        VkResult _vkResult = (expr);                                    \
        if (_vkResult != VK_SUCCESS)                                    \
        {                                                               \
            throw std::runtime_error(                                   \
                std::string("Vulkan error: ") + std::to_string(static_cast<int>(_vkResult)) + \
                " at " + __FILE__ + ":" + std::to_string(__LINE__));    \
        }                                                               \
    } while (false)


//------------------------------------------------------------------------------------------------------------------------
//                                                    VERTEX LAYOUT
//------------------------------------------------------------------------------------------------------------------------

struct FilamentVertex
{
    float position[3];                           // [cm]  - World-space vertex position
    float normal[3];                             // [-]   - Unit normal vector
    float uv[2];                                 // [0-1] - Texture coordinate
};


//------------------------------------------------------------------------------------------------------------------------
//                                                    SCENE UBO (SET 0)
//------------------------------------------------------------------------------------------------------------------------

// ⚙️ Aligned to std140 layout — 256 bytes total padded
struct SceneUBO
{
    float viewMatrix[16];                        // [-] - Camera view matrix (column-major)
    float projMatrix[16];                        // [-] - Projection matrix (column-major)
    float invViewMatrix[16];                     // [-] - Inverse view matrix
    float lightDirection[4];                     // [-] - Directional light direction (w unused)
    float lightColor[4];                         // [-] - Light color + intensity in w
    float cameraPosition[4];                     // [cm] - Camera world position (w unused)
    float timeAndResolution[4];                  // [s, px, px, -] - time, width, height, pad
};

// ⚙️ Push constant for per-object model matrix
struct ModelPushConstant
{
    float modelMatrix[16];                       // [-] - Model transform (column-major)
    float normalMatrix[16];                      // [-] - Transpose-inverse of model for normals
};
