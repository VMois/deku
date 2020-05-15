#include "Agent.h"

Agent::Agent(zsock_t* pipe) {
    // TODO: remove connect() from here
    redis_discover_.connect();

    pipe_ = pipe;
    request_ = NULL;
    router_ = zsock_new(ZMQ_ROUTER);
    poller_ = zpoller_new(pipe_);
    zpoller_add(poller_, router_);
    servers_ = zhash_new();

    // update server list asap when Agent starts
    next_discovery_ = zclock_mono();
}

Agent::~Agent() {
    zsock_destroy(&router_);
    zhash_destroy(&servers_);
    zmsg_destroy(&request_);
    zpoller_destroy(&poller_);
}

void Agent::unlock_servers() {
    Server* item = (Server*) zhash_first(servers_);
    while(item) {
        item->busy_ = false;
        item = (Server*) zhash_next(servers_);
    }
}

static void s_server_free (void *argument) {
    Server* server = (Server *) argument;
    delete server;
}

void Agent::process_control() {
    zmsg_t *msg = zmsg_recv(pipe_);
    char *command = zmsg_popstr(msg);

    if (streq(command, "REQUEST")) {
        if (request_ != NULL) {
            throw std::logic_error("Request already exists, cannot process a new one");
        }

        // Take ownership of request message
        request_ = msg;
        msg = NULL;

        expires_ = zclock_mono() + REQUEST_TIMEOUT;
    }
    free(command);
    zmsg_destroy(&msg);
}

void Agent::discover_servers() {
    if (zclock_mono() < next_discovery_) {
        // too early to discover new servers, skip
        return;
    }

    // TODO: this is a quick and not efficient implementation, need improvement in future.
    std::vector<std::string> all_tasks = redis_discover_.getTasks();
    for (const std::string& task_name : all_tasks) {
        std::vector<std::string> responders = redis_discover_.getAddresses(task_name);

        for (const std::string& endpoint: responders) {
            if (zhash_lookup(servers_, endpoint.c_str()) == NULL) {
                int rc = zsock_connect(router_, endpoint.c_str());
                if (rc != 0) continue;

                // TODO: use zmonitor to check if enpoint is connected.
                // need to add a small sleep so new connection can appear in socket
                zclock_sleep(5);

                Server *server = new Server(endpoint);
                zhash_insert(servers_, endpoint.c_str(), server);
                zhash_freefn(servers_, endpoint.c_str(), s_server_free);
            }
        }
    }
    next_discovery_ = zclock_mono() + DISCOVERY_INTERVAL;
    tickless_ = next_discovery_;
}

// this method processes a single message from a connected server
void Agent::process_router() {
    zmsg_t *reply = zmsg_recv(router_);

    // frame 0 is server that replied
    char *endpoint = zmsg_popstr(reply);
    Server *server = (Server *) zhash_lookup(servers_, endpoint);
    if (server == NULL) return;

    free(endpoint);
    server->alive_ = true;
    server->refreshTimers();

    // process frame 1, which is a control frame
    char *control = zmsg_popstr(reply);

    if (streq(control, "TASK")) {
        char *status = zmsg_popstr(reply);
        if (streq(status, "OK")) {
            zmsg_destroy(&request_);
            request_ = NULL;
            unlock_servers();
        } else if (streq(status, "RESULT")) {
            zmsg_send(&reply, pipe_);
            zmsg_destroy(&request_);
            request_ = NULL;
            unlock_servers();
        } else if (streq(status, "BUSY")) {
            server->busy_ = true;
        }
    } else {
        zmsg_destroy (&reply);
    }
}

void Agent::start() {
    bool all_servers_busy = true;

    while (true) {
        // calculate tickless timer, up to 1 hour
        tickless_ = zclock_mono() + 1000 * 3600;
        if (request_ && tickless_ > expires_) {
            tickless_ = expires_;
        }

        Server* item = (Server*) zhash_first(servers_);
        while(item) {
            if (tickless_ > item->ping_at_) {
                tickless_ = item->ping_at_;
            }
            item = (Server*) zhash_next(servers_);
        }

        discover_servers();

        zsock_t *which = (zsock_t *) zpoller_wait (poller_, (tickless_ - zclock_mono ()) * ZMQ_POLL_MSEC);
        if (which == pipe_) {
            process_control();
        }
        if (which == router_) {
            process_router();
        }

        // processing a request, choose a server
        if (request_) {
            if (zclock_mono() >= expires_) {
                //  Request expired, kill it
                zstr_send(pipe_, "FAILED");
                zmsg_destroy(&request_);
                request_ = NULL;
                unlock_servers();
            } else {
                // Find server to talk to, remove any expired ones
                Server* server = (Server*) zhash_first(servers_);
                while(server) {
                    if (zclock_mono () >= server->expires_) {
                        server->alive_ = false;
                    } else if (!server->busy_) {
                        zmsg_t *request = zmsg_dup(request_);
                        zmsg_pushstr(request, server->endpoint_.c_str());
                        zmsg_send(&request, router_);
                        server->busy_ = true;
                        break;
                    }
                    server = (Server*) zhash_next(servers_);
                }
            }
        }

        // Disconnect and delete any expired servers
        // Send heartbeats to idle servers if needed
        all_servers_busy = true;
        item = (Server*) zhash_first(servers_);
        while(item) {
            item->ping(router_);
            if (!item->busy_) {
                all_servers_busy = false;
            }
            item = (Server*) zhash_next(servers_);
        }

        if (all_servers_busy) {
            unlock_servers();
        }
    }
}
