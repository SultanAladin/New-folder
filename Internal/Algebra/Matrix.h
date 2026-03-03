// File: Source/Filament/Internal/Algebra/Matrix.h
#pragma once

/*====================================================================================================================================
                                                          MATRIX.H
====================================================================================================================================*/
// 🧩 Mat3, Mat4 — column-major matrices for the Filament math library.

#include "Vector.h"

//------------------------------------------------------------------------------------------------------------------------
//                                                    MAT3
//------------------------------------------------------------------------------------------------------------------------

struct Mat3
{
    // ⚙️ Column-major storage: cols[c][r] where c = column, r = row
    Vec3 cols[3];                                // [-] - Three column vectors

    constexpr Mat3() : cols{ {1,0,0}, {0,1,0}, {0,0,1} } {}

    constexpr Mat3(const Vec3& c0, const Vec3& c1, const Vec3& c2)
        : cols{ c0, c1, c2 }
    {}

    constexpr Mat3(float m00, float m10, float m20,
                   float m01, float m11, float m21,
                   float m02, float m12, float m22)
        : cols{ {m00, m10, m20}, {m01, m11, m21}, {m02, m12, m22} }
    {}

    constexpr Vec3& operator[](int c)       { return cols[c]; }
    constexpr const Vec3& operator[](int c) const { return cols[c]; }

    static constexpr Mat3 identity()
    {
        return Mat3();
    }

    constexpr bool operator==(const Mat3& r) const
    {
        return cols[0] == r.cols[0] && cols[1] == r.cols[1] && cols[2] == r.cols[2];
    }

    constexpr bool operator!=(const Mat3& r) const { return !(*this == r); }
};

// ⚙️ Matrix-vector multiply: M * v (column vector convention)
[[nodiscard]] inline constexpr Vec3 operator*(const Mat3& m, const Vec3& v) noexcept
{
    return m.cols[0] * v.x + m.cols[1] * v.y + m.cols[2] * v.z;
}

// ⚙️ Matrix-matrix multiply
[[nodiscard]] inline constexpr Mat3 operator*(const Mat3& a, const Mat3& b) noexcept
{
    return Mat3(a * b.cols[0], a * b.cols[1], a * b.cols[2]);
}

[[nodiscard]] inline constexpr Mat3 transpose(const Mat3& m) noexcept
{
    return Mat3(
        m.cols[0].x, m.cols[0].y, m.cols[0].z,
        m.cols[1].x, m.cols[1].y, m.cols[1].z,
        m.cols[2].x, m.cols[2].y, m.cols[2].z
    );
}

[[nodiscard]] Mat3 inverse(const Mat3& m) noexcept;


//------------------------------------------------------------------------------------------------------------------------
//                                                    MAT4
//------------------------------------------------------------------------------------------------------------------------

struct Mat4
{
    // ⚙️ Column-major storage: cols[c] is column c, each Vec4(row0, row1, row2, row3)
    Vec4 cols[4];                                // [-] - Four column vectors

    constexpr Mat4() : cols{ {1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0,0,1} } {}

    constexpr Mat4(const Vec4& c0, const Vec4& c1, const Vec4& c2, const Vec4& c3)
        : cols{ c0, c1, c2, c3 }
    {}

    constexpr Mat4(float m00, float m10, float m20, float m30,
                   float m01, float m11, float m21, float m31,
                   float m02, float m12, float m22, float m32,
                   float m03, float m13, float m23, float m33)
        : cols{
            {m00, m10, m20, m30},
            {m01, m11, m21, m31},
            {m02, m12, m22, m32},
            {m03, m13, m23, m33}
        }
    {}

    constexpr Vec4& operator[](int c)       { return cols[c]; }
    constexpr const Vec4& operator[](int c) const { return cols[c]; }

    static constexpr Mat4 identity()
    {
        return Mat4();
    }

    constexpr bool operator==(const Mat4& r) const
    {
        return cols[0] == r.cols[0] && cols[1] == r.cols[1] &&
               cols[2] == r.cols[2] && cols[3] == r.cols[3];
    }

    constexpr bool operator!=(const Mat4& r) const { return !(*this == r); }

    // ⚙️ Extract upper-left 3x3 submatrix (for normal matrix computation)
    [[nodiscard]] constexpr Mat3 extractMat3() const
    {
        return Mat3(
            cols[0].xyz(),
            cols[1].xyz(),
            cols[2].xyz()
        );
    }
};

// ⚙️ Matrix-vector multiply: M * v
[[nodiscard]] inline constexpr Vec4 operator*(const Mat4& m, const Vec4& v) noexcept
{
    return m.cols[0] * v.x + m.cols[1] * v.y + m.cols[2] * v.z + m.cols[3] * v.w;
}

// ⚙️ Matrix-matrix multiply
[[nodiscard]] inline constexpr Mat4 operator*(const Mat4& a, const Mat4& b) noexcept
{
    return Mat4(a * b.cols[0], a * b.cols[1], a * b.cols[2], a * b.cols[3]);
}

// ⚙️ Scalar multiply
[[nodiscard]] inline constexpr Mat4 operator*(const Mat4& m, float s) noexcept
{
    return Mat4(m.cols[0] * s, m.cols[1] * s, m.cols[2] * s, m.cols[3] * s);
}

[[nodiscard]] inline constexpr Mat4 transpose(const Mat4& m) noexcept
{
    return Mat4(
        m.cols[0].x, m.cols[0].y, m.cols[0].z, m.cols[0].w,
        m.cols[1].x, m.cols[1].y, m.cols[1].z, m.cols[1].w,
        m.cols[2].x, m.cols[2].y, m.cols[2].z, m.cols[2].w,
        m.cols[3].x, m.cols[3].y, m.cols[3].z, m.cols[3].w
    );
}

// ⚙️ Translation matrix construction
[[nodiscard]] inline constexpr Mat4 translationMatrix(const Vec3& t) noexcept
{
    Mat4 r;
    r.cols[3] = Vec4(t, 1.0f);
    return r;
}

// ⚙️ Uniform scale matrix construction
[[nodiscard]] inline constexpr Mat4 scaleMatrix(const Vec3& s) noexcept
{
    Mat4 r;
    r.cols[0].x = s.x;
    r.cols[1].y = s.y;
    r.cols[2].z = s.z;
    return r;
}

[[nodiscard]] Mat4 inverse(const Mat4& m) noexcept;
