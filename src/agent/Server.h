#pragma once
#include <string>
#include <set>
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
        bool busy_;  // is server busy with job
        int64_t ping_at_;  // when to ping next
        int64_t expires_;  // when server will be considered as dead
        std::set<std::string> supported_tasks_;  // list of tasks that server supports
        Server(std::string endpoint);

        void refreshTimers();
        void ping(zsock_t *socket);
};
