#pragma once
#include <string>
extern "C" {
    #include "czmq.h"
}

//  PING interval for servers we think are alive
#define PING_INTERVAL   3000    //  msecs
//  Server considered dead if silent for this long
#define SERVER_TTL      6000    //  msecs

class Server {
    public:
        std::string endpoint_;
        bool alive_;
        bool busy_;
        int64_t ping_at_;
        int64_t expires_;
        Server(std::string endpoint);
        void refreshTimers();
        void ping(zsock_t *socket);
};
