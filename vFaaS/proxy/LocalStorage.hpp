#ifndef _LOCALSTORAGE_H
#define _LOCALSTORAGE_H

#include <map>
#include <pthread.h>
#include <alibabacloud/oss/OssClient.h>
using namespace AlibabaCloud::OSS;

extern bool isOss;

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
            printf("\n%s Hold The Lock\n", _request_id.c_str());
        }
    }

    void offLock(){
        printf("\n%s Release The Lock\n", this->request_id.c_str());
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

    /* For aliyun oss */
    std::string AccessKeyId = "LTAI5tRZXe88NZcq97rcYnwZ";
    std::string AccessKeySecret = "tIUcNI88UtCXmXETN2E2mbJYrQtVhg";
    std::string Endpoint = "oss-cn-shanghai-internal.aliyuncs.com";
    std::string BucketName = "xfaas";

    bool ossRead(std::string& key, std::string& value){
        ClientConfiguration conf;
        OssClient client(Endpoint, AccessKeyId, AccessKeySecret, conf);
        GetObjectRequest request(BucketName, key);
        auto outcome = client.GetObject(request);
        if(!outcome.isSuccess()){
            printf("Pity! Fail to read from OSS\n");
            return false;
        }
        auto& stream = outcome.result().Content();
        std::stringstream tmpBuffer;
        tmpBuffer << stream->rdbuf();
        value = tmpBuffer.str();
        return true;
    }

    bool ossWrite(std::string& key, std::string& value){
        ClientConfiguration conf;
        OssClient client(Endpoint, AccessKeyId, AccessKeySecret, conf);
        std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();
        *content << value;
        PutObjectRequest request(BucketName, key, content);
        auto outcome = client.PutObject(request);
        if(!outcome.isSuccess()){
            printf("Pity! Fail to write to OSS\n");
        }
        return outcome.isSuccess();
    }

    public:
    LocalStorage(){
        InitializeSdk();
        pthread_rwlock_init(&mutex, NULL);
        pthread_rwlock_init(&lockMutex, NULL);
    }

    ~LocalStorage(){
        ShutdownSdk();
        pthread_rwlock_destroy(&mutex);
        pthread_rwlock_destroy(&lockMutex);
        for(auto it = lockBase.begin();it != lockBase.end();++it) delete it->second;
    }

    bool readState(std::string& key, std::string& value){
        pthread_rwlock_rdlock(&mutex);
        if(isOss == false){
            auto findPtr = dataBase.find(key);
            if(findPtr == dataBase.end()){
                pthread_rwlock_unlock(&mutex);
                return false;
            }
            value = (*findPtr).second;
        }else{
            if(!ossRead(key, value)){
                pthread_rwlock_unlock(&mutex);
                return false;
            }
        }
        pthread_rwlock_unlock(&mutex);
        return true;
    }
    
    void writeState(std::string& key, std::string& value){
        pthread_rwlock_wrlock(&mutex);
        if(isOss == false){
            dataBase[key] = value;
        }else{
            ossWrite(key, value);
        }
        pthread_rwlock_unlock(&mutex);
    }

    bool readStateLock(std::string& key, std::string& value, const std::string& request_id){
        pthread_rwlock_rdlock(&lockMutex);
        auto findLockPtr = lockBase.find(key);
        uniqueLock* lockPtr = nullptr;
        if(findLockPtr == lockBase.end()){
            pthread_rwlock_unlock(&lockMutex);
            printf("No lock for: %s\n", request_id.c_str());
            return false;
        }else lockPtr = findLockPtr->second;
        pthread_rwlock_unlock(&lockMutex);

        printf("%s Try to get on Lock#1\n", request_id.c_str());
        lockPtr->onLock(request_id);
        printf("%s Try to get on Lock#2\n", request_id.c_str());
        pthread_rwlock_rdlock(&mutex);
        printf("%s get in successfully\n", request_id.c_str());
        if(isOss == false){
            auto findPtr = dataBase.find(key);
            if(findPtr == dataBase.end()){
                pthread_rwlock_unlock(&mutex);
                lockPtr->offLock();
                return false;
            }
            value = (*findPtr).second;
        }else{
            if(!ossRead(key, value)){
                pthread_rwlock_unlock(&mutex);
                lockPtr->offLock();
                return false;
            }
            printf("%s Successfully read from OSS\n", request_id.c_str());
        }
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
        if(isOss == false){
            dataBase[key] = value;
        }else{
            ossWrite(key, value);
            printf("%s Successfully write to OSS\n", request_id.c_str());
        }
        pthread_rwlock_unlock(&mutex);
        lockPtr->offLock();
    }
};

#endif