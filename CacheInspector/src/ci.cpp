#include <iostream>
#include <unistd.h>
#include <ci/ci.hpp>

using namespace cacheinspector;

int main(int argc, char** argv) {
    std::cout << "The new cache inspector." << std::endl;

    sequential_throughput_test_result_t result; 

    if(sequential_throughput_cliffs(default_sequential_throughput_test_schedule,result)) {
        std::cerr << "profiling cliff failed" << std::endl;
        return -1;
    }

    for(auto fourtuple: result) {
        std::cout << std::get<0>(fourtuple) << " "
                  << std::get<1>(fourtuple) << " "
                  << std::get<2>(fourtuple) << " "
                  << std::get<3>(fourtuple) << std::endl;
    }

    return 0;
}
