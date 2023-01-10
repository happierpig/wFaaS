#include "../include/utils.hpp"
#include <map>
#include <pthread.h>

class LocalStorage{
    private:
    std::map<std::string, std::string> dataBase;
    pthread_rwlock_t mutex;

    public:
    LocalStorage(){
        pthread_rwlock_init(&mutex, NULL);
    }

    ~LocalStorage(){
        pthread_rwlock_destroy(&mutex);
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
    
    void writeState(std::string& key, std::string&value){
        pthread_rwlock_wrlock(&mutex);
        dataBase[key] = value;
        pthread_rwlock_unlock(&mutex);
    }
};