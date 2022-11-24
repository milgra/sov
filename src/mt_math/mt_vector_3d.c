#ifndef mt_vector_3d_h
#define mt_vector_3d_h

#include <math.h>

typedef struct _v3_t v3_t;
struct _v3_t
{
    float x, y, z;
};

v3_t v3_init(float x, float y, float z);
v3_t v3_add(v3_t a, v3_t b);
v3_t v3_sub(v3_t a, v3_t b);
v3_t v3_scale(v3_t a, float f);
v3_t v3_cross(v3_t left, v3_t right);
v3_t v3_normalize(v3_t matrix);

v3_t v3_rotatearoundx(v3_t vector, float the_angle);
v3_t v3_rotatearoundy(v3_t vector, float the_angle);
v3_t v3_rotatearoundz(v3_t vector, float the_angle);
v3_t v3_getxyunitrotation(v3_t vx, v3_t vy);
v3_t v3_intersectwithplane(v3_t linev1, v3_t linev2, v3_t planev, v3_t planen);

float v3_dot(v3_t a, v3_t b);
float v3_angle(v3_t a, v3_t b);
float v3_length(v3_t a);
void  v3_toarray(v3_t* vector, float* result);
float v3_distance(v3_t vectorA, v3_t vectorB);

#endif
#if __INCLUDE_LEVEL__ == 0

/* inits vector3 */

v3_t v3_init(float x, float y, float z)
{
    v3_t result;

    result.x = x;
    result.y = y;
    result.z = z;

    return result;
}

/* add two vectors */

v3_t v3_add(v3_t a, v3_t b)
{
    v3_t result;

    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;

    return result;
}

/* substracts b from a */

v3_t v3_sub(v3_t a, v3_t b)
{
    v3_t result;

    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;

    return result;
}

/* scales vector */

v3_t v3_scale(v3_t a, float f)
{
    v3_t result;

    result.x = a.x * f;
    result.y = a.y * f;
    result.z = a.z * f;

    return result;
}

/* creates cross product of two vectors */

v3_t v3_cross(v3_t left, v3_t right)
{
    v3_t v;

    v.x = left.y * right.z - left.z * right.y;
    v.y = left.z * right.x - left.x * right.z;
    v.z = left.x * right.y - left.y * right.x;

    return v;
}

/* normalizes vector */

v3_t v3_normalize(v3_t matrix)
{
    float scale;

    scale = 1.0f / sqrtf(matrix.x * matrix.x + matrix.y * matrix.y + matrix.z * matrix.z);

    matrix.x *= scale;
    matrix.y *= scale;
    matrix.z *= scale;

    return matrix;
}

/* rotates vector around it x axis */

v3_t v3_rotatearoundx(v3_t vector, float the_angle)
{
    float epsilon = 0.00001;
    if (fabs(vector.y) > epsilon || fabs(vector.z) > epsilon)
    {
	float angle  = atan2(vector.z, vector.y);
	float length = sqrtf(vector.y * vector.y + vector.z * vector.z);

	vector.z = sin(angle + the_angle) * length;
	vector.y = cos(angle + the_angle) * length;
    }
    return vector;
}

/* rotates vector around it y axis */

v3_t v3_rotatearoundy(v3_t vector, float the_angle)
{
    float epsilon = 0.00001;
    if (fabs(vector.x) > epsilon || fabs(vector.z) > epsilon)
    {
	float angle  = atan2(vector.z, vector.x);
	float length = sqrtf(vector.x * vector.x + vector.z * vector.z);

	vector.z = sin(angle + the_angle) * length;
	vector.x = cos(angle + the_angle) * length;
    }
    return vector;
}

/* rotates vector around it z axis */

v3_t v3_rotatearoundz(v3_t vector, float the_angle)
{
    float epsilon = 0.00001;
    if (fabs(vector.y) > epsilon || fabs(vector.x) > epsilon)
    {
	float angle  = atan2(vector.y, vector.x);
	float length = sqrtf(vector.x * vector.x + vector.y * vector.y);

	vector.y = sin(angle + the_angle) * length;
	vector.x = cos(angle + the_angle) * length;
    }
    return vector;
}

/* rotates back plane to origo, returns rotation vector */

v3_t v3_getxyunitrotation(v3_t vx, v3_t vy)
{
    v3_t rotation;

    float angle;
    float epsilon = 0.00001;

    // rotate back final_x to base_x on Z axis

    if (fabs(vx.x) > epsilon || fabs(vx.y) > epsilon)
    {

	angle = atan2(vx.y, vx.x);

	rotation.z = angle;

	vx = v3_rotatearoundz(vx, -angle);
	vy = v3_rotatearoundz(vy, -angle);
    }
    else
	rotation.z = 0.0;

    // rotate back final_x to base_x on Y axis

    if (fabs(vx.x) > epsilon || fabs(vx.z) > epsilon)
    {

	angle = atan2(vx.z, vx.x);

	rotation.y = -angle;

	vx = v3_rotatearoundy(vx, -angle);
	vy = v3_rotatearoundy(vy, -angle);
    }
    else
	rotation.y = 0.0;

    // finally rotate back final_y to base_y on X axis

    if (fabs(vy.y) > epsilon || fabs(vy.z) > epsilon)
    {

	angle = atan2(vy.z, vy.y);

	rotation.x = angle;

	vx = v3_rotatearoundx(vx, -angle);
	vy = v3_rotatearoundx(vy, -angle);
    }
    else
	rotation.x = 0.0;

    return rotation;
}

/* intersects vector3 with plane */

v3_t v3_intersectwithplane(v3_t linev1, v3_t linev2, v3_t planev, v3_t planen)
{
    v3_t  u, w, scale, result;
    float div;

    u      = v3_sub(linev2, linev1);
    w      = v3_sub(planev, linev1);
    div    = v3_dot(planen, w) / v3_dot(planen, u);
    scale  = v3_scale(u, div);
    result = v3_add(linev1, scale);

    return result;
}

/* creates dot product of two vectors */

float v3_dot(v3_t a, v3_t b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

/* calculates angle between two vectors */

float v3_angle(v3_t a, v3_t b)
{
    return acosf(v3_dot(a, b) / (v3_length(a) * v3_length(b)));
}

/* calculates vector length */

float v3_length(v3_t a)
{
    return sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
}

/* converts vector to array */

void v3_toarray(v3_t* vector, float* result)
{
    result[0] = vector->x;
    result[1] = vector->y;
    result[2] = vector->z;
}

/* calculates distance of two vectors */

float v3_distance(v3_t vectorA, v3_t vectorB)
{
    float dx, dy, dz;

    dx = vectorB.x - vectorA.x;
    dy = vectorB.y - vectorA.y;
    dz = vectorB.z - vectorA.z;

    return sqrtf(dx * dx + dy * dy + dz * dz);
}

#endif
