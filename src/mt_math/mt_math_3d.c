#ifndef mt_math_3d_h
#define mt_math_3d_h

#include "mt_matrix_4d.c"
#include "mt_vector_3d.c"
#include "mt_vector_4d.c"
#include <math.h>

void m4_extractangles(m4_t matrix, float* x, float* y, float* z);
v4_t m4_multiply_vector4(m4_t matrix, v4_t vector);

v4_t m4_world_to_screen_coords(m4_t proj_matrix, v4_t vector, float width, float height);
v3_t m4_screen_to_world_coords(m4_t proj_matrix, v4_t vector, float width, float height);
void m4_extract(m4_t trafo, v3_t* scale, v3_t* rotation, v3_t* translation);
v3_t v4_quadrelativecoors(v4_t ulc, v4_t urc, v4_t llc, v3_t point_is);
v3_t v4_quadlineintersection(v4_t pointul, v4_t pointur, v4_t pointll, v3_t linea, v3_t lineb);

typedef union _matrix4array_t matrix4array_t;
union _matrix4array_t
{
    m4_t  matrix;
    float array[16];
};

#endif
#if __INCLUDE_LEVEL__ == 0

#include <float.h>

/* extract rotation angles from matrix */

void m4_extractangles(m4_t matrix, float* x, float* y, float* z)
{
    float y1, y2, x1, x2, z1, z2;
    x1 = x2 = y1 = y2 = z1 = z2 = 0.0;

    if (fabs(matrix.m20) != 1)
    {
	y1 = -asinf(matrix.m20);
	y2 = M_PI - y1;
	x1 = atan2(matrix.m21 / cosf(y1), matrix.m22 / cos(y1));
	x2 = atan2(matrix.m21 / cosf(y2), matrix.m22 / cos(y2));
	z1 = atan2(matrix.m10 / cosf(y1), matrix.m00 / cos(y1));
	z2 = atan2(matrix.m10 / cosf(y2), matrix.m00 / cos(y2));
    }
    else
    {
	z1 = 0;
	if (matrix.m20 == -1.0)
	{
	    y1 = M_PI / 2.0;
	    x1 = z1 + atan2(matrix.m01, matrix.m02);
	}
	else
	{
	    y1 = -M_PI / 2.0;
	    x1 = -z1 + atan2(-matrix.m01, -matrix.m02);
	}
    }
    // printf( "angles %f %f %f , %f %f %f" , x1 * 180 / M_PI , y1 * 180 / M_PI,
    // z1* 180 / M_PI , x2* 180 / M_PI , y2* 180 / M_PI , z2* 180 / M_PI );
    *x = x1;
    *y = y1;
    *z = z1;
}

/* multiplies matrix4 with vector 4 */

v4_t m4_multiply_vector4(m4_t matrix, v4_t vector)
{
    v4_t result;

    result.x = matrix.m00 * vector.x + matrix.m10 * vector.y +
	       matrix.m20 * vector.z + matrix.m30 * vector.w;
    result.y = matrix.m01 * vector.x + matrix.m11 * vector.y +
	       matrix.m21 * vector.z + matrix.m31 * vector.w;
    result.z = matrix.m02 * vector.x + matrix.m12 * vector.y +
	       matrix.m22 * vector.z + matrix.m32 * vector.w;
    result.w = matrix.m03 * vector.x + matrix.m13 * vector.y +
	       matrix.m23 * vector.z + matrix.m33 * vector.w;

    return result;
}

/* projects model space vector4 to screen space */

v4_t m4_world_to_screen_coords(m4_t matrix, v4_t srcvector, float width, float height)
{
    v4_t vector;
    vector.x = srcvector.x;
    vector.y = srcvector.y;
    vector.z = 0;
    vector.w = 1;

    vector = m4_multiply_vector4(matrix, vector);

    if (vector.w == 0)
	return vector;

    // perspective divide to normalized device coordinates

    vector.x /= vector.w;
    vector.y /= vector.w;
    vector.z /= vector.w;

    // viewport transformation

    v4_t result;
    result.x = (vector.x + 1.0) * width * 0.5;
    result.y = (vector.y + 1.0) * height * 0.5;
    result.z = vector.z;
    result.w = vector.w;

    return result;
}

/* projects screen space vector4 to model space */

v3_t m4_screen_to_world_coords(m4_t mvpmatrix, v4_t scrvector, float width, float height)
{
    // get normalized device coordinates
    // ( src.x - ( width / 2.0 ) ) / ( width / 2.0 )
    // ( src.y - ( height / 2.0 ) ) / ( height / 2.0 )

    v4_t vector;
    vector.x = scrvector.x / width * 2.0 - 1.0;
    vector.y = scrvector.y / height * 2.0 - 1.0;
    vector.z = scrvector.z;
    vector.w = 1.0;

    // invert projection matrix

    char success = 1;
    m4_t invert  = m4_invert(mvpmatrix, &success);

    // multiply transposed inverted projection matrix with vector

    v4_t result = m4_multiply_vector4(invert, vector);

    if (result.w == 0)
	return v3_init(FLT_MAX, FLT_MAX, FLT_MAX);

    // get world space coordinates

    result.w = 1.0 / result.w;
    result.x *= result.w;
    result.y *= result.w;
    result.z *= result.w;

    return v3_init(result.x, result.y, result.z);
}

