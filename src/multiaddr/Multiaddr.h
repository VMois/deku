#include <string>
#include <vector>
//#include <map>
#include "Protocol.h"

class Multiaddr {
    std::string multiaddr_;

    static std::vector<Protocol> protocols; // for future

    public:
        Multiaddr(std::string address): multiaddr_(address) {};
        Multiaddr(char* address);
        std::string toString();
        std::string getIP4Address();
        int getPort();
};
