// File: Source/Filament/Internal/Algebra/Quaternion.h
#pragma once

/*====================================================================================================================================
                                                        QUATERNION.H
====================================================================================================================================*/
// 🧩 Quat — quaternion type for rotations.

#include "Vector.h"
#include "Matrix.h"
#include <cmath>

//------------------------------------------------------------------------------------------------------------------------
//                                                    QUATERNION
//------------------------------------------------------------------------------------------------------------------------

struct Quat
{
    float x = 0.0f;                              // [-] - i component
    float y = 0.0f;                              // [-] - j component
    float z = 0.0f;                              // [-] - k component
    float w = 1.0f;                              // [-] - Scalar component

    constexpr Quat() = default;
    constexpr Quat(float px, float py, float pz, float pw)
        : x(px), y(py), z(pz), w(pw)
    {}

    // ⚙️ Construct from axis-angle (axis must be normalized)
    static inline Quat fromAxisAngle(const Vec3& axis, float angleRad) noexcept
    {
        float half  = angleRad * 0.5f;
        float sinH  = std::sin(half);
        float cosH  = std::cos(half);
        return { axis.x * sinH, axis.y * sinH, axis.z * sinH, cosH };
    }

    // ⚙️ Quaternion multiply (Hamilton product)
    constexpr Quat operator*(const Quat& r) const
    {
        return {
            w * r.x + x * r.w + y * r.z - z * r.y,
            w * r.y - x * r.z + y * r.w + z * r.x,
            w * r.z + x * r.y - y * r.x + z * r.w,
            w * r.w - x * r.x - y * r.y - z * r.z
        };
    }

    constexpr Quat& operator*=(const Quat& r)
    {
        *this = *this * r;
        return *this;
    }

    constexpr bool operator==(const Quat& r) const { return x == r.x && y == r.y && z == r.z && w == r.w; }
    constexpr bool operator!=(const Quat& r) const { return !(*this == r); }
};

[[nodiscard]] inline constexpr float dot(const Quat& a, const Quat& b) noexcept
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

[[nodiscard]] inline float length(const Quat& q) noexcept
{
    return std::sqrt(dot(q, q));
}

[[nodiscard]] inline Quat normalize(const Quat& q) noexcept
{
    float len = length(q);
    if (len < 1e-8f) return Quat();
    float inv = 1.0f / len;
    return { q.x * inv, q.y * inv, q.z * inv, q.w * inv };
}

[[nodiscard]] inline constexpr Quat conjugate(const Quat& q) noexcept
{
    return { -q.x, -q.y, -q.z, q.w };
}

// ⚙️ Spherical linear interpolation
[[nodiscard]] inline Quat slerp(const Quat& a, const Quat& b, float t) noexcept
{
    float d = dot(a, b);

    // ⚙️ Ensure shortest path
    Quat target = b;
    if (d < 0.0f)
    {
        d = -d;
        target = { -b.x, -b.y, -b.z, -b.w };
    }

    // ⚙️ Linear fallback for near-identical quaternions
    if (d > 0.9995f)
    {
        Quat r = {
            a.x + t * (target.x - a.x),
            a.y + t * (target.y - a.y),
            a.z + t * (target.z - a.z),
            a.w + t * (target.w - a.w)
        };
        return normalize(r);
    }

    float theta     = std::acos(d);
    float sinTheta  = std::sin(theta);
    float wa        = std::sin((1.0f - t) * theta) / sinTheta;
    float wb        = std::sin(t * theta)           / sinTheta;

    return {
        a.x * wa + target.x * wb,
        a.y * wa + target.y * wb,
        a.z * wa + target.z * wb,
        a.w * wa + target.w * wb
    };
}

// ⚙️ Convert quaternion to 4x4 rotation matrix
[[nodiscard]] inline Mat4 quatToMat4(const Quat& q) noexcept
{
    float x2 = q.x + q.x, y2 = q.y + q.y, z2 = q.z + q.z;
    float xx = q.x * x2,   yy = q.y * y2,   zz = q.z * z2;
    float xy = q.x * y2,   xz = q.x * z2,   yz = q.y * z2;
    float wx = q.w * x2,   wy = q.w * y2,   wz = q.w * z2;

    Mat4 result;
    result[0] = Vec4(1.0f - (yy + zz), xy + wz,           xz - wy,           0.0f);
    result[1] = Vec4(xy - wz,          1.0f - (xx + zz),  yz + wx,           0.0f);
    result[2] = Vec4(xz + wy,          yz - wx,           1.0f - (xx + yy),  0.0f);
    result[3] = Vec4(0.0f,             0.0f,              0.0f,              1.0f);
    return result;
}

// ⚙️ Rotate a Vec3 by a quaternion
[[nodiscard]] inline Vec3 rotateByQuat(const Vec3& v, const Quat& q) noexcept
{
    Vec3 qv = { q.x, q.y, q.z };
    Vec3 uv = cross(qv, v);
    Vec3 uuv = cross(qv, uv);
    return v + (uv * q.w + uuv) * 2.0f;
}
