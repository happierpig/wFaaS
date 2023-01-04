#include "native.hpp"
#include "utils.hpp"
#include "functionConfig.hpp"

static std::string wasmFilePath = "../wasmCode/test.wasm";
static const char* functionConfigPath = "../wasmCode/func.config";

class wasrModule{
  private:
    char error_buf[128];
    wasm_module_t module;
    wasm_module_inst_t module_inst;
    wasm_function_inst_t func;
    wasm_exec_env_t exec_env;

    std::vector<uint8_t> codeBytes;
    functionConfiguration funcConfig;

    void collectInput(const unsigned char* inputBuffer, int inputLength){
      int cPos = 0;
      for(int i = 0;i < argCollection.size();++i){
        int argLen = funcConfig.argsv[i];
        uint8_t* tmpPtr = new uint8_t[argLen];
        std::copy(inputBuffer+cPos, inputBuffer+cPos+argLen, tmpPtr);
        argCollection[i] = tmpPtr;
        cPos += argLen;
      }
      if(cPos != inputLength) throw "[WASR] Dismatch arguments input.";
    }

    void cleanInput(){
      for(int i = 0;i < argCollection.size();++i)
        free(argCollection[i]);
    }
    
  public:

    wasrModule(){
      funcConfig.setFromFile(functionConfigPath);
      util::readFileToBytes(wasmFilePath, codeBytes);
      resultBuffer = new uint8_t[funcConfig.return_size];

      argCollection.resize(funcConfig.argc);
      argLengths = funcConfig.argsv;
    }

    ~wasrModule(){
      free(resultBuffer);
    }

    void constructRuntime(){
      wasm_runtime_init();
      if(!wasm_runtime_register_natives("env", ns, sizeof(ns) / sizeof(NativeSymbol))) 
        throw "[Runtime] Fail to register the native fucntion.";
      module = wasm_runtime_load(codeBytes.data(), codeBytes.size(), error_buf, sizeof(error_buf));
      module_inst = wasm_runtime_instantiate(module, funcConfig.stack_size, funcConfig.heap_size, error_buf, sizeof(error_buf));
      func = wasm_runtime_lookup_function(module_inst, "main", nullptr);
      exec_env = wasm_runtime_create_exec_env(module_inst, funcConfig.stack_size);
    }

    void deconstructRuntime(){
      wasm_runtime_destroy_exec_env(exec_env);
      wasm_runtime_deinstantiate(module_inst);
      wasm_runtime_unload(module);
      wasm_runtime_destroy();
    }

    //todo: add setReturn value host_interface function
    void runWasmCode(unsigned char* _resultBuffer, const unsigned char* inputBuffer, int inputLength){
      collectInput(inputBuffer, inputLength);
      std::vector<uint32_t> argv = {0,0};
      // Call the wasm code and the argument get from native function
      if (wasm_runtime_call_wasm(exec_env, func, 2, argv.data()) ) {
        /* the return value is stored in argv[0] */
        printf("fib function return: %d\n", argv[0]);
      }else printf("%s\n", wasm_runtime_get_exception(module_inst));
      cleanInput();
      std::copy(resultBuffer, resultBuffer+funcConfig.getInputSize(), _resultBuffer);
    }

};