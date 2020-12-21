#include <ci/ci.hpp>
#include <unistd.h>
using namespace cacheinspector;

int main(int argc, char** argv) {
    const cache_info_t* ci = get_cache_info();
    if (ci == nullptr) {
        std::cerr << "Warning: failed to get cache info page. You need to start this example in 'ci --runapp'" << std::endl;
        return -1;
    } else {
        int nloop = 10;
        while(--nloop) {
            sleep(2);
            std::cout << "cache_size[0]=" << ci->page.cache_size[0] << std::endl;
        }
    }
    return 0;
}
