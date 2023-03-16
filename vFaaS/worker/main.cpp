#include "worker.hpp"
#include <time.h>

int main(){
    wasrModule wasmRuntime;
    int _inputSize = wasmRuntime.getInputSize(), _returnSize = wasmRuntime.getReturnSize();
    auto inputBuffer = new unsigned char[_inputSize];
    auto returnBuffer = resultBuffer;
    std::cout << "[Worker] New Worker!!" << std::endl;
    while(true){
        PIPE_COMMAND cmd = PIPE_COMMAND_READY;
        util::writePIPECommand(PIPE_WRITE_FD, cmd);
        
        util::readBytes(0, inputBuffer, _inputSize);

        timeval startTime, endTime;
        gettimeofday(&startTime, NULL);
        wasmRuntime.runWasmCode(inputBuffer);
        gettimeofday(&endTime, NULL);
        std::cout << "[InnerWorker] time running cost: " << endTime.tv_usec - startTime.tv_usec << std::endl;


        cmd = PIPE_COMMAND_RETURN;
        util::writePIPECommand(PIPE_WRITE_FD, cmd);
        write(PIPE_WRITE_FD, returnBuffer, _returnSize);
    }
    delete[] inputBuffer;
    return 0;
}