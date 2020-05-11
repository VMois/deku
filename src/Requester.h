#pragma once
#include <string>
#include <sstream>
extern "C" {
    #include <czmq.h>
}

#include "client/Client.h"
#include "discover/RedisDiscover.h"

class Requester {
    Client cli_;
    RedisDiscover redis_discover_;
    public:
        Requester();
        std::stringstream send(std::string task_name, std::string data);
};
