// File: Source/Filament/Internal/TextureSubsystem/FilamentMaterial.h
#pragma once

/*====================================================================================================================================
                                                    FILAMENTMATERIAL.H
====================================================================================================================================*/
// 🧩 64-byte PBR material struct with 5 channels + descriptor helpers.

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

#include "../Algebra/Vector.h"
#include <cstdint>
#include <string>

//------------------------------------------------------------------------------------------------------------------------
//                                                    MATERIAL STRUCT
//------------------------------------------------------------------------------------------------------------------------

// 📌 std140 layout — 64 bytes total. Matches GLSL uniform block exactly.
struct FilamentMaterial
{
    Vec4  albedo           = { 0.8f, 0.8f, 0.8f, 1.0f }; // [-]   - Base color RGBA
    float roughness        = 0.5f;                         // [0-1] - Surface roughness
    float metallic         = 0.0f;                         // [0-1] - Metalness
    float specularF0       = 0.04f;                        // [0-1] - Fresnel reflectance at normal incidence
    float refractionStrength = 0.0f;                       // [0-1] - 0 = opaque, >0 = refractive
    Vec4  _pad0            = {};                           // [-]   - Padding for std140 alignment
    Vec4  _pad1            = {};                           // [-]   - Padding for std140 alignment
};                                                         // Total: 64 bytes

static_assert(sizeof(FilamentMaterial) == 64, "FilamentMaterial must be 64 bytes");


//------------------------------------------------------------------------------------------------------------------------
//                                                    SCENE OBJECT
//------------------------------------------------------------------------------------------------------------------------

struct SceneObject
{
    std::string      name;                       // [-]   - Display name for outliner
    uint32_t         meshIndex    = 0;           // [idx] - Index into scene mesh array
    FilamentMaterial material;                   // [-]   - PBR material properties
    Vec3             position     = {};          // [cm]  - World-space position
    Vec3             scale        = { 1, 1, 1 }; // [-]   - Scale factors
    Vec3             rotation     = {};          // [rad] - Euler angles (yaw, pitch, roll)
};


//------------------------------------------------------------------------------------------------------------------------
//                                                    PRESET MATERIALS
//------------------------------------------------------------------------------------------------------------------------

namespace MaterialPresets
{
    inline FilamentMaterial gold()
    {
        FilamentMaterial m;
        m.albedo     = { 1.0f, 0.766f, 0.336f, 1.0f };
        m.roughness  = 0.3f;
        m.metallic   = 1.0f;
        m.specularF0 = 0.04f;
        return m;
    }

    inline FilamentMaterial plastic()
    {
        FilamentMaterial m;
        m.albedo     = { 0.2f, 0.5f, 0.9f, 1.0f };
        m.roughness  = 0.4f;
        m.metallic   = 0.0f;
        m.specularF0 = 0.04f;
        return m;
    }

    inline FilamentMaterial glass()
    {
        FilamentMaterial m;
        m.albedo             = { 0.95f, 0.95f, 1.0f, 0.3f };
        m.roughness          = 0.05f;
        m.metallic           = 0.0f;
        m.specularF0         = 0.08f;
        m.refractionStrength = 0.8f;
        return m;
    }

    inline FilamentMaterial roughMetal()
    {
        FilamentMaterial m;
        m.albedo     = { 0.56f, 0.57f, 0.58f, 1.0f };
        m.roughness  = 0.8f;
        m.metallic   = 1.0f;
        m.specularF0 = 0.04f;
        return m;
    }

    inline FilamentMaterial floor()
    {
        FilamentMaterial m;
        m.albedo     = { 0.4f, 0.4f, 0.4f, 1.0f };
        m.roughness  = 0.9f;
        m.metallic   = 0.0f;
        m.specularF0 = 0.02f;
        return m;
    }
}
