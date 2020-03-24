#include "Responder.h"

Responder::Responder() {
    // TODO: assign proper local address
    std::string address = "/ip4/0.0.0.0/tcp/3434";
    multiaddr_ = Multiaddr(address);
    logger_ = spdlog::stdout_color_st("Responder");
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
        const int CHUNK_SIZE = 4;
        std::array<char, CHUNK_SIZE> buffer;
        std::stringstream stream;

        // TODO: add feature to exit the thread if requested
        while (1) {
            // clear inputs
            buffer = {'\0'};
            stream.str(std::string());

            std::size_t bytes_read = read(socket, buffer.data(), CHUNK_SIZE);
            if (bytes_read == 0) {
                spdlog::info("socket {} is closed", socket);
                close(socket);
                return;  
            }
            // TODO: check if bytes_read is equal to CHUNK_SIZE, abort if not
            stream.write(buffer.data() + 1, CHUNK_SIZE - 1);
            while (buffer[0] == '\x01') {
                bytes_read = read(socket, buffer.data(), CHUNK_SIZE);
                stream.write(buffer.data() + 1, CHUNK_SIZE - 1);
            }


            msgpack::object_handle oh = msgpack::unpack(stream.str().data(), 
                                                        stream.str().size());
            msgpack::object deserialized = oh.get();
            msgpack::type::tuple<int, std::string> msg;
            deserialized.convert(msg);

            switch (msg.get<0>())
            {
                case 1:
                    {
                    spdlog::info("ping, not implemented yet");
                    }
                    break;
                case 2: 
                    {
                    spdlog::info("new job");
                    std::string job_raw_data = msg.get<1>();
                    oh = msgpack::unpack(job_raw_data.data(), job_raw_data.size());
                    deserialized = oh.get();
                    msgpack::type::tuple<std::string, std::string> job_metadata;
                    deserialized.convert(job_metadata);

                    std::string function_name = job_metadata.get<0>();
                    spdlog::info("new job, {}", function_name);
                    std::stringstream input(job_metadata.get<1>());
                    std::stringstream output;
                    this->run(function_name, input, output);

                    spdlog::info("Return data: {}", spdlog::to_hex(output.str()));

                    output.seekg(0);
                    size_t return_data_len = output.str().size();
                    int chunks_num = std::ceil(float(return_data_len) / (CHUNK_SIZE - 1));

                    char *buffer = new char[chunks_num * CHUNK_SIZE];
                    for (int chunk_i = 0; chunk_i < chunks_num; chunk_i++) {
                        int chunk_start_position  = CHUNK_SIZE * chunk_i;
                        if (chunk_i != (chunks_num - 1)) {
                            buffer[chunk_start_position] = '\x01';
                            output.read(buffer + chunk_start_position + 1, 3);
                        } else {
                            buffer[chunk_start_position] = '\x00';
                            int bytes_to_read = return_data_len - chunk_i * (CHUNK_SIZE - 1);
                            output.read(buffer + chunk_start_position + 1, bytes_to_read);
                            for (int i = bytes_to_read + 1; i < CHUNK_SIZE; i++) {
                                buffer[chunk_start_position + i] = '\x00';
                            }
                        }
                    }
                    send(socket, buffer, CHUNK_SIZE * chunks_num, 0);
                    delete[] buffer;
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
