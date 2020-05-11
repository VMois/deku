#pragma once
#include <iostream>
#include <string>
extern "C" {
    #include <czmq.h>
}
#include "Agent.h"

class Client {
    zactor_t *worker_; 
    public:
        Client();
        ~Client();
        void connect(std::string endpoint);
        zmsg_t* request(zmsg_t **msg);
};
