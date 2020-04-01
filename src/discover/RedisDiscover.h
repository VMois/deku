#pragma once
#include <string.h>
#include <vector>
#include <algorithm> 
#include <chrono>
#include <thread>
extern "C" {
    #include <hiredis/hiredis.h>
}
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

        std::string getRedisHostNameFromEnv();
        void connect();
};