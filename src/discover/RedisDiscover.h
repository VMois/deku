#include <string.h>
#include <vector>
extern "C" {
    #include <hiredis/hiredis.h>
}
#include "Discover.h"

class RedisDiscover: public Discover {
    public:
        RedisDiscover();
        ~RedisDiscover();
        virtual std::vector<Multiaddr> getPeers();
        virtual void notifyPeers(Multiaddr address);
    private:
        redisContext *redis_client_ = NULL;

        // default name of discover list in Redis
        std::string discover_list_ = "deku_discover";

        std::string getRedisHostNameFromEnv();
        void connect();
};