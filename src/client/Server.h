#pragma once
#include <string>
extern "C" {
    #include "czmq.h"
}

//  If no server replies within this time, abandon request
#define REQUEST_TIMEOUT  3000    //  msecs
//  PING interval for servers we think are alive
#define PING_INTERVAL   2000    //  msecs
//  Server considered dead if silent for this long
#define SERVER_TTL      6000    //  msecs

class Server {
    public:
        std::string endpoint_;
        bool alive_;
        bool used_;
        int64_t ping_at_;
        int64_t expires_;
        Server(std::string endpoint);
        void ping(zsock_t *socket);
};
