/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdio.h>
#include "host_interface.hpp"

int main(){
    int* buf;
    printf("Hello world!\n");
    buf = reinterpret_cast<int *> (wFaaSGetArg(0));
    printf("Int: %d\n",*buf);
    free(buf);
    return 0;
}
