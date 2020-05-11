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
    bool all_servers_busy = true;

    while (true) {
        // calculate tickless timer, up to 1 hour
        uint64_t tickless = zclock_mono() + 1000 * 3600;
        if (agent.request_ && tickless > agent.expires_) {
            tickless = agent.expires_;
        }

        Server* item = (Server*) zhash_first(agent.servers_);
        while(item) {
            if (tickless > item->ping_at_) {
                tickless = item->ping_at_;
            }
            item = (Server*) zhash_next(agent.servers_);
        }

        zsock_t *which = (zsock_t *) zpoller_wait (agent.poller_, (tickless - zclock_mono ()) * ZMQ_POLL_MSEC);
        if (which == agent.pipe_) {
            agent.process_control();
        }
        if (which == agent.router_) {
            agent.process_router();
        }

        //  If we're processing a request, dispatch to next server
        if (agent.request_) {
            if (zclock_mono() >= agent.expires_) {
                //  Request expired, kill it
                zstr_send(agent.pipe_, "FAILED");
                zmsg_destroy(&agent.request_);
                agent.request_ = NULL;
                agent.unlock_servers();
            } else {
                // Find server to talk to, remove any expired ones
                Server* server = (Server*) zhash_first(agent.servers_);
                while(server) {
                    if (zclock_mono () >= server->expires_) {
                        server->alive_ = false;
                    } else if (!server->used_) {
                        zmsg_t *request = zmsg_dup(agent.request_);
                        zmsg_pushstr(request, server->endpoint_.c_str());
                        zmsg_send(&request, agent.router_);
                        server->used_ = true;
                        break;
                    }
                    server = (Server*) zhash_next(agent.servers_);
                }
            }
        }

        //  Disconnect and delete any expired servers
        //  Send heartbeats to idle servers if needed
        all_servers_busy = true;
        item = (Server*) zhash_first(agent.servers_);
        while(item) {
            item->ping(agent.router_);
            if (!item->used_) {
                all_servers_busy = false;
            }
            item = (Server*) zhash_next(agent.servers_);
        }

        if (all_servers_busy) {
            agent.unlock_servers();
        }
    }
}
