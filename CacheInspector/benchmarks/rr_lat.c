#include <assert.h>
#include <inttypes.h>
#include <malloc.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>

typedef struct bitmap {
    uint64_t num_bits;
    uint64_t bitmap[];
} bitmap_t;

int bitlen = 64;

bitmap_t*
bitmap_new(uint64_t num_bits) {
    uint64_t size = ((num_bits + bitlen - 1) / bitlen);
    bitmap_t* map = (bitmap_t*)calloc(1, (size * sizeof(uint64_t)) + sizeof(bitmap_t));
    map->num_bits = num_bits;
    return map;
}

int set_one(bitmap_t* map, uint64_t loc) {
    uint64_t index = loc / bitlen;
    uint64_t offset = loc % bitlen;
    uint64_t x = 1;
    (map->bitmap)[index] |= (x << offset);
    return 0;
}

int is_one(bitmap_t* map, uint64_t loc) {
    uint64_t index = loc / bitlen;
    uint64_t offset = loc % bitlen;
    uint64_t x = 1;
    return (((map->bitmap)[index]) >> offset) & x;
}

//http://stackoverflow.com/questions/7812044/finding-trailing-0s-in-a-binary-number
int bitcount(unsigned long value) {
    if(!value)
        return bitlen;

    value &= -value;  // the lowest bit that is 1.
    unsigned int lsb = (unsigned)value | (unsigned)(value >> 32);
    return (int)(((((((((((unsigned)(value >> 32) != 0) * 2)
                        + ((lsb & 0xffff0000) != 0))
                       * 2)
                      + ((lsb & 0xff00ff00) != 0))
                     * 2)
                    + ((lsb & 0xf0f0f0f0) != 0))
                   * 2)
                  + ((lsb & 0xcccccccc) != 0))
                 * 2)
           + ((lsb & 0xaaaaaaaa) != 0);
}

uint64_t next_zero(bitmap_t* map, uint64_t start) {
    uint64_t offset, k;
    uint64_t size = ((map->num_bits + bitlen - 1) / bitlen);

    uint64_t index = start > map->num_bits ? 0 : (int)(start / bitlen);
    for(k = 0; k < (size); k++) {
        uint64_t v = ~(map->bitmap[index]);
        offset = bitcount(v);
        if(offset < bitlen && (((index * bitlen) + offset) < map->num_bits)) {
            return (index * bitlen) + offset;
        }
        index = index >= (size - 1) ? 0 : index + 1;
    }

    printf("error bitmap full size %" PRIu64 " num_bits %" PRIu64 "\n", size, map->num_bits);
    return 0;
}

void bitmap_free(bitmap_t* map) {
    free(map);
}

uint64_t random_number(uint64_t min, uint64_t max, bitmap_t* map) {
    uint64_t x = (rand() % max) + min;
    if(is_one(map, x)) {
        x = next_zero(map, x);
    }
    set_one(map, x);
    return x;
}

uint64_t* random_arr(uint64_t size_kb) {
    uint64_t memsize = (size_kb << 10);
    uint64_t arr_size = memsize / sizeof(uint64_t);
    bitmap_t* arr_bitmap = bitmap_new(arr_size);
    uint64_t* arr = (uint64_t*)memalign(1024, memsize);
    time_t t;

    srand((unsigned)time(&t));

    uint64_t r0 = random_number(0, arr_size, arr_bitmap);
    uint64_t count = 0;
    uint64_t rc = r0;
    while(count++ < arr_size - 1) {
        uint64_t rn = random_number(0, arr_size, arr_bitmap);
        arr[rc] = (rn * sizeof(uint64_t)) + (uint64_t)arr;
        rc = rn;
    }
    arr[rc] = (r0 * sizeof(uint64_t)) + (uint64_t)arr;
    bitmap_free(arr_bitmap);
    return arr;
}

uint64_t get_clock_instrument_overhead() {
    uint32_t i;
    uint64_t sum = 0L;
    struct timespec t1, t2;
    // warm up
    for(i = 0; i < 100; i++) {
        clock_gettime(CLOCK_REALTIME, &t1);
        clock_gettime(CLOCK_REALTIME, &t2);
    }
// test
#define CHECK_COUNT (10000)
    for(i = 0; i < CHECK_COUNT; i++) {
        clock_gettime(CLOCK_REALTIME, &t1);
        clock_gettime(CLOCK_REALTIME, &t2);
        sum += ((t2.tv_sec - t1.tv_sec) * 1000000000L + t2.tv_nsec - t1.tv_nsec);
    }

    if(sum / CHECK_COUNT > 40)
        return get_clock_instrument_overhead();

    return sum / CHECK_COUNT;
}

int main(int argc, char const* argv[]) {
    if(argc != 3) {
        printf("USAGE:%s <buffer size(KB)> <number of datapoints>\n", argv[0]);
        return -1;
    }

    int max_pri = sched_get_priority_max(SCHED_FIFO);
    struct sched_param sch_parm;
    sch_parm.sched_priority = max_pri;
    int ret = sched_setscheduler(0, SCHED_FIFO, &sch_parm);
    if(ret != 0) {
        printf("sched_setscheduler()\n");
        return -1;
    }

    uint64_t size_kb = atoi(argv[1]);  //size of memory in kb
    uint64_t num_points = (uint64_t)atol(argv[2]);
    uint64_t size = size_kb << 10;  //size of memory in bytes

    uint64_t* arr = random_arr(size_kb);

    register uint64_t i, j;
    //        register volatile uint64_t temp;
    register uint64_t temp;
    volatile uint64_t x;
    temp = arr[0];
    uint64_t clock_overhead = get_clock_instrument_overhead();
    // WARM UP
    for(i = 0; i < 100000; ++i) {
        temp = *((uint64_t*)temp);
    }
    // TEST
    for(i = 0; i < num_points; i++) {
        struct timespec t1, t2;
        clock_gettime(CLOCK_REALTIME, &t1);
        register int cnt;
        for(cnt = 0; cnt < 1048576; cnt += 128) {
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
            temp = *((uint64_t*)temp);
        }
        clock_gettime(CLOCK_REALTIME, &t2);
        printf("%" PRIu64 " KB %.2f ns\n", size_kb, ((double)((t2.tv_sec - t1.tv_sec) * 1000000000l + t2.tv_nsec - t1.tv_nsec) - clock_overhead) / 1048576);
    }
    x = temp;
    free(arr);
    return 0;
}
