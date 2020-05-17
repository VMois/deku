#pragma once
#include <iostream>
#include <string>
extern "C" {
    #include <czmq.h>
}
#include "src/RedisDiscover.h"
#include "Server.h"

//  If no server replies within this time, abandon request
#define REQUEST_TIMEOUT  4000   //  msecs

/* 
How often should we update servers list from Redis.
Should be less than REQUEST_TIMEOUT to avoid edge case bug 
*/
#define DISCOVERY_INTERVAL 1500  // msecs

class Agent {
    RedisDiscover redis_discover_;
    zsock_t *pipe_;
    zsock_t *router_;
    zpoller_t *poller_;

    zhash_t *servers_;
    zmsg_t *request_;

    uint64_t tickless_;  // how long to wait to the next event scheduled (ping, discover_servers or request expired)
    uint64_t next_discovery_;  // when to fetch list of Responders from Redis
    uint64_t expires_;   // when request will expire

    // send a request if server is available
    void process_request();

    void update_ping_time();

    // fetch list of Responders from Redis
    void discover_servers();

    // process a single message from the Requester
    void process_control();

    // this method processes a single message from a connected Responder
    void process_router();
    void unlock_servers();

    public:
        Agent(zsock_t* pipe);
        ~Agent();

        void start();
};
