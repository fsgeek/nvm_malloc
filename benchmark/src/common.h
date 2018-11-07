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

#ifdef USE_PMOBJ
#include <libpmemobj.h>
#include <libpmem.h>
#include <assert.h>
#endif // USE_PMOBJ

#ifdef USE_MAKALU
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libpmem.h>
#include <assert.h>
#include <makalu.h>
#endif // USE_MAKALU

namespace nvb {


void* nvb_abs(void *rel_ptr, void *base_ptr);
void* nvb_rel(void *abs_ptr, void *base_ptr);

// for malloc, emulate object table through a concurrent hashmap
#if defined(USE_MALLOC) || defined(USE_PMDK) || defined(USE_PMOBJ) || defined(USE_MAKALU)
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

#if defined(USE_PMOBJ)
extern PMEMoid root_oid;
extern PMEMobjpool *pop;
int create_dynamic_block(PMEMobjpool *pop, void *ptr, void *arg);
#endif // USE_PMOBJ

#if USE_PMDK
    extern PMEMctopool *pcp;
#elif USE_MAKALU
    extern void *pmem_baseaddr;
    extern void *pmem_curraddr;
    extern size_t pmem_size;
    extern void *pmem_ret;
    int __nvm_region_allocator(void** memptr, size_t alignment, size_t size);
#endif

inline void* initialize(const std::string workspace_path, int recover_if_possible) {
#ifdef USE_MALLOC
    return nullptr;
#elif USE_NVM_MALLOC
    return nvm_initialize(workspace_path.c_str(), recover_if_possible);
#elif USE_PMDK
    static const char *path = "/mnt/pmfs/pmcto-test";
    static const char *layout_name = "bench";
    static const size_t pool_size = (size_t)10 * 1024 * 1024 * 1024; 
    int retry = 0;

    while (NULL == pcp) {
	assert(retry++ < 10);
        (void) unlink(path);

        pcp = pmemcto_create(path, layout_name, pool_size, 0600);
    }
    return (PMEMctopool *)0;
#elif USE_PMOBJ
    static const char *path = "/mnt/pmfs/pmobj-test";
    static const char *layout_name = "bench";
    static const size_t pool_size = 1024 * 1024 * 1024; 
    int retry = 0;

    while (NULL == pop) {
    	assert(retry++ < 10);
        (void) unlink(path);

        pop = pmemobj_create(path, layout_name, pool_size, 0600);
    }
    assert(NULL != pop);
    pmemobj_alloc(pop, &root_oid, 64, 0, create_dynamic_block, NULL);
    assert(!OID_IS_NULL(root_oid));
    return (void *)0;

#elif USE_MAKALU
    static const char *path="/mnt/pmfs/makalu-test";
    static const size_t pool_size = 1024 * 1024 * 1024;
    int is_pmem;

    pmem_baseaddr = pmem_map_file(path, pool_size, PMEM_FILE_CREATE,
                                  0644, &pmem_size, &is_pmem);

    assert(NULL != pmem_baseaddr);
    assert(is_pmem);

    pmem_curraddr = (char*) ((size_t) pmem_baseaddr + 3 * sizeof(intptr_t));

    pmem_ret = MAK_start(&__nvm_region_allocator);
    assert(NULL != pmem_ret);
    
    return NULL;

#endif
}

inline void* reserve(uint64_t n_bytes) {
    n_bytes = ((n_bytes + 63) & (~63)); // round up to nearest 64 bytes
#ifdef USE_MALLOC
    return malloc(n_bytes);
#elif USE_NVM_MALLOC
    return nvm_reserve(n_bytes);
#elif USE_PMDK
    assert(pcp != NULL);
    return pmemcto_malloc(pcp, (size_t) n_bytes);
#elif USE_PMOBJ
    PMEMoid oid;
    void *p = NULL;

    assert(pop != NULL);
    pmemobj_alloc(pop, &oid, 64, 0, create_dynamic_block, NULL);
    assert(!OID_IS_NULL(oid));
    p = pmemobj_direct(oid);
    assert(NULL != p);
    return p;
#elif USE_MAKALU
    return MAK_malloc(n_bytes);
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
#elif USE_PMOBJ
    assert(0); // not implemented
#elif USE_MAKALU
    void *ptr = MAK_malloc((size_t) n_bytes);
    _object_table.insert({id, ptr});
    return ptr;
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
#else
    if (link_ptr1) {
        *link_ptr1 = target1;
        if (link_ptr2) {
            *link_ptr2 = target2;
        }
    }
#endif
}

inline void activate_id(const std::string id) {
#if USE_NVM_MALLOC
    nvm_activate_id(id.c_str());
#else
    // nothing to be done here
#endif
}

inline void* get_id(const std::string id) {
#if USE_NVM_MALLOC
    return nvm_get_id(id.c_str());
#else
    object_table_t::const_accessor acc;
    _object_table.find(acc, id);
    return acc->second;
#endif
}

inline void free(void *ptr, void **link_ptr1=nullptr, void *target1=nullptr, void **link_ptr2=nullptr, void *target2=nullptr) {
#ifdef USE_NVM_MALLOC
    nvm_free(ptr, link_ptr1, target1, link_ptr2, target2);
#else
#ifdef USE_MALLOC
    ::free(ptr);
#endif // USE_MALLOC
#if USE_PMDK
    pmemcto_free(pcp, ptr);
#endif
#if USE_PMOBJ
    PMEMoid oid = pmemobj_oid(ptr);
    pmemobj_free(&oid);
#endif
#if USE_MAKALU
    //MAK_free(ptr); - have to figure out the thread specific stuff
#endif
    if (link_ptr1) {
        *link_ptr1 = target1;
        if (link_ptr2) {
            *link_ptr2 = target2;
        }
    }
#endif
}

inline void free_id(const std::string id) {
#ifdef USE_NVM_MALLOC
    nvm_free_id(id.c_str());
#else
    object_table_t::const_accessor acc;
    _object_table.find(acc, id);
    void *ptr = acc->second;

#ifdef USE_MALLOC
    ::free(ptr);
#endif // USE_MALLOC

#ifdef USE_PMDK
    pmemcto_free(pcp, ptr);
#endif // USE_PMDK

#ifdef USE_PMOBJ
    PMEMoid oid = pmemobj_oid(ptr);
    pmemobj_free(&oid);
#endif // USE_PMOBJ

#ifdef USE_MAKALU
    MAK_free(ptr);
#endif
    _object_table.erase(acc);
#endif // else USE_NVM_ALLOC
}

inline void persist(const void *ptr, uint64_t n_bytes) {
#ifdef USE_MALLOC
    // nothing to be done here
#elif USE_NVM_MALLOC
    nvm_persist(ptr, n_bytes);
#else
    pmem_persist(ptr, n_bytes);
#endif
}

inline void* abs(void *rel_ptr) {
#ifdef USE_MALLOC
    return rel_ptr;
#elif USE_NVM_MALLOC
    return nvm_abs(rel_ptr);
#elif USE_PMDK
    return nvb_abs(rel_ptr, pcp);
#elif USE_PMOBJ
    PMEMoid oid;
    assert(root_oid.pool_uuid_lo != 0);
    oid.pool_uuid_lo = root_oid.pool_uuid_lo;
    oid.off = (uint64_t)rel_ptr;
    return pmemobj_direct(oid);
#elif USE_MAKALU
    return nvb_abs(rel_ptr, pmem_baseaddr);
#endif
}

inline void* rel(void *abs_ptr) {
#ifdef USE_MALLOC
    return abs_ptr;
#elif USE_NVM_MALLOC
    return nvm_rel(abs_ptr);
#elif USE_PMDK
    return nvb_rel(abs_ptr, pcp);
#elif USE_PMOBJ
    PMEMoid oid = pmemobj_oid(abs_ptr);
    return (void *) oid.off;
#elif USE_MAKALU
    return nvb_rel(abs_ptr, pmem_baseaddr);
#endif
}

inline void teardown() {
#ifdef USE_NVM_MALLOC
    nvm_teardown();
#elif USE_PMDK
    pmemcto_close(pcp);
    pcp = NULL;
#elif USE_PMOBJ
    pmemobj_free(&root_oid);
    root_oid = OID_NULL;
    pmemobj_close(pop);
    pop = NULL;
#elif USE_MAKALU
    MAK_close();
    pmem_unmap(pmem_baseaddr, pmem_size);
    pmem_baseaddr = NULL;
    pmem_curraddr = NULL;
    pmem_size = 0;
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
