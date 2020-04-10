#include "Transport.h"

// Returns local IP address
std::string Transport::getLocalIPv4Address() {
    // https://stackoverflow.com/questions/49335001/get-local-ip-address-in-c
    int sock = socket(PF_INET, SOCK_DGRAM, 0);
    sockaddr_in loopback;

    if (sock == -1) {
        throw "Could not create socket";
    }

    std::memset(&loopback, 0, sizeof(loopback));
    loopback.sin_family = AF_INET;
    loopback.sin_addr.s_addr = INADDR_LOOPBACK;   // using loopback ip address
    loopback.sin_port = htons(9);                 // using debug port

    if (connect(sock, reinterpret_cast<sockaddr*>(&loopback), sizeof(loopback)) == -1) {
        close(sock);
        throw "Could not connect to socket";
    }

    socklen_t addrlen = sizeof(loopback);
    if (getsockname(sock, reinterpret_cast<sockaddr*>(&loopback), &addrlen) == -1) {
        close(sock);
        throw "Could not getsockname";
    }

    close(sock);

    char buf[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &loopback.sin_addr, buf, INET_ADDRSTRLEN) == 0x0) 
    {
        throw "Could not inet_ntop";
    } else 
    {
        std::string value(buf, INET_ADDRSTRLEN);
        return value;
    }
}

// Returs random free port
int Transport::getFreePort() 
{
    // TODO: find a way to check free ports and choose one
    return 3434;
}

void *Transport::worker(zmq::context_t &context)
{
    zmq::socket_t workers (context, ZMQ_REP);
    workers.connect("inproc://workers");

    while (true) {
        new_data_function_(workers);
    }
}


void Transport::listen(zmq::context_t &context, std::string address) { 
    zmq::socket_t clients(context, ZMQ_ROUTER);
    clients.bind(address);
    zmq::socket_t workers(context, ZMQ_DEALER);
    workers.bind("inproc://workers");

    for (int thread_nbr = 0; thread_nbr != 2; thread_nbr++) {
        std::thread worker(&Transport::worker, this, std::ref(context));
        worker.detach();
    }
    zmq::proxy(static_cast<void*>(clients),
               static_cast<void*>(workers),
               nullptr);
};


void Transport::on_new_data(std::function <void(zmq::socket_t&)> func) {
    new_data_function_ = func;
};
