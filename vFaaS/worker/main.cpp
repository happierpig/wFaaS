#include "worker.hpp"

int main(){
    wasrModule wasmRuntime;
    int _inputSize = wasmRuntime.getInputSize(), _returnSize = wasmRuntime.getReturnSize();
    auto inputBuffer = new unsigned char[_inputSize];
    auto returnBuffer = resultBuffer;
    while(true){
        wasmRuntime.constructRuntime();
        PIPE_COMMAND cmd = util::readPIPECommand(0);
        util::readBytes(0, inputBuffer, _inputSize);
        wasmRuntime.runWasmCode(inputBuffer);
        util::writePIPECommand(1, PIPE_COMMAND_RETURN);
        write(1, returnBuffer, _returnSize);
        wasmRuntime.deconstructRuntime();
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
//     wasmRuntime.constructRuntime();
//     wasmRuntime.runWasmCode(inputBuffer);
//     wasmRuntime.deconstructRuntime();
//     printf("The result being set is %d.",(*(reinterpret_cast<int*> (resultBuffer))));
//     return 0;
// }