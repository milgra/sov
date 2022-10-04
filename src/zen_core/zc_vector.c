#ifndef mtvec_h
#define mtvec_h

#include "zc_memory.c"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VNEW() vec_new()
#define VADD(VEC, OBJ) vec_add(VEC, OBJ)
#define VADDR(VEC, OBJ) vec_add_rel(VEC, OBJ)
#define VREM(VEC, OBJ) vec_rem(VEC, OBJ)

#define REL_VEC_ITEMS(X) vec_decrese_item_retcount(X)

typedef enum _vsdir_t
{
  VSD_ASC,
  VSD_DSC
} vsdir_t;

typedef struct _vec_t vec_t;
struct _vec_t
{
  void**   data;
  void**   next;
  uint32_t length;
  uint32_t length_real;
};

vec_t*   vec_new(void);
void     vec_reset(vec_t* vector);
void     vec_dec_retcount(vec_t* vector);
void     vec_add(vec_t* vector, void* data);
void     vec_add_rel(vec_t* vector, void* data);
void     vec_ins(vec_t* vector, void* data, size_t index);
void     vec_add_in_vector(vec_t* vec_a, vec_t* vec_b);
void     vec_add_unique_data(vec_t* vector, void* data);
void     vec_ins_unique_data(vec_t* vector, void* data, size_t index);
void     vec_replace_at_index(vec_t* vector, void* data, size_t index);
char     vec_rem(vec_t* vector, void* data);
char     vec_rem_at_index(vec_t* vector, uint32_t index);
void     vec_rem_in_range(vec_t* vector, uint32_t start, uint32_t end);
void     vec_rem_in_vector(vec_t* vec_a, vec_t* vec_b);
void     vec_reverse(vec_t* vector);
void*    vec_head(vec_t* vector);
void*    vec_tail(vec_t* vector);
uint32_t vec_index_of_data(vec_t* vector, void* data);
void     vec_sort(vec_t* vector, vsdir_t dir, int (*comp)(void* left, void* right));

void vec_describe(void* p, int level);

#endif
#if __INCLUDE_LEVEL__ == 0

void vec_del(void* vector);
void vec_describe_data(void* p, int level);
void vec_describe_mtvn(void* p, int level);

/* creates new vector */

vec_t* vec_new()
{
  vec_t* vector       = CAL(sizeof(vec_t), vec_del, vec_describe);
  vector->data        = CAL(sizeof(void*) * 10, NULL, vec_describe_data);
  vector->length      = 0;
  vector->length_real = 10;
  return vector;
}

/* deletes vector */

void vec_del(void* pointer)
{
  vec_t* vector = pointer;
  for (uint32_t index = 0; index < vector->length; index++)
    REL(vector->data[index]);
  REL(vector->data);
}

/* resets vector */

void vec_reset(vec_t* vector)
{
  for (uint32_t index = 0; index < vector->length; index++)
    REL(vector->data[index]);
  vector->length = 0;
}

/* decreases retain count of items. use when you add items inline and don't want to release every item
        one by one. Be careful with it, don't release them til dealloc!*/

void vec_dec_retcount(vec_t* vector)
{
  for (uint32_t index = 0; index < vector->length; index++)
    REL(vector->data[index]);
}

/* expands storage */

void vec_expand(vec_t* vector)
{
  if (vector->length == vector->length_real)
  {
    vector->length_real += 10;
    vector->data = mem_realloc(vector->data, sizeof(void*) * vector->length_real);
  }
}

/* adds single data */

void vec_add(vec_t* vector, void* data)
{
  RET(data);
  vec_expand(vector);
  vector->data[vector->length] = data;
  vector->length += 1;
}

/* adds and releases single data, for inline use */

void vec_add_rel(vec_t* vector, void* data)
{
  vec_add(vector, data);
  REL(data);
}

/* adds data at given index */

void vec_ins(vec_t* vector, void* data, size_t index)
{
  if (index > vector->length) index = vector->length;
  RET(data);
  vec_expand(vector);
  memmove(vector->data + index + 1, vector->data + index, (vector->length - index) * sizeof(void*));
  vector->data[index] = data;
  vector->length += 1;
}

/* adds all items in vector to vector */

void vec_add_in_vector(vec_t* vec_a, vec_t* vec_b)
{
  for (uint32_t index = 0; index < vec_b->length; index++) RET(vec_b->data[index]);
  vec_a->length_real += vec_b->length_real;
  vec_a->data = mem_realloc(vec_a->data, sizeof(void*) * vec_a->length_real);
  memcpy(vec_a->data + vec_a->length, vec_b->data, vec_b->length * sizeof(void*));
  vec_a->length += vec_b->length;
}

