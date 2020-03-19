#pragma once
#include <string>
#include <vector>

class Multiaddr {
    std::string multiaddr_;

    public:
        Multiaddr();
        Multiaddr(std::string address): multiaddr_(address) {};
        Multiaddr(char* address);
        bool operator==(const Multiaddr& rhs) const;
        std::string toString() const;
        std::string getIP4Address();
        int getPort();
};
