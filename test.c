#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>
#include "my_vm.h"


int main(){

    set_physical_mem();

    size_t n = 100000;

    void *ptr = t_malloc(n*sizeof(int));
    for(int i = 0;i<n;i++){
        put_value(ptr+i*sizeof(int), &i, sizeof(int));
        int val;
        get_value(ptr+i*sizeof(int), &val, sizeof(int));
        assert(val == i);
    }

    assert(t_free(ptr, n*sizeof(int)) == 0);

    ptr = t_malloc(n*sizeof(int));
    for(int i = 0;i<n;i++){
        put_value(ptr+i*sizeof(int), &i, sizeof(int));
        int val;
        get_value(ptr+i*sizeof(int), &val, sizeof(int));
        assert(val == i);
    }

    t_free(ptr, n*sizeof(int));

    n = 10;
    void *a= t_malloc(n*n*sizeof(int)), *b = t_malloc(n*n*sizeof(int)), *c = t_malloc(n*n*sizeof(int));
    int aa[n][n], bb[n][n], cc[n][n];
    for(int i = 0;i<n*n;i++){
        put_value(a+i*sizeof(int), &i, sizeof(int));
        put_value(b+i*sizeof(int), &i, sizeof(int));
        aa[i/n][i%n] = i;
        bb[i/n][i%n] = i;
    }
    //mult aa*bb = cc
    for(int i = 0;i<n;i++){
        for(int j = 0;j<n;j++){
            cc[i][j] = 0;
            for(int k = 0;k<n;k++){
                cc[i][j] += aa[i][k]*bb[k][j];
            }
        }
    }

    mat_mult(a, b, c, n, n, n);

    for(int i = 0;i<n*n;i++){
        int val;
        get_value(c+i*sizeof(int), &val, sizeof(int));
        assert(val == cc[i/n][i%n]);
    }

    t_free(a, n*n*sizeof(int));
    t_free(b, n*n*sizeof(int));
    t_free(c, n*n*sizeof(int));

    print_TLB_missrate();

    return 0;
}