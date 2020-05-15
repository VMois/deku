#include "Requester.h"
#include <iostream>

static void worker(zsock_t *pipe, void *args);

Requester::Requester() {
    redis_discover_.connect();

    // start a new backround thread 
    // to process incoming/outgoing requests
    worker_ = zactor_new(worker, NULL);
}

Requester::~Requester() {
    zactor_destroy(&worker_);
}

void Requester::connect(std::string endpoint) {
    zmsg_t *msg = zmsg_new();
    zmsg_addstr(msg, "CONNECT");
    zmsg_addstr(msg, endpoint.c_str());
    zactor_send(worker_, &msg);
}

zmsg_t* Requester::request(zmsg_t **msg) {
    zmsg_pushstr(*msg, "REQUEST");
    zmsg_send(msg, worker_);
    return zmsg_recv(worker_);
}

std::stringstream Requester::send(std::string task_name, std::string data) {
    std::vector<std::string> responders = redis_discover_.getAddresses(task_name);
    for (int i = 0; i < responders.size(); i++) {
        connect(responders[i]);
    }

    zmsg_t *msg = zmsg_new();
    zmsg_addstr(msg, "TASK");
    zmsg_addstr(msg, task_name.c_str());
    zmsg_addstr(msg, data.c_str());

    zmsg_t *reply = request(&msg);
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

static void worker(zsock_t *pipe, void *args) {
    // send a signal to pipe to indicate a start, required by ZeroMQ
    zsock_signal(pipe, 0);
    
    Agent agent = Agent(pipe);
    agent.start();
}
