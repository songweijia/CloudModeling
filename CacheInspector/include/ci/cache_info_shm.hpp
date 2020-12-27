#pragma once
#include <ci/config.h>
#include <inttypes.h>

/**
 * CacheInspector expose this dynamic cache information to user application.
 */

namespace cacheinspector {

#define DEFAULT_CACHE_INFO_SHM_FILE "cache_info"
#define MAX_CACHE_LEVELS         (4)
#define MAX_CACHE_SIZE_SAMPLES   (10)

typedef struct {
    union {
        uint64_t    cache_size[MAX_CACHE_LEVELS][MAX_CACHE_SIZE_SAMPLES];
        uint8_t     _bytes[4096];
    } page;
} cache_info_t;

/**
 * Get cache info
 * @param ci_shm_file the shm file name.
 * @return the cache info pointer
 */
extern const cache_info_t* get_cache_info(const char* ci_shm_file = DEFAULT_CACHE_INFO_SHM_FILE);
}
