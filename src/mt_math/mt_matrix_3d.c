#ifndef mt_matrix_3d_h
#define mt_matrix_3d_h

#include "mt_vector_3d.c"
#include <stdio.h>

typedef struct _m3_t m3_t;
struct _m3_t
{
    float m00, m01, m02;
    float m10, m11, m12;
    float m20, m21, m22;
};

m3_t m3_defaultidentity(void);
m3_t m3_defaultscale(float x, float y);
m3_t m3_defaulttranslation(float x, float y);
m3_t m3_defaultrotationx(float radian);
m3_t m3_defaultrotationy(float radian);
m3_t m3_defaultrotationz(float radian);
m3_t m3_multiply(m3_t a, m3_t b);
m3_t m3_invert(m3_t source, char* success);
m3_t m3_transpose(m3_t matrix);
v3_t m3_multiply_vector3(m3_t matrix, v3_t vector);
//    v3_t    m3_multiply_v3_transposed( m3_t matrix , v3_t vector );
void m3_multiplywithnumber(m3_t* matrix, float number);
void m3_describe(m3_t matrix);

#endif
#if __INCLUDE_LEVEL__ == 0

#include <math.h>

/* creates identity matrix */

m3_t m3_defaultidentity()
{
    m3_t matrix;

    matrix.m00 = 1.0f;
    matrix.m01 = 0.0f;
    matrix.m02 = 0.0f;
    matrix.m10 = 0.0f;
    matrix.m11 = 1.0f;
    matrix.m12 = 0.0f;
    matrix.m20 = 0.0f;
    matrix.m21 = 0.0f;
    matrix.m22 = 1.0f;

    return matrix;
}

/* creates identity matrix */

m3_t m3_defaultscale(float x, float y)
{
    m3_t matrix = m3_defaultidentity();

    matrix.m00 = x;
    matrix.m11 = y;

    return matrix;
}

/* creates translation matrix */

m3_t m3_defaulttranslation(float x, float y)
{
    m3_t result;

    result     = m3_defaultidentity();
    result.m02 = x;
    result.m12 = y;

    return result;
}

/* creates rotationx matrix */

m3_t m3_defaultrotationx(float rad)
{
    m3_t result;

    result     = m3_defaultidentity();
    result.m11 = cosf(rad);
    result.m12 = -sinf(rad);
    result.m21 = sinf(rad);
    result.m22 = cosf(rad);

    return result;
}

/* creates rotationy matrix */

m3_t m3_defaultrotationy(float rad)
{
    m3_t result;

    result     = m3_defaultidentity();
    result.m00 = cosf(rad);
    result.m02 = sinf(rad);
    result.m20 = -sinf(rad);
    result.m22 = cosf(rad);

    return result;
}

/* creates rotationz matrix */

m3_t m3_defaultrotationz(float rad)
{
    m3_t result;

    result     = m3_defaultidentity();
    result.m00 = cosf(rad);
    result.m01 = sinf(rad);
    result.m10 = -sinf(rad);
    result.m11 = cosf(rad);

    return result;
}

/* multiplies two matrixes */

m3_t m3_multiply(m3_t a, m3_t b)
{
    m3_t matrix;

    matrix.m00 = a.m00 * b.m00 + a.m01 * b.m10 + a.m02 * b.m20;
    matrix.m10 = a.m10 * b.m00 + a.m11 * b.m10 + a.m12 * b.m20;
    matrix.m20 = a.m20 * b.m00 + a.m21 * b.m10 + a.m22 * b.m20;

    matrix.m01 = a.m00 * b.m01 + a.m01 * b.m11 + a.m02 * b.m21;
    matrix.m11 = a.m10 * b.m01 + a.m11 * b.m11 + a.m12 * b.m21;
    matrix.m21 = a.m20 * b.m01 + a.m21 * b.m11 + a.m22 * b.m21;

    matrix.m02 = a.m00 * b.m02 + a.m01 * b.m12 + a.m02 * b.m22;
    matrix.m12 = a.m10 * b.m02 + a.m11 * b.m12 + a.m12 * b.m22;
    matrix.m22 = a.m20 * b.m02 + a.m21 * b.m12 + a.m22 * b.m22;

    return matrix;
}

