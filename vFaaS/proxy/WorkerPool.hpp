#ifndef _WORKERPOOL_H
#define _WORKERPOOL_H

#include "WorkerUnit.hpp"
#include <cassert>

static void* daemon_cleaner(void* ptr);

extern std::string status; // from Proxy.cpp

class WorkerPool{
    public:

    vector<WorkerUnit*> pool;
    pthread_mutex_t mutex;

    int getAliveWorker(){
        return pool.size();
    }

    WorkerUnit* addWorker(){
        WorkerUnit* ptr = new WorkerUnit();
        ptr->tryOccupy();
        pthread_mutex_lock(&mutex);
        ptr->setId(pool.size());
        pool.push_back(ptr);
        pthread_mutex_unlock(&mutex);
        return ptr;
    }

    public:

    WorkerPool(){
        sharedConfig.setFromFile(functionConfigPath);
        pthread_mutex_init(&mutex, NULL);
        // pool.push_back(new WorkerUnit(30, 0)); // The first process last more time and this is moved to proxy init.
        // Update daemon thread
        pthread_t ppid;
        pthread_create(&ppid, NULL, daemon_cleaner, (void*)this);
    }

    ~WorkerPool(){
        pthread_mutex_destroy(&mutex);
        for(int i=0;i < getAliveWorker();++i) delete pool[i];
    }

    bool dispatch_request(const unsigned char* inputBuffer, unsigned char* resultBuffer, std::string& request_id, int* echo){
        WorkerUnit* candidate = nullptr;
        bool flag = false;

        // timeval startTime1, endTime1, startTime2, endTime2;
        
        if((*echo) == -1){  // First pass for one request
            pthread_mutex_lock(&mutex);
            printf("------Start Dealing------");std::cout<<std::endl;

            for(int i = 0;i < getAliveWorker();++i){
                candidate = pool[i];
                std::cout << "[Dispatch] Test the " << i << "-th worker "; // debug
                if(candidate->tryOccupy()){
                    std::cout << "Success" << std::endl; // debug
                    flag = true;
                    break;
                }else std::cout << "Fail" << std::endl; // debug
            }
            pthread_mutex_unlock(&mutex);

            if(!flag){
                /* Directly Return Without Running the Code for Cgroup */
                candidate = addWorker(); // Add new WASM Process in it; May cause id bug but no problem
                status = "run";
                *echo = candidate->getId();
                return flag;
            }
        }else{ // Second Pass
            assert(getAliveWorker() > (*echo));
            candidate = pool[(*echo)];
            assert(candidate->getStatus() == false);
            flag = true;
        }
        
        printf("[WorkerUnit] Worker %d-th Start runing", candidate->getId());std::cout<<std::endl;
        *echo = candidate->getId();
        candidate->runCode(inputBuffer, sharedConfig.getInputSize(), resultBuffer, sharedConfig.return_size, request_id);
        candidate->setIdle(true);

        // gettimeofday(&endTime2, NULL);
        // (*duration2) = (endTime1.tv_sec - startTime1.tv_sec) * 1000000 + (endTime1.tv_usec - startTime1.tv_usec);
        return flag;
    }

    void try_remove(){
        if(getAliveWorker() == 0) return;
        pthread_mutex_lock(&mutex);
        WorkerUnit* ptr = pool[getAliveWorker() - 1];
        bool flag = ptr->tryOccupy(); // Maybe in running
        if(!flag){ // Fail to lock
            pthread_mutex_unlock(&mutex);
            return;
        }
        if(ptr->checkValid()){
            ptr->setIdle(true);
            pthread_mutex_unlock(&mutex);
            return;
        }
        pool.pop_back();
        if(getAliveWorker() == 0) status = "idle";
        pthread_mutex_unlock(&mutex);
        delete ptr; // Avoid longtime mutex
    }
};

static void* daemon_cleaner(void* ptr){
    while(true){
        sleep(10);
        ((WorkerPool*)ptr)->try_remove();
    }
}

#endif