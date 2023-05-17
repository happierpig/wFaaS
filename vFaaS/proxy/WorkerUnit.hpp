#ifndef _WORKERUNIT_H
#define _WORKERUNIT_H

#include "../include/utils.hpp"
#include <sys/types.h>
#include <signal.h>
#include "../include/functionConfig.hpp"
#include <pthread.h>
#include "LocalStorage.hpp"

using json = nlohmann::json;

functionConfiguration sharedConfig;
extern bool isMain;
extern std::string mainIp;
extern int mainPort;
extern LocalStorage states;

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
        WorkerUnit(){
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

        void setId(int _id){
            this->id = _id;
        }

        int getId(){
            return this->id;
        }

        void runCode(const unsigned char* inputBuffer, int bufferLength, unsigned char* result, int outputLength, std::string& request_id){
            PIPE_COMMAND cmd;
            printf("[DEBUG]%d try read ready signal", this->id);std::cout<<std::endl;
            this->readMsg((unsigned char*)(&cmd), sizeof(PIPE_COMMAND));
            if(cmd != PIPE_COMMAND_READY){
                printf("[DEBUG] Fucking crazy\n");
                throw std::runtime_error("[WorkerUnit] Fail to test worker ready :(");
            }
            this->sendMsg(inputBuffer, bufferLength);
            printf("[DEBUG]%d already send input data, and request_id is %s", this->id, request_id.c_str());std::cout<<std::endl;
            while(true){
                this->readMsg((unsigned char*)(&cmd), sizeof(PIPE_COMMAND));
                if(cmd == PIPE_COMMAND_RETURN){
                    this->readMsg((unsigned char *)result, outputLength);
                    std::cout << "[PIPE] Worker " << this->id << " msg from the other side: " << cmd << "  " << *((int*)result) << std::endl;
                    this->timeStamp = time(nullptr); // Update the last running time
                    //debug
                    std::cout << "[WorkerUnit] " << this->id << " running completes." << std::endl;
                    break;
                }else if(cmd == PIPE_COMMAND_STATE_READ){
                    printf("Start READ %s\n", request_id.c_str());
                    int keyLength, resultLength;
                    this->readMsg((unsigned char*)(&keyLength), sizeof(int));
                    this->readMsg((unsigned char*)(&resultLength), sizeof(int));
                    char * key = new char[keyLength];
                    uint8_t* resultBuffer = new uint8_t[resultLength];
                    this->readMsg((unsigned char*)key, keyLength);

                    /*********/
                    int mode = 0;
                    this->readMsg((unsigned char*)(&mode), sizeof(int));
                    /*********/

                    std::string stateKey(key);
                    std::string value = "";
                    bool exists;
                    if(isMain){
                        if(mode == 0){
                            // mode for non-uniformity
                            exists = states.readState(stateKey, value);
                        }else if(mode == 1){
                            exists = states.readStateLock(stateKey, value, request_id);
                        }
                    }else{
                        json reqData;
                        reqData["key"] = stateKey;
                        reqData["mode"] = mode; // Pass responsibility for proxy to deal with the uniformity
                        if(mode == 1) reqData["request_id"] = request_id;
                        httplib::Client cli("172.17.0.1", mainPort); //todo
                        cli.set_connection_timeout(30, 0); // 300 milliseconds
                        cli.set_read_timeout(20, 0); // 5 seconds
                        cli.set_write_timeout(20, 0);
                        auto res = cli.Post("/state/read", reqData.dump(), "application/json");
                        printf("Read,id: %s, error info %s\n",request_id.c_str(), to_string(res.error()).c_str());
                        if(res && res->status == 200){
                            printf("SUCEESS READING\n");
                            auto decodedJson = json::parse(res->body);
                            exists = decodedJson["exists"];
                            value = decodedJson["value"];
                        }else exists = false; // Attention : maybe is network problem.
                    }
                    if(exists){
                        cmd = PIPE_COMMAND_STATE_FOUND;
                        this->sendMsg((unsigned char*)(&cmd), sizeof(PIPE_COMMAND));
                        util::readFromJson(value, resultBuffer, resultLength);
                        this->sendMsg(resultBuffer, resultLength);
                    }else{
                        cmd = PIPE_COMMAND_STATE_NOT_FOUND;
                        this->sendMsg((unsigned char*)(&cmd), sizeof(PIPE_COMMAND));
                    }
                    delete [] key;
                    delete [] resultBuffer;
                }else if(cmd == PIPE_COMMAND_STATE_WRITE){
                    printf("Start WRITE %s\n", request_id.c_str());
                    int keyLength,valueLength;
                    this->readMsg((unsigned char*)(&keyLength), sizeof(int));
                    this->readMsg((unsigned char*)(&valueLength), sizeof(int));
                    char *key = new char[keyLength];
                    uint8_t* valueBuffer = new uint8_t[valueLength];
                    this->readMsg((unsigned char*)key, keyLength);
                    this->readMsg((unsigned char*)valueBuffer, valueLength);

                    /********/
                    int mode;
                    this->readMsg((unsigned char*)(&mode), sizeof(int));
                    /********/

                    std::string stateKey(key);
                    std::string stateValue = util::writeToJson(valueBuffer, valueLength);
                    bool success = true;
                    if(isMain){
                        if(mode == 0){
                            // for non-uniformity data
                            states.writeState(stateKey, stateValue);
                        }else if(mode == 1){
                            states.writeStateLock(stateKey, stateValue, request_id);
                        }
                    }else{
                        json reqData;
                        reqData["key"] = stateKey;
                        reqData["value"] = stateValue;
                        reqData["mode"] = mode;
                        if(mode == 1) reqData["request_id"] = request_id;
                        httplib::Client cli("172.17.0.1", mainPort); //todo MACRO DEF
                        cli.set_connection_timeout(30, 0); // 300 milliseconds
                        cli.set_read_timeout(20, 0); // 5 seconds
                        cli.set_write_timeout(20, 0);
                        auto res = cli.Post("/state/write", reqData.dump(), "application/json");
                        printf("Write,id: %s, error info %s\n",request_id.c_str(), to_string(res.error()).c_str());
                        if(res && res->status == 200) success = true;
                        else success = false;
                    }
                    cmd = ((success == true) ? PIPE_COMMAND_STATE_FOUND : PIPE_COMMAND_STATE_NOT_FOUND);
                    this->sendMsg((unsigned char *)(&cmd), sizeof(PIPE_COMMAND));
                    delete [] key;
                    delete [] valueBuffer;
                }
            }
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

        bool getStatus(){
            return this->idle;
        }

        bool checkValid(){
            // debug
            std::cout << "[Expire] Checking " << this->id << "'s Validity: " << ((difftime(time(nullptr), timeStamp) < expireTime) ? "True" : "False") << std::endl;
            return (difftime(time(nullptr), timeStamp) < expireTime);
        }
};

#endif