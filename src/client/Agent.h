#pragma once
#include <iostream>
#include <string>
extern "C" {
    #include <czmq.h>
}
#include "Server.h"

//  If no server replies within this time, abandon request
#define REQUEST_TIMEOUT  3000    //  msecs
//  PING interval for servers we think are alive
#define PING_INTERVAL   2000    //  msecs
//  Server considered dead if silent for this long
#define SERVER_TTL      6000    //  msecs

class Agent {
    public:
        zsock_t *pipe_;
        zpoller_t *poller_;
        zsock_t *router_;
        zhash_t *servers_;
        zmsg_t *request_;
        uint64_t expires_; 

        Agent(zsock_t* pipe);
        ~Agent();

        void process_control();
        void process_router();
        void unlock_servers();
};