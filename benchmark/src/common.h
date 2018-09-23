#pragma once

#include <nvm_malloc.h>

#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <random>
#include <string>
#include <sys/time.h>
#include <thread>
#include <vector>

#include <tbb/concurrent_hash_map.h>

#ifdef USE_PMDK
#include <libpmemcto.h>
#include <libpmem.h>
#include <assert.h>
#endif // USE_PMDK


namespace nvb {

// for malloc, emulate object table through a concurrent hashmap
#if defined(USE_MALLOC) || defined(USE_PMDK)
struct StringHashCompare {
    static size_t hash(const std::string x) {
        size_t h = 0;
        for (const char* s = x.c_str(); *s; ++s)
            h = (h*17)^*s;
        return h;
    }
    static bool equal(const std::string x, const std::string y) {
        return x==y;
    }
};
typedef tbb::concurrent_hash_map<std::string, void*, StringHashCompare> object_table_t;
extern object_table_t _object_table;
#endif // USE_MALLOC or USE_PMDK

#if USE_PMDK
    extern PMEMctopool *pcp;
#elif USE_MAKALU
#endif

inline void* initialize(const std::string workspace_path, int recover_if_possible) {
#ifdef USE_MALLOC
    return nullptr;
#elif USE_NVM_MALLOC
    return nvm_initialize(workspace_path.c_str(), recover_if_possible);
#elif USE_PMDK
    static const char *path = "/mnt/pmem/pmdk-test";
    static const char *layout_name = "bench";
    static const size_t pool_size = 1024 * 1024 * 1024; 
	pcp = pmemcto_create(path, layout_name, pool_size, 0666);

	if (NULL == pcp) {
		pcp = pmemcto_open(path, layout_name);
    }

    return (PMEMctopool *)0;
#elif USE_MAKALU
#error MAKALU is not yet implemented
#endif
}

inline void* reserve(uint64_t n_bytes) {
#ifdef USE_MALLOC
    return malloc(n_bytes);
#elif USE_NVM_MALLOC
    return nvm_reserve(n_bytes);
#elif USE_PMDK
    return pmemcto_malloc(pcp, (size_t) n_bytes);
#elif USE_MAKALU
    return MAK_malloc(n_bytes);
#error MAKALU is not yet implemented
#endif
}

inline void* reserve_id(const std::string id, uint64_t n_bytes) {
#ifdef USE_MALLOC
    void *ptr = malloc(n_bytes);
    _object_table.insert({id, ptr});
    return ptr;
#elif USE_NVM_MALLOC
    return nvm_reserve_id(id.c_str(), n_bytes);
#elif USE_PMDK
    void *ptr = pmemcto_malloc(pcp, (size_t) n_bytes);
    _object_table.insert({id, ptr});
    return ptr;
#elif USE_MAKALU
#error MAKALU is not yet implemented
#endif
}

inline void activate(void *ptr, void **link_ptr1=nullptr, void *target1=nullptr, void **link_ptr2=nullptr, void *target2=nullptr) {
#ifdef USE_MALLOC
    if (link_ptr1) {
        *link_ptr1 = target1;
        if (link_ptr2) {
            *link_ptr2 = target2;
        }
    }
#elif USE_NVM_MALLOC
    nvm_activate(ptr, link_ptr1, target1, link_ptr2, target2);
#elif USE_PMDK
    if (link_ptr1) {
        *link_ptr1 = target1;
        if (link_ptr2) {
            *link_ptr2 = target2;
        }
    }
#elif USE_MAKALU
#error MAKALU is not yet implemented
#endif
}

inline void activate_id(const std::string id) {
#ifdef USE_MALLOC
    // nothing to be done here
#elif USE_NVM_MALLOC
    nvm_activate_id(id.c_str());
#elif USE_PMDK
    // nothing to be done here
#elif USE_MAKALU
#error MAKALU is not yet implemented
#endif
}

inline void* get_id(const std::string id) {
#ifdef USE_MALLOC
    object_table_t::const_accessor acc;
    _object_table.find(acc, id);
    return acc->second;
#elif USE_NVM_MALLOC
    return nvm_get_id(id.c_str());
#elif USE_PMDK
    object_table_t::const_accessor acc;
    _object_table.find(acc, id);
    return acc->second;
#elif USE_MAKALU
#error MAKALU is not yet implemented
#endif
}

inline void free(void *ptr, void **link_ptr1=nullptr, void *target1=nullptr, void **link_ptr2=nullptr, void *target2=nullptr) {
#ifdef USE_MALLOC
    ::free(ptr);
    if (link_ptr1) {
        *link_ptr1 = target1;
        if (link_ptr2) {
            *link_ptr2 = target2;
        }
    }
#elif USE_NVM_MALLOC
    nvm_free(ptr, link_ptr1, target1, link_ptr2, target2);
#elif USE_PMDK
    pmemcto_free(pcp, ptr);
    if (link_ptr1) {
        *link_ptr1 = target1;
        if (link_ptr2) {
            *link_ptr2 = target2;
        }
    }
#elif USE_MAKALU
#error MAKALU is not yet implemented
#endif
}

inline void free_id(const std::string id) {
#ifdef USE_MALLOC
    object_table_t::const_accessor acc;
    _object_table.find(acc, id);
    void *ptr = acc->second;
    ::free(ptr);
    _object_table.erase(acc);
#elif USE_NVM_MALLOC
    nvm_free_id(id.c_str());
#elif USE_PMDK
    object_table_t::const_accessor acc;
    _object_table.find(acc, id);
    void *ptr = acc->second;
    pmemcto_free(pcp, ptr);
    _object_table.erase(acc);
#elif USE_MAKALU
#error MAKALU is not yet implemented
#endif
}

inline void persist(const void *ptr, uint64_t n_bytes) {
#ifdef USE_MALLOC
    // nothing to be done here
#elif USE_NVM_MALLOC
    nvm_persist(ptr, n_bytes);
#elif USE_PMDK
    pmem_persist(ptr, n_bytes);
#elif USE_MAKALU
#error MAKALU is not yet implemented
#endif
}

inline void* abs(void *rel_ptr) {
#ifdef USE_MALLOC
    return rel_ptr;
#elif USE_NVM_MALLOC
    return nvm_abs(rel_ptr);
#elif USE_PMDK
    // TODO: this isn't right
    assert(0);
    return rel_ptr;
#elif USE_MAKALU
#error MAKALU is not yet implemented
#endif
}

inline void* rel(void *abs_ptr) {
#ifdef USE_MALLOC
    return abs_ptr;
#elif USE_NVM_MALLOC
    return nvm_rel(abs_ptr);
#elif USE_PMDK
    // TODO: this isn't right
    return abs_ptr;
#elif USE_MAKALU
#error MAKALU is not yet implemented
#endif
}

inline void teardown() {
#ifdef USE_NVM_MALLOC
    nvm_teardown();
#elif USE_PMDK
    pmemcto_close(pcp);
    pcp = NULL;
#elif USE_MAKALU
#endif
}

inline void execute_in_pool(std::function<void(int)> func, size_t n_workers) {
    std::vector<std::thread> threadpool; threadpool.reserve(n_workers);
    for (size_t i=0; i<n_workers; ++i)
        threadpool.push_back(std::thread(func, i));
    for (auto &thread : threadpool)
        thread.join();
}

struct timer {
    struct timeval t_start = {0, 0};
    struct timeval t_end = {0, 0};
    inline void start() {
        gettimeofday(&t_start, nullptr);
    }
    inline uint64_t stop() {
        gettimeofday(&t_end, nullptr);
        return ((uint64_t)t_end.tv_sec - t_start.tv_sec) * 1000000ul + (t_end.tv_usec - t_start.tv_usec);
    }
};

}
