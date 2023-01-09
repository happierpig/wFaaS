#include "../include/httplib.h"
#include "../include/utils.hpp"
#include "../include/json.hpp"
#include "WorkerPool.hpp"
using json = nlohmann::json;

WorkerPool runner;
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
        bool newWorker = runner.dispatch_request(inputBuffer, returnBuffer);


        delete [] inputBuffer;
        delete [] returnBuffer;
    });

    svr.listen("127.0.0.1", 18000);
}