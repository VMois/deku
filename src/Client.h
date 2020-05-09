#pragma once
#include <iostream>
#include <string>
extern "C" {
    #include "czmq.h"
}

//  If no server replies within this time, abandon request
#define GLOBAL_TIMEOUT  3000    //  msecs
//  PING interval for servers we think are alive
#define PING_INTERVAL   2000    //  msecs
//  Server considered dead if silent for this long
#define SERVER_TTL      6000    //  msecs

class Client {
    zactor_t *worker_; 
    public:
        Client();
        ~Client();
        void connect(std::string endpoint);
        zmsg_t* request(zmsg_t **msg);
};

class Server {
    public:
        std::string endpoint_;
        bool alive_;
        int64_t ping_at_;
        int64_t expires_;
        Server(std::string endpoint);
        void ping(zsock_t *socket);
        void tickless(uint64_t& tickless);
};

class Agent {
    public:
        zsock_t *pipe_;                 //  Socket to talk back to application
        zpoller_t *poller_;
        zsock_t *router_;               //  Socket to talk to servers
        zhash_t *servers_;           //  Servers we've connected to
        zmsg_t *request_;            //  Current request if any
        uint64_t expires_;  
        Agent(zsock_t* pipe);
        ~Agent();
};