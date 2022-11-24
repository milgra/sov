#ifndef mt_maputil_h
#define mt_maputil_h

#include "mt_map.c"
#include "mt_string.c"

typedef struct _mpair_t
{
    char* key;
    char* value;
} mpair_t;

mt_map_t* mapu_pair(mpair_t pair);

#endif

#if __INCLUDE_LEVEL__ == 0

mt_map_t* mapu_pair(mpair_t pair)
{
    mt_map_t* result = MNEW();
    char*     str    = mt_string_new_cstring(pair.value); // REL 0
    MPUT(result, pair.key, str);
    REL(str); // REL 0
    REL(pair.value);
    return result;
}

#endif
