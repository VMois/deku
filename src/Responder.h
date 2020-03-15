#pragma once
#include <functional>
#include <sstream>
#include <map>
#include <thread>
#include "spdlog/spdlog.h"
#include "src/multiaddr/Multiaddr.h"
#include "src/discover/RedisDiscover.h"

class Responder {
    Multiaddr multiaddr_;
    RedisDiscover redis_discover_;
    std::map<std::string, std::function <std::stringstream(std::stringstream)>> handlers_;

    std::vector<std::string> listTasks();
  public:
    Responder();
    void on(std::string task_name, std::function <std::stringstream(std::stringstream)> handler);

    void start();
};
