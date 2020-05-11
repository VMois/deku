#include "Requester.h"
#include <iostream>

Requester::Requester() {
    redis_discover_.connect();
}

std::stringstream Requester::send(std::string task_name, const std::string data) {
    std::vector<std::string> responders = redis_discover_.getAddresses(task_name);
    for (int i = 0; i < responders.size(); i++) {
        cli_.connect(responders[i]);
    }

    zmsg_t *msg = zmsg_new();
    zmsg_addstr(msg, task_name.c_str());
    zmsg_addstr(msg, data.c_str());

    zmsg_t *reply = cli_.request(&msg);
    if (zmsg_size(reply) < 2) {
        throw std::logic_error("Reply size is less than 2");
    }

    std::stringstream result_stream;
    zframe_t *task = zmsg_pop(reply);
    zframe_t *output = zmsg_pop(reply);
    if (zframe_streq(task, task_name.c_str())) {
        result_stream.write((char*) zframe_data(output), zframe_size(output));
        return result_stream;
    }
    throw std::logic_error("Submitted task name is not equal to reply task name. Possible bug");
}

