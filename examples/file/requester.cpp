#include <iostream>
#include "src/Requester.h"

int main() {
    Requester req = Requester();
    std::stringstream reply; 

    std::cout << "saving data..." << std::endl;
    reply = req.send("save", "my cool data");
    if (reply.str() == "FAIL") {
        std::cout << "[1] Failed to save to remote file" << std::endl;
        return 1;
    }

    std::cout << "loading data..." << std::endl;
    reply = req.send("load");
    std::cout << "[1] File contains: " << reply.str() << std::endl;

    std::cout << "saving data... again..." << std::endl;
    reply = req.send("save", " more cool data");
    if (reply.str() == "FAIL") {
        std::cout << "[2] Failed to save to remote file" << std::endl;
        return 1;
    }

    std::cout << "loading data... again..." << std::endl;
    reply = req.send("load");
    std::cout << "[2] File contains: " << reply.str() << std::endl;
    return 0;
}