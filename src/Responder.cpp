#include "Responder.h"

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
    /*std::thread discover_thread(&RedisDiscover::notifyService, &redis_discover_, multiaddr_, listTasks());
    if (discover_thread.joinable()) {
        discover_thread.detach();
    }*/
    transport_.on_new_data([&] (zmq::socket_t &host) {
        zmq::message_t request;
        host.recv(&request);

        msgpack::object_handle oh = msgpack::unpack((char*) request.data(), 
                                                    request.size());
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
                int size = output.str().size();

                zmq::message_t reply (size);
                memcpy((void *) reply.data(), output.str().data(), size);
                host.send(reply);
                }
                break;
            default:
                spdlog::error("Cannot recognize protocol code");
                break;
        }
    });
    transport_.listen(context_, "tcp://0.0.0.0:3434");

    // while (1) {};
}
