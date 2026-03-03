// File: Source/Filament/Internal/Algebra/Utilities.h
#pragma once

/*====================================================================================================================================
                                                        UTILITIES.H
====================================================================================================================================*/
// 🧩 Math utilities — radians, clamp, lerp, perspectiveVK, lookAt, ortho.

#include "Vector.h"
#include "Matrix.h"
#include <cmath>
#include <algorithm>

//------------------------------------------------------------------------------------------------------------------------
//                                                    CONSTANTS
//------------------------------------------------------------------------------------------------------------------------

constexpr float PI_F      = 3.14159265358979323846f;  // [-] - Pi
constexpr float TWO_PI_F  = 6.28318530717958647692f;  // [-] - 2 * Pi
constexpr float HALF_PI_F = 1.57079632679489661923f;  // [-] - Pi / 2
constexpr float INV_PI_F  = 0.31830988618379067154f;  // [-] - 1 / Pi


//------------------------------------------------------------------------------------------------------------------------
//                                                    SCALAR UTILITIES
//------------------------------------------------------------------------------------------------------------------------

[[nodiscard]] inline constexpr float toRadians(float degrees) noexcept
{
    return degrees * (PI_F / 180.0f);
}

[[nodiscard]] inline constexpr float toDegrees(float radians) noexcept
{
    return radians * (180.0f / PI_F);
}

template<typename T>
[[nodiscard]] inline constexpr T clampValue(T v, T lo, T hi) noexcept
{
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

[[nodiscard]] inline constexpr float lerpScalar(float a, float b, float t) noexcept
{
    return a + t * (b - a);
}

[[nodiscard]] inline constexpr float saturate(float v) noexcept
{
    return clampValue(v, 0.0f, 1.0f);
}

[[nodiscard]] inline constexpr float stepValue(float edge, float x) noexcept
{
    return x < edge ? 0.0f : 1.0f;
}

[[nodiscard]] inline constexpr float smoothstepValue(float edge0, float edge1, float x) noexcept
{
    float t = clampValue((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}


//------------------------------------------------------------------------------------------------------------------------
//                                                    PROJECTION MATRICES
//------------------------------------------------------------------------------------------------------------------------

// ⚙️ Perspective projection for Vulkan clip space:
//     - Y is flipped (Vulkan NDC y points downward)
//     - Depth range [0, 1] (not [-1, 1] like OpenGL)
//     - Right-handed coordinate system
[[nodiscard]] inline Mat4 perspectiveVK(float fovYRadians,
                                        float aspect,
                                        float nearPlane,
                                        float farPlane) noexcept
{
    float tanHalf = std::tan(fovYRadians * 0.5f);

    Mat4 result;
    result[0] = Vec4(0.0f);
    result[1] = Vec4(0.0f);
    result[2] = Vec4(0.0f);
    result[3] = Vec4(0.0f);

    result[0][0] =  1.0f / (aspect * tanHalf);
    result[1][1] = -1.0f / tanHalf;              // ⚙️ Y-flip for Vulkan
    result[2][2] =  farPlane / (nearPlane - farPlane);
    result[2][3] = -1.0f;
    result[3][2] = (nearPlane * farPlane) / (nearPlane - farPlane);

    return result;
}

// ⚙️ Orthographic projection for Vulkan clip space (Y-flipped, depth [0,1])
[[nodiscard]] inline Mat4 orthoVK(float left,   float right,
                                   float bottom, float top,
                                   float nearP,  float farP) noexcept
{
    Mat4 result;
    result[0] = Vec4(0.0f);
    result[1] = Vec4(0.0f);
    result[2] = Vec4(0.0f);
    result[3] = Vec4(0.0f);

    result[0][0] =  2.0f / (right - left);
    result[1][1] = -2.0f / (top - bottom);       // ⚙️ Y-flip for Vulkan
    result[2][2] = -1.0f / (farP - nearP);
    result[3][0] = -(right + left) / (right - left);
    result[3][1] = -(top + bottom) / (top - bottom);
    result[3][2] = -nearP / (farP - nearP);
    result[3][3] =  1.0f;

    return result;
}


//------------------------------------------------------------------------------------------------------------------------
//                                                    VIEW MATRIX
//------------------------------------------------------------------------------------------------------------------------

// ⚙️ Right-handed look-at matrix
[[nodiscard]] inline Mat4 lookAtRH(const Vec3& eye,
                                    const Vec3& target,
                                    const Vec3& worldUp) noexcept
{
    Vec3 forward = normalize(eye - target);      // ⚙️ RH: camera looks along -Z
    Vec3 right   = normalize(cross(worldUp, forward));
    Vec3 up      = cross(forward, right);

    Mat4 result;
    result[0] = Vec4(right.x,   up.x,   forward.x,   0.0f);
    result[1] = Vec4(right.y,   up.y,   forward.y,   0.0f);
    result[2] = Vec4(right.z,   up.z,   forward.z,   0.0f);
    result[3] = Vec4(-dot(right, eye), -dot(up, eye), -dot(forward, eye), 1.0f);

    return result;
}


//------------------------------------------------------------------------------------------------------------------------
//                                                    ROTATION MATRICES
//------------------------------------------------------------------------------------------------------------------------

// ⚙️ Rotation around an arbitrary axis
[[nodiscard]] inline Mat4 rotationMatrix(const Vec3& axis, float angleRad) noexcept
{
    float c  = std::cos(angleRad);
    float s  = std::sin(angleRad);
    float t  = 1.0f - c;
    Vec3  a  = normalize(axis);

    Mat4 result;
    result[0] = Vec4(t * a.x * a.x + c,       t * a.x * a.y + s * a.z, t * a.x * a.z - s * a.y, 0.0f);
    result[1] = Vec4(t * a.x * a.y - s * a.z, t * a.y * a.y + c,       t * a.y * a.z + s * a.x, 0.0f);
    result[2] = Vec4(t * a.x * a.z + s * a.y, t * a.y * a.z - s * a.x, t * a.z * a.z + c,       0.0f);
    result[3] = Vec4(0.0f,                    0.0f,                    0.0f,                    1.0f);

    return result;
}
