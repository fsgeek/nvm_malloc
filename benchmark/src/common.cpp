#include "common.h"

namespace nvb {

#if defined(USE_MALLOC) || defined(USE_PMDK) || defined(USE_MAKALU)
object_table_t _object_table;
#endif

#if defined(USE_PMDK)
PMEMctopool *pcp;
#endif

#if defined(USE_MAKALU)
void *pmem_baseaddr;
void *pmem_curraddr;
void *pmem_ret;
size_t pmem_size;

int __nvm_region_allocator(void** memptr, size_t alignment, size_t size)
{
    char* next;
    char* res;
    if (size < 0) return 1;

    if (((alignment & (~alignment + 1)) != alignment)  ||    //should be multiple of 2
        (alignment < sizeof(void*))) return 1; //should be atleast the size of void*
    size_t aln_adj = (size_t) pmem_curraddr & (alignment - 1);

    if (aln_adj != 0) {
        pmem_curraddr = (void *) (((uintptr_t)pmem_curraddr) + (alignment - aln_adj));       
    }

    res = (char *)pmem_curraddr;
    next = ((char *)pmem_curraddr) + size;
    if ((uintptr_t) next > (uintptr_t) pmem_baseaddr + pmem_size){
       printf("\n----Ran out of space in mmaped file-----\n");
       assert(0);
       return 1;
    }
    pmem_curraddr = next;
    *memptr = res;
    //printf("Current NVM Region Addr: %p\n", pmem_curraddr);

    return 0;
}

#endif // USE_MAKALU

void* nvb_abs(void *rel_ptr, void *base_ptr) {
    void *r = NULL;

    if (NULL != rel_ptr) {
	r = (void *)(((uintptr_t) rel_ptr) + (uintptr_t)base_ptr);
    }
    return r;
}

void* nvb_rel(void *abs_ptr, void *base_ptr) {
    void *r = NULL;

    if (NULL != abs_ptr) {
	r = (void *)(((uintptr_t) abs_ptr) - (uintptr_t)base_ptr);
    }
    return r;
}


}
