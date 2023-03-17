#ifndef _LOCALSTORAGE_H
#define _LOCALSTORAGE_H

#include <map>
#include <pthread.h>

class uniqueLock{
    private:
    std::string request_id;
    pthread_mutex_t realLock;
    
    public:
    uniqueLock(){
        pthread_mutex_init(&realLock, NULL);
        request_id = "";
    }
    
    ~uniqueLock(){
        pthread_mutex_destroy(&realLock);
    }

    void onLock(const std::string & _request_id){
        if(_request_id == this->request_id) return;
        else{
            pthread_mutex_lock(&realLock);
            this->request_id = _request_id;
        }
    }

    void offLock(){
        this->request_id = "";
        pthread_mutex_unlock(&realLock);
    }
};

class LocalStorage{
    private:
    std::map<std::string, std::string> dataBase;
    std::map<std::string, uniqueLock*> lockBase;
    pthread_rwlock_t mutex;
    pthread_rwlock_t lockMutex;

    public:
    LocalStorage(){
        pthread_rwlock_init(&mutex, NULL);
        pthread_rwlock_init(&lockMutex, NULL);
    }

    ~LocalStorage(){
        pthread_rwlock_destroy(&mutex);
        pthread_rwlock_destroy(&lockMutex);
        for(auto it = lockBase.begin();it != lockBase.end();++it) delete it->second;
    }

    bool readState(std::string& key, std::string& value){
        pthread_rwlock_rdlock(&mutex);
        auto findPtr = dataBase.find(key);
        if(findPtr == dataBase.end()){
            pthread_rwlock_unlock(&mutex);
            return false;
        }
        value = (*findPtr).second;
        pthread_rwlock_unlock(&mutex);
        return true;
    }
    
    void writeState(std::string& key, std::string& value){
        pthread_rwlock_wrlock(&mutex);
        dataBase[key] = value;
        pthread_rwlock_unlock(&mutex);
    }

    bool readStateLock(std::string& key, std::string& value, const std::string& request_id){
        pthread_rwlock_rdlock(&lockMutex);
        auto findLockPtr = lockBase.find(key);
        uniqueLock* lockPtr = nullptr;
        if(findLockPtr == lockBase.end()){
            pthread_rwlock_unlock(&lockMutex);
            return false;
        }else lockPtr = findLockPtr->second;
        pthread_rwlock_unlock(&lockMutex);

        lockPtr->onLock(request_id);
        pthread_rwlock_rdlock(&mutex);
        auto findPtr = dataBase.find(key);
        if(findPtr == dataBase.end()){
            pthread_rwlock_unlock(&mutex);
            lockPtr->offLock();
            return false;
        }
        value = (*findPtr).second;
        pthread_rwlock_unlock(&mutex);
        // Keep holding the lock;
        return true;
    }

    void writeStateLock(std::string& key, std::string&value, const std::string& request_id){
        pthread_rwlock_wrlock(&lockMutex);
        auto findLockPtr = lockBase.find(key);
        uniqueLock* lockPtr = nullptr;
        if(findLockPtr == lockBase.end()){
            lockPtr = new uniqueLock();
            lockBase[key] = lockPtr;
        }else lockPtr = (*findLockPtr).second;
        pthread_rwlock_unlock(&lockMutex);

        lockPtr->onLock(request_id);
        pthread_rwlock_wrlock(&mutex);
        dataBase[key] = value;
        pthread_rwlock_unlock(&mutex);
        lockPtr->offLock();
    }
};

#endif