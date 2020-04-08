#pragma once
#include <functional>
#include <sstream>
#include <map>
#include <thread>
#include "spdlog/spdlog.h"
#include "spdlog/fmt/bin_to_hex.h"
#include "src/multiaddr/Multiaddr.h"
#include "src/discover/RedisDiscover.h"
#include "src/Transport.h"
#include "src/Packer.h"
#include "msgpack.hpp"

class Responder {
    Multiaddr multiaddr_;
    RedisDiscover redis_discover_;
    Transport transport_;
    std::map<std::string, std::function <void(const std::stringstream&, std::stringstream&)>> handlers_;
    std::vector<std::string> listTasks();
  public:
    Responder();
    void on(std::string task_name, std::function <void(const std::stringstream&, std::stringstream&)> handler);
    void run(std:: string task_name, const std::stringstream& input, std::stringstream& output);

    void start();
};
