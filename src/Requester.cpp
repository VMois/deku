#include "Requester.h"

zmsg_t* Requester::send(std::string task_name, const std::stringstream& data) {
    redis_discover_.connect();
    std::vector<std::string> responders = redis_discover_.getAddresses(task_name);
    for (int i = 0; i < responders.size(); i++) {
        cli_.connect(responders[i]);
    }

    msgpack::type::tuple<std::string, std::string> job(task_name, data.str());
    std::stringstream buffer;
    msgpack::pack(buffer, job);
    buffer.seekg(0);

    zmsg_t *msg = zmsg_new ();
    zmsg_addstr(msg, buffer.str().c_str());
    zmsg_t *reply = cli_.request(&msg);
    return reply;
}

