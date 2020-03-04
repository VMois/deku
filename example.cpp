#include "src/discover/RedisDiscover.h"

int main() {
    RedisDiscover redis_discover = RedisDiscover();
    redis_discover.notifyPeers();
    return 0;
}