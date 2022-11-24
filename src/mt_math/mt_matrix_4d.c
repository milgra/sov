#ifndef mt_matrix_4d_h
#define mt_matrix_4d_h

#include "mt_matrix_3d.c"

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#ifndef M_PI_2
    #define M_PI_2 3.14159265358979323846 * 2
#endif

typedef struct _m4_t m4_t;
struct _m4_t
{
    float m00, m01, m02, m03;
    float m10, m11, m12, m13;
    float m20, m21, m22, m23;
    float m30, m31, m32, m33;
};

m4_t m4_defaultidentity(void);
m4_t m4_defaultscale(float x, float y, float z);
m4_t m4_defaultrotation(float x, float y, float z);
m4_t m4_defaulttranslation(float x, float y, float z);
m4_t m4_defaultortho(float left, float right, float bottom, float top, float near, float far);
m4_t m4_defaultperspective(float fovy, float aspect, float nearz, float farz);
m4_t m4_scale(m4_t matrix, float x, float y, float z);
m4_t m4_rotate(m4_t matrix, float x, float y, float z);
m4_t m4_translate(m4_t other, float x, float y, float z);
m4_t m4_invert(m4_t source, char* success);
m4_t m4_multiply(m4_t a, m4_t b);
m4_t m4_transpose(m4_t matrix);
void m4_describe(m4_t matrix);

#endif
#if __INCLUDE_LEVEL__ == 0

#include <math.h>
#include <stdio.h>

/* internal */

void m4_multiplywithnumber(m4_t* matrix, float number)
{
    matrix->m00 *= number;
    matrix->m01 *= number;
    matrix->m02 *= number;
    matrix->m03 *= number;
    matrix->m10 *= number;
    matrix->m11 *= number;
    matrix->m12 *= number;
    matrix->m13 *= number;
    matrix->m20 *= number;
    matrix->m21 *= number;
    matrix->m22 *= number;
    matrix->m23 *= number;
    matrix->m30 *= number;
    matrix->m31 *= number;
    matrix->m32 *= number;
    matrix->m33 *= number;
}

/* creates identity matrix */

m4_t m4_defaultidentity()
{
    m4_t matrix;

    matrix.m00 = 1.0f;
    matrix.m01 = 0.0f;
    matrix.m02 = 0.0f;
    matrix.m03 = 0.0f;
    matrix.m10 = 0.0f;
    matrix.m11 = 1.0f;
    matrix.m12 = 0.0f;
    matrix.m13 = 0.0f;
    matrix.m20 = 0.0f;
    matrix.m21 = 0.0f;
    matrix.m22 = 1.0f;
    matrix.m23 = 0.0f;
    matrix.m30 = 0.0f;
    matrix.m31 = 0.0f;
    matrix.m32 = 0.0f;
    matrix.m33 = 1.0f;

    return matrix;
}

/* creates scale matrix */

m4_t m4_defaultscale(float x, float y, float z)
{
    m4_t matrix = m4_defaultidentity();

    matrix.m00 = x;
    matrix.m11 = y;
    matrix.m22 = z;

    return matrix;
}

/* creates rotation matrix */

m4_t m4_defaultrotation(float x, float y, float z)
{
    float max = fabs(x) > fabs(y) ? x : y;
    max       = fabs(z) > fabs(max) ? z : max;

    if (max == 0.0)
	return m4_defaultidentity();

    x = x / max;
    y = y / max;
    z = z / max;

    float nx, ny, nz, scale, sin, cos, cosp;
    m4_t  matrix;

    // normalize values

    nx = x;
    ny = y;
    nz = z;

    scale = 1.0f / sqrtf(nx * nx + ny * ny + nz * nz);

    nx *= scale;
    ny *= scale;
    nz *= scale;

    // precalc

    sin  = sinf(max);
    cos  = cosf(max);
    cosp = 1.0f - cos;

    // create matrix

    matrix.m00 = cos + cosp * nx * nx;
    matrix.m01 = cosp * nx * ny + nz * sin;
    matrix.m02 = cosp * nx * nz - ny * sin;
    matrix.m03 = 0.0f;
    matrix.m10 = cosp * nx * ny - nz * sin;
    matrix.m11 = cos + cosp * ny * ny;
    matrix.m12 = cosp * ny * nz + nx * sin;
    matrix.m13 = 0.0f;
    matrix.m20 = cosp * nx * nz + ny * sin;
    matrix.m21 = cosp * ny * nz - nx * sin;
    matrix.m22 = cos + cosp * nz * nz;
    matrix.m23 = 0.0f;
    matrix.m30 = 0.0f;
    matrix.m31 = 0.0f;
    matrix.m32 = 0.0f;
    matrix.m33 = 1.0f;

    return matrix;
}

