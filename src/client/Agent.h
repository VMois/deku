#pragma once
#include <iostream>
#include <string>
extern "C" {
    #include <czmq.h>
}
#include "src/discover/RedisDiscover.h"
#include "Server.h"

//  If no server replies within this time, abandon request
#define REQUEST_TIMEOUT  4000   //  msecs

/* 
How often should we update servers list from Redis.
Should be less than REQUEST_TIMEOUT to avoid edge case bug 
*/
#define DISCOVERY_INTERVAL 2000  // msecs

class Agent {
    RedisDiscover redis_discover_;
    zsock_t *pipe_;
    zsock_t *router_;
    zpoller_t *poller_;

    zhash_t *servers_;
    zmsg_t *request_;

    uint64_t tickless_;
    uint64_t next_discovery_;
    uint64_t expires_; 

    bool is_task_supported(char* task_name);
    void process_request();
    void update_ping_time();
    void discover_servers();
    void process_control();
    void process_router();
    void unlock_servers();

    public:
        Agent(zsock_t* pipe);
        ~Agent();

        void start();
};
