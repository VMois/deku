#pragma once
#include <functional>
#include <sstream>
#include <map>
#include <thread>
#include <iostream>
extern "C" {
    #include <czmq.h>
}

#include "spdlog/spdlog.h"
#include "spdlog/fmt/bin_to_hex.h"
#include "src/discover/RedisDiscover.h"
#include "msgpack.hpp"

class Responder {
    RedisDiscover redis_discover_;
    std::map<std::string, std::function <void(const std::stringstream&, std::stringstream&)>> handlers_;
    std::vector<std::string> listTasks();
    void worker(zsock_t* task_receiver, zsock_t* result_submitter);
  public:
    void on(std::string task_name, std::function <void(const std::stringstream&, std::stringstream&)> handler);
    void run(std:: string task_name, const std::stringstream& input, std::stringstream& output);

    void start();
};