/* adds single unique data */

void vec_add_unique_data(vec_t* vector, void* data)
{
  if (vec_index_of_data(vector, data) == UINT32_MAX) vec_add(vector, data);
}

/* adds single unique data at index */

void vec_ins_unique_data(vec_t* vector, void* data, size_t index)
{
  if (vec_index_of_data(vector, data) == UINT32_MAX) vec_ins(vector, data, index);
}

/* replaces data at given index */

void vec_replace_at_index(vec_t* vector, void* data, size_t index)
{
  REL(vector->data[index]);
  RET(data);
  vector->data[index] = data;
}

/* removes single data, returns 1 if data is removed and released during removal */

char vec_rem(vec_t* vector, void* data)
{
  uint32_t index = vec_index_of_data(vector, data);
  if (index < UINT32_MAX)
  {
    vec_rem_at_index(vector, index);
    return 1;
  }
  return 0;
}

/* removes single data at index, returns 1 if data is removed and released during removal */

char vec_rem_at_index(vec_t* vector, uint32_t index)
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

void vec_rem_in_range(vec_t* vector, uint32_t start, uint32_t end)
{
  for (uint32_t index = start; index < end; index++)
    REL(vector->data[index]);
  memmove(vector->data + start, vector->data + end + 1, (vector->length - end - 1) * sizeof(void*));
  vector->length -= end - start + 1;
}

/* removes data in vector */

void vec_rem_in_vector(vec_t* vec_a, vec_t* vec_b)
{
  for (int index = 0; index < vec_b->length; index++)
  {
    vec_rem(vec_a, vec_b->data[index]);
  }
}

/* reverses item order */

void vec_reverse(vec_t* vector)
{
  int length = vector->length;
  for (int index = length - 1; index > -1; index--)
  {
    vec_add(vector, vector->data[index]);
  }
  vec_rem_in_range(vector, 0, length - 1);
}

/* returns head item of vector */

void* vec_head(vec_t* vector)
{
  if (vector->length > 0)
    return vector->data[0];
  else
    return NULL;
}

/* returns tail item of vector */

void* vec_tail(vec_t* vector)
{
  if (vector->length > 0)
    return vector->data[vector->length - 1];
  else
    return NULL;
}

/* returns index of data or UINT32_MAX if not found */

uint32_t vec_index_of_data(vec_t* vector, void* data)
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

// TODO use node pool

void vec_sort_ins(mtvn_t* node, void* data, vsdir_t dir, int (*comp)(void* left, void* right))
{
  if (node->c == NULL)
  {
    node->c = data;
  }
  else
  {
    int smaller = comp(data, node->c) < 0;
    if (dir == VSD_DSC) smaller = 1 - smaller;

    if (smaller)
    {
      if (node->l == NULL) node->l = CAL(sizeof(mtvn_t), NULL, vec_describe_mtvn);
      vec_sort_ins(node->l, data, dir, comp);
    }
    else
    {
      if (node->r == NULL) node->r = CAL(sizeof(mtvn_t), NULL, vec_describe_mtvn);
      vec_sort_ins(node->r, data, dir, comp);
    }
  }
}

void vec_sort_ord(mtvn_t* node, vec_t* vector, int* index)
{
  if (node->l) vec_sort_ord(node->l, vector, index);
  vector->data[*index] = node->c;
  *index += 1;

  // cleanup node
  mtvn_t* right = node->r;
  REL(node);

  if (right) vec_sort_ord(right, vector, index);
}

// sorts values in vector, needs a comparator function
// or just use strcmp for strings

void vec_sort(vec_t* vector, vsdir_t dir, int (*comp)(void* left, void* right))
{
  mtvn_t* node = CAL(sizeof(mtvn_t), NULL, vec_describe_mtvn);
  for (int index = 0; index < vector->length; index++)
  {
    vec_sort_ins(node, vector->data[index], dir, comp);
  }
  int index = 0;
  vec_sort_ord(node, vector, &index);
}

void vec_describe(void* pointer, int level)
{
  vec_t* vector = pointer;
  for (uint32_t index = 0; index < vector->length; index++)
  {
    mem_describe(vector->data[index], level + 1);
    printf("\n");
  }
}

void vec_describe_data(void* p, int level)
{
  printf("vec data\n");
}

void vec_describe_mtvn(void* p, int level)
{
  printf("vec describe mtvn\n");
}

#endif
