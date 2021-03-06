/*
 * Copyright (C) 2018 Tony Mason
 */

#include <stdlib.h>
#include <malloc.h>
#include <x86intrin.h>
#include <xmmintrin.h>
#include <emmintrin.h>

//
//  Cache data type
//
typedef struct {
    unsigned id;
    unsigned level;
    unsigned type;
    char type_name[32];
    unsigned sets;
    unsigned line_size;
    unsigned partitions;
    unsigned associativity;
    unsigned cache_size;
    unsigned fully_associative;
    unsigned self_initializing;
} cpu_cache_data_t;

//
// Some CPU functions
//
int cpu_has_avx2(void);
int cpu_has_avx512(void);
int cpu_has_rtm(void);
int cpu_has_hle(void);
int cpu_has_clflush(void);
int cpu_has_clwb(void);
int cpu_has_clflushopt(void);
size_t cpu_cacheline_size(void);
unsigned cpu_frequency(void);
unsigned cpu_rdtsc(void);
void cpu_prefetch_l1(const void *addr);
void cpu_prefetch_l2(const void *addr);
void cpu_prefetch_l3(const void *addr);
void cpu_prefetch_oneuse(const void *addr);
void cpu_sfence(void);
void cpu_mfence(void);

#define cpu_xabort(mm) do { \
    if (0 == cpu_info.has_rtm) return; \
    _xabort(mm); \
} while(0)

unsigned char cpu_xtest(void);
unsigned int cpu_xbegin(void);
void cpu_xend(void);

extern void (*cpu_clflush)(void const *addr);
extern void (*cpu_clflushopt)(void const *addr);
extern void (*cpu_clwb)(void const *addr);
extern void (*cpu_init)(void);


cpu_cache_data_t *cpu_get_cache_info(unsigned cache);
void cpu_free_cache_info(cpu_cache_data_t *cd);