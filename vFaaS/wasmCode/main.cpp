/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdio.h>
#include "host_interface.hpp"
#include <stdlib.h>
#include <time.h> 

int main(){
    int* a;
    int* b;
    printf("Hello world!\n");
    a = reinterpret_cast<int *> (wFaaSGetArg(0, sizeof(int)));
    b = reinterpret_cast<int *> (wFaaSGetArg(1, sizeof(int)));
    int result = (*a) + (*b);
    int matrixA[32][32];
    int matrixB[32][32];
    srand(time(NULL));
    for(int i = 0;i < 32;++i){
        for(int j = 0;j < 32;++j){
            matrixA[i][j] = rand() % 10;
            matrixB[i][j] = rand() % 10;
        }
    }

    int matrixC[32][32];
    for(int i = 0;i < 32;++i){
        for(int j = 0;j < 32;++j){
            matrixC[i][j] = 0;
            for(int k = 0;k < 32;++k){
                matrixC[i][j] += matrixA[i][k] * matrixB[k][j];
            }
        }
    }

    set_output((unsigned char*) (&result), sizeof(int));
    free(a);
    free(b);
    return 0;
}
