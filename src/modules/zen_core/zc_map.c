#ifndef zc_map_h
#define zc_map_h

#include "zc_vector.c"
#include <stdio.h>
#include <stdlib.h>

#define MNEW() map_new()
#define MPUT(MAP, ID, OBJ) map_put(MAP, ID, OBJ)
#define MPUTR(MAP, ID, OBJ) map_put_rel(MAP, ID, OBJ)
#define MGET(MAP, ID) map_get(MAP, ID)
#define MDEL(MAP, OBJ) map_del(MAP, OBJ)

typedef struct pair_t pair_t;
struct pair_t
{
  char* key;
  void* value;
};

typedef struct bucket_t bucket_t;
struct bucket_t
{
  unsigned int count;
  pair_t*      pairs;
};

typedef struct _map_t map_t;
struct _map_t
{
  unsigned int count_real;
  unsigned int count;
  bucket_t*    buckets;
};

map_t* map_new(void);
void   map_dealloc(void* pointer);
void   map_reset(map_t* map);
int    map_put(map_t* map, const char* key, void* value);
int    map_put_rel(map_t* map, const char* key, void* value);
void*  map_get(map_t* map, const char* key);
void   map_del(map_t* map, const char* key);
void   map_keys(map_t* map, vec_t* res);
void   map_values(map_t* map, vec_t* res);
void   map_print_keys(map_t* map);
void   map_describe(void* p, int level);

#ifdef DEBUG
void map_test(void);
#endif

#endif

#if __INCLUDE_LEVEL__ == 0

#include "zc_memory.c"
#include <string.h>

void map_desc_buckets(void* p, int level)
{
  printf("map buckets\n");
}

void map_desc_bucket_pairs(void* p, int level)
{
  printf("map bucket pair\n");
}

void map_desc_string(void* p, int level)
{
  printf("%s", (char*)p);
}

/* creates map */

map_t* map_new()
{
  map_t* map = CAL(sizeof(map_t), map_dealloc, map_describe);

  map->count_real = 10;
  map->count      = 0;
  map->buckets    = CAL(map->count_real * sizeof(bucket_t), NULL, map_desc_buckets);

  if (map->buckets == NULL)
  {
    REL(map);
    return NULL;
  }

  return map;
}

/* deletes map */

void map_dealloc(void* pointer)

{
  map_t* map = pointer;

  unsigned int index, bindex;
  bucket_t*    bucket;
  pair_t*      pair;
  pair_t*      last;

  bucket = map->buckets;
  index  = 0;

  // clean up buckets and pairs

  while (index < map->count_real)
  {
    last = pair = bucket->pairs;
    if (pair != NULL)
    {
      bindex = 0;
      while (bindex < bucket->count)
      {
        REL(pair->key);
        REL(pair->value);
        pair++;
        bindex++;
      }
    }
    if (last != NULL) REL(last);
    bucket++;
    index++;
  }

  if (map->buckets) REL(map->buckets);
}

/* resets map */

void map_reset(map_t* map)
{
  map_dealloc(map);

  map->count_real = 10;
  map->count      = 0;
  map->buckets    = CAL(map->count_real * sizeof(bucket_t), NULL, map_desc_buckets);
}

/* resizes map */

void map_resize(map_t* map)
{
  // create new map

  map_t* newmap      = CAL(sizeof(map_t), map_dealloc, map_describe);
  newmap->count_real = map->count_real * 2;
  newmap->count      = 0;
  newmap->buckets    = CAL(newmap->count_real * sizeof(bucket_t), NULL, map_desc_buckets);

  // put old values in new map

  vec_t* oldkeys = VNEW();
  map_keys(map, oldkeys);
  for (uint32_t index = 0; index < oldkeys->length; index++)
  {
    char* key   = oldkeys->data[index];
    void* value = map_get(map, key);
    map_put(newmap, key, value);
  }
  REL(oldkeys);

  // dealloc old map

  map_dealloc(map);

  map->count_real = newmap->count_real;
  map->buckets    = newmap->buckets;
  map->count      = newmap->count;

  newmap->count_real = 0;
  newmap->buckets    = NULL;
  newmap->count      = 0;

  // release memory block for newmap

  REL(newmap);
}

/* returns corresponding pair from bucket */

static pair_t* get_pair(bucket_t* bucket, const char* key)
{
  unsigned int index;
  pair_t*      pair;

  if (bucket->count == 0) return NULL;

  pair  = bucket->pairs;
  index = 0;

  while (index < bucket->count)
  {
    if (pair->key != NULL && pair->value != NULL)
    {
      if (strcmp(pair->key, key) == 0) return pair;
    }
    pair++;
    index++;
  }

  return NULL;
}

/* returns a hash code for the provided string */

static unsigned long hash(const char* str)
{
  unsigned long hash = 5381;
  int           chr;
  while ((chr = *str++))
    hash = ((hash << 5) + hash) + chr;
  return hash;
}

/* puts in value with key */

