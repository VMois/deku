#include "msgpack.hpp"
#include "src/Client.h"

int main() {
    msgpack::type::tuple<std::string, std::string> job("echo", "hello");
    std::stringstream buffer;
    msgpack::pack(buffer, job);
    buffer.seekg(0);

    Client cli = Client();
    cli.connect("tcp://localhost:5543");
    zmsg_t *msg = zmsg_new ();
    zmsg_addstr(msg, buffer.str().c_str());
    zmsg_t *reply = cli.request(&msg);
    // zmsg_dump(reply);
    return 0;
}