#include <iostream>
#include "src/Requester.h"

int main() {
    Requester req = Requester();
    
    std::stringstream reply = req.send("echo", "hello");
    std::cout << reply.str() << std::endl;

    std::cout << "going next" << std::endl;

    reply = req.send("echo", "world");
    std::cout << reply.str() << std::endl;

    return 0;
}