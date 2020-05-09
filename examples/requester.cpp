#include <iostream>
#include "src/Requester.h"

int main() {
    Requester req = Requester();
    std::stringstream s;
    s.write("hell1", 5);
    zmsg_t *reply = req.send("echo", s);
    std::cout << reply << std::endl;
    std::cout << zmsg_size(reply) << std::endl;
    zmsg_print(reply);
    // zclock_sleep(500);
    // std::cout << "going next" << std::endl;
    /*std::stringstream g;
    g.write("world", 6);
    reply = req.send("echo", g);
    std::cout << reply << std::endl;
    zmsg_print(reply);
    std::cout << zmsg_size(reply) << std::endl;*/
    while (1) {};
    return 0;
}