#include "WorkerUnit.hpp"

class WorkerPool;

static void* daemon_cleaner(void* ptr){
    while(true){
        sleep(10);
        ((WorkerPool*)ptr)->try_remove();
    }
}

class WorkerPool{
    private:

    vector<WorkerUnit*> pool;
    pthread_mutex_t mutex;

    int getAliveWorker(){
        return pool.size();
    }

    WorkerUnit* addWorker(){
        WorkerUnit* ptr = new WorkerUnit();
        ptr->tryOccupy();
        pthread_mutex_lock(&mutex);
        pool.push_back(ptr);
        pthread_mutex_unlock(&mutex);
        return ptr;
    }

    public:

    WorkerPool(){
        pthread_mutex_init(&mutex, NULL);
        pool.push_back(new WorkerUnit(60)); // The first process last more time.
        // Update daemon thread
        pthread_t ppid;
        pthread_create(&ppid, NULL, daemon_cleaner, (void*)this);
    }

    ~WorkerPool(){
        pthread_mutex_destroy(&mutex);
    }

    bool dispatch_request(const unsigned char* inputBuffer, unsigned char* resultBuffer){
        WorkerUnit* candidate = nullptr;
        bool flag = false;
        pthread_mutex_lock(&mutex);
        for(int i = 0;i < getAliveWorker();++i){
            candidate = pool[i];
            if(candidate->tryOccupy()){
                flag = true;
                break;
            }
        }
        pthread_mutex_unlock(&mutex);
        if(!flag) candidate = addWorker(); // Add new WASM Process in it;
        candidate->runCode(inputBuffer, sharedConfig.getInputSize(), resultBuffer, sharedConfig.return_size);
    }

    void try_remove(){
        pthread_mutex_lock(&mutex);
        WorkerUnit* ptr = pool[getAliveWorker() - 1];
        bool flag = ptr->tryOccupy();
        if((!flag) || ptr->checkValid()){
            pthread_mutex_unlock(&mutex);
            return;
        }
        delete ptr;
        pool.pop_back();
        pthread_mutex_unlock(&mutex);
    }
};