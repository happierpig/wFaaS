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
    std::string stateKey(key);

    json reqData;
    reqData["key"] = stateKey;
    httplib::Client cli("127.0.0.1", 18000);
    auto res = cli.Post("/state/read", reqData.dump(), "application/json");
    
    if(res && res->status == 200){
        auto decodedJson = json::parse(res->body);
        bool exists = decodedJson["exists"];
        if(!exists) return 0;
        std::string valueString = decodedJson["value"];
        util::readFromJson(valueString, buffer, buffLength);
        return 1;
    }else throw "[Host_Iface_Func] Call Proxy Server for Reading state Fails!";
}

static void write_state_native(wasm_exec_env_t exec_env, char* key, uint8_t* buffer, int32_t buffLength){
    std::string stateKey(key);
    std::string stateValue = util::writeToJson(buffer, buffLength);
    json reqData;
    reqData["key"] = stateKey;
    reqData["value"] = stateValue;
    httplib::Client cli("127.0.0.1", 18000);
    auto res = cli.Post("/state/write", reqData.dump(), "application/json");
    
    if(res && res->status == 200) return;
    else throw "[Host_Iface_Func] Call Proxy Server for Writing state Fails!";
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