/* inverts matrix */

m3_t m3_invert(m3_t source, char* success)
{
    float determinant;
    m3_t  inverse;

    inverse.m00 = source.m11 * source.m22 - source.m12 * source.m21;

    inverse.m10 = -1 * (source.m01 * source.m22 - source.m02 * source.m21);

    inverse.m20 = source.m01 * source.m12 - source.m02 * source.m11;

    inverse.m01 = -1 * (source.m10 * source.m22 - source.m12 * source.m20);

    inverse.m11 = source.m00 * source.m22 - source.m02 * source.m20;

    inverse.m21 = -1 * (source.m00 * source.m12 - source.m02 * source.m10);

    inverse.m02 = source.m10 * source.m21 - source.m11 * source.m20;

    inverse.m12 = -1 * (source.m00 * source.m21 - source.m01 * source.m20);

    inverse.m22 = source.m00 * source.m11 - source.m01 * source.m10;

    determinant = source.m00 * inverse.m00 + source.m01 * inverse.m01 +
		  source.m02 * inverse.m02;

    if (determinant == 0 && success != NULL)
	*success = 0;

    m3_multiplywithnumber(&inverse, 1.0 / determinant);

    return inverse;
}

/* transposes matrix */

m3_t m3_transpose(m3_t matrix)
{
    m3_t result;

    result.m00 = matrix.m00;
    result.m11 = matrix.m11;
    result.m22 = matrix.m22;

    result.m10 = matrix.m01;
    result.m01 = matrix.m10;
    result.m20 = matrix.m02;
    result.m02 = matrix.m20;

    result.m21 = matrix.m12;
    result.m12 = matrix.m21;

    return result;
}

/* multiplies matrix4 with vector 3 */

v3_t m3_multiply_vector3(m3_t matrix, v3_t vector)
{
    v3_t result;

    result.x =
	matrix.m00 * vector.x + matrix.m01 * vector.y + matrix.m02 * vector.z;
    result.y =
	matrix.m10 * vector.x + matrix.m11 * vector.y + matrix.m12 * vector.z;
    result.z =
	matrix.m20 * vector.x + matrix.m21 * vector.y + matrix.m22 * vector.z;

    return result;
}

//	/* multiplies transposed matrix with vector = multiplies vector with
// matrix */
//
//	v3_t m3_multiply_v3_transposed( m3_t matrix , v3_t vector )
//	{
//		v3_t result;
//
//    	result.x = matrix.m00 * vector.x + matrix.m10  * vector.y + matrix.m20 *
//    vector.z;
//		result.y = matrix.m01 * vector.x + matrix.m11  * vector.y +
// matrix.m21 * vector.z; 		result.z = matrix.m02 * vector.x +
// matrix.m12  * vector.y + matrix.m22 * vector.z;
//
//	    return result;
//	}

/* multiplies matrix with number */

void m3_multiplywithnumber(m3_t* matrix, float number)
{
    matrix->m00 *= number;
    matrix->m01 *= number;
    matrix->m02 *= number;
    matrix->m10 *= number;
    matrix->m11 *= number;
    matrix->m12 *= number;
    matrix->m20 *= number;
    matrix->m21 *= number;
    matrix->m22 *= number;
}

/* describes matrix */

void m3_describe(m3_t matrix)
{
    printf("%.2f %.2f %.2f | %.2f %.2f %.2f | %.2f %.2f %.2f", matrix.m00, matrix.m01, matrix.m02, matrix.m10, matrix.m11, matrix.m12, matrix.m20, matrix.m21, matrix.m22);
}

#endif
