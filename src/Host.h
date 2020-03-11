#pragma once
#include <vector>
#include <mutex>

#include "discover/RedisDiscover.h"
#include "Transport.h"

class Host {
    Transport transport_;
    RedisDiscover discover_;
    std::vector<Multiaddr> peers_;
    std::mutex peers_mutex_;
    public:
        Host();
        void start();
};
