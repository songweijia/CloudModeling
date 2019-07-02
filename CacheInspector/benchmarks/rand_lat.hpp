#ifndef RAND_LAT_HPP
#include <map>
#include <optional>
#include <vector>

void random_latency(int64_t buffer_size, int num_points, double* output, std::optional<std::vector<std::map<std::string, long long>>>& counters);

#endif  //RAND_LAT_HPP
