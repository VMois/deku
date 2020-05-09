#include <iostream>
#include "src/Requester.h"

int main() {
    Requester req = Requester();
    std::stringstream s;
    s.write("hell1", 5);
    zmsg_t *reply = req.send("echo", s);
    zmsg_print(reply);
    std::cout << "going next" << std::endl;
    std::stringstream g;
    g.write("world", 6);
    reply = req.send("echo", g);
    zmsg_print(reply);
    // while (1) {};
    return 0;
}