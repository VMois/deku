#pragma once
#include <string.h>
#include <vector>
#include <algorithm> 
#include <chrono>
#include <thread>
extern "C" {
    #include <hiredis/hiredis.h>
}
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "src/multiaddr/Multiaddr.h"

class RedisDiscover {
    public:
        RedisDiscover();
        ~RedisDiscover();

        std::vector<Multiaddr> getAddresses(std::string task_name);
        std::vector<std::string> getTasks();
        void notifyPeers(Multiaddr address, std::vector<std::string> tasks);
        void notifyService(Multiaddr address, std::vector<std::string> tasks);
    private:
        redisContext *redis_client_;
        std::string tasks_list_;
        std::shared_ptr<spdlog::logger> logger_;

        std::string getRedisHostNameFromEnv();
        void connect();
};