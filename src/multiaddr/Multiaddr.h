#include <string>

class Multiaddr {
    std::string multiaddr_;
    public:
        Multiaddr(std::string address);
        Multiaddr(char* address);
        std::string toString();
};
