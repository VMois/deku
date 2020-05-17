#include <iostream>
#include "src/Requester.h"

int main() {
    Requester req = Requester();
    
    std::stringstream reply; 
    reply = req.send("echo", "hello world");
    if (reply.str() != "hello world") {
            // should never be excecuted
            throw std::logic_error("echo task returned wrong response");
    }

    // responder task "exception" will throw an error and it wil be returned here
    // as an exception
    reply = req.send("exception");
    return 0;
}