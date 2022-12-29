#include <wasm_exec_env.h>
#include <wasm_export.h>
#include "utils.h"
int main(){
    std::string wasmFilePath = "../wasmCode/test.wasm";
    char error_buf[128];
    wasm_module_t module;
    wasm_module_inst_t module_inst;
    wasm_function_inst_t func;
    wasm_exec_env_t exec_env;
    uint32 size, stack_size = 8092, heap_size = 8092;

    /* initialize the wasm runtime by default configurations */
    wasm_runtime_init();

    /* read WASM file into a memory buffer */
    std::vector<uint8_t> codeBytes; 
    util::readFileToBytes(wasmFilePath, codeBytes);
    size = codeBytes.size();
    
    /* parse the WASM file from buffer and create a WASM module */
    module = wasm_runtime_load(codeBytes.data(), size, error_buf, sizeof(error_buf));
    /* create an instance of the WASM module (WASM linear memory is ready) */
    module_inst = wasm_runtime_instantiate(module, stack_size, heap_size, error_buf, sizeof(error_buf));
    func = wasm_runtime_lookup_function(module_inst, "main", nullptr);
    exec_env = wasm_runtime_create_exec_env(module_inst, stack_size);
    std::vector<uint32_t> argv = {0,0};
    if (wasm_runtime_call_wasm(exec_env, func, 2, argv.data()) ) {
      /* the return value is stored in argv[0] */
      printf("fib function return: %d\n", argv[0]);
    }else printf("%s\n", wasm_runtime_get_exception(module_inst));
    wasm_runtime_destroy_exec_env(exec_env);
    wasm_runtime_deinstantiate(module_inst);
    wasm_runtime_unload(module);
    wasm_runtime_destroy();
    return 0;
}