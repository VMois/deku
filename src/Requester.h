#pragma once
#include <string>
extern "C" {
    #include <czmq.h>
}

#include "Client.h"
#include "msgpack.hpp"
#include "discover/RedisDiscover.h"

class Requester {
    Client cli_;
    RedisDiscover redis_discover_;
    public:
        zmsg_t* send(std::string task_name, const std::stringstream& data);
};
