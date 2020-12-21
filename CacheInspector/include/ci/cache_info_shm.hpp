#pragma once
#include <ci/config.h>
#include <inttypes.h>

/**
 * CacheInspector expose this dynamic cache information to user application.
 */

namespace cacheinspector {

#define NUM_CACHE_SIZE_SAMPLE   (1)
#define DEFAULT_CACHE_INFO_SHM_FILE "cache_info"

typedef struct {
    union {
        uint64_t    cache_size[NUM_CACHE_SIZE_SAMPLE];
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
