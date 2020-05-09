#include "Client.h"

static void flcliapi_agent (zsock_t *pipe, void *args);

Client::Client() {
    worker_ = zactor_new(flcliapi_agent, NULL);
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

Server::Server(std::string endpoint) {
    endpoint_ = endpoint;
    alive_ = false;
    used_ = false;
    ping_at_ = zclock_mono() + PING_INTERVAL;
    expires_ = zclock_mono() + SERVER_TTL;
}

void Server::ping(zsock_t* socket) {
    if (zclock_mono() >= ping_at_) {
        zmsg_t *ping = zmsg_new();
        zmsg_addstr (ping, endpoint_.c_str());
        zmsg_addstr (ping, "PING");
        zmsg_send (&ping, socket);
        ping_at_ = zclock_mono () + PING_INTERVAL;
    }
}

void Server::tickless(uint64_t& tickless) {
    if (tickless > ping_at_) {
        tickless = ping_at_;
    }
}

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

void agent_control_message (Agent& self)
{
    zmsg_t *msg = zmsg_recv (self.pipe_);
    char *command = zmsg_popstr (msg);

    if (streq(command, "CONNECT")) {
        char *endpoint = zmsg_popstr (msg);
        // check if server is already on the list of connected server
        if (zhash_lookup(self.servers_, endpoint) == NULL) {
            // std::cout << "Connecting to " << endpoint << std::endl;
            int rc = zsock_connect(self.router_, endpoint);
            // TODO: replace assert with something more reliable
            assert (rc == 0);

            // TODO: use zmonitor to check if enpoint is connected
            zclock_sleep(5);

            Server *server = new Server(endpoint);
            zhash_insert(self.servers_, endpoint, server);
            zhash_freefn(self.servers_, endpoint, s_server_free);
        }
        free(endpoint);
    } else if (streq (command, "REQUEST")) {
        assert(!self.request_);
        zmsg_pushstr(msg, "TASK");

        //  Take ownership of request message
        self.request_ = msg;
        msg = NULL;
        //  Request expires after request timeout
        self.expires_ = zclock_mono () + REQUEST_TIMEOUT;
    }
    free(command);
    zmsg_destroy(&msg);
}

//  This method processes one message from a connected server
void agent_router_message (Agent& self) {
    zmsg_t *reply = zmsg_recv (self.router_);

    // frame 0 is server that replied
    char *endpoint = zmsg_popstr (reply);
    Server *server = (Server *) zhash_lookup (self.servers_, endpoint);
    assert (server);
    free (endpoint);
    if (!server->alive_) {
        server->alive_ = true;
    }
    server->ping_at_ = zclock_mono () + PING_INTERVAL;
    server->expires_ = zclock_mono () + SERVER_TTL;

    char *control = zmsg_popstr (reply);

    if (streq(control, "TASK")) {
        char *status = zmsg_popstr(reply);
        if (streq(status, "MORE")) {
            zmsg_send(&reply, self.pipe_);
        } else if (streq(status, "FAILED")) {
            zmsg_t* failed = zmsg_new();
            zmsg_addstr(failed, "FAILED");
            zmsg_send(&failed, self.pipe_);
        }
        zmsg_destroy(&self.request_);
        self.request_ = NULL;
        self.unlock_servers();
    } else {
        zmsg_destroy (&reply);
    }
}


static void flcliapi_agent(zsock_t *pipe, void *args)
{
    zsock_signal(pipe, 0);
    Agent self = Agent(pipe);

    while (true) {
        //  Calculate tickless timer, up to 1 hour
        uint64_t tickless = zclock_mono () + 1000 * 3600;
        if (self.request_ && tickless > self.expires_) {
            tickless = self.expires_;
        }

        Server* item = (Server*) zhash_first(self.servers_);
        while(item) {
            item->tickless(tickless);
            item = (Server*) zhash_next(self.servers_);
        }

        zsock_t *which = (zsock_t *) zpoller_wait (self.poller_, (tickless - zclock_mono ()) * ZMQ_POLL_MSEC);
        if (which == self.pipe_) {
            agent_control_message(self);
        }
        if (which == self.router_) {
            agent_router_message(self);
        }

        //  If we're processing a request, dispatch to next server
        if (self.request_) {
            if (zclock_mono() >= self.expires_) {
                //  Request expired, kill it
                std::cout << "set failed" << std::endl; 
                zstr_send(self.pipe_, "FAILED");
                zmsg_destroy(&self.request_);
                self.request_ = NULL;
                self.unlock_servers();
            } else {
                // Find server to talk to, remove any expired ones
                Server* server = (Server*) zhash_first(self.servers_);
                while(server) {
                    if (zclock_mono () >= server->expires_) {
                        server->alive_ = false;
                    } else if (!server->used_) {
                        zmsg_t *request = zmsg_dup(self.request_);
                        zmsg_pushstr(request, server->endpoint_.c_str());
                        // zmsg_print(request);
                        zmsg_send(&request, self.router_);
                        server->used_ = true;
                        // zmsg_destroy(&self.request_);
                        // self.request_ = NULL;
                        break;
                    }
                    server = (Server*) zhash_next(self.servers_);
                }
            }
        }

        //  Disconnect and delete any expired servers
        //  Send heartbeats to idle servers if needed
        item = (Server*) zhash_first(self.servers_);
        while(item) {
            item->ping(self.router_);
            item = (Server*) zhash_next(self.servers_);
        }
    }
}