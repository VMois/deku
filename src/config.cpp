#include "config.h"

// Returns local IP address
std::string getLocalIPv4Address() {
    // https://stackoverflow.com/questions/49335001/get-local-ip-address-in-c
    int sock = socket(PF_INET, SOCK_DGRAM, 0);
    sockaddr_in loopback;

    if (sock == -1) {
        throw "Could not create socket";
    }

    std::memset(&loopback, 0, sizeof(loopback));
    loopback.sin_family = AF_INET;
    loopback.sin_addr.s_addr = INADDR_LOOPBACK;   // using loopback ip address
    loopback.sin_port = htons(9);                 // using debug port

    if (connect(sock, reinterpret_cast<sockaddr*>(&loopback), sizeof(loopback)) == -1) {
        close(sock);
        throw "Could not connect to socket";
    }

    socklen_t addrlen = sizeof(loopback);
    if (getsockname(sock, reinterpret_cast<sockaddr*>(&loopback), &addrlen) == -1) {
        close(sock);
        throw "Could not getsockname";
    }

    close(sock);

    char buf[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &loopback.sin_addr, buf, INET_ADDRSTRLEN) == 0x0) 
    {
        throw "Could not inet_ntop";
    } else 
    {
        std::string value(buf);
        value.insert(0, "tcp://");
        value.append(":3434");
        return value;
    }
}
