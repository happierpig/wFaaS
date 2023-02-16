// #define DEBUG

#include "../include/httplib.h"
#include "../include/utils.hpp"
#include "../include/json.hpp"
#include "WorkerPool.hpp"
#include "LocalStorage.hpp"

using json = nlohmann::json;

WorkerPool runner;
LocalStorage states;

std::string status = "ok";
std::string functionName;
std::string containerID;



int main(){
    httplib::Server svr;

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
        auto decodedJson = json::parse(req.body);
        functionName = decodedJson["function"];
        containerID = decodedJson["id"];
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

        timeval startTime, endTime;
        gettimeofday(&startTime, NULL);
        bool newWorker = runner.dispatch_request(inputBuffer, returnBuffer);
        gettimeofday(&endTime, NULL);

        #ifdef DEBUG
        std::cout << "[Proxy] 2." << std::endl; 
        std::cout << "Return Value :" << *((int*)(returnBuffer)) << std::endl;
        #endif

        json data;
        std::string returnString = "";
        if(sharedConfig.return_size > 0) returnString = util::writeToJson(returnBuffer, sharedConfig.return_size);
        data["start_time"] = startTime.tv_sec + startTime.tv_usec / 1000000;
        data["end_time"] = endTime.tv_sec + endTime.tv_usec / 1000000;
        data["duration"] = (endTime.tv_sec + endTime.tv_usec / 1000000) - (startTime.tv_sec + startTime.tv_usec / 1000000);
        data["output"] = returnString;
        data["new_worker"] = newWorker;

        delete [] inputBuffer;
        delete [] returnBuffer;

        #ifdef DEBUG
        std::cout << "[Proxy] 3." << std::endl; 
        #endif

        res.set_content(data.dump(), "application/json");
    });

    /*
        Handler for reading states
    */
    svr.Post("/state/read", [](const httplib::Request& req, httplib::Response& res) {
        auto decodedJson = json::parse(req.body);
        std::string key = decodedJson["key"];
        std::string value = "";
        bool exists = states.readState(key, value);
        
        json data;
        data["exists"] = exists;
        data["value"] = value;
        res.set_content(data.dump(), "application/json");
    });

    /*
        Handler for modifying states
    */
    svr.Post("/state/write", [](const httplib::Request& req, httplib::Response& res) {
        auto decodedJson = json::parse(req.body);
        std::string key = decodedJson["key"];
        std::string value = decodedJson["value"];
        states.writeState(key, value);
        res.status = 200;
    });

    std::cout << "[Proxy] Start running." << std::endl;
    svr.listen("127.0.0.1", 18000);
}