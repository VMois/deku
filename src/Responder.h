#pragma once
#include <functional>
#include <sstream>
#include <map>
#include <thread>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/fmt/bin_to_hex.h"
#include "src/multiaddr/Multiaddr.h"
#include "src/discover/RedisDiscover.h"
#include "src/Transport.h"
#include "msgpack.hpp"

class Responder {
    Multiaddr multiaddr_;
    RedisDiscover redis_discover_;
    Transport transport_;
    std::map<std::string, std::function <std::stringstream(std::stringstream)>> handlers_;
    std::vector<std::string> listTasks();
    std::shared_ptr<spdlog::logger> logger_;
  public:
    Responder();
    void on(std::string task_name, std::function <std::stringstream(std::stringstream)> handler);

    void start();
};
