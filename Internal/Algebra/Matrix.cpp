// File: Source/Filament/Internal/Algebra/Matrix.cpp

/*====================================================================================================================================
                                                          MATRIX.CPP
====================================================================================================================================*/
// 🧩 Mat3 and Mat4 inverse implementations.

#include "Matrix.h"

//------------------------------------------------------------------------------------------------------------------------
//                                                    MAT3 INVERSE
//------------------------------------------------------------------------------------------------------------------------

Mat3 inverse(const Mat3& m) noexcept
{
    // ⚙️ Cofactor expansion for 3x3 inverse
    const Vec3& c0 = m.cols[0];
    const Vec3& c1 = m.cols[1];
    const Vec3& c2 = m.cols[2];

    Vec3 r0 = cross(c1, c2);
    Vec3 r1 = cross(c2, c0);
    Vec3 r2 = cross(c0, c1);

    float invDet = 1.0f / dot(c0, r0);

    return Mat3(
        r0.x * invDet, r1.x * invDet, r2.x * invDet,
        r0.y * invDet, r1.y * invDet, r2.y * invDet,
        r0.z * invDet, r1.z * invDet, r2.z * invDet
    );
}


//------------------------------------------------------------------------------------------------------------------------
//                                                    MAT4 INVERSE
//------------------------------------------------------------------------------------------------------------------------

Mat4 inverse(const Mat4& m) noexcept
{
    // ⚙️ Cramer's rule / cofactor expansion for general 4x4 inverse
    //    Accessing column-major: m[col][row]
    float a00 = m[0][0], a01 = m[0][1], a02 = m[0][2], a03 = m[0][3];
    float a10 = m[1][0], a11 = m[1][1], a12 = m[1][2], a13 = m[1][3];
    float a20 = m[2][0], a21 = m[2][1], a22 = m[2][2], a23 = m[2][3];
    float a30 = m[3][0], a31 = m[3][1], a32 = m[3][2], a33 = m[3][3];

    // ① Compute 2x2 determinant pairs
    float s0 = a00 * a11 - a10 * a01;
    float s1 = a00 * a12 - a10 * a02;
    float s2 = a00 * a13 - a10 * a03;
    float s3 = a01 * a12 - a11 * a02;
    float s4 = a01 * a13 - a11 * a03;
    float s5 = a02 * a13 - a12 * a03;

    float c5 = a22 * a33 - a32 * a23;
    float c4 = a21 * a33 - a31 * a23;
    float c3 = a21 * a32 - a31 * a22;
    float c2 = a20 * a33 - a30 * a23;
    float c1 = a20 * a32 - a30 * a22;
    float c0 = a20 * a31 - a30 * a21;

    // ② Compute determinant
    float det = s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0;
    float invDet = 1.0f / det;

    // ③ Compute adjugate and multiply by 1/det
    Mat4 result;

    result[0][0] = ( a11 * c5 - a12 * c4 + a13 * c3) * invDet;
    result[0][1] = (-a01 * c5 + a02 * c4 - a03 * c3) * invDet;
    result[0][2] = ( a31 * s5 - a32 * s4 + a33 * s3) * invDet;
    result[0][3] = (-a21 * s5 + a22 * s4 - a23 * s3) * invDet;

    result[1][0] = (-a10 * c5 + a12 * c2 - a13 * c1) * invDet;
    result[1][1] = ( a00 * c5 - a02 * c2 + a03 * c1) * invDet;
    result[1][2] = (-a30 * s5 + a32 * s2 - a33 * s1) * invDet;
    result[1][3] = ( a20 * s5 - a22 * s2 + a23 * s1) * invDet;

    result[2][0] = ( a10 * c4 - a11 * c2 + a13 * c0) * invDet;
    result[2][1] = (-a00 * c4 + a01 * c2 - a03 * c0) * invDet;
    result[2][2] = ( a30 * s4 - a31 * s2 + a33 * s0) * invDet;
    result[2][3] = (-a20 * s4 + a21 * s2 - a23 * s0) * invDet;

    result[3][0] = (-a10 * c3 + a11 * c1 - a12 * c0) * invDet;
    result[3][1] = ( a00 * c3 - a01 * c1 + a02 * c0) * invDet;
    result[3][2] = (-a30 * s3 + a31 * s1 - a32 * s0) * invDet;
    result[3][3] = ( a20 * s3 - a21 * s1 + a22 * s0) * invDet;

    return result;
}
