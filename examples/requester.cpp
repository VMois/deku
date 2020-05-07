#include <iostream>
#include "src/Requester.h"

int main() {
    Requester req = Requester();
    std::stringstream s;
    s.write("hello", 5);
    zmsg_t *reply = req.send("echo", s);
    std::cout << reply << std::endl;
    zmsg_dump(reply);
    return 0;
}