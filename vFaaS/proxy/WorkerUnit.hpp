#include "../include/utils.hpp"
#include <sys/types.h>
#include <signal.h>
#include "../include/functionConfig.hpp"
#include <pthread.h>

functionConfiguration sharedConfig;

class WorkerUnit{
    private:
        int workerPid;
        int readPipe;
        int writePipe;

        int id;

        time_t timeStamp;
        double expireTime = 15;

        bool idle;
        pthread_mutex_t mutex;

        void sendMsg(const unsigned char* buffer, int bufferLength){
            write(writePipe, buffer, bufferLength);
        }

        void readMsg(unsigned char* buffer, int bufferLength){
            util::readBytes(readPipe, buffer, bufferLength);
        }

    public:
        WorkerUnit(int _id){
            id = _id;
            idle = true;
            timeStamp = time(nullptr);
            if(! util::startWorker(&workerPid, &writePipe, &readPipe))
                throw "Fail to create a Process";
            pthread_mutex_init(&mutex, NULL);
        }

        WorkerUnit(double _expireTime, int _id){
            id = _id;
            idle = true;
            timeStamp = time(nullptr);
            expireTime = _expireTime;
            if(! util::startWorker(&workerPid, &writePipe, &readPipe))
                throw "Fail to create a Process";
            
            pthread_mutex_init(&mutex, NULL);
        }

        ~WorkerUnit(){
            close(readPipe);
            close(writePipe);
            pthread_mutex_destroy(&mutex);
            if(workerPid > 0) kill(workerPid, SIGKILL);
            // todo: Maybe need to wait to repead the process.
        }

        void runCode(const unsigned char* inputBuffer, int bufferLength, unsigned char* result, int outputLength){
            PIPE_COMMAND cmd = PIPE_COMMAND_INPUT;
            this->sendMsg((unsigned char*)(&cmd), sizeof(PIPE_COMMAND));
            this->sendMsg(inputBuffer, bufferLength);

            this->readMsg((unsigned char*)(&cmd), sizeof(PIPE_COMMAND));
            this->readMsg((unsigned char *)result, outputLength);
            std::cout << "[PIPE] msg from the other side: " << cmd << "  " << *((int*)result) << std::endl;
            this->timeStamp = time(nullptr); // Update the last running time

            //debug
            std::cout << "[WorkerUnit] " << this->id << " running completes." << std::endl;
        }

        bool tryOccupy(){
            pthread_mutex_lock(&mutex);
            bool returnFlag = this->idle;
            this->idle = false;
            pthread_mutex_unlock(&mutex);
            return returnFlag;
        }

        void setIdle(bool flag){
            pthread_mutex_lock(&mutex);
            this->idle = flag;
            pthread_mutex_unlock(&mutex);
        }

        bool checkValid(){
            // debug
            std::cout << "[Expire] Checking " << this->id << "'s Validity: " << ((difftime(time(nullptr), timeStamp) < expireTime) ? "True" : "False") << std::endl;
            return (difftime(time(nullptr), timeStamp) < expireTime);
        }
};