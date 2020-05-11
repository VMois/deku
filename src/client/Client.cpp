#include "Client.h"

static void worker(zsock_t *pipe, void *args);

Client::Client() {
    // start a new thread
    worker_ = zactor_new(worker, NULL);
}

Client::~Client() {
    zactor_destroy(&worker_);
}

void Client::connect(std::string endpoint) {
    zmsg_t *msg = zmsg_new();
    zmsg_addstr(msg, "CONNECT");
    zmsg_addstr(msg, endpoint.c_str());
    zactor_send(worker_, &msg);
}

zmsg_t* Client::request(zmsg_t **msg) {
    zmsg_pushstr(*msg, "REQUEST");
    zmsg_send(msg, worker_);
    return zmsg_recv(worker_);
}

static void worker(zsock_t *pipe, void *args)
{
    // send a signal to pipe to indicate a start, required by ZeroMQ
    zsock_signal(pipe, 0);
    
    Agent agent = Agent(pipe);

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
                std::cout << "set failed" << std::endl; 
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
        item = (Server*) zhash_first(agent.servers_);
        while(item) {
            item->ping(agent.router_);
            item = (Server*) zhash_next(agent.servers_);
        }
    }
}
