#include "Client.h"

static void flcliapi_agent (zsock_t *pipe, void *args);

Client::Client() {
    pipe_ = zactor_new(flcliapi_agent, NULL);
}

Client::~Client() {
    zactor_destroy(&pipe_);
}

void Client::connect(std::string endpoint) {
    zmsg_t *msg = zmsg_new();
    zmsg_addstr(msg, "CONNECT");
    zmsg_addstr(msg, endpoint.c_str());
    zactor_send(pipe_, &msg);
    zclock_sleep (100);
}

zmsg_t* Client::request(zmsg_t **msg) {
    zmsg_pushstr (*msg, "REQUEST");
    zmsg_send (msg, pipe_);
    zmsg_t *reply = zmsg_recv(pipe_);
    if (reply) {
        char *status = zmsg_popstr (reply);
        if (streq (status, "FAILED"))
            zmsg_destroy (&reply);
        free (status);
    }
    return reply;
}

Server::Server(std::string endpoint) {
    endpoint_ = endpoint;
    alive_ = false;
    ping_at_ = zclock_time () + PING_INTERVAL;
    expires_ = zclock_time () + SERVER_TTL;
}

void Server::ping(zsock_t* socket) {
    if (zclock_time() >= ping_at_) {
        zmsg_t *ping = zmsg_new();
        zmsg_addstr (ping, endpoint_.c_str());
        zmsg_addstr (ping, "PING");
        zmsg_send (&ping, socket);
        ping_at_ = zclock_time () + PING_INTERVAL;
    }
}

void Server::tickless(uint64_t tickless) {
    if (tickless > ping_at_) {
        tickless = ping_at_;
    }
}

Agent::Agent(zsock_t* pipe) {
    pipe_ = pipe;
    router_ = zsock_new(ZMQ_ROUTER);
    poller_ = zpoller_new(pipe_);
    zpoller_add(poller_, router_);
    servers_ = zhash_new ();
    actives_ = zlist_new ();
}

Agent::~Agent() {
    zhash_destroy (&servers_);
    zlist_destroy (&actives_);
    zmsg_destroy (&request_);
    zmsg_destroy (&reply_);
    zpoller_destroy(&poller_);
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

    if (streq (command, "CONNECT")) {
        char *endpoint = zmsg_popstr (msg);
        printf ("I: connecting to %s…\n", endpoint);
        int rc = zsock_connect(self.router_, endpoint);
        assert (rc == 0);
        Server *server = new Server(endpoint);
        zhash_insert (self.servers_, endpoint, server);
        zhash_freefn (self.servers_, endpoint, s_server_free);
        zlist_append (self.actives_, server);
        server->ping_at_ = zclock_time () + PING_INTERVAL;
        server->expires_ = zclock_time () + SERVER_TTL;
        free (endpoint);
    } else if (streq (command, "REQUEST")) {
        assert(!self.request_);    //  Strict request-reply cycle
        //  Prefix request with sequence number and empty envelope
        // char sequence_text [10];
        // sprintf (sequence_text, "%u", ++self->sequence);
        zmsg_pushstr(msg, "REQUEST");
        //  Take ownership of request message
        self.request_ = msg;
        msg = NULL;
        //  Request expires after global timeout
        self.expires_ = zclock_time () + GLOBAL_TIMEOUT;
    }
    free (command);
    zmsg_destroy (&msg);
}

//  This method processes one message from a connected
//  server:

void agent_router_message (Agent& self)
{
    zmsg_t *reply = zmsg_recv (self.router_);

    //  Frame 0 is server that replied
    char *endpoint = zmsg_popstr (reply);
    Server *server = (Server *) zhash_lookup (self.servers_, endpoint);
    assert (server);
    free (endpoint);
    if (!server->alive_) {
        zlist_append (self.actives_, server);
        server->alive_ = 1;
    }
    server->ping_at_ = zclock_time () + PING_INTERVAL;
    server->expires_ = zclock_time () + SERVER_TTL;

    //  Frame 1 may be sequence number for reply
    char *sequence = zmsg_popstr (reply);

    // TODO: remove
    zmsg_dump(reply);
    if (atoi (sequence) == self.sequence_) {
        zmsg_pushstr (reply, "OK");
        zmsg_send (&reply, self.pipe_);
        zmsg_destroy(&self.request_);
    } else {
        zmsg_destroy (&reply);
    }
}


static void flcliapi_agent (zsock_t *pipe, void *args)
{
    Agent self = Agent(pipe);
    zsock_signal (pipe, 0);

    while (1) {
        //  Calculate tickless timer, up to 1 hour
        uint64_t tickless = zclock_time () + 1000 * 3600;
        if (self.request_ && tickless > self.expires_) {
            tickless = self.expires_;
        }

        // zhash_foreach (self->servers, server_tickless, &tickless);
        zlist_t *servers = zhash_keys (self.servers_);
        Server* item = (Server*) zhash_first(self.servers_);
        while(item) {
            item->tickless(tickless);
            item = (Server*) zhash_next(self.servers_);
        }

        zsock_t *which = (zsock_t *) zpoller_wait (self.poller_, (tickless - zclock_time ()) * ZMQ_POLL_MSEC);
        if (which == self.pipe_) {
            agent_control_message(self);
        }
        if (which == self.router_) {
            agent_router_message(self);
        }

        //  If we're processing a request, dispatch to next server
        if (self.request_) {
            if (zclock_time() >= self.expires_) {
                //  Request expired, kill it
                zstr_send (self.pipe_, "FAILED");
                zmsg_destroy (&self.request_);
            } else {
                //  Find server to talk to, remove any expired ones
                while (zlist_size (self.actives_)) {
                    Server *server = (Server*) zlist_first (self.actives_);
                    if (zclock_time () >= server->expires_) {
                        zlist_pop (self.actives_);
                        server->alive_ = 0;
                    } else {
                        zmsg_t *request = zmsg_dup(self.request_);
                        zmsg_pushstr (request, server->endpoint_.c_str());
                        zmsg_send (&request, self.router_);
                        break;
                    }
                }
            }
        }

        //  Disconnect and delete any expired servers
        //  Send heartbeats to idle servers if needed
        //  zhash_foreach (self->servers, server_ping, self->router);
        item = (Server*) zhash_first(self.servers_);
        while(item) {
            item->ping(self.router_);
            item = (Server*) zhash_next(self.servers_);
        }
    }
}