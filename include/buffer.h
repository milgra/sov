#ifndef _SOV_BUFFER_H
#define _SOV_BUFFER_H

#include <stddef.h>

int sov_shm_create();

void* sov_shm_alloc(int shmid, size_t size);

#endif