/* creates translation matrix */

m4_t m4_defaulttranslation(float x, float y, float z)
{
    m4_t result;

    result     = m4_defaultidentity();
    result.m00 = 1;

    result.m30 = x;
    result.m31 = y;
    result.m32 = z;

    return result;
}

/* creates ortographic projection */

m4_t m4_defaultortho(float left, float right, float bottom, float top, float near, float far)
{
    float rpl, rml, tpb, tmb, fpn, fmn;
    m4_t  matrix;

    rpl = right + left;
    rml = right - left;
    tpb = top + bottom;
    tmb = top - bottom;
    fpn = far + near;
    fmn = far - near;

    matrix.m00 = 2.0f / rml;
    matrix.m01 = 0.0f;
    matrix.m02 = 0.0f;
    matrix.m03 = 0.0f;
    matrix.m10 = 0.0f;
    matrix.m11 = 2.0f / tmb;
    matrix.m12 = 0.0f;
    matrix.m13 = 0.0f;
    matrix.m20 = 0.0f;
    matrix.m21 = 0.0f;
    matrix.m22 = -2.0f / fmn;
    matrix.m23 = 0.0f;
    matrix.m30 = -rpl / rml;
    matrix.m31 = -tpb / tmb;
    matrix.m32 = -fpn / fmn;
    matrix.m33 = 1.0f;

    return matrix;
}

/* create perspective projection */

m4_t m4_defaultperspective(float fovy, float aspect, float nearz, float farz)
{
    float cotan;
    m4_t  matrix;

    cotan = 1.0f / tanf(fovy / 2.0f);

    matrix.m00 = cotan / aspect;
    matrix.m01 = 0.0f;
    matrix.m02 = 0.0f;
    matrix.m03 = 0.0f;

    matrix.m10 = 0.0f;
    matrix.m11 = cotan;
    matrix.m12 = 0.0f;
    matrix.m13 = 0.0f;

    matrix.m20 = 0.0f;
    matrix.m21 = 0.0f;
    matrix.m22 = (farz + nearz) / (nearz - farz);
    matrix.m23 = -1.0f;

    matrix.m30 = 0.0f;
    matrix.m31 = 0.0f;
    matrix.m32 = (2.0f * farz * nearz) / (nearz - farz);
    matrix.m33 = 0.0f;

    return matrix;
}

/* scales matrix */

m4_t m4_scale(m4_t matrix, float x, float y, float z)
{
    matrix.m00 *= x;
    matrix.m11 *= y;
    matrix.m22 *= z;

    return matrix;
}

/* rotates matrix */

m4_t m4_rotate(m4_t matrix, float x, float y, float z)
{
    m4_t rotation;

    rotation = m4_defaultrotation(x, y, z);
    return m4_multiply(matrix, rotation);
}

/* translates matrix */

m4_t m4_translate(m4_t other, float x, float y, float z)
{
    other.m30 = other.m00 * x + other.m10 * y + other.m20 * z + other.m30;
    other.m31 = other.m01 * x + other.m11 * y + other.m21 * z + other.m31;
    other.m32 = other.m02 * x + other.m12 * y + other.m22 * z + other.m32;

    return other;
}

/* inverts matrix */

