#ifndef mt_math_2d_h
#define mt_math_2d_h

#include "mt_vector_2d.c"
#include <float.h>
#include <stdint.h>

typedef struct _r2_t r2_t;
struct _r2_t
{
    float x;
    float y;
    float w;
    float h;
};

typedef struct _r2i_t r2i_t;
struct _r2i_t
{
    int x;
    int y;
    int w;
    int h;
};

typedef struct _segment2_t segment2_t;
struct _segment2_t
{
    v2_t trans;
    v2_t basis;
};

v2_t  v2_mirror(v2_t axis, v2_t vector);
v2_t  v2_intersect_lines(v2_t transa, v2_t basisa, v2_t transb, v2_t basisb);
char  v2_point_inside_vector(v2_t transa, v2_t basisa, v2_t point);
v2_t  v2_intersect_vectors(v2_t transa, v2_t basisa, v2_t transb, v2_t basisb);
char  v2_box_intersect(v2_t basisa, v2_t transa, v2_t basisb, v2_t transb, float extra_distance);
float v2_endpoint_proximity(v2_t transa, v2_t basisa, v2_t transb, v2_t basisb);
v2_t  v2_intersect_with_proximity(v2_t trans_a, v2_t basis_a, v2_t trans_b, v2_t basis_b, float proximity);
v2_t  v2_intersect_with_nearby(v2_t trans_a, v2_t basis_a, v2_t trans_b, v2_t basis_b, float proximity);
v2_t  v2_triangle_with_bases(v2_t point_a, v2_t point_b, float segmentlength, int8_t direction);

segment2_t v2_collide_and_fragment(v2_t transa, v2_t basisa, v2_t transb, v2_t basisb);
segment2_t segment2_init(v2_t trans, v2_t basis);

r2_t r2_expand(r2_t rect, float d);
int  r2_equals(r2_t r1, r2_t r2);

#define kSquareOverlappingNone 0
#define kSquareOverlappingInside 1
#define kSquareOverlappingPartial 2

typedef struct square2_t square2_t;
struct square2_t
{
    v2_t origo;
    v2_t extent;
};

square2_t square_alloc(v2_t origo, v2_t extent);
square2_t square_intersect(square2_t squarea, square2_t squareb);
uint8_t   square_checkoverlapping(square2_t squarea, square2_t squareb);

#endif

#if __INCLUDE_LEVEL__ == 0

/* intersects lines by linear equation
   Ax + By = C
   A = y2 - y1
   B = x1 - x2
   C = A * x1 + B * y1 */

v2_t v2_intersect_lines(v2_t transa, v2_t basisa, v2_t transb, v2_t basisb)
{
    if (!(fabs(basisa.x) > 0.0 || fabs(basisa.y) > 0.0))
	return v2_init(FLT_MAX, FLT_MAX);
    if (!(fabs(basisb.x) > 0.0 || fabs(basisb.y) > 0.0))
	return v2_init(FLT_MAX, FLT_MAX);

    float aA = basisa.y;
    float aB = -basisa.x;

    float bA = basisb.y;
    float bB = -basisb.x;

    float determinant = bA * aB - bB * aA;

    if (determinant != 0)
    {
	float aC = aA * transa.x + aB * transa.y;
	float bC = bA * transb.x + bB * transb.y;

	float x = (aB * bC - bB * aC) / determinant;
	float y = (bA * aC - aA * bC) / determinant;

	return v2_init(x, y);
    }

    /* lines are parallel */

    return v2_init(FLT_MAX, FLT_MAX);
}

/* mirrors vector on axis */

v2_t v2_mirror(v2_t axis, v2_t vector)
{
    v2_t projected_point = v2_intersect_lines(
	v2_init(0.0, 0.0), axis, vector, v2_rotate_90_right(axis));

    v2_t projected_vector = v2_sub(projected_point, vector);
    return v2_add(projected_point, projected_vector);
}

/* checking if point is inside a vector, the point must be on the line defined
 * by the vector */

char v2_point_inside_vector(v2_t transa, v2_t basisa, v2_t point)
{
    float dx = point.x - transa.x;
    float dy = point.y - transa.y;

    // check sign similarity

    if (basisa.x < 0 && dx >= 0)
	return 0;
    if (basisa.x > 0 && dx < 0)
	return 0;
    if (basisa.y < 0 && dy >= 0)
	return 0;
    if (basisa.y > 0 && dy < 0)
	return 0;

    float absdx = fabs(dx);
    float absdy = fabs(dy);
    float absbx = fabs(basisa.x);
    float absby = fabs(basisa.y);

    // check length

    if (absbx == 0.0)
    {
	if (absdx > 0.001)
	    return 0;
    }
    else if (absbx < absdx + 0.001)
	return 0;

    if (absby == 0.0)
    {
	if (absdy > 0.001)
	    return 0;
    }
    else if (absby < absdy + 0.001)
	return 0;

    return 1;
}

