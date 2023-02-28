/*
    This header defines C++ API interacting with host runtime.
*/
#include <cstdint>
#include <cstdlib>

/*
    Directly return sizeof(arg[index]) when bufferLen is zero
*/
int read_input(int argIndex, unsigned char* buffer, int bufferLen);

void set_output(unsigned char* buffer, int bufferLen);

/*
    Use host to connect proxy server.
    Return value: 0 for None in States ; 1 for successfully reading
*/
int read_state(char* key, unsigned char* buffer, int bufferLen);

int write_state(char* key, unsigned char* buffer, int bufferLen);

uint8_t* wFaaSGetArg(int argIndex, int argLength){
    auto buffer = (unsigned char *)malloc(argLength);
    read_input(argIndex, buffer, argLength);
    return buffer;
}