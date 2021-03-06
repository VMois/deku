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
    redisContext *redis_client_;
    std::string tasks_list_;
    
    std::string getRedisHostNameFromEnv();

    public:
        RedisDiscover(): redis_client_(NULL), tasks_list_("deku_tasks") {};
        ~RedisDiscover();

        std::vector<std::string> getAddresses(std::string task_name);
        std::vector<std::string> getTasks();
        void notifyPeers(std::string address, std::vector<std::string> tasks);
        void connect();
};