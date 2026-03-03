// File: Source/Filament/Internal/Algebra/Vector.h
#pragma once

/*====================================================================================================================================
                                                          VECTOR.H
====================================================================================================================================*/
// 🧩 Vec2, Vec3, Vec4 — lightweight vector types for the Filament math library.

#include <cmath>
#include <cstdint>

//------------------------------------------------------------------------------------------------------------------------
//                                                    VEC2
//------------------------------------------------------------------------------------------------------------------------

struct Vec2
{
    float x = 0.0f;                              // [-] - First component
    float y = 0.0f;                              // [-] - Second component

    constexpr Vec2() = default;
    constexpr Vec2(float s) : x(s), y(s) {}
    constexpr Vec2(float px, float py) : x(px), y(py) {}

    constexpr float& operator[](int i)       { return (&x)[i]; }
    constexpr float  operator[](int i) const { return (&x)[i]; }

    constexpr Vec2 operator+(const Vec2& r) const { return { x + r.x, y + r.y }; }
    constexpr Vec2 operator-(const Vec2& r) const { return { x - r.x, y - r.y }; }
    constexpr Vec2 operator*(float s)        const { return { x * s,   y * s   }; }
    constexpr Vec2 operator/(float s)        const { float inv = 1.0f / s; return { x * inv, y * inv }; }
    constexpr Vec2 operator*(const Vec2& r) const { return { x * r.x, y * r.y }; }
    constexpr Vec2 operator/(const Vec2& r) const { return { x / r.x, y / r.y }; }
    constexpr Vec2 operator-()               const { return { -x, -y }; }

    constexpr Vec2& operator+=(const Vec2& r) { x += r.x; y += r.y; return *this; }
    constexpr Vec2& operator-=(const Vec2& r) { x -= r.x; y -= r.y; return *this; }
    constexpr Vec2& operator*=(float s)       { x *= s;   y *= s;   return *this; }
    constexpr Vec2& operator/=(float s)       { float inv = 1.0f / s; x *= inv; y *= inv; return *this; }

    constexpr bool operator==(const Vec2& r) const { return x == r.x && y == r.y; }
    constexpr bool operator!=(const Vec2& r) const { return !(*this == r); }
};

inline constexpr Vec2 operator*(float s, const Vec2& v) { return v * s; }

[[nodiscard]] inline constexpr float dot(const Vec2& a, const Vec2& b) noexcept
{
    return a.x * b.x + a.y * b.y;
}

[[nodiscard]] inline float length(const Vec2& v) noexcept
{
    return std::sqrt(dot(v, v));
}

[[nodiscard]] inline Vec2 normalize(const Vec2& v) noexcept
{
    float len = length(v);
    return (len > 1e-8f) ? v / len : Vec2(0.0f);
}


//------------------------------------------------------------------------------------------------------------------------
//                                                    VEC3
//------------------------------------------------------------------------------------------------------------------------

struct Vec3
{
    float x = 0.0f;                              // [-] - First component
    float y = 0.0f;                              // [-] - Second component
    float z = 0.0f;                              // [-] - Third component

    constexpr Vec3() = default;
    constexpr Vec3(float s) : x(s), y(s), z(s) {}
    constexpr Vec3(float px, float py, float pz) : x(px), y(py), z(pz) {}
    constexpr Vec3(const Vec2& v, float pz) : x(v.x), y(v.y), z(pz) {}

    constexpr float& operator[](int i)       { return (&x)[i]; }
    constexpr float  operator[](int i) const { return (&x)[i]; }

    constexpr Vec3 operator+(const Vec3& r) const { return { x + r.x, y + r.y, z + r.z }; }
    constexpr Vec3 operator-(const Vec3& r) const { return { x - r.x, y - r.y, z - r.z }; }
    constexpr Vec3 operator*(float s)        const { return { x * s,   y * s,   z * s   }; }
    constexpr Vec3 operator/(float s)        const { float inv = 1.0f / s; return { x * inv, y * inv, z * inv }; }
    constexpr Vec3 operator*(const Vec3& r) const { return { x * r.x, y * r.y, z * r.z }; }
    constexpr Vec3 operator/(const Vec3& r) const { return { x / r.x, y / r.y, z / r.z }; }
    constexpr Vec3 operator-()               const { return { -x, -y, -z }; }

    constexpr Vec3& operator+=(const Vec3& r) { x += r.x; y += r.y; z += r.z; return *this; }
    constexpr Vec3& operator-=(const Vec3& r) { x -= r.x; y -= r.y; z -= r.z; return *this; }
    constexpr Vec3& operator*=(float s)       { x *= s;   y *= s;   z *= s;   return *this; }
    constexpr Vec3& operator/=(float s)       { float inv = 1.0f / s; x *= inv; y *= inv; z *= inv; return *this; }

