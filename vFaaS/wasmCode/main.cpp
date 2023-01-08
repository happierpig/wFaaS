/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdio.h>
#include "host_interface.hpp"

int main(){
    int* a;
    int* b;
    printf("Hello world!\n");
    a = reinterpret_cast<int *> (wFaaSGetArg(0, sizeof(int)));
    b = reinterpret_cast<int *> (wFaaSGetArg(1, sizeof(int)));
    int result = (*a) + (*b);
    set_output((unsigned char*) (&result), sizeof(int));
    free(a);
    free(b);
    return 0;
}
