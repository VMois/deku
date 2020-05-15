#include "Agent.h"

Agent::Agent(zsock_t* pipe) {
    pipe_ = pipe;
    request_ = NULL;
    router_ = zsock_new(ZMQ_ROUTER);
    poller_ = zpoller_new(pipe_);
    zpoller_add(poller_, router_);
    servers_ = zhash_new ();
}

Agent::~Agent() {
    zsock_destroy(&router_);
    zhash_destroy (&servers_);
    zmsg_destroy (&request_);
    zpoller_destroy(&poller_);
}

void Agent::unlock_servers() {
    Server* item = (Server*) zhash_first(servers_);
    while(item) {
        item->busy_ = false;
        item = (Server*) zhash_next(servers_);
    }
}

static void s_server_free (void *argument)
{
    Server* server = (Server *) argument;
    delete server;
}

void Agent::process_control()
{
    zmsg_t *msg = zmsg_recv(pipe_);
    char *command = zmsg_popstr(msg);

    if (streq(command, "CONNECT")) {
        char *endpoint = zmsg_popstr(msg);
        // check if server is already on the list of connected server
        if (zhash_lookup(servers_, endpoint) == NULL) {
            int rc = zsock_connect(router_, endpoint);
            // TODO: replace assert with something more reliable
            assert (rc == 0);

            // TODO: use zmonitor to check if enpoint is connected
            zclock_sleep(5);

            Server *server = new Server(endpoint);
            zhash_insert(servers_, endpoint, server);
            zhash_freefn(servers_, endpoint, s_server_free);
        }
        free(endpoint);
    } else if (streq(command, "REQUEST")) {
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

//  This method processes one message from a connected server
void Agent::process_router() {
    zmsg_t *reply = zmsg_recv (router_);

    // frame 0 is server that replied
    char *endpoint = zmsg_popstr(reply);
    Server *server = (Server *) zhash_lookup(servers_, endpoint);
    // TODO: replace assert with something else
    assert(server);
    free(endpoint);
    server->alive_ = true;
    server->refreshTimers();
    
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
        uint64_t tickless = zclock_mono() + 1000 * 3600;
        if (request_ && tickless > expires_) {
            tickless = expires_;
        }

        Server* item = (Server*) zhash_first(servers_);
        while(item) {
            if (tickless > item->ping_at_) {
                tickless = item->ping_at_;
            }
            item = (Server*) zhash_next(servers_);
        }

        zsock_t *which = (zsock_t *) zpoller_wait (poller_, (tickless - zclock_mono ()) * ZMQ_POLL_MSEC);
        if (which == pipe_) {
            process_control();
        }
        if (which == router_) {
            process_router();
        }

        //  If we're processing a request, dispatch to next server
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

        //  Disconnect and delete any expired servers
        //  Send heartbeats to idle servers if needed
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
