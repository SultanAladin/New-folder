// File: Source/Filament/Internal/SceneContext/FilamentCamera.h
#pragma once

/*====================================================================================================================================
                                                     FILAMENTCAMERA.H
====================================================================================================================================*/
// 🧩 Arcball camera — continuous orbit, zoom, pan, view/projection matrices.

#include "../Algebra/Vector.h"
#include "../Algebra/Matrix.h"
#include "../Algebra/Utilities.h"
#include <cmath>

//------------------------------------------------------------------------------------------------------------------------
//                                                    ARCBALL CAMERA
//------------------------------------------------------------------------------------------------------------------------

class FilamentCamera
{
public:
    Vec3  target          = { 0.0f, 0.0f, 0.0f };  // [cm]  - Orbit center
    float azimuth         = 0.0f;                   // [rad] - Horizontal rotation angle
    float elevation       = 0.3f;                   // [rad] - Vertical rotation angle
    float radius          = 6.0f;                   // [cm]  - Distance from target
    float fovY            = toRadians(45.0f);       // [rad] - Vertical field of view
    float nearPlane       = 0.1f;                   // [cm]  - Near clip plane
    float farPlane        = 100.0f;                 // [cm]  - Far clip plane
    float orbitSpeed      = 0.4f;                   // [rad/s] - Auto-orbit angular speed
    bool  autoOrbit       = true;                   // [-]   - Whether camera auto-orbits

    // ⚙️ Advance the camera orbit by elapsed time
    void advanceTime(float deltaSeconds) noexcept
    {
        if (autoOrbit)
        {
            azimuth += orbitSpeed * deltaSeconds;
            if (azimuth > TWO_PI_F) azimuth -= TWO_PI_F;
        }
    }

    // ⚙️ Compute camera world position from spherical coordinates
    [[nodiscard]] Vec3 queryPosition() const noexcept
    {
        float cosElev = std::cos(elevation);
        return {
            target.x + radius * cosElev * std::cos(azimuth),
            target.y + radius * std::sin(elevation),
            target.z + radius * cosElev * std::sin(azimuth)
        };
    }

    // ⚙️ Compute view matrix (right-handed look-at)
    [[nodiscard]] Mat4 queryViewMatrix() const noexcept
    {
        Vec3 eye = queryPosition();
        return lookAtRH(eye, target, Vec3(0.0f, 1.0f, 0.0f));
    }

    // ⚙️ Compute projection matrix (Vulkan clip space)
    [[nodiscard]] Mat4 queryProjectionMatrix(float aspectRatio) const noexcept
    {
        return perspectiveVK(fovY, aspectRatio, nearPlane, farPlane);
    }

    // ⚙️ Apply mouse drag orbit input
    void applyOrbitInput(float deltaAzimuth, float deltaElevation) noexcept
    {
        azimuth   += deltaAzimuth;
        elevation += deltaElevation;
        elevation  = clampValue(elevation, -HALF_PI_F + 0.01f, HALF_PI_F - 0.01f);
    }

    // ⚙️ Apply scroll zoom input
    void applyZoomInput(float delta) noexcept
    {
        radius -= delta * 0.5f;
        radius  = clampValue(radius, 1.0f, 50.0f);
    }

    // ⚙️ Apply middle-drag pan input
    void applyPanInput(float dx, float dy) noexcept
    {
        Vec3 eye     = queryPosition();
        Vec3 forward = normalize(target - eye);
        Vec3 right   = normalize(cross(forward, Vec3(0.0f, 1.0f, 0.0f)));
        Vec3 up      = cross(right, forward);

        target = target + right * (-dx * 0.01f) + up * (dy * 0.01f);
    }
};
