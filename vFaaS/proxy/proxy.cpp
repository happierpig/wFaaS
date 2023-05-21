// #define DEBUG
#define CPPHTTPLIB_THREAD_POOL_COUNT 128

#include "../include/httplib.h"
#include "../include/utils.hpp"
#include "../include/json.hpp"
#include "WorkerPool.hpp"
#include "LocalStorage.hpp"

using json = nlohmann::json;

WorkerPool runner;
LocalStorage states;

std::string status = "idle"; // idle init ok run
std::string functionName;
std::string containerID;
bool isMain = false;
std::string mainIp = "";
int mainPort;

bool isOss = false;
bool isRedis = false;

int main(){
    httplib::Server svr;
    svr.set_read_timeout(20, 0); // 5 seconds
    svr.set_write_timeout(20, 0); // 5 seconds

    /*
        Handler represents the success of container initializing.
    */
    svr.Get("/status", [](const httplib::Request& req, httplib::Response& res){
        json data;
        data["status"] = status;
        data["workdir"] = get_current_dir_name();
        res.set_content(data.dump(), "application/json");
    });

    /*
        Handler for initializing the Proxy
    */
    svr.Post("/init", [](const httplib::Request& req, httplib::Response& res) {
        status = "init";
        // Allocate the first worker which last for more seconds.
        runner.pool.push_back(new WorkerUnit(30, 0));
        
        auto decodedJson = json::parse(req.body);
        functionName = decodedJson["function"];
        containerID = decodedJson["id"];
        isMain = decodedJson["isMain"];
        mainIp = decodedJson["ip"];
        mainPort = decodedJson["mainPort"];
        /*
            For data transfer via Oss.
        */
        isOss = decodedJson["isOss"];
        isRedis = decodedJson["isRedis"];
        
        status = "ok";
        res.status = 200;
    });

    /*
        Handler for running the function
    */
    svr.Post("/run", [](const httplib::Request& req, httplib::Response& res) {
        auto decodedJson = json::parse(req.body);

        #ifdef DEBUG
        std::cout << "[Proxy] Request Running: " << decodedJson.dump() << std::endl; 
        #endif

        std::string inputString = decodedJson["input"];
        std::string requestId = decodedJson["request_id"];
        int echo = decodedJson["echo"]; // This is for newly created worker.
        uint8_t* inputBuffer = new uint8_t[sharedConfig.getInputSize()];
        uint8_t* returnBuffer = new uint8_t[sharedConfig.return_size];

        #ifdef DEBUG
        memset(returnBuffer, 0, sharedConfig.return_size);
        std::cout << "[Proxy] 0. " << inputString << std::endl; 
        std::cout << "[Proxy] Check Endian." << std::endl;
        int tmpx = 1;
        char* tmpy = (char*)(&tmpx);
        if(*tmpy) std::cout << "Little Endian" << std::endl;
        else std::cout << "Big Endian" << std::endl;
        #endif

        util::readFromJson(inputString, inputBuffer, sharedConfig.getInputSize());

        int duration2, duration3;

        timeval startTime, endTime;
        gettimeofday(&startTime, NULL);
        bool newWorker = runner.dispatch_request(inputBuffer, returnBuffer, requestId, &echo);
        gettimeofday(&endTime, NULL);

        #ifdef DEBUG
        std::cout << "[Proxy] 2." << std::endl; 
        std::cout << "Return Value :" << *((int*)(returnBuffer)) << std::endl;
        #endif

        json data;
        std::string returnString = "";
        if(sharedConfig.return_size > 0) returnString = util::writeToJson(returnBuffer, sharedConfig.return_size);
        data["duration"] = (endTime.tv_sec - startTime.tv_sec) * 1000000 + (endTime.tv_usec - startTime.tv_usec);
        data["output"] = returnString;
        data["new_worker"] = newWorker;
        data["request_id"] = requestId;
        data["echo"] = echo; // This is for second call.

        delete [] inputBuffer;
        delete [] returnBuffer;

        #ifdef DEBUG
        std::cout << "[Proxy] 3." << std::endl; 
        #endif

        res.set_content(data.dump(), "application/json");
        res.status = 200;
    });

    /*
        Handler for reading states
    */
    svr.Post("/state/read", [](const httplib::Request& req, httplib::Response& res) {
        static int x = 0; x = x + 1;
        auto decodedJson = json::parse(req.body);
        std::string key = decodedJson["key"];
        int mode = decodedJson["mode"];
        std::string value = "";
        bool exists;
        if(mode == 0){
            // non-uniformity
            exists = states.readState(key, value);
        }else if(mode == 1){
            std::string _request_id = decodedJson["request_id"];
            printf("Read****RequestId: %s*****And the value is: %d\n", _request_id.c_str(),x);
            exists = states.readStateLock(key, value, _request_id);
        }
        
        json data;
        data["exists"] = exists;
        data["value"] = value;
        res.set_content(data.dump(), "application/json");
        res.status = 200;
    });

    /*
        Handler for modifying states
    */
    svr.Post("/state/write", [](const httplib::Request& req, httplib::Response& res) {
        static int x = 0; x += 1;
        auto decodedJson = json::parse(req.body);
        std::string key = decodedJson["key"];
        std::string value = decodedJson["value"];
        int mode = decodedJson["mode"];
        if(mode == 0){
            // for non-uniformity
            states.writeState(key, value);
        }else if(mode == 1){
            std::string _request_id = decodedJson["request_id"];
            printf("Write****RequestId: %s*****And the value is: %d\n", _request_id.c_str(),x);
            states.writeStateLock(key, value, _request_id);
        }
        res.status = 200;
    });

    printf("[Proxy] Start running at %d pool size.\n", CPPHTTPLIB_THREAD_POOL_COUNT);
    svr.listen("0.0.0.0", 18000);
}