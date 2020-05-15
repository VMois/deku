#pragma once
#include <string>
#include <sstream>
extern "C" {
    #include <czmq.h>
}

#include "client/Agent.h"

class Requester {
    zactor_t *worker_;
    zmsg_t* request(zmsg_t **msg);

    public:
        Requester();
        ~Requester();
        std::stringstream send(std::string task_name, std::string data);
};
