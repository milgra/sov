#ifndef mt_vector_h
#define mt_vector_h

/* TODO separate unit tests */

#include "mt_memory.c"
#include "mt_time.c"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VNEW() mt_vector_new()
#define VADD(VEC, OBJ) mt_vector_add(VEC, OBJ)
#define VADDR(VEC, OBJ) mt_vector_add_rel(VEC, OBJ)
#define VREM(VEC, OBJ) mt_vector_rem(VEC, OBJ)

#define REL_VEC_ITEMS(X) mt_vector_decrese_item_retcount(X)

typedef struct _mt_vector_t mt_vector_t;
struct _mt_vector_t
{
    void**   data;
    void**   next;
    uint32_t length;
    uint32_t length_real;
};

mt_vector_t* mt_vector_new(void);
void         mt_vector_reset(mt_vector_t* vector);
void         mt_vector_dec_retcount(mt_vector_t* vector);
void         mt_vector_add(mt_vector_t* vector, void* data);
void         mt_vector_add_rel(mt_vector_t* vector, void* data);
void         mt_vector_ins(mt_vector_t* vector, void* data, size_t index);
void         mt_vector_add_in_vector(mt_vector_t* mt_vector_a, mt_vector_t* mt_vector_b);
void         mt_vector_add_unique_data(mt_vector_t* vector, void* data);
void         mt_vector_ins_unique_data(mt_vector_t* vector, void* data, size_t index);
void         mt_vector_replace_at_index(mt_vector_t* vector, void* data, size_t index);
char         mt_vector_rem(mt_vector_t* vector, void* data);
char         mt_vector_rem_at_index(mt_vector_t* vector, uint32_t index);
void         mt_vector_rem_in_range(mt_vector_t* vector, uint32_t start, uint32_t end);
void         mt_vector_rem_in_vector(mt_vector_t* mt_vector_a, mt_vector_t* mt_vector_b);
void         mt_vector_reverse(mt_vector_t* vector);
void*        mt_vector_head(mt_vector_t* vector);
void*        mt_vector_tail(mt_vector_t* vector);
uint32_t     mt_vector_index_of_data(mt_vector_t* vector, void* data);
void         mt_vector_sort(mt_vector_t* vector, int (*comp)(void* left, void* right));

void mt_vector_describe(void* p, int level);

#endif
#if __INCLUDE_LEVEL__ == 0

void mt_vector_del(void* vector);
void mt_vector_describe_data(void* p, int level);
void mt_vector_describe_mtvn(void* p, int level);

/* creates new vector */

mt_vector_t* mt_vector_new()
{
    mt_vector_t* vector = CAL(sizeof(mt_vector_t), mt_vector_del, mt_vector_describe);
    vector->data        = CAL(sizeof(void*) * 10, NULL, mt_vector_describe_data);
    vector->length      = 0;
    vector->length_real = 10;
    return vector;
}

/* deletes vector */

void mt_vector_del(void* pointer)
{
    mt_vector_t* vector = pointer;
    for (uint32_t index = 0; index < vector->length; index++)
	REL(vector->data[index]);
    REL(vector->data);
}

/* resets vector */

void mt_vector_reset(mt_vector_t* vector)
{
    for (uint32_t index = 0; index < vector->length; index++)
	REL(vector->data[index]);
    vector->length = 0;
}

/* decreases retain count of items. use when you add items inline and don't want to release every item
	one by one. Be careful with it, don't release them til dealloc!*/

void mt_vector_dec_retcount(mt_vector_t* vector)
{
    for (uint32_t index = 0; index < vector->length; index++)
	REL(vector->data[index]);
}

/* expands storage */

void mt_vector_expand(mt_vector_t* vector)
{
    if (vector->length == vector->length_real)
    {
	vector->length_real += 10;
	vector->data = mt_memory_realloc(vector->data, sizeof(void*) * vector->length_real);
    }
}

/* adds single data */

void mt_vector_add(mt_vector_t* vector, void* data)
{
    RET(data);
    mt_vector_expand(vector);
    vector->data[vector->length] = data;
    vector->length += 1;
}

/* adds and releases single data, for inline use */

void mt_vector_add_rel(mt_vector_t* vector, void* data)
{
    mt_vector_add(vector, data);
    REL(data);
}

/* adds data at given index */

void mt_vector_ins(mt_vector_t* vector, void* data, size_t index)
{
    if (index > vector->length) index = vector->length;
    RET(data);
    mt_vector_expand(vector);
    memmove(vector->data + index + 1, vector->data + index, (vector->length - index) * sizeof(void*));
    vector->data[index] = data;
    vector->length += 1;
}

/* adds all items in vector to vector */

void mt_vector_add_in_vector(mt_vector_t* mt_vector_a, mt_vector_t* mt_vector_b)
{
    for (uint32_t index = 0; index < mt_vector_b->length; index++) RET(mt_vector_b->data[index]);
    mt_vector_a->length_real += mt_vector_b->length_real;
    mt_vector_a->data = mt_memory_realloc(mt_vector_a->data, sizeof(void*) * mt_vector_a->length_real);
    memcpy(mt_vector_a->data + mt_vector_a->length, mt_vector_b->data, mt_vector_b->length * sizeof(void*));
    mt_vector_a->length += mt_vector_b->length;
}

/* adds single unique data */

