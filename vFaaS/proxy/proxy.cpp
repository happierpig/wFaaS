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
        std::string inputString = decodedJson["input"];
        uint8_t* inputBuffer = new uint8_t[sharedConfig.getInputSize()];
        uint8_t* returnBuffer = new uint8_t[sharedConfig.return_size];
        util::readFromJson(inputString, inputBuffer, sharedConfig.getInputSize());

        timeval startTime, endTime;
        gettimeofday(&startTime, NULL);
        bool newWorker = runner.dispatch_request(inputBuffer, returnBuffer);
        gettimeofday(&endTime, NULL);

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

    svr.listen("127.0.0.1", 18000);
}