/* extracts data from matrix4 */

void m4_extract(m4_t trafo, v3_t* scale, v3_t* rotation, v3_t* translation)
{
    v4_t base_o = v4_init(0, 0, 0, 1);
    v4_t base_x = v4_init(1, 0, 0, 1);
    v4_t base_y = v4_init(0, 1, 0, 1);

    base_o = m4_multiply_vector4(trafo, base_o);
    base_x = m4_multiply_vector4(trafo, base_x);
    base_y = m4_multiply_vector4(trafo, base_y);

    v3_t final_o = v3_init(base_o.x, base_o.y, base_o.z);
    v3_t final_x = v3_init(base_x.x, base_x.y, base_x.z);
    v3_t final_y = v3_init(base_y.x, base_y.y, base_y.z);

    translation->x = final_o.x;
    translation->y = final_o.y;
    translation->z = final_o.z;

    final_x = v3_sub(final_x, final_o);
    final_y = v3_sub(final_y, final_o);

    float scale_factor = v3_length(final_x);

    scale->x = scale_factor;
    scale->y = scale_factor;
    scale->z = scale_factor;

    *rotation = v3_getxyunitrotation(final_x, final_y);
}

/* returns point_is coordiantes in the local coordinate system of the quad
 * described by ulc, urc, llc */

v3_t v4_quadrelativecoors(v4_t ulc, v4_t urc, v4_t llc, v3_t point_is)
{
    v3_t plane_a, plane_b, plane_d;
    v3_t vec_ab, vec_ad, vec_n, vec_ai;

    plane_a = v3_init(ulc.x, ulc.y, ulc.z);
    plane_b = v3_init(llc.x, llc.y, llc.z);
    plane_d = v3_init(urc.x, urc.y, urc.z);

    // create plane vectors and normal

    vec_ab = v3_sub(plane_b, plane_a);
    vec_ad = v3_sub(plane_d, plane_a);
    vec_n  = v3_cross(vec_ab, vec_ad);

    // get angle of AI from AB and AC to build up the frame square in its actual
    // position

    vec_ai = v3_sub(point_is, plane_a);

    float angle_ab_ai   = v3_angle(vec_ab, vec_ai);
    float angle_ad_ai   = v3_angle(vec_ad, vec_ai);
    float length_vec_ai = v3_length(vec_ai);

    // get relative coordinates

    v3_t relative;
    relative.x = cos(angle_ad_ai) * length_vec_ai;
    relative.y = -sin(angle_ad_ai) * length_vec_ai;

    // check quarters

    if (angle_ab_ai > M_PI / 2 && angle_ad_ai > M_PI / 2)
	relative.y *= -1;
    else if (angle_ab_ai > M_PI / 2 && angle_ad_ai < M_PI / 2)
	relative.y *= -1;

    if (relative.x > 0.0 && relative.x < v3_length(vec_ad) &&
	relative.y < 0.0 && relative.y > -v3_length(vec_ab))
    {
	return relative;
    }
    else
	return v3_init(FLT_MAX, FLT_MAX, FLT_MAX);
}

/* calculates intersection point of quad and line */

v3_t v4_quadlineintersection(v4_t ulc, v4_t urc, v4_t llc, v3_t linea, v3_t lineb)
{
    v3_t plane_a, plane_b, plane_d;
    v3_t vec_ab, vec_ad, vec_n, vec_ai, point_is;

    plane_a = v3_init(ulc.x, ulc.y, ulc.z);
    plane_b = v3_init(llc.x, llc.y, llc.z);
    plane_d = v3_init(urc.x, urc.y, urc.z);

    // create plane vectors and normal

    vec_ab = v3_sub(plane_b, plane_a);
    vec_ad = v3_sub(plane_d, plane_a);
    vec_n  = v3_cross(vec_ab, vec_ad);

    // get intersection point

    point_is = v3_intersectwithplane(linea, lineb, plane_a, vec_n);

    // get angle of AI from AB and AC to build up the frame square in its actual
    // position

    vec_ai = v3_sub(point_is, plane_a);

    float angle_ab_ai   = v3_angle(vec_ab, vec_ai);
    float angle_ad_ai   = v3_angle(vec_ad, vec_ai);
    float length_vec_ai = v3_length(vec_ai);

    // get relative coordinates

    float x = cos(angle_ad_ai) * length_vec_ai;
    float y = -sin(angle_ad_ai) * length_vec_ai;

    // check quarters

    if (angle_ab_ai > M_PI / 2 && angle_ad_ai > M_PI / 2)
	y *= -1;
    else if (angle_ab_ai > M_PI / 2 && angle_ad_ai < M_PI / 2)
	y *= -1;

    if (x > 0.0 && x < v3_length(vec_ad) && y < 0.0 && y > -v3_length(vec_ab))
	return point_is;
    else
	return v3_init(FLT_MAX, FLT_MAX, FLT_MAX);
}

#endif
