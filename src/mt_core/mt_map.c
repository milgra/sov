#ifndef mt_map_h
#define mt_map_h

#include "mt_vector.c"
#include <stdio.h>
#include <stdlib.h>

#define MNEW() mt_map_new()
#define MPUT(MAP, ID, OBJ) mt_map_put(MAP, ID, OBJ)
#define MPUTR(MAP, ID, OBJ) mt_map_put_rel(MAP, ID, OBJ)
#define MGET(MAP, ID) mt_map_get(MAP, ID)
#define MDEL(MAP, OBJ) mt_map_del(MAP, OBJ)

typedef struct pair_t pair_t;
struct pair_t
{
    char* key;
    void* value;
};

typedef struct bucket_t bucket_t;
struct bucket_t
{
    size_t  count;
    pair_t* pairs;
};

typedef struct _mt_map_t mt_map_t;
struct _mt_map_t
{
    size_t    count_real;
    size_t    count;
    bucket_t* buckets;
};

mt_map_t* mt_map_new(void);
void      mt_map_dealloc(void* pointer);
void      mt_map_reset(mt_map_t* map);
int       mt_map_put(mt_map_t* map, const char* key, void* value);
int       mt_map_put_rel(mt_map_t* map, const char* key, void* value);
void*     mt_map_get(mt_map_t* map, const char* key);
void      mt_map_del(mt_map_t* map, const char* key);
void      mt_map_keys(mt_map_t* map, mt_vector_t* res);
void      mt_map_values(mt_map_t* map, mt_vector_t* res);
void      mt_map_describe(void* p, int level);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "mt_memory.c"
#include <string.h>

void mt_map_desc_buckets(void* p, int level)
{
    printf("map buckets\n");
}

void mt_map_desc_bucket_pairs(void* p, int level)
{
    printf("map bucket pair\n");
}

void mt_map_desc_string(void* p, int level)
{
    printf("%s", (char*) p);
}

/* creates map */

mt_map_t* mt_map_new()
{
    mt_map_t* map = CAL(sizeof(mt_map_t), mt_map_dealloc, mt_map_describe);

    map->count_real = 10;
    map->count      = 0;
    map->buckets    = CAL(map->count_real * sizeof(bucket_t), NULL, mt_map_desc_buckets);

    if (map->buckets == NULL)
    {
	REL(map);
	return NULL;
    }

    return map;
}

/* deletes map */

void mt_map_dealloc(void* pointer)
{
    mt_map_t* map = pointer;

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

void mt_map_reset(mt_map_t* map)
{
    mt_map_dealloc(map);

    map->count_real = 10;
    map->count      = 0;
    map->buckets    = CAL(map->count_real * sizeof(bucket_t), NULL, mt_map_desc_buckets);
}

/* resizes map */

void mt_map_resize(mt_map_t* map)
{
    // create new map

    mt_map_t* newmap   = CAL(sizeof(mt_map_t), mt_map_dealloc, mt_map_describe);
    newmap->count_real = map->count_real * 2;
    newmap->count      = 0;
    newmap->buckets    = CAL(newmap->count_real * sizeof(bucket_t), NULL, mt_map_desc_buckets);

    // put old values in new map

    mt_vector_t* oldkeys = VNEW();
    mt_map_keys(map, oldkeys);
    for (size_t index = 0; index < oldkeys->length; index++)
    {
	char* key   = oldkeys->data[index];
	void* value = mt_map_get(map, key);
	mt_map_put(newmap, key, value);
    }
    REL(oldkeys);

    // dealloc old map

    mt_map_dealloc(map);

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

int mt_map_put(mt_map_t* map, const char* key, void* value)
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

	bucket->pairs = CAL(sizeof(pair_t), NULL, mt_map_desc_bucket_pairs);
	if (bucket->pairs == NULL) return 0;
	bucket->count = 1;
    }
    else
    {
	// the bucket wasn't empty but no pair existed that matches the provided
	// key, so create a new key-value pair.

	tmp_pairs = mt_memory_realloc(bucket->pairs, (bucket->count + 1) * sizeof(pair_t));
	if (tmp_pairs == NULL) return 0;
	bucket->pairs = tmp_pairs;
	bucket->count++;
    }

    // get the last pair in the chain for the bucket

    pair        = &(bucket->pairs[bucket->count - 1]);
    pair->key   = CAL((strlen(key) + 1) * sizeof(char), NULL, mt_map_desc_string); // REL 0
    pair->value = value;

    map->count += 1;

    // copy the key

    strcpy(pair->key, key);

    if (map->count == map->count_real) mt_map_resize(map);

    return 1;
}

/* puts in and releases value with key for inline use*/

int mt_map_put_rel(mt_map_t* map, const char* key, void* value)
{
    int res = mt_map_put(map, key, value);
    REL(value);
    return res;
}

/* returns value for key */

void* mt_map_get(mt_map_t* map, const char* key)
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

void mt_map_del(mt_map_t* map, const char* key)
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

void mt_map_keys(mt_map_t* map, mt_vector_t* result)
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
	    mt_vector_add(result, pair->key);
	    pair++;
	    bindex++;
	}
	bucket++;
	index++;
    }
}

/* returns all values in map */

void mt_map_values(mt_map_t* map, mt_vector_t* result)
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
	    mt_vector_add(result, pair->value);
	    pair++;
	    bindex++;
	}
	bucket++;
	index++;
    }
}

/* prints keys */

void mt_map_printkeys(mt_map_t* map)
{
    mt_vector_t* keys = VNEW();
    mt_map_keys(map, keys);
    printf(" \n");
    for (size_t index = 0; index < keys->length; index++)
	printf(" %s", (char*) keys->data[index]);
}

void mt_map_describe(void* p, int level)
{
    mt_map_t*    map  = p;
    mt_vector_t* keys = VNEW(); // REL 0
    mt_map_keys(map, keys);
    printf("{");
    for (size_t index = 0; index < keys->length; index++)
    {
	char* key = (char*) keys->data[index];
	printf("\n%*s(K)%s\n%*s(V)", level, " ", key, level, " ");
	mt_memory_describe(mt_map_get(map, key), level + 1);
    }
    printf("\n%*s}", level, " ");
    REL(keys);
}

#endif
