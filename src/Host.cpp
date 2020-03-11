#include "Host.h"

Host::Host() {
    transport_ = Transport();
    discover_ = RedisDiscover();
};

void Host::start() {
     Multiaddr address("/ip4/0.0.0.0/tcp/3434");
     transport_.listenMultiaddr(address);
}