/* intersect vectors */

v2_t v2_intersect_vectors(v2_t transa, v2_t basisa, v2_t transb, v2_t basisb)
{
    v2_t isp_inertia_segment = v2_intersect_lines(transa, basisa, transb, basisb);

    /* lines are not parallel */

    if (isp_inertia_segment.x != FLT_MAX && isp_inertia_segment.y != FLT_MAX)
    {
	if (v2_point_inside_vector(transa, basisa, isp_inertia_segment) == 1 &&
	    v2_point_inside_vector(transb, basisb, isp_inertia_segment) == 1)
	{
	    return isp_inertia_segment;
	}
    }
    return v2_init(FLT_MAX, FLT_MAX);
}

// checking collosion of vector bounding boxes

char v2_box_intersect(v2_t basisa, v2_t transa, v2_t basisb, v2_t transb, float extra_distance)
{
    // bounding box checking
    float dcx = fabs((transa.x + basisa.x / 2.0) - (transb.x + basisb.x / 2.0));
    float dcy = fabs((transa.y + basisa.y / 2.0) - (transb.y + basisb.y / 2.0));

    float maxx = fabs(basisa.x / 2.0) + fabs(basisb.x / 2.0) + extra_distance;
    float maxy = fabs(basisa.y / 2.0) + fabs(basisb.y / 2.0) + extra_distance;

    if (dcx < maxx && dcy < maxy)
	return 1;
    return 0;
}

float v2_endpoint_proximity(v2_t transa, v2_t basisa, v2_t transb, v2_t basisb)
{
    // line intersection point
    v2_t isp_inertia_segment = v2_intersect_lines(transa, basisa, transb, basisb);

    // lines are not parallel
    if (isp_inertia_segment.x != FLT_MAX && isp_inertia_segment.y != FLT_MAX)
    {
	v2_t halfa = v2_add(transa, v2_scale(basisa, 0.5));
	v2_t halfb = v2_add(transb, v2_scale(basisb, 0.5));

	v2_t vectora = v2_sub(halfa, isp_inertia_segment);
	v2_t vectorb = v2_sub(halfb, isp_inertia_segment);

	float asize   = v2_length(basisa) / 2.0;
	float bsize   = v2_length(basisb) / 2.0;
	float alength = v2_length(vectora);
	float blength = v2_length(vectorb);

	alength -= asize;
	blength -= bsize;

	if (alength < 0.0)
	    alength = 0.0;
	if (blength < 0.0)
	    blength = 0.0;

	return alength > blength ? alength : blength;
    }
    else
    {
	float dx = fabs(transa.x - transb.x);
	float dy = fabs(transa.y - transb.y);
	return dx > dy ? dx : dy;
    }
}

v2_t v2_intersect_with_proximity(v2_t trans_a, v2_t basis_a, v2_t trans_b, v2_t basis_b, float proximity)
{
    v2_t isp = v2_intersect_vectors(trans_a, basis_a, trans_b, basis_b);

    if (isp.x == FLT_MAX && proximity > 0.0)
    {
	float distance = v2_endpoint_proximity(trans_a, basis_a, trans_b, basis_b);
	if (distance < proximity)
	    isp = v2_intersect_lines(trans_a, basis_a, trans_b, basis_b);
    }

    return isp;
}

float v2_endpoint_nearby(v2_t transa, v2_t basisa, v2_t transb, v2_t basisb)
{
    // line intersection point
    v2_t isp_inertia_segment = v2_intersect_lines(transa, basisa, transb, basisb);

    // lines are not parallel
    if (isp_inertia_segment.x != FLT_MAX && isp_inertia_segment.y != FLT_MAX)
    {
	/* endpoint part */

	v2_t point_start = v2_add(transa, basisa);
	v2_t point_end   = v2_add(transb, v2_scale(basisb, 0.5));
	v2_t connector   = v2_sub(point_end, point_start);

	float angle = v2_circular_angle_between(connector, basisb);

	float dx = fabs(v2_length(connector) * cosf(angle));
	float dy = fabs(v2_length(connector) * sinf(angle));

	float basislength = v2_length(basisb);

	if (dx > basislength / 2.0)
	    dx -= basislength / 2.0;
	else
	    dx = 0.0;

	return dx > dy ? dx : dy;
    }
    else
    {
	float dx = fabs(transa.x - transb.x);
	float dy = fabs(transa.y - transb.y);
	return dx > dy ? dx : dy;
    }
}

v2_t v2_intersect_with_nearby(v2_t trans_a, v2_t basis_a, v2_t trans_b, v2_t basis_b, float proximity)
{
    v2_t isp = v2_intersect_vectors(trans_a, basis_a, trans_b, basis_b);

    if (isp.x == FLT_MAX && proximity > 0.0)
    {
	float distance = v2_endpoint_nearby(trans_a, basis_a, trans_b, basis_b);
	if (distance < proximity)
	    isp = v2_intersect_lines(trans_a, basis_a, trans_b, basis_b);
    }

    return isp;
}

