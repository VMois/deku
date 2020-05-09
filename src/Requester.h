#pragma once
#include <string>
#include <sstream>
extern "C" {
    #include <czmq.h>
}

#include "Client.h"
#include "discover/RedisDiscover.h"

class Requester {
    Client cli_;
    RedisDiscover redis_discover_;
    public:
        Requester();
        zmsg_t* send(std::string task_name, const std::stringstream& data);
};