void mt_vector_add_unique_data(mt_vector_t* vector, void* data)
{
    if (mt_vector_index_of_data(vector, data) == UINT32_MAX) mt_vector_add(vector, data);
}

/* adds single unique data at index */

void mt_vector_ins_unique_data(mt_vector_t* vector, void* data, size_t index)
{
    if (mt_vector_index_of_data(vector, data) == UINT32_MAX) mt_vector_ins(vector, data, index);
}

/* replaces data at given index */

void mt_vector_replace_at_index(mt_vector_t* vector, void* data, size_t index)
{
    REL(vector->data[index]);
    RET(data);
    vector->data[index] = data;
}

/* removes single data, returns 1 if data is removed and released during removal */

char mt_vector_rem(mt_vector_t* vector, void* data)
{
    uint32_t index = mt_vector_index_of_data(vector, data);
    if (index < UINT32_MAX)
    {
	mt_vector_rem_at_index(vector, index);
	return 1;
    }
    return 0;
}

/* removes single data at index, returns 1 if data is removed and released during removal */

char mt_vector_rem_at_index(mt_vector_t* vector, uint32_t index)
{
    if (index < vector->length)
    {
	REL(vector->data[index]);

	if (index < vector->length - 1)
	{
	    // have to shift elements after element to left
	    memmove(vector->data + index, vector->data + index + 1, (vector->length - index - 1) * sizeof(void*));
	}

	vector->length -= 1;
	return 1;
    }
    return 0;
}

/* removes data in range */

void mt_vector_rem_in_range(mt_vector_t* vector, uint32_t start, uint32_t end)
{
    for (uint32_t index = start; index < end; index++)
	REL(vector->data[index]);
    memmove(vector->data + start, vector->data + end + 1, (vector->length - end - 1) * sizeof(void*));
    vector->length -= end - start + 1;
}

/* removes data in vector */

void mt_vector_rem_in_vector(mt_vector_t* mt_vector_a, mt_vector_t* mt_vector_b)
{
    for (int index = 0; index < mt_vector_b->length; index++)
    {
	mt_vector_rem(mt_vector_a, mt_vector_b->data[index]);
    }
}

/* reverses item order */

void mt_vector_reverse(mt_vector_t* vector)
{
    int length = vector->length;
    for (int index = length - 1; index > -1; index--)
    {
	mt_vector_add(vector, vector->data[index]);
    }
    mt_vector_rem_in_range(vector, 0, length - 1);
}

/* returns head item of vector */

void* mt_vector_head(mt_vector_t* vector)
{
    if (vector->length > 0)
	return vector->data[0];
    else
	return NULL;
}

/* returns tail item of vector */

void* mt_vector_tail(mt_vector_t* vector)
{
    if (vector->length > 0)
	return vector->data[vector->length - 1];
    else
	return NULL;
}

/* returns index of data or UINT32_MAX if not found */

uint32_t mt_vector_index_of_data(mt_vector_t* vector, void* data)
{
    for (int index = 0; index < vector->length; index++)
    {
	if (vector->data[index] == data) return index;
    }
    return UINT32_MAX;
}

// mt vector node for sorting

typedef struct _mtvn_t mtvn_t;
struct _mtvn_t
{
    void*   c; // content
    mtvn_t* l; // left
    mtvn_t* r; // right
};

mtvn_t*  cache;
uint32_t cachei;

// TODO use node pool

void mt_vector_sort_ins(mtvn_t* node, void* data, int (*comp)(void* left, void* right))
{
    if (node->c == NULL)
    {
	node->c = data;
    }
    else
    {
	int smaller = comp(data, node->c) < 0;

	if (smaller)
	{
	    if (node->l == NULL) node->l = &cache[cachei++];
	    mt_vector_sort_ins(node->l, data, comp);
	}
	else
	{
	    if (node->r == NULL) node->r = &cache[cachei++];
	    mt_vector_sort_ins(node->r, data, comp);
	}
    }
}

void mt_vector_sort_ord(mtvn_t* node, mt_vector_t* vector, int* index)
{
    if (node->l) mt_vector_sort_ord(node->l, vector, index);
    vector->data[*index] = node->c;
    *index += 1;

    // cleanup node
    mtvn_t* right = node->r;

    if (right) mt_vector_sort_ord(right, vector, index);
}

// sorts values in vector, needs a comparator function
// or just use strcmp for strings

void mt_vector_sort(mt_vector_t* vector, int (*comp)(void* left, void* right))
{
    /* create cache */
    /* TODO make it local to make it thread safe */

    cache  = CAL(sizeof(mtvn_t) * vector->length, NULL, NULL);
    cachei = 1;

    for (int index = 0; index < vector->length; index++) mt_vector_sort_ins(cache, vector->data[index], comp);
    int index = 0;

    mt_vector_sort_ord(cache, vector, &index);

    REL(cache);
}

void mt_vector_describe(void* pointer, int level)
{
    mt_vector_t* vector = pointer;
    for (uint32_t index = 0; index < vector->length; index++)
    {
	mt_memory_describe(vector->data[index], level + 1);
	printf("\n");
    }
}

void mt_vector_describe_data(void* p, int level)
{
    printf("vec data\n");
}

void mt_vector_describe_mtvn(void* p, int level)
{
    printf("vec describe mtvn\n");
}

#endif
