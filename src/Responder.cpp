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

void Responder::worker(zsock_t* task_receiver, zsock_t* result_submitter) {
    while(true) {
        zmsg_t* task = zmsg_recv(task_receiver);
        zframe_t *identity = zmsg_pop(task);
        zframe_t *control = zmsg_pop(task);
        zframe_t *func_name = zmsg_pop(task);
        zframe_t *input_frame = zmsg_pop(task);

        std::string function_name((char*) zframe_data(func_name), zframe_size(func_name));
        std::stringstream input;
        input.write((char*) zframe_data(input_frame), zframe_size(input_frame));
        std::stringstream output;
        this->run(function_name, input, output);

        zmsg_t* results = zmsg_new();
        zmsg_prepend(results, &identity);
        zmsg_append(results, &control);
        zmsg_addstr(results, "RESULT");
        zmsg_addstr(results, function_name.c_str());
        zmsg_addmem(results, output.str().data(), output.str().size());
        zmsg_send(&results, result_submitter);
    } 
}

void Responder::start() {
    address_ = getLocalIPv4Address();

    std::thread discover_thread(&RedisDiscover::notifyService, &redis_discover_, address_, listTasks());
    if (discover_thread.joinable()) {
        discover_thread.detach();
    }
    
    // TODO: destroy sockets on Ctrl+C/Z
    zsock_t *server = zsock_new(ZMQ_ROUTER);
    zsock_set_identity(server, address_.c_str());
    zsock_bind(server, "tcp://*:3434");

    zsock_t *task_sender = zsock_new(ZMQ_PUSH);
    zsock_bind(task_sender, "inproc://tasks");

    zsock_t *task_receiver = zsock_new(ZMQ_PULL);
    zsock_connect(task_receiver, "inproc://tasks");

    zsock_t *result_receiver = zsock_new(ZMQ_PULL);
    zsock_connect(result_receiver, "inproc://tasks_sink");

    zsock_t *result_submitter = zsock_new(ZMQ_PUSH);
    zsock_bind(result_submitter, "inproc://tasks_sink");

    zpoller_t* poller = zpoller_new(server);
    zpoller_add(poller, result_receiver);

    // launch single worker
    // TODO: think about replacing it with zactor from ZeroMQ
    std::thread w(&Responder::worker, this, task_receiver, result_submitter);
    w.detach();

    bool already_job = false;
    while (true) {
        zsock_t* which = (zsock_t *) zpoller_wait (poller, 1 * ZMQ_POLL_MSEC);
        if (which == result_receiver) {
            // job results are ready to be sent
            zmsg_t *job = zmsg_recv(result_receiver);
            zmsg_send(&job, server);
            already_job = false;
        } else if (which == server) {
            // process incoming request
            zmsg_t *request = zmsg_recv(server);
            zmsg_dump(request);
            zframe_t *identity = zmsg_pop(request);
            zframe_t *control = zmsg_pop(request);

            zmsg_t *reply = zmsg_new ();

            if (zframe_streq (control, "PING")) {
                zmsg_addstr(reply, "PONG");
            } else if (zframe_streq(control, "TASK")) {
                // new job
                if (!already_job) {
                    zframe_t *function_name_frame = zmsg_pop(request);
                    zframe_t *function_data_frame = zmsg_pop(request);

                    zmsg_t *task = zmsg_new ();
                    zframe_t* identity_copy = zframe_dup(identity);
                    zframe_t* control_copy = zframe_dup(control);
                    zmsg_prepend(task, &identity_copy);
                    zmsg_append(task, &control_copy);
                    zmsg_append(task, &function_name_frame);
                    // TODO: instead of copying data pass a pointer
                    zmsg_append(task, &function_data_frame);

                    zmsg_send(&task, task_sender);
                    already_job = true;
                    zmsg_append(reply, &control);
                    zmsg_addstr(reply, "OK");
                } else {
                    zmsg_append(reply, &control);
                    zmsg_addstr(reply, "BUSY");
                }
            } else {
                // TODO: replace with some general error response
                zmsg_add (reply, control);
                zmsg_addstr (reply, "Opcode is not found");
            }

            zmsg_destroy(&request);
            zmsg_prepend(reply, &identity);
            zmsg_send(&reply, server);
        }
    }
}
