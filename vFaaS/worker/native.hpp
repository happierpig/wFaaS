/*
    This file used to define the native function hooked by host_interface.h .
*/

#include <cstdint>
#include <vector>
#include <wasm_exec_env.h>
#include <wasm_export.h>
#include "../include/httplib.h"
#include "../include/json.hpp"
#include "../include/utils.hpp"

using json = nlohmann::json;

std::vector<int> argLengths;
std::vector<uint8_t*> argCollection;
uint8_t* resultBuffer;
int resultLength;

static int read_input_native(wasm_exec_env_t exec_env , int32_t argIndex, uint8_t* inBuffer, int32_t inLength){
    if(argIndex >= argLengths.size()) throw "[Host_Iface_Func] Arg index out of bound";
    if(inLength == 0) return argLengths[argIndex];
    if(inLength != argLengths[argIndex]) throw "[Host_Iface_Func] Wrong arg length";
    uint8_t* tmpPtr = argCollection[argIndex];
    std::copy(tmpPtr, tmpPtr+inLength, inBuffer);
    return inLength;
}

static void set_output_native(wasm_exec_env_t exec_env, uint8_t* inBuffer, int32_t inLength){
    if(inLength != resultLength) throw "[Host_Iface_Func] Return Buffer Length doesn't match.";
    std::copy(inBuffer, inBuffer+inLength, resultBuffer);
}

static int read_state_native(wasm_exec_env_t exec_env, char* key, uint8_t* buffer, int32_t buffLength){
    PIPE_COMMAND cmd = PIPE_COMMAND_STATE_READ;
    int keyLength = strlen(key) + 1;
    write(10, (unsigned char*)(&keyLength), sizeof(int));
    write(10, (unsigned char*)(&buffLength), sizeof(int));
    write(10, (unsigned char*) key, keyLength);
    util::readBytes(0, (unsigned char*)(&cmd), sizeof(PIPE_COMMAND));
    if(cmd == PIPE_COMMAND_STATE_NOT_FOUND) return 0;
    else{
        util::readBytes(0, (unsigned char*)buffer, buffLength);
        return 1;
    }
}

static void write_state_native(wasm_exec_env_t exec_env, char* key, uint8_t* buffer, int32_t buffLength){
    PIPE_COMMAND cmd = PIPE_COMMAND_STATE_WRITE;
    int keyLength = strlen(key) + 1;
    write(10, (unsigned char*)(&keyLength), sizeof(int));
    write(10, (unsigned char*)(&buffLength), sizeof(int));
    write(10, (unsigned char*)key, keyLength);
    write(10, (unsigned char*)buffer, buffLength);
    //todo: whether to wait?
}

static NativeSymbol ns[] = {
    {
        "_Z10read_inputiPhi", 	    // the name of WASM function name
     	(void *)read_input_native,    // the native function pointer
        "(i*~)i"		           // the function prototype signature
    },
    {
        "_Z10set_outputPhi",
        (void *)set_output_native,
        "(*~)"
    },
    {
        "_Z10read_stateiPhi",
        (void *)read_state_native,
        "($*~)i"
    },
    {
        "_Z10write_statePhi",
        (void *)write_state_native,
        "($*~)"
    }
};