m4_t m4_invert(m4_t source, char* success)
{
    float determinant;
    m4_t  inverse;

    inverse.m00 = source.m11 * source.m22 * source.m33 -
		  source.m11 * source.m23 * source.m32 -
		  source.m21 * source.m12 * source.m33 +
		  source.m21 * source.m13 * source.m32 +
		  source.m31 * source.m12 * source.m23 -
		  source.m31 * source.m13 * source.m22;

    inverse.m10 = -source.m10 * source.m22 * source.m33 +
		  source.m10 * source.m23 * source.m32 +
		  source.m20 * source.m12 * source.m33 -
		  source.m20 * source.m13 * source.m32 -
		  source.m30 * source.m12 * source.m23 +
		  source.m30 * source.m13 * source.m22;

    inverse.m20 = source.m10 * source.m21 * source.m33 -
		  source.m10 * source.m23 * source.m31 -
		  source.m20 * source.m11 * source.m33 +
		  source.m20 * source.m13 * source.m31 +
		  source.m30 * source.m11 * source.m23 -
		  source.m30 * source.m13 * source.m21;

    inverse.m30 = -source.m10 * source.m21 * source.m32 +
		  source.m10 * source.m22 * source.m31 +
		  source.m20 * source.m11 * source.m32 -
		  source.m20 * source.m12 * source.m31 -
		  source.m30 * source.m11 * source.m22 +
		  source.m30 * source.m12 * source.m21;

    inverse.m01 = -source.m01 * source.m22 * source.m33 +
		  source.m01 * source.m23 * source.m32 +
		  source.m21 * source.m02 * source.m33 -
		  source.m21 * source.m03 * source.m32 -
		  source.m31 * source.m02 * source.m23 +
		  source.m31 * source.m03 * source.m22;

    inverse.m11 = source.m00 * source.m22 * source.m33 -
		  source.m00 * source.m23 * source.m32 -
		  source.m20 * source.m02 * source.m33 +
		  source.m20 * source.m03 * source.m32 +
		  source.m30 * source.m02 * source.m23 -
		  source.m30 * source.m03 * source.m22;

    inverse.m21 = -source.m00 * source.m21 * source.m33 +
		  source.m00 * source.m23 * source.m31 +
		  source.m20 * source.m01 * source.m33 -
		  source.m20 * source.m03 * source.m31 -
		  source.m30 * source.m01 * source.m23 +
		  source.m30 * source.m03 * source.m21;

    inverse.m31 = source.m00 * source.m21 * source.m32 -
		  source.m00 * source.m22 * source.m31 -
		  source.m20 * source.m01 * source.m32 +
		  source.m20 * source.m02 * source.m31 +
		  source.m30 * source.m01 * source.m22 -
		  source.m30 * source.m02 * source.m21;

    inverse.m02 = source.m01 * source.m12 * source.m33 -
		  source.m01 * source.m13 * source.m32 -
		  source.m11 * source.m02 * source.m33 +
		  source.m11 * source.m03 * source.m32 +
		  source.m31 * source.m02 * source.m13 -
		  source.m31 * source.m03 * source.m12;

    inverse.m12 = -source.m00 * source.m12 * source.m33 +
		  source.m00 * source.m13 * source.m32 +
		  source.m10 * source.m02 * source.m33 -
		  source.m10 * source.m03 * source.m32 -
		  source.m30 * source.m02 * source.m13 +
		  source.m30 * source.m03 * source.m12;

    inverse.m22 = source.m00 * source.m11 * source.m33 -
		  source.m00 * source.m13 * source.m31 -
		  source.m10 * source.m01 * source.m33 +
		  source.m10 * source.m03 * source.m31 +
		  source.m30 * source.m01 * source.m13 -
		  source.m30 * source.m03 * source.m11;

    inverse.m32 = -source.m00 * source.m11 * source.m32 +
		  source.m00 * source.m12 * source.m31 +
		  source.m10 * source.m01 * source.m32 -
		  source.m10 * source.m02 * source.m31 -
		  source.m30 * source.m01 * source.m12 +
		  source.m30 * source.m02 * source.m11;

    inverse.m03 = -source.m01 * source.m12 * source.m23 +
		  source.m01 * source.m13 * source.m22 +
		  source.m11 * source.m02 * source.m23 -
		  source.m11 * source.m03 * source.m22 -
		  source.m21 * source.m02 * source.m13 +
		  source.m21 * source.m03 * source.m12;

    inverse.m13 = source.m00 * source.m12 * source.m23 -
		  source.m00 * source.m13 * source.m22 -
		  source.m10 * source.m02 * source.m23 +
		  source.m10 * source.m03 * source.m22 +
		  source.m20 * source.m02 * source.m13 -
		  source.m20 * source.m03 * source.m12;

    inverse.m23 = -source.m00 * source.m11 * source.m23 +
		  source.m00 * source.m13 * source.m21 +
		  source.m10 * source.m01 * source.m23 -
		  source.m10 * source.m03 * source.m21 -
		  source.m20 * source.m01 * source.m13 +
		  source.m20 * source.m03 * source.m11;

    inverse.m33 = source.m00 * source.m11 * source.m22 -
		  source.m00 * source.m12 * source.m21 -
		  source.m10 * source.m01 * source.m22 +
		  source.m10 * source.m02 * source.m21 +
		  source.m20 * source.m01 * source.m12 -
		  source.m20 * source.m02 * source.m11;

    determinant = source.m00 * inverse.m00 + source.m01 * inverse.m10 +
		  source.m02 * inverse.m20 + source.m03 * inverse.m30;

    if (determinant == 0 && success != NULL)
	*success = 0;

    m4_multiplywithnumber(&inverse, 1.0 / determinant);

    return inverse;
}

