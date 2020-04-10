#pragma once
#include <functional>
#include <sstream>
#include <map>
#include <thread>
#include <zmq.hpp>
#include "spdlog/spdlog.h"
#include "spdlog/fmt/bin_to_hex.h"
#include "src/multiaddr/Multiaddr.h"
#include "src/discover/RedisDiscover.h"
#include "src/Transport.h"
#include "src/Packer.h"
#include "msgpack.hpp"

class Responder {
    RedisDiscover redis_discover_;
    Transport transport_;
    zmq::context_t context_;
    std::map<std::string, std::function <void(const std::stringstream&, std::stringstream&)>> handlers_;
    std::vector<std::string> listTasks();
  public:
    Responder(): context_(zmq::context_t(1)) {};
    void on(std::string task_name, std::function <void(const std::stringstream&, std::stringstream&)> handler);
    void run(std:: string task_name, const std::stringstream& input, std::stringstream& output);

    void start();
};
