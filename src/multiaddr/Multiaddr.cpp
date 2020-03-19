#include "Multiaddr.h"

Multiaddr::Multiaddr() {
    multiaddr_ = "";
}

bool Multiaddr::operator==(const Multiaddr& rhs) const
{
    return multiaddr_.compare(rhs.toString()) == 0;
}

Multiaddr::Multiaddr(char* address) {
    // TODO: possible check for validity of Multiaddr string
    //if (address[0] != '/')
    //    throw "Multiaddr must start from '/'";

    std::string value(address);
    multiaddr_ = value;
}

std::string Multiaddr::getIP4Address() {
    // Temporary workaround. In future, proper implementation of Multiaddr is required

    std::size_t start_pos = multiaddr_.find("/ip4/");
    if (start_pos == std::string::npos) 
        throw "No ip4 address is found";

    std::size_t end_pos = multiaddr_.find('/', 5);
    if (end_pos == std::string::npos) 
        throw "Cannot determine size of ip4 address";
    
    return multiaddr_.substr(start_pos + 5, end_pos - start_pos - 5);
}

int Multiaddr::getPort() {
    std::size_t start_pos = multiaddr_.find("tcp/");
    if (start_pos == std::string::npos) 
        throw "No ip4 address is found";

    std::size_t end_pos = multiaddr_.find('/', 4);
    if (end_pos == std::string::npos) 
        throw "Cannot determine size of ip4 address";
    
    return std::stoi(multiaddr_.substr(start_pos + 4, multiaddr_.length() - start_pos - 4));
}

std::string Multiaddr::toString() const {
    return multiaddr_;
}