/* multiplies matrices */

m4_t m4_multiply(m4_t a, m4_t b)
{
    m4_t matrix;

    matrix.m00 = a.m00 * b.m00 + a.m10 * b.m01 + a.m20 * b.m02 + a.m30 * b.m03;
    matrix.m10 = a.m00 * b.m10 + a.m10 * b.m11 + a.m20 * b.m12 + a.m30 * b.m13;
    matrix.m20 = a.m00 * b.m20 + a.m10 * b.m21 + a.m20 * b.m22 + a.m30 * b.m23;
    matrix.m30 = a.m00 * b.m30 + a.m10 * b.m31 + a.m20 * b.m32 + a.m30 * b.m33;

    matrix.m01 = a.m01 * b.m00 + a.m11 * b.m01 + a.m21 * b.m02 + a.m31 * b.m03;
    matrix.m11 = a.m01 * b.m10 + a.m11 * b.m11 + a.m21 * b.m12 + a.m31 * b.m13;
    matrix.m21 = a.m01 * b.m20 + a.m11 * b.m21 + a.m21 * b.m22 + a.m31 * b.m23;
    matrix.m31 = a.m01 * b.m30 + a.m11 * b.m31 + a.m21 * b.m32 + a.m31 * b.m33;

    matrix.m02 = a.m02 * b.m00 + a.m12 * b.m01 + a.m22 * b.m02 + a.m32 * b.m03;
    matrix.m12 = a.m02 * b.m10 + a.m12 * b.m11 + a.m22 * b.m12 + a.m32 * b.m13;
    matrix.m22 = a.m02 * b.m20 + a.m12 * b.m21 + a.m22 * b.m22 + a.m32 * b.m23;
    matrix.m32 = a.m02 * b.m30 + a.m12 * b.m31 + a.m22 * b.m32 + a.m32 * b.m33;

    matrix.m03 = a.m03 * b.m00 + a.m13 * b.m01 + a.m23 * b.m02 + a.m33 * b.m03;
    matrix.m13 = a.m03 * b.m10 + a.m13 * b.m11 + a.m23 * b.m12 + a.m33 * b.m13;
    matrix.m23 = a.m03 * b.m20 + a.m13 * b.m21 + a.m23 * b.m22 + a.m33 * b.m23;
    matrix.m33 = a.m03 * b.m30 + a.m13 * b.m31 + a.m23 * b.m32 + a.m33 * b.m33;

    return matrix;
}

/* transposes matrix */

m4_t m4_transpose(m4_t matrix)
{
    m4_t result;

    result.m00 = matrix.m00;
    result.m11 = matrix.m11;
    result.m22 = matrix.m22;
    result.m33 = matrix.m33;

    result.m10 = matrix.m01;
    result.m01 = matrix.m10;
    result.m20 = matrix.m02;
    result.m02 = matrix.m20;
    result.m30 = matrix.m03;
    result.m03 = matrix.m30;

    result.m21 = matrix.m12;
    result.m12 = matrix.m21;
    result.m31 = matrix.m13;
    result.m13 = matrix.m31;
    result.m32 = matrix.m23;
    result.m23 = matrix.m32;

    return result;
}

/* describes matrix */

void m4_describe(m4_t matrix)
{
    printf(
	"%.2f %.2f %.2f %.2f | %.2f %.2f %.2f %.2f | %.2f %.2f %.2f %.2f | "
	"%.2f %.2f %.2f %.2f",
	matrix.m00,
	matrix.m01,
	matrix.m02,
	matrix.m03,
	matrix.m10,
	matrix.m11,
	matrix.m12,
	matrix.m13,
	matrix.m20,
	matrix.m21,
	matrix.m22,
	matrix.m23,
	matrix.m30,
	matrix.m31,
	matrix.m32,
	matrix.m33);
}

#endif
