#include "Requester.h"
#include <iostream>

static void worker(zsock_t *pipe, void *args);

Requester::Requester() {
    // start a new backround thread 
    // to process incoming/outgoing requests
    worker_ = zactor_new(worker, NULL);
}

Requester::~Requester() {
    zactor_destroy(&worker_);
}

zmsg_t* Requester::request(zmsg_t **msg) {
    zmsg_pushstr(*msg, "REQUEST");
    zmsg_send(msg, worker_);
    return zmsg_recv(worker_);
}

std::stringstream Requester::send(std::string task_name, std::string data) {
    zmsg_t *msg = zmsg_new();
    zmsg_addstr(msg, "TASK");
    zmsg_addstr(msg, task_name.c_str());
    zmsg_addstr(msg, data.c_str());

    zmsg_t *reply = request(&msg);

    if (zmsg_size(reply) < 2) {
        throw std::logic_error("Message size is less than 2");
    }

    zframe_t* status = zmsg_pop(reply);
    zframe_t* output = zmsg_pop(reply);
    zmsg_destroy(&reply);

    if (zframe_streq(status, "INTERNAL_ERROR")) {
        zframe_destroy(&status);
        std::logic_error error =  std::logic_error((char*) zframe_data(output));
        zframe_destroy(&output);
        throw error;
    }

    if (zframe_streq(status, "RESULT")) {
        std::stringstream result_stream;
        result_stream.write((char*) zframe_data(output), zframe_size(output));
        zframe_destroy(&status);
        zframe_destroy(&output);
        return result_stream;
    }

    zframe_destroy(&status);
    zframe_destroy(&output);
    throw std::logic_error("Unspecified status was returned. Possible bug");
}

static void worker(zsock_t *pipe, void *args) {
    // send a signal to pipe to indicate a start of data exchange
    // required by ZeroMQ
    zsock_signal(pipe, 0);
    
    Agent agent = Agent(pipe);
    agent.start();
}
