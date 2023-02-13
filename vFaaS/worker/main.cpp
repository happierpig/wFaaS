#include "worker.hpp"
#include <stdlib.h>

int main(){
    srand(time(NULL));
    wasrModule wasmRuntime;
    int _inputSize = wasmRuntime.getInputSize(), _returnSize = wasmRuntime.getReturnSize();
    auto inputBuffer = new unsigned char[_inputSize];
    auto returnBuffer = resultBuffer;
    while(true){
        PIPE_COMMAND cmd = util::readPIPECommand(0);
        util::readBytes(0, inputBuffer, _inputSize);
        wasmRuntime.runWasmCode(inputBuffer);
        cmd = PIPE_COMMAND_RETURN;
        util::writePIPECommand(10, cmd);
        write(10, returnBuffer, _returnSize);
    }
    delete[] inputBuffer;
    return 0;
}

// int main(){
//     wasrModule wasmRuntime;
//     int _inputSize = wasmRuntime.getInputSize(), _returnSize = wasmRuntime.getReturnSize();
//     auto inputBuffer = new uint8[8];
//     int x = 1,y = 2;
//     std::copy((uint8*)(&x), ((uint8*)(&x)+4),inputBuffer);
//     std::copy((uint8*)(&y), ((uint8*)(&y)+4),inputBuffer+4);
//     fstream tmpfile; tmpfile.open("/home/dreamer/epcc/vFaaS/test.txt", std::ios::out | std::ios::in);
//     for(int i = 1;i < 10;++i){
//         wasmRuntime.runWasmCode(inputBuffer);
//         printf("The result being set is %d.",(*(reinterpret_cast<int*> (resultBuffer))));
//     }
//     srand(time(NULL));
//     int id = rand();
//     std::cout << "Identifier Number: " << id << std::endl;
//     tmpfile << id;
//     return 0;
// }