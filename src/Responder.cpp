#include "Responder.h"

Responder::Responder() {
    // TODO: assign proper local address
    std::string address = "/ip4/0.0.0.0/tcp/3434";
    multiaddr_ = Multiaddr(address);
}

void Responder::on(std::string task_name, 
                   std::function <void(const std::stringstream&, std::stringstream&)> handler) {
    handlers_[task_name] = handler;
};

void Responder::run(std::string task_name, const std::stringstream& input, std::stringstream& output) {
    handlers_[task_name](input, output);
};

std::vector<std::string> Responder::listTasks() {
    std::vector<std::string> tasks;
    for (std::map<std::string, std::function <void(const std::stringstream&, std::stringstream&)>>::iterator it = handlers_.begin(); 
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
    transport_.on("new_connection", [&] (int socket) {
        char* buffer = new char[Packer::getChunkSize()];
        std::stringstream stream;

        while (1) {
            // clear inputs
            stream.str(std::string());

            std::size_t bytes_read = read(socket, buffer, Packer::getChunkSize());
            if (bytes_read == 0) {
                delete[] buffer;
                close(socket);
                return;  
            }

            // TODO: add check if bytes_read == chunk_size
            // As for now, I don't know what to do in this situation
            // but this important in case of network delays/errors
            // in high-performance env
            while(Packer::decode_chunk(stream, buffer)) {
                bytes_read = read(socket, buffer, Packer::getChunkSize());
            }

            msgpack::object_handle oh = msgpack::unpack(stream.str().data(), 
                                                        stream.str().size());
            msgpack::object deserialized = oh.get();
            msgpack::type::tuple<int, std::string> msg;
            deserialized.convert(msg);

            switch (msg.get<0>())
            {
                case 2: 
                    {
                    std::string job_raw_data = msg.get<1>();
                    oh = msgpack::unpack(job_raw_data.data(), job_raw_data.size());
                    deserialized = oh.get();
                    msgpack::type::tuple<std::string, std::string> job_metadata;
                    deserialized.convert(job_metadata);

                    std::string function_name = job_metadata.get<0>();
                    std::stringstream input(job_metadata.get<1>());
                    std::stringstream output;
                    this->run(function_name, input, output);

                    spdlog::info("Return data: {}", spdlog::to_hex(output.str()));
                    char* return_buffer;
                    int size;
                    Packer::encode(output, return_buffer, size);

                    // TODO: check how many bytes were send
                    // decide what to do in the case if not all bytes were sent
                    send(socket, return_buffer, size, 0);
                    
                    delete[] return_buffer;
                    }
                    break;
                default:
                    spdlog::error("Cannot recognize protocol code");
                    break;
            }
        }
    });
    
    transport_.listenMultiaddr(multiaddr_);
    transport_.start();

    // while (1) {};
}
