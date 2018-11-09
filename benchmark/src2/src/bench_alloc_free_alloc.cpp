#include "common.h"

#include <cstring>
#include <sstream>
#include <getopt.h>

std::vector<uint64_t> workerTimes;
uint64_t allocation_size_min = 64;
uint64_t allocation_size_max = 64;

void worker(int id) {
    volatile char* pointerlist[100000];
    nvb::timer timer;
    std::default_random_engine generator;
    std::uniform_int_distribution<uint64_t> distribution(allocation_size_min, allocation_size_max);
    auto randomSize = std::bind(distribution, generator);

    timer.start();
    for (int i=0; i<100000; ++i) {
        pointerlist[i] = (volatile char*) nvb::reserve(randomSize());
        memset((void*)pointerlist[i], 5, 64);
        nvb::activate((void*) pointerlist[i]);
    }
    for (int i=0; i<100000; ++i) {
        nvb::free((void*) pointerlist[i]);
    }
    for (int i=0; i<100000; ++i) {
        pointerlist[i] = (volatile char*) nvb::reserve(randomSize());
        memset((void*)pointerlist[i], 5, 64);
        nvb::activate((void*) pointerlist[i]);
    }

    // save result
    workerTimes[id] = timer.stop();
}

int main(int argc, char **argv) {
    const char *short_opts = (char *)"a:vb:t:m:x:h";
    const option long_opts[] = {
        {"backing", optional_argument, nullptr, 'b'},
        {"threads", optional_argument, nullptr, 't'},
        {"minsize", optional_argument, nullptr, 'm'},
        {"maxsize", optional_argument, nullptr, 'x'},
        {"allocator", optional_argument, nullptr, 'a'},
        {"verbose", no_argument, nullptr, 'v'},
        {"help",    no_argument, nullptr, 'h'},
        {nullptr,  no_argument, nullptr, 0}
    };
    const char *help_string[] = { 
        (char *)"--backing: <path to file>      Set backing files\n",
        (char *)"--threads: <number of threads> Number of threads to use (default 8)\n",
        (char *)"--minsize: <size>              Minimum allocation size (default 64)\n",
        (char *)"--maxsize: <size>              Maximum allocation size (default 64)\n",
        (char *)"--verbose:                     Print diagnostic information\n",
        (char *)"--allocator: <name>            Which allocator to use\n",
        (char *)"--help                         Show this information\n",
        NULL
    };
    std::string backing_path;
    unsigned thread_count = 8;
    size_t min_size = 64;
    size_t max_size = 64;
    bool verbose = false;
    
    while(true) {
        const auto opt = getopt_long(argc, (char *const *)argv, short_opts, long_opts, nullptr);

        if (-1 == opt) {
            break;
        }

        switch(opt) {
            case 'b':
                backing_path = optarg;
                break;
            case 't': {
                char *end;
                thread_count = std::strtoul(optarg, &end, 10);
            }
            break;
            case 'm': {
                char *end;
                min_size = std::strtoull(optarg, &end, 10);
            }
            break;
            case 'x': {
                char *end;
                max_size = std::strtoul(optarg, &end, 10);
            }
            break;
            case 'v' : {
                verbose = true;
            }
            break;
            case 'h':
            case '?':
            default: {
                for (auto index = 0; nullptr != help_string[index]; index++) {
                    std::cout << help_string[index];
                }
                return 0;
            }
            break;
        }
    }

    if (min_size < 64) {
        std::cout << "WARNING: specified min allocation size was less than minimum, using 64 bytes instead" << std::endl;
        min_size = 64;
    }
    if (max_size < min_size) {
        std::cout << "WARNING: max allocation size was less than min, using min instead" << std::endl;
        max_size = min_size;
    }

    if (verbose) {
        if (backing_path.empty()) {
            std::cout << "backing path is: " << backing_path << std::endl;
        }
        std::cout << "thread count is: " << thread_count << std::endl;
        std::cout << "    min size is: " << min_size << std::endl;
        std::cout << "    max size is: " << max_size << std::endl;
    }

    workerTimes.resize(thread_count, 0);
    nvb::initialize(backing_path, 0);
    nvb::execute_in_pool(worker, thread_count);
    nvb::teardown();
    uint64_t avg = 0;
    for (auto t : workerTimes)
        avg += t;
    avg /= thread_count;
    std::cout << avg << std::endl;
    return 0;
}

#if 0
int main(int argc, char **argv) {
    if (argc < 3 || argc > 4) {
        std::cout << "usage: " << argv[0] << " <num_threads> <allocation_size_min> [allocation_size_max]" << std::endl;
        return -1;
    }
    size_t n_threads = atoi(argv[1]);
    allocation_size_min = atoi(argv[2]);
    if (allocation_size_min < 64) {
        std::cout << "WARNING: specified min allocation size was less than minimum, using 64 bytes instead" << std::endl;
        allocation_size_min = 64;
    }
    if (argc == 4) {
        allocation_size_max = atoi(argv[3]);
        if (allocation_size_max < allocation_size_min) {
            std::cout << "WARNING: max allocation size was less than min, using min instead" << std::endl;
            allocation_size_max = allocation_size_min;
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
#endif // 0