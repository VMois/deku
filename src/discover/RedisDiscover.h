#include <string.h>
extern "C" {
    #include <hiredis/hiredis.h>
}
#include "Discover.h"

class RedisDiscover: public Discover {
    public:
        RedisDiscover();
        ~RedisDiscover();
        virtual void getPeers();
        virtual void notifyPeers();
    private:
        redisContext *redis_client_ = NULL;
        char* discover_list_ = (char*) "deku_discover";

        char* getRedisHostNameFromEnv();
        void connect();
};