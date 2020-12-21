#include <ci/cache_info_shm.hpp>
#include <ci/util.hpp>
#include <mutex>
#include <shared_mutex>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <errno.h>
#include <map>
#include <unistd.h>
#include <sys/types.h>

namespace cacheinspector {

class CacheInfoShm {
private:
    const char* file_name;
    void*       readonly_ptr = nullptr;
    int         fd;
    mutable std::mutex  readonly_ptr_mutex;
public:
    // constructor
    CacheInfoShm(const char* ci_shm_file):
        file_name(ci_shm_file),
        readonly_ptr(nullptr){}
    // get_cache_info
    const cache_info_t* get_cache_info() {
        if (readonly_ptr == nullptr) {
            std::lock_guard<std::mutex> lck(readonly_ptr_mutex);
            if (readonly_ptr == nullptr) {
                fd = shm_open(file_name,O_RDONLY,S_IRUSR);
                if (fd == -1) {
                    std::cerr << "failed to open cache info shm file:" << file_name << std::endl;
                    return nullptr;
                }
                readonly_ptr = mmap(nullptr,sizeof(cache_info_t),PROT_READ,MAP_SHARED, fd, 0);
                if (readonly_ptr == MAP_FAILED) {
                    std::cerr << "mmap failed:" << strerror(errno) << std::endl;
                    readonly_ptr = nullptr;
                    return nullptr;
                }
            }
        }
        return static_cast<const cache_info_t*>(readonly_ptr);
    }
    // destructor
    virtual ~CacheInfoShm() {
        if (readonly_ptr) {
            munmap(readonly_ptr,sizeof(cache_info_t));
        }
    }
};

std::map<std::string,CacheInfoShm>  g_cache_info_map;
std::shared_mutex                   g_cache_info_map_mutex;

const cache_info_t* get_cache_info(const char* ci_shm_file) {
    std::shared_lock rlck(g_cache_info_map_mutex);
    if (g_cache_info_map.find(ci_shm_file) == g_cache_info_map.end()) {
        rlck.unlock();
        std::unique_lock wlck(g_cache_info_map_mutex);
        if (g_cache_info_map.find(ci_shm_file) == g_cache_info_map.end()) {
            g_cache_info_map.emplace(ci_shm_file,ci_shm_file);
        }
    }
    return g_cache_info_map.at(ci_shm_file).get_cache_info();
}

/**
 * Initialize cache info in shared memory. For "ci" command only.
 *
 * @param ci_shm_file file name
 *
 * @return cache_info_t
 */
cache_info_t* initialize_cache_info(const char* ci_shm_file) {
    int fd = shm_open(ci_shm_file, O_CREAT|O_EXCL|O_RDWR, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        std::cerr << "failed to open shared file:" << ci_shm_file << "," << strerror(errno) << std::endl;
        return nullptr;
    }
    if (ftruncate(fd, sizeof(cache_info_t)) == -1) {
        std::cerr << "failed to truncate cache info size:" << strerror(errno) << std::endl;
        shm_unlink(ci_shm_file);
        return nullptr;
    }
    cache_info_t* ci = static_cast<cache_info_t*>(mmap(nullptr,sizeof(cache_info_t),PROT_READ|PROT_WRITE,MAP_SHARED,fd,0));
    if (ci == MAP_FAILED) {
        std::cerr << "failed to map cache info:" << strerror(errno) << std::endl;
        shm_unlink(ci_shm_file);
        ci = nullptr;
    }
    return ci;
}

void destroy_cache_info(const char* ci_shm_file) {
    shm_unlink(ci_shm_file);
}
}
