#pragma once
#include <string.h>
#include <vector>
#include <algorithm> 
#include <chrono>
#include <thread>
extern "C" {
    #include <hiredis/hiredis.h>
}

class RedisDiscover {
    public:
        RedisDiscover();
        ~RedisDiscover();

        std::vector<std::string> getAddresses(std::string task_name);
        std::vector<std::string> getTasks();
        void notifyPeers(std::string address, std::vector<std::string> tasks);
        void notifyService(std::string address, std::vector<std::string> tasks);
        void connect();
    private:
        redisContext *redis_client_;
        std::string tasks_list_;

        std::string getRedisHostNameFromEnv();
};