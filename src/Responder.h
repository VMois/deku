#pragma once
#include <functional>
#include <sstream>
#include <map>
#include <thread>
#include <iostream>
extern "C" {
    #include <czmq.h>
}

#include "RedisDiscover.h"
#include "config.h"

class Responder {
    std::string address_;
    RedisDiscover redis_discover_;
    std::map<std::string, std::function <void(const std::stringstream&, std::stringstream&)>> handlers_;
    std::vector<std::string> listTasks();

    // worker method to process jobs
    void worker(zsock_t* task_receiver, zsock_t* result_submitter);

    public:
      // register lambda function under particular task name
      void on(std::string task_name, std::function <void(const std::stringstream&, std::stringstream&)> handler);
      void run(std:: string task_name, const std::stringstream& input, std::stringstream& output);

      // launch workers and start listening for incoming connections
      void start();
};
