/*
    This header defines C++ API interacting with host runtime.
*/
#include <cstdint>
#include <cstdlib>


#define HOST_FUNC __attribute__((weak))

/*
    Directly return sizeof(arg[index])
*/

int read_input(int argIndex, unsigned char* buffer, int bufferLen);

uint8_t* wFaaSGetArg(int argIndex){
    uint8_t occupy[1] = {0};
    int argLenth = read_input(argIndex, occupy, 0);
    auto buffer = (unsigned char *)malloc(argLenth);
    read_input(argIndex, buffer, argLenth);
    return buffer;
}