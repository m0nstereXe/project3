#include "my_vm.h"
#include <stdio.h>
char real_mem[MEMSIZE];
unsigned int bitmap[BITMAP_SIZE];

void panic(char *msg){
    printf("%s\n", msg);
    exit(1);
}

void * translate(unsigned int vp){
    unsigned int ppage = page_map(vp);
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
    unsigned int l1_msk = (1<<L1_BITS)-1, l2_msk = (1<<L2_BITS)-1;
    unsigned int l1_idx = vp&l1_msk, l2_idx = vp&l2_msk;
    l1_idx>>=L2_BITS+PAGE_BITS, l2_idx>>=PAGE_BITS;

    if(L1[l1_idx] == NULL){ //if L1 entry is NULL, create a new L2_NODE
        L1[l1_idx] = (L2_NODE*)malloc(sizeof(L2_NODE));
        L1[l1_idx]->cnt = 0;
        for(int i = 0; i < L2_SIZE; i++){
            L1[l1_idx]->ppage[i] = PAGE_COUNT;
        }
    }

    unsigned int *l2 = L1[l1_idx]->ppage;
    if(l2[l2_idx] == PAGE_COUNT){ //if L2 entry is PAGE_COUNT, create a new page
        l2[l2_idx] = page_mex(); //set the page as used
        L1[l1_idx]->cnt++; //increment the number of pages in this L2 entry
    }

    return l2[l2_idx];
}

void * t_malloc(size_t n){ //finds contigious free section in virtual memory, then assigns physical memory to it
    unsigned int pages_needed = (n+PAGE_SIZE-1)>>PAGE_BITS; //cieling
    unsigned int l = 0, r=0;
    for(r=0;r<PAGE_COUNT;r++){ //stupid two ptr for finding contigious free section
        if(r-l+1 == pages_needed) break;
        if(bitmap[r / 32]& (1<<(r%32)) ) l = r+1;
    }
    if(r == PAGE_COUNT) panic("Not enough memory!");

    for(int i = l; i <= r; i++){
        unsigned int ppage = page_map(i*PAGE_SIZE); //goes off and finds pages and maps them to our interval yay
        bitmap[i/32] |= (1<<(i%32)); //set the bitmap for this page as used
    }

    return real_mem + l*PAGE_SIZE;
}

//returns 0 if successful, -1 if failed
int t_free(unsigned int vp, size_t n){
    unsigned int l = vp - vp%PAGE_SIZE, r = vp + n - 1;
    r = r - r%PAGE_SIZE;
    int flag = 0;
    for(int i = l;i <= r; i++){
        unsigned int ppage = page_map(i);
        if(!(bitmap[i/32] & (1<<(i%32)))) flag = -1;
        bitmap[i/32] &= ~(1<<(i%32));
    }
    return flag;
}

int put_value(unsigned int vp,void *val, size_t n){
    while(n){
        unsigned int ppage = page_map(vp);
        unsigned int offset = vp&((1<<PAGE_BITS)-1);
        unsigned int bytes = PAGE_SIZE - offset;
        if(bytes > n) bytes = n;

        if(bitmap[ppage/32] & (1<<(ppage%32))) 
            return -1; //page was not yet allocated

        if(memcpy(real_mem + ppage*PAGE_SIZE + offset, val, bytes))
            return -1; //failed to copy

        n -= bytes, val += bytes, vp += bytes;
    }
    return 0;
}

int get_value(unsigned int vp, void *dst, size_t n){
    while(n){
        unsigned int ppage = page_map(vp);
        unsigned int offset = vp&((1<<PAGE_BITS)-1);
        unsigned int bytes = PAGE_SIZE - offset;
        if(bytes > n) bytes = n;

        if(bitmap[ppage/32] & (1<<(ppage%32))) 
            return -1; //page was not yet allocated

        if(memcpy(dst, real_mem + ppage*PAGE_SIZE + offset, bytes))
            return -1; //failed to copy

        n -= bytes, dst += bytes, vp += bytes;
    }
    return 0;
}

void mat_mult(unsigned int a, unsigned int b, unsigned int c, size_t l, size_t m, size_t n){
    for(int i = 0; i < l; i++){
        for(int j = 0; j < n; j++){
            unsigned int sum = 0;
            for(int k = 0; k < m; k++){
                unsigned int x,y;
                get_value(a + i*m + k, &x, sizeof(unsigned int));
                get_value(b + k*n + j, &y, sizeof(unsigned int));
                sum += x*y;
            }
            put_value(c + i*n + j, &sum, sizeof(unsigned int));
        }
    }
}

void add_TLB(unsigned int vpage, unsigned int ppage){
    //TODO: Finish
}

int check_TLB(unsigned int vpage){
    //TODO: Finish
}

void print_TLB_missrate(){
    //TODO: Finish
}
