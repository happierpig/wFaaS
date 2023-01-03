#include "WorkerUnit.hpp"

int main(){
    WorkerUnit *testWorker = new WorkerUnit();
    char* msg = "12345"; 
    char* output = new char[6];
    testWorker->runCode((unsigned char*) msg, 6, (unsigned char*) output, 6);
    std::cout << output << std::endl;
    free(testWorker);
    free(output);
    return 0;
}