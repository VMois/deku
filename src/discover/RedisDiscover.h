#pragma once
#include <string.h>
#include <vector>
extern "C" {
    #include <hiredis/hiredis.h>
}
#include "../multiaddr/Multiaddr.h"

class RedisDiscover {
    public:
        RedisDiscover();
        ~RedisDiscover();
        std::vector<Multiaddr> getPeers();
        void notifyPeers(Multiaddr address);
    private:
        redisContext *redis_client_ = NULL;

        // default name of discover list in Redis
        std::string discover_list_ = "deku_discover";

        std::string getRedisHostNameFromEnv();
        void connect();
};