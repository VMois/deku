#pragma once
#include <string>
#include <vector>
#include "Protocol.h"

class Multiaddr {
    std::string multiaddr_;

    static std::vector<Protocol> protocols; // for future

    public:
        Multiaddr();
        Multiaddr(std::string address): multiaddr_(address) {};
        Multiaddr(char* address);
        std::string toString() const;
        std::string getIP4Address();
        int getPort();
};
