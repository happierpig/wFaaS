#include "worker/utils.hpp"
#include <sys/types.h>
#include <signal.h>

class WorkerUnit{
    private:
        int workerPid;
        int readPipe;
        int writePipe;

        void sendMsg(const unsigned char* buffer, int bufferLength){
            write(writePipe, buffer, bufferLength);
        }

        void readMsg(unsigned char* buffer, int bufferLength){
            util::readBytes(readPipe, buffer, bufferLength);
        }

    public:
        WorkerUnit(){
           if(! util::startWorker(&workerPid, &writePipe, &readPipe))
            throw "Fail to create a Process";
        }

        ~WorkerUnit(){
            close(readPipe);
            close(writePipe);
            if(workerPid > 0) kill(workerPid, SIGKILL);
            // todo: Maybe need to wait to repead the process.
        }

        void runCode(const unsigned char* inputBuffer, int bufferLength, unsigned char* result, int outputLength){
            PIPE_COMMAND cmd = PIPE_COMMAND_INPUT;
            this->sendMsg((unsigned char*)(&cmd), sizeof(PIPE_COMMAND));
            this->sendMsg(inputBuffer, bufferLength);
            this->readMsg((unsigned char*)(&cmd), sizeof(PIPE_COMMAND));
            this->readMsg((unsigned char *)result, outputLength);
            std::cout << "[PIPE] msg from the other side: " << cmd << "  " << (char*)result << std::endl;
        }
};