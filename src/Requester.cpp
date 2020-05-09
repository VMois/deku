#include "Requester.h"
#include <iostream>

Requester::Requester() {
    redis_discover_.connect();
}

zmsg_t* Requester::send(std::string task_name, const std::stringstream& data) {
    std::vector<std::string> responders = redis_discover_.getAddresses(task_name);
    for (int i = 0; i < responders.size(); i++) {
        cli_.connect(responders[i]);
    }

    zmsg_t *msg = zmsg_new();
    zmsg_addstr(msg, task_name.c_str());
    zmsg_addstr(msg, data.str().c_str());
    std::cout << "send request" << std::endl;
    zmsg_t *reply = cli_.request(&msg);
    // std::cout << "response" << std::endl;
    // std::cout << reply << std::endl;
    return reply;
}

