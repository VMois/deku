#include "Multiaddr.h"

Multiaddr::Multiaddr(std::string address) {
    multiaddr_ = address;
}

Multiaddr::Multiaddr(char* address) {
    std::string value(address);
    multiaddr_ = value;
}

std::string Multiaddr::toString() {
    return multiaddr_;
}