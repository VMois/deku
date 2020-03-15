#include "Responder.h"

Responder::Responder() {
    // TODO: assign proper local address
    std::string address = "/ipv4/0.0.0.0/tcp/3434";
    multiaddr_ = Multiaddr(address);
}

void Responder::on(std::string task_name, 
                   std::function <std::stringstream(std::stringstream)> handler) {
    handlers_[task_name] = handler;
};

std::vector<std::string> Responder::listTasks() {
    std::vector<std::string> tasks;
    for (std::map<std::string, std::function <std::stringstream(std::stringstream)>>::iterator it = handlers_.begin(); 
        it != handlers_.end(); ++it) {
            tasks.push_back(it->first);
        }
    return tasks;
}

void Responder::start() {
    std::thread discover_thread(&RedisDiscover::notifyService, &redis_discover_, multiaddr_, listTasks());
    if (discover_thread.joinable()) {
        discover_thread.detach();
    }

    while (1) {};
}
