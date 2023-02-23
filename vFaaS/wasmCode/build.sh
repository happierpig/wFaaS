# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

echo "Build wasm app .."
# /opt/wasi-sdk/bin/clang -O3 \
#         -z stack-size=4096 -Wl,--initial-memory=65536 \
#         -o test.wasm main.cpp \
#         -Wl,--export=main -Wl,--export=__main_argc_argv \
#         -Wl,--export=__data_end -Wl,--export=__heap_base \
#         -Wl,--strip-all,--no-entry \
#         -Wl,--allow-undefined \
#         -nostdlib \

echo "Done"
