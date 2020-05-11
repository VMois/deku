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
        item->used_ = false;
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
            server->used_ = true;
            zmsg_t *request = zmsg_dup(request_);
            zmsg_pushstr(request, "REQUEST");
            zmsg_send(&request, pipe_);
        }
    } else {
        zmsg_destroy (&reply);
    }
}
