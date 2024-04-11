#include <stddef.h>

#define MAX_MEMSIZE (1ULL<<32)
#define MEMSIZE (1ULL<<30)
#define TLB_ENTRIES 256
#define TLB_BITS __builtin_ctz(TLB_ENTRIES)

#define PAGE_BITS 12
#define PAGE_SIZE (1ULL<<PAGE_BITS)
#define L1_BITS (((32)-PAGE_BITS)/2)
#define L2_BITS ((((32)-PAGE_BITS)/2)+((32)-PAGE_BITS)%2)
#define L1_SIZE (1ULL<<L1_BITS)
#define L2_SIZE (1ULL<<L2_BITS)

#define PAGE_COUNT (MEMSIZE/PAGE_SIZE)

#define BITMAP_SIZE ((PAGE_COUNT/32) + (PAGE_COUNT%32 != 0))

typedef unsigned int uint;
typedef unsigned long long ull;

typedef struct{
    unsigned int ppage[L2_SIZE];
} L2_NODE;
typedef struct{
    ull vpage;
    uint ppage;
} TLB_ENTRY;



void set_physical_mem();

void * translate(unsigned int vp);

unsigned int page_map(unsigned int vp);

void * t_malloc(size_t n);

int t_free(unsigned int vp, size_t n);

int put_value(unsigned int vp, void *val, size_t n);

int get_value(unsigned int vp, void *dst, size_t n);

void mat_mult(unsigned int a, unsigned int b, unsigned int c, size_t l, size_t m, size_t n);

void add_TLB(unsigned int vpage, unsigned int ppage);

int check_TLB(unsigned int vpage);

void print_TLB_missrate();

uint page_map_tlb_wrapper(unsigned int vp);