int map_put(map_t* map, const char* key, void* value)
{
  RET(value);

  size_t    index;
  bucket_t* bucket;
  pair_t *  tmp_pairs, *pair;

  if (map == NULL) return 0;
  if (key == NULL) return 0;

  // get a pointer to the bucket the key string hashes to

  index  = hash(key) % map->count_real;
  bucket = &(map->buckets[index]);

  // check if we can handle insertion by simply replacing
  // an existing value in a key-value pair in the bucket.

  if ((pair = get_pair(bucket, key)) != NULL)
  {
    // the bucket contains a pair that matches the provided key,
    // change the value for that pair to the new value.

    REL(pair->value);
    pair->value = value;
    return 1;
  }

  // create a key-value pair

  if (bucket->count == 0)
  {
    // the bucket is empty, lazily allocate space for a single
    // key-value pair.

    bucket->pairs = CAL(sizeof(pair_t), NULL, map_desc_bucket_pairs);
    if (bucket->pairs == NULL) return 0;
    bucket->count = 1;
  }
  else
  {
    // the bucket wasn't empty but no pair existed that matches the provided
    // key, so create a new key-value pair.

    tmp_pairs = mem_realloc(bucket->pairs, (bucket->count + 1) * sizeof(pair_t));
    if (tmp_pairs == NULL) return 0;
    bucket->pairs = tmp_pairs;
    bucket->count++;
  }

  // get the last pair in the chain for the bucket

  pair        = &(bucket->pairs[bucket->count - 1]);
  pair->key   = CAL((strlen(key) + 1) * sizeof(char), NULL, map_desc_string); // REL 0
  pair->value = value;

  map->count += 1;

  // copy the key

  strcpy(pair->key, key);

  if (map->count == map->count_real) map_resize(map);

  return 1;
}

/* puts in and releases value with key for inline use*/

int map_put_rel(map_t* map, const char* key, void* value)
{
  int res = map_put(map, key, value);
  REL(value);
  return res;
}

/* returns value for key */

void* map_get(map_t* map, const char* key)
{
  unsigned int index;
  bucket_t*    bucket;
  pair_t*      pair;

  if (map == NULL) return NULL;
  if (key == NULL) return NULL;

  index  = hash(key) % map->count_real;
  bucket = &(map->buckets[index]);

  pair = get_pair(bucket, key);
  if (pair == NULL) return NULL;
  return pair->value;
}

/* removes value for key */

void map_del(map_t* map, const char* key)
{
  unsigned int index, found = 0;
  bucket_t*    bucket;
  pair_t*      pair;

  index  = hash(key) % map->count_real;
  bucket = &(map->buckets[index]);

  if (bucket->count > 0)
  {
    pair  = bucket->pairs;
    index = 0;
    while (index < bucket->count)
    {
      if (pair->key != NULL && pair->value != NULL)
      {
        if (strcmp(pair->key, key) == 0)
        {
          found = 1;
          break;
        }
      }
      pair++;
      index++;
    }

    if (found == 1)
    {
      REL(pair->key);
      REL(pair->value);

      pair = bucket->pairs;
      if (index < bucket->count)
      {
        memmove(pair + index, pair + index + 1, (bucket->count - index - 1) * sizeof(bucket_t));
      }
      bucket->count -= 1;

      if (bucket->count == 0)
      {
        REL(bucket->pairs);
        bucket->pairs = NULL;
      }
      map->count -= 1;
    }
  }
}

/* returns all keys in map */

void map_keys(map_t* map, vec_t* result)
{
  unsigned int index, bindex;
  bucket_t*    bucket;
  pair_t*      pair;

  bucket = map->buckets;
  index  = 0;
  while (index < map->count_real)
  {
    pair   = bucket->pairs;
    bindex = 0;
    while (bindex < bucket->count)
    {
      vec_add(result, pair->key);
      pair++;
      bindex++;
    }
    bucket++;
    index++;
  }
}

/* returns all values in map */

void map_values(map_t* map, vec_t* result)
{
  unsigned int index, bindex;
  bucket_t*    bucket;
  pair_t*      pair;

  bucket = map->buckets;
  index  = 0;
  while (index < map->count_real)
  {
    pair   = bucket->pairs;
    bindex = 0;
    while (bindex < bucket->count)
    {
      vec_add(result, pair->value);
      pair++;
      bindex++;
    }
    bucket++;
    index++;
  }
}

/* prints keys */

void map_printkeys(map_t* map)
{
  vec_t* keys = VNEW();
  map_keys(map, keys);
  printf(" \n");
  for (int index = 0; index < keys->length; index++)
    printf(" %s", (char*)keys->data[index]);
}

void map_describe(void* p, int level)
{
  map_t* map  = p;
  vec_t* keys = VNEW(); // REL 0
  map_keys(map, keys);
  printf("{");
  for (int index = 0; index < keys->length; index++)
  {
    char* key = (char*)keys->data[index];
    printf("\n%*s(K)%s\n%*s(V)", level, " ", key, level, " ");
    mem_describe(map_get(map, key), level + 1);
  }
  printf("\n%*s}", level, " ");
  REL(keys);
}

// tests map

#ifdef DEBUG
void map_test()
{
  printf("MAP TEST SESSION START");
  printf("1 CREATE EMPTY");
  map_t* m1 = map_new();
  printf("2 DELETE EMPTY");
  REL(m1);
  printf("3 ADDING DATA");
  map_t* m2 = map_new();
  map_put(m2, "fakk", "fakkvalue");
  map_put(m2, "makk", "makkvalue");
  map_put(m2, "takk", "takkvalue");
  map_put(m2, "kakk", "kakkvalue");
  printf("4 GETTING DATA");
  printf(" VALUE FOR makk : %s", (char*)map_get(m2, "makk"));
  printf("5 SETTING DATA TO NULL");
  map_put(m2, "takk", NULL);
  printf(" VALUE FOR takk : %s", (char*)map_get(m2, "takk"));
  printf("MAP TEST SESSION END");
}
#endif

#endif