    constexpr bool operator==(const Vec3& r) const { return x == r.x && y == r.y && z == r.z; }
    constexpr bool operator!=(const Vec3& r) const { return !(*this == r); }

    [[nodiscard]] constexpr Vec2 xy() const { return { x, y }; }
};

inline constexpr Vec3 operator*(float s, const Vec3& v) { return v * s; }

[[nodiscard]] inline constexpr float dot(const Vec3& a, const Vec3& b) noexcept
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

[[nodiscard]] inline constexpr Vec3 cross(const Vec3& a, const Vec3& b) noexcept
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

[[nodiscard]] inline float length(const Vec3& v) noexcept
{
    return std::sqrt(dot(v, v));
}

[[nodiscard]] inline Vec3 normalize(const Vec3& v) noexcept
{
    float len = length(v);
    return (len > 1e-8f) ? v / len : Vec3(0.0f);
}

[[nodiscard]] inline constexpr Vec3 mix(const Vec3& a, const Vec3& b, float t) noexcept
{
    return a * (1.0f - t) + b * t;
}


//------------------------------------------------------------------------------------------------------------------------
//                                                    VEC4
//------------------------------------------------------------------------------------------------------------------------

struct Vec4
{
    float x = 0.0f;                              // [-] - First component
    float y = 0.0f;                              // [-] - Second component
    float z = 0.0f;                              // [-] - Third component
    float w = 0.0f;                              // [-] - Fourth component

    constexpr Vec4() = default;
    constexpr Vec4(float s) : x(s), y(s), z(s), w(s) {}
    constexpr Vec4(float px, float py, float pz, float pw) : x(px), y(py), z(pz), w(pw) {}
    constexpr Vec4(const Vec3& v, float pw) : x(v.x), y(v.y), z(v.z), w(pw) {}
    constexpr Vec4(const Vec2& v, float pz, float pw) : x(v.x), y(v.y), z(pz), w(pw) {}

    constexpr float& operator[](int i)       { return (&x)[i]; }
    constexpr float  operator[](int i) const { return (&x)[i]; }

    constexpr Vec4 operator+(const Vec4& r) const { return { x + r.x, y + r.y, z + r.z, w + r.w }; }
    constexpr Vec4 operator-(const Vec4& r) const { return { x - r.x, y - r.y, z - r.z, w - r.w }; }
    constexpr Vec4 operator*(float s)        const { return { x * s,   y * s,   z * s,   w * s   }; }
    constexpr Vec4 operator/(float s)        const { float inv = 1.0f / s; return { x * inv, y * inv, z * inv, w * inv }; }
    constexpr Vec4 operator*(const Vec4& r) const { return { x * r.x, y * r.y, z * r.z, w * r.w }; }
    constexpr Vec4 operator/(const Vec4& r) const { return { x / r.x, y / r.y, z / r.z, w / r.w }; }
    constexpr Vec4 operator-()               const { return { -x, -y, -z, -w }; }

    constexpr Vec4& operator+=(const Vec4& r) { x += r.x; y += r.y; z += r.z; w += r.w; return *this; }
    constexpr Vec4& operator-=(const Vec4& r) { x -= r.x; y -= r.y; z -= r.z; w -= r.w; return *this; }
    constexpr Vec4& operator*=(float s)       { x *= s;   y *= s;   z *= s;   w *= s;   return *this; }
    constexpr Vec4& operator/=(float s)       { float inv = 1.0f / s; x *= inv; y *= inv; z *= inv; w *= inv; return *this; }

    constexpr bool operator==(const Vec4& r) const { return x == r.x && y == r.y && z == r.z && w == r.w; }
    constexpr bool operator!=(const Vec4& r) const { return !(*this == r); }

    [[nodiscard]] constexpr Vec3 xyz()  const { return { x, y, z }; }
    [[nodiscard]] constexpr Vec2 xy()   const { return { x, y }; }
};

inline constexpr Vec4 operator*(float s, const Vec4& v) { return v * s; }

[[nodiscard]] inline constexpr float dot(const Vec4& a, const Vec4& b) noexcept
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

[[nodiscard]] inline float length(const Vec4& v) noexcept
{
    return std::sqrt(dot(v, v));
}

[[nodiscard]] inline Vec4 normalize(const Vec4& v) noexcept
{
    float len = length(v);
    return (len > 1e-8f) ? v / len : Vec4(0.0f);
}
