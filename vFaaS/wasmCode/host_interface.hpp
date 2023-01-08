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

void set_output(unsigned char* buffer, int bufferLen);

uint8_t* wFaaSGetArg(int argIndex, int argLength){
    auto buffer = (unsigned char *)malloc(argLength);
    read_input(argIndex, buffer, argLength);
    return buffer;
}