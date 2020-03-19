#include "Responder.h"

Responder::Responder() {
    // TODO: assign proper local address
    std::string address = "/ip4/0.0.0.0/tcp/3434";
    multiaddr_ = Multiaddr(address);
    logger_ = spdlog::stdout_color_st("Responder");
}

void Responder::on(std::string task_name, 
                   std::function <std::stringstream(std::stringstream)> handler) {
    handlers_[task_name] = handler;
};

std::vector<std::string> Responder::listTasks() {
    std::vector<std::string> tasks;
    for (std::map<std::string, std::function <std::stringstream(std::stringstream)>>::iterator it = handlers_.begin(); 
        it != handlers_.end(); ++it) {
            tasks.push_back(it->first);
        }
    return tasks;
}

void Responder::start() {
    std::thread discover_thread(&RedisDiscover::notifyService, &redis_discover_, multiaddr_, listTasks());
    if (discover_thread.joinable()) {
        discover_thread.detach();
    }
    transport_.on("new_connection", [] (int socket) {
        const int CHUNK_SIZE = 4;
        std::array<char, CHUNK_SIZE> buffer = {'\0'};
        std::stringstream stream;

        // process data from socket
        std::size_t bytes_read = read(socket, buffer.data(), CHUNK_SIZE);
        stream.write(buffer.data() + 1, CHUNK_SIZE - 1);
        while (buffer[0] == '\x01') {
            bytes_read = read(socket, buffer.data(), CHUNK_SIZE);
            stream.write(buffer.data() + 1, CHUNK_SIZE - 1);
        }

        spdlog::info("{}", spdlog::to_hex(stream.str()));

        msgpack::object_handle oh = msgpack::unpack(stream.str().data(), 
                                                    stream.str().size());
        msgpack::object deserialized = oh.get();
        std::vector<int> dst;
        deserialized.convert(dst);
        spdlog::info("{0}, {1}, {2}, {3}", dst[0], dst[1], dst[2], dst.size());
    });
    
    transport_.listenMultiaddr(multiaddr_);
    transport_.start();

    // while (1) {};
}
