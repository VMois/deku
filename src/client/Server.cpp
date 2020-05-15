#include "Server.h"

Server::Server(std::string endpoint) {
    endpoint_ = endpoint;
    alive_ = false;
    busy_ = false;
    ping_at_ = zclock_mono() + PING_INTERVAL;
    expires_ = zclock_mono() + SERVER_TTL;
}

void Server::refreshTimers() {
    ping_at_ = zclock_mono() + PING_INTERVAL;
    expires_ = zclock_mono() + SERVER_TTL;
}

void Server::ping(zsock_t* socket) {
    if (zclock_mono() >= ping_at_) {
        zmsg_t *ping = zmsg_new();
        zmsg_addstr (ping, endpoint_.c_str());
        zmsg_addstr (ping, "PING");
        zmsg_send (&ping, socket);
        ping_at_ = zclock_mono () + PING_INTERVAL;
    }
}
