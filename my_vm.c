#include "my_vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
char* real_mem;
unsigned int bitmap[BITMAP_SIZE]; //1 means this page was mapped to mem in page_map
unsigned int virtmap[BITMAP_SIZE];//1 means this page is currently allocated
TLB_ENTRY TLB[TLB_ENTRIES];
ull tlb_hits = 0, tlb_queries = 0;

//mutex lock
pthread_mutex_t lock;

void panic(char *msg){
    printf("%s\n", msg);
    exit(1);
}

int getbit(uint *mp,uint ppage){
    return mp[ppage/32] & (1<<(ppage%32));
}
void togglebit(uint *mp,uint ppage){
    mp[ppage/32] ^= (1<<(ppage%32));
}
ull tlb_hash(ull vpage){
    return vpage & ((1<<TLB_BITS)-1);
}
void set_physical_mem(){
    pthread_mutex_init(&lock, NULL);
    ull page_table_size = sizeof(L2_NODE*)*L1_SIZE;

    real_mem = (char*)malloc(MEMSIZE+page_table_size);
    L2_NODE **L1 = (L2_NODE**)(real_mem+MEMSIZE);
    
    for(int i = 0; i < BITMAP_SIZE; i++){
        bitmap[i] = 0, virtmap[i] = 0;
    }
    for(int i = 0; i < L1_SIZE; i++){
        L1[i] = NULL;
    }
    for(int i = 0; i < TLB_ENTRIES; i++){
        TLB[i].vpage = MAX_MEMSIZE;
        TLB[i].ppage = -1;
    }
}

void * translate(unsigned int vp){
    unsigned int ppage = page_map_tlb_wrapper(vp);
    add_TLB(vp, ppage); //add tlb yipee
    uint ptr = real_mem + (ppage*PAGE_SIZE) + (vp&((1<<PAGE_BITS)-1));
    return real_mem + (ppage*PAGE_SIZE) + (vp&((1<<PAGE_BITS)-1));   
}

//get next available page
//minimum excluding integer
unsigned int page_mex(){
    for(int i = 0; i < BITMAP_SIZE; ++i){
        for(int l = 0;l<32;l++)
            if((bitmap[i]>>l)%2) continue;
            else return i*32+l;
    }
    panic("Page table is full!");
}

unsigned int page_map(unsigned int vp){ //returns the page # that covers this vp, creates one if it doesnt exist
    ull page_table_size = sizeof(L2_NODE*)*L1_SIZE;
    L2_NODE **L1 = (L2_NODE**)(real_mem+MEMSIZE);

    ull l1msk = (1<<L1_BITS)-1, l2msk = (1<<L2_BITS)-1;
    l1msk <<= L2_BITS+PAGE_BITS, l2msk <<= PAGE_BITS;
    uint l1_idx = (vp&l1msk) >> (L2_BITS+PAGE_BITS);
    uint l2_idx = (vp&l2msk) >> PAGE_BITS;

    if(L1[l1_idx] == NULL){ //if L1 entry is NULL, create a new L2_NODE
        L1[l1_idx] = (L2_NODE*)malloc(sizeof(L2_NODE));
        for(int i = 0; i < L2_SIZE; i++){
            L1[l1_idx]->ppage[i] = PAGE_COUNT;
        }
    }

    unsigned int *l2 = L1[l1_idx]->ppage;
    if(l2[l2_idx] == PAGE_COUNT){ //if L2 entry is PAGE_COUNT, create a new page
        l2[l2_idx] = page_mex(); 
        togglebit(bitmap, l2[l2_idx]); //mark this page as used
        add_TLB(vp, l2[l2_idx]);
    }
    // printf("page_map: %d %d %d\n", vp, l1_idx, l2_idx);
    return l2[l2_idx];
}
uint page_map_tlb_wrapper(unsigned int vp){
    if(check_TLB(vp)){
        return TLB[tlb_hash(vp)].ppage;
    }
    uint ppage  = page_map(vp);
    add_TLB(vp, ppage);
    return ppage;
}

void * t_malloc(size_t n){ //finds contigious free section in virtual memory, then assigns physical memory to it
    pthread_mutex_lock(&lock);
    uint pages = (n+PAGE_SIZE-1)/(PAGE_SIZE);
    uint l =0, r = 0;
    for(r;r<PAGE_SIZE;r++){
        uint ppage = page_map_tlb_wrapper(r*PAGE_SIZE);
        if(getbit(virtmap,ppage)) l = r+1;
        if(r-l+1 == pages) break;
    }
    if(r == PAGE_COUNT) panic("Not enough memory!");
    for(uint i = l; i <= r; i++){
        uint ppage = page_map_tlb_wrapper(i*PAGE_SIZE);
        togglebit(virtmap,ppage);
        assert(getbit(virtmap,ppage));
    }
    pthread_mutex_unlock(&lock);
    return (void*)(l*PAGE_SIZE); //return virtual address
}

//returns 0 if successful, -1 if failed
int t_free(unsigned int vp, size_t n){
    pthread_mutex_lock(&lock);
    uint pages = (n+PAGE_SIZE-1)/PAGE_SIZE;
    uint l = vp/PAGE_SIZE, r = (vp+n - 1)/PAGE_SIZE;
    for(uint i = l; i <= r; i++){
        uint ppage = page_map_tlb_wrapper(i*PAGE_SIZE);
        if(!getbit(virtmap,ppage)){ 
            pthread_mutex_unlock(&lock);
            return -1;
        }
        togglebit(virtmap,ppage);
    }
    pthread_mutex_unlock(&lock);
    return 0;
}

int put_value(unsigned int vp,void *val, size_t n){
    pthread_mutex_lock(&lock);
    for(int i = 0; i < n; i++){
        uint ppage = page_map_tlb_wrapper(vp+i);
        if(!getbit(virtmap,ppage)) return -1;
        *(char*)translate(vp+i) = ((char*)val)[i];
    }
    pthread_mutex_unlock(&lock);
    return 0;
}

int get_value(unsigned int vp, void *dst, size_t n){
    pthread_mutex_lock(&lock);
    for(int i = 0; i < n; i++){
        uint ppage = page_map_tlb_wrapper(vp+i);
        if(!getbit(virtmap,ppage)) return -1;
        ((char*)dst)[i] = *(char*)translate(vp+i);
    }
    pthread_mutex_unlock(&lock);
    return 0;
}

void mat_mult(unsigned int a, unsigned int b, unsigned int c, size_t l, size_t m, size_t n){
    for(int i = 0; i < l; i++){
        for(int j = 0; j < n; j++){
            int sum = 0;
            for(int k = 0; k < m; k++){
                int x, y;
                get_value(a+(i*m+k)*sizeof(int), &x, sizeof(int));
                get_value(b+(k*n+j)*sizeof(int), &y, sizeof(int));
                sum += x*y;
            }
            put_value(c+(i*n+j)*sizeof(int), &sum, sizeof(int));
        }
    }
}

void add_TLB(unsigned int vpage, unsigned int ppage){
    ull hash = tlb_hash(vpage);
    //simply overwrite the old entry, is this eviction ???
    TLB[hash].vpage = vpage;
    TLB[hash].ppage = ppage;
}

int check_TLB(unsigned int vpage){
    ull hash = tlb_hash(vpage);
    tlb_queries++;
    tlb_hits += TLB[hash].vpage == vpage;
    return TLB[hash].vpage == vpage;
}

void print_TLB_missrate(){
    printf("TLB hit rate: %f\n", (double)tlb_hits/tlb_queries);
}
