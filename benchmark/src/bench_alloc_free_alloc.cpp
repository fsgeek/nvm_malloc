#include "common.h"

#include <cstring>
#include <sstream>

std::vector<uint64_t> workerTimes;
static const uint64_t allocation_unit_size = 64; // cache line
uint64_t allocation_size_min = 1;
uint64_t allocation_size_max = 1;

void worker(int id) {
    volatile char* pointerlist[100000];
    nvb::timer timer;
    std::default_random_engine generator;
    std::uniform_int_distribution<uint64_t> distribution(allocation_size_min, allocation_size_max);
    auto randomSize = std::bind(distribution, generator);

    timer.start();
    for (int i=0; i<100000; ++i) {
        uint64_t size = allocation_unit_size * randomSize();
        pointerlist[i] = (volatile char*) nvb::reserve(size);
        memset((void*)pointerlist[i], 5, 64);
        nvb::activate((void*) pointerlist[i]);
    }
    for (int i=0; i<100000; ++i) {
        nvb::free((void*) pointerlist[i]);
    }
    for (int i=0; i<100000; ++i) {
        uint64_t size = allocation_unit_size * randomSize();
        pointerlist[i] = (volatile char*) nvb::reserve(size);
        memset((void*)pointerlist[i], 5, 64);
        nvb::activate((void*) pointerlist[i]);
    }

    // save result
    workerTimes[id] = timer.stop();
}

int main(int argc, char **argv) {
    if (argc < 3 || argc > 4) {
        std::cout << "usage: " << argv[0] << " <num_threads> <allocation_size_min> [allocation_size_max]" << std::endl;
        return -1;
    }
    size_t n_threads = atoi(argv[1]);
    allocation_size_min = atoi(argv[2]);
    if (allocation_size_min < allocation_unit_size) {
        std::cout << "WARNING: specified min allocation size was less than minimum, using 64 bytes instead" << std::endl;
        allocation_size_min = 1;
    }
    else {
        allocation_size_min /= allocation_unit_size;
    }
    if (argc == 4) {
        allocation_size_max = atoi(argv[3]);
        if (allocation_size_max < allocation_size_min) {
            std::cout << "WARNING: max allocation size was less than min, using min instead" << std::endl;
            allocation_size_max = allocation_size_min;
        }
        uint64_t max = allocation_size_max /= allocation_unit_size;
        if (max * allocation_unit_size != allocation_size_max) {
            allocation_size_max = max + 1; // round up
        }
        else {
            allocation_size_max = max;
        }
    } else {
        allocation_size_max = allocation_size_min;
    }
    workerTimes.resize(n_threads, 0);
    nvb::initialize("/mnt/pmfs/nvb", 0);
    nvb::execute_in_pool(worker, n_threads);
    nvb::teardown();
    uint64_t avg = 0;
    for (auto t : workerTimes)
        avg += t;
    avg /= n_threads;
    std::cout << avg << std::endl;
    return 0;
}
