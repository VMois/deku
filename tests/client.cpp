#include <zmq.hpp>
#include <string>
#include <iostream>
#include "msgpack.hpp"
#include <iostream>
#include <sstream>

int main ()
{
    //  Prepare our context and socket
    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REQ);

    std::cout << "Connecting to hello world server…" << std::endl;
    socket.connect ("tcp://localhost:3434");
    msgpack::type::tuple<std::string, std::string> job("echo", "hello");
    std::stringstream buffer;
    msgpack::pack(buffer, job);
    buffer.seekg(0);
    msgpack::type::tuple<int, std::string> package(2, buffer.str());

    // serialize the object into the buffer.
    // any classes that implements write(const char*,size_t) can be a buffer.
    std::stringstream buffer1;
    msgpack::pack(buffer1, package);
    buffer1.seekg(0);

    //  Do 10 requests, waiting each time for a response
    for (int request_nbr = 0; request_nbr != 10; request_nbr++) {
        zmq::message_t request (buffer1.str().size());
        memcpy(request.data(), buffer1.str().data(), buffer1.str().size());
        socket.send(request);

        //  Get the reply.
        zmq::message_t reply;
        socket.recv(&reply);
        std::cout << "Data: " << (char*) reply.data() << std::endl;
    }
    return 0;
}