v2_t v2_triangle_with_bases(v2_t point_a, v2_t point_b, float segmentlength, int8_t direction)
{
    v2_t vector  = v2_sub(point_b, point_a);
    vector       = v2_scale(vector, .5);
    float length = v2_length(vector);
    if (length < segmentlength)
    {
	float needed = sqrtf(segmentlength * segmentlength - length * length);
	v2_t  distance =
	    v2_init((float) direction * -vector.y, (float) direction * vector.x);
	distance = v2_resize(distance, needed);
	vector   = v2_add(point_a, vector);
	vector   = v2_add(vector, distance);
    }
    else
	vector = v2_add(point_a, vector);
    return vector;
}

r2_t r2_expand(r2_t rect, float d)
{
    rect.x -= d;
    rect.y -= d;
    rect.w += 2 * d;
    rect.h += 2 * d;

    return rect;
}

int r2_equals(r2_t r1, r2_t r2)
{
    return (r1.x == r2.x && r1.y == r2.y && r1.w == r2.w && r1.h == r2.h);
}

segment2_t
v2_collide_and_fragment(v2_t transa, v2_t basisa, v2_t transb, v2_t basisb)
{
    v2_t new_basis = v2_mirror(basisb, basisa);
    v2_t new_trans = v2_intersect_lines(transa, basisa, transb, basisb);

    float basis_length = v2_length(basisa);
    float chunk_length = v2_length(v2_sub(new_trans, transa));
    float final_length = basis_length - chunk_length;

    if (final_length > 0.0)
	new_basis = v2_resize(new_basis, final_length);
    else
	new_basis = v2_init(0.0, 0.0);

    return segment2_init(new_trans, new_basis);
}

/* creates segment from two vectors */

segment2_t
segment2_init(v2_t trans, v2_t basis)
{
    segment2_t result;
    result.trans = trans;
    result.basis = basis;
    return result;
}

/* creates square from two vectors */

square2_t
square_alloc(v2_t origo, v2_t extent)
{
    square2_t square;
    square.origo  = origo;
    square.extent = extent;
    return square;
}

/* returns intersection product */

square2_t
square_intersect(square2_t squarea, square2_t squareb)
{
    float new_width  = squareb.extent.x;
    float new_height = squareb.extent.y;
    float new_left   = fmin(squarea.origo.x, squareb.origo.x);
    float new_top    = fmax(squarea.origo.y, squareb.origo.y);

    if (squareb.origo.x < squarea.origo.x)
    {
	new_width = squareb.origo.x + squareb.extent.x - squarea.origo.x;
	new_left  = squarea.origo.x - squareb.origo.x;
    }

    if (squareb.origo.y > squarea.origo.y)
    {
	new_height = squareb.origo.y + squareb.extent.y - squarea.origo.y;
	new_top    = squarea.origo.y - squareb.origo.y;
    }

    if (squareb.origo.x + squareb.extent.x > squarea.origo.x + squarea.extent.x)
    {
	new_width = squarea.origo.x + squarea.extent.x - squareb.origo.x;
    }

    if (squareb.origo.y + squareb.extent.y < squarea.origo.y + squarea.extent.y)
    {
	new_height = squarea.origo.y + squarea.extent.y - squareb.origo.y;
    }

    if (new_width > squarea.extent.x)
	new_width = squarea.extent.x;
    if (new_height < squarea.extent.y)
	new_height = squarea.extent.y;

    square2_t result;
    result.origo  = v2_init(new_left, new_top);
    result.extent = v2_init(new_width, new_height);

    return result;
}

/*  checks overlapping
returns 0 if theres no overlapping
returns 1 if squareb is inside squarea
returns 2 if there is partial overlapping */

uint8_t
square_checkoverlapping(square2_t squarea, square2_t squareb)
{
    if (squareb.origo.x + squareb.extent.x < squarea.origo.x ||
	squareb.origo.x > squarea.origo.x + squarea.extent.x ||
	squareb.origo.y + squareb.extent.y > squarea.origo.y ||
	squareb.origo.y < squarea.origo.y + squarea.extent.y)
    {
	return kSquareOverlappingNone;
    }
    else if (squareb.origo.x >= squarea.origo.x && squareb.origo.x + squareb.extent.x <= squarea.origo.x + squarea.extent.x && squareb.origo.y <= squarea.origo.y && squareb.origo.y + squareb.extent.y >= squarea.origo.y + squarea.extent.y)
    {
	return kSquareOverlappingInside;
    }
    else
	return kSquareOverlappingPartial;
}

#endif
