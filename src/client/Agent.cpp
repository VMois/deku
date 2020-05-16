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

// function to free the memory when zhash element is destroyed
static void s_server_free (void *argument) {
    Server* server = (Server *) argument;
    delete server;
}

bool Agent::is_task_supported(char* task_name) {
    const std::string task_name_str(task_name);
    Server* item = (Server*) zhash_first(servers_);
    while(item) {
        if(item->supported_tasks_.find(task_name_str) != item->supported_tasks_.end()) {
            return true;
        }
        item = (Server*) zhash_next(servers_);
    }
    return false;
}

void Agent::process_control() {
    zmsg_t *msg = zmsg_recv(pipe_);
    char *command = zmsg_popstr(msg);

    if (streq(command, "REQUEST")) {
        if (request_ != NULL) {
            throw std::logic_error("Request already exists, cannot process a new one");
        }
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

    // TODO: this is a quick and not efficient implementation, 
    // need improvement in RedisDiscovery. Will do if will have time.
    Server* server;
    std::vector<std::string> all_tasks = redis_discover_.getTasks();
    for (const std::string& task_name : all_tasks) {
        std::vector<std::string> responders = redis_discover_.getAddresses(task_name);

        for (const std::string& endpoint: responders) {
            server = (Server*) zhash_lookup(servers_, endpoint.c_str());
            if (server == NULL) {
                int rc = zsock_connect(router_, endpoint.c_str());

                // if connection failed, skip this server
                if (rc != 0) continue;

                // TODO: use zmonitor to check if enpoint is connected.
                // need to add a small sleep so new connection can appear in socket
                zclock_sleep(5);

                server = new Server(endpoint);
                server->supported_tasks_.insert(task_name);
                zhash_insert(servers_, endpoint.c_str(), server);
                zhash_freefn(servers_, endpoint.c_str(), s_server_free);
            } else {
                server->supported_tasks_.insert(task_name);
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
    free(endpoint);

    if (server == NULL) return;

    server->alive_ = true;
    server->refreshTimers();

    // frame 1 is a control frame
    char *control = zmsg_popstr(reply);

    if (streq(control, "TASK")) {
        char *status = zmsg_popstr(reply);
        if (streq(status, "OK")) {
            zmsg_destroy(&request_);
            request_ = NULL;
            unlock_servers();
        } else if (streq(status, "RESULT")) {
            zmsg_pushstr(reply, status);
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

void Agent::update_ping_time() {
    Server* item = (Server*) zhash_first(servers_);
    while(item) {
        if (tickless_ > item->ping_at_) {
            tickless_ = item->ping_at_;
        }
        item = (Server*) zhash_next(servers_);
    }
}

// find server to send request to, remove any expired servers
void Agent::process_request() {
    if (!request_) return;

    if (zclock_mono() >= expires_) {
        //  Request expired, kill it
        zmsg_t* expired = zmsg_new();
        zmsg_pushstr(expired, "EXPIRED");
        zmsg_pushstr(expired, "INTERNAL_ERROR");
        zmsg_send(&expired, pipe_);
        zmsg_destroy(&request_);
        request_ = NULL;
        unlock_servers();
        return;
    }

    // TODO: packing/unpacking needs improvement or a new interface
    // unpack message to get task_name, pack it back after
    char* protocol_name = zmsg_popstr(request_);
    char* task_name = zmsg_popstr(request_);
    std::string task_name_str(task_name);
    zmsg_pushstr(request_, task_name);
    zmsg_pushstr(request_, protocol_name);
    free(task_name);
    free(protocol_name);

    Server* server = (Server*) zhash_first(servers_);
    while(server) {
        // TODO: separate server expire check and request sending
        if (zclock_mono () >= server->expires_) {
            server->alive_ = false;
        } else if (!server->busy_ && 
                (server->supported_tasks_.find(task_name_str) != server->supported_tasks_.end())) {
            // if server is not busy and supports the task, send a request
            zmsg_t *request = zmsg_dup(request_);
            zmsg_pushstr(request, server->endpoint_.c_str());
            zmsg_send(&request, router_);
            server->busy_ = true;
            break;
        }
        server = (Server*) zhash_next(servers_);
    }
}

void Agent::start() {
    while (true) {
        // calculate tickless timer, up to 1 hour
        tickless_ = zclock_mono() + 1000 * 3600;

        if (request_ && tickless_ > expires_) {
            tickless_ = expires_;
        }

        update_ping_time();

        discover_servers();

        // wait until something will happen on pipe_ or router_ sockets
        zsock_t *which = (zsock_t *) zpoller_wait(poller_, (tickless_ - zclock_mono ()) * ZMQ_POLL_MSEC);
        if (which == pipe_) {
            process_control();
        }
        if (which == router_) {
            process_router();
        }

        process_request();

        // send heartbeats to idle servers if needed
        // unlock servers if all are busy, maybe they are available now
        bool all_servers_busy = true;
        Server* item = (Server*) zhash_first(servers_);
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
