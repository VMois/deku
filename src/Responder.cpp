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
        // wait for a new job
        zmsg_t* job = zmsg_recv(task_receiver);

        // unpack job details
        zframe_t *identity = zmsg_pop(job);
        zframe_t *control = zmsg_pop(job);
        zframe_t *func_name = zmsg_pop(job);
        zframe_t *input_frame = zmsg_pop(job);

        std::string function_name((char*) zframe_data(func_name), zframe_size(func_name));
        std::stringstream input;
        input.write((char*) zframe_data(input_frame), zframe_size(input_frame));
        std::stringstream output;

        // TODO: add try/catch block
        zmsg_t* results = zmsg_new();
        zmsg_append(results, &control);
        try {
            this->run(function_name, input, output);
            zmsg_addstr(results, "RESULT");
            // TODO: what will happen if output stream is empty?
            zmsg_addmem(results, output.str().data(), output.str().size());
        } catch (std::exception& e) {
            zmsg_addstr(results, "ERROR");
            zmsg_addstr(results, e.what());
        }
        zmsg_prepend(results, &identity);
        zmsg_send(&results, result_submitter);
    } 
}

void Responder::start() {
    address_ = getLocalEndpointAddress();

    // save task in Redis
    redis_discover_.notifyPeers(address_, listTasks());

    // prepare sockets for main thread and worker
    zsock_t *server = zsock_new(ZMQ_ROUTER);
    zsock_set_identity(server, address_.c_str());

    // listen for incoming connections on port 3434 (default)
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

    // launch a single worker
    std::thread w(&Responder::worker, this, task_receiver, result_submitter);
    w.detach();

    bool already_job = false;
    while (true) {
        // wait for incoming message to server or for job to be finished by worker
        zsock_t* which = (zsock_t *) zpoller_wait(poller, 1 * ZMQ_POLL_MSEC);
        
        if (which == result_receiver) {
            // job is finished, send results back
            zmsg_t *job = zmsg_recv(result_receiver);
            zmsg_send(&job, server);
            already_job = false;
        } else if (which == server) {
            // process incoming message from the Requester
            zmsg_t *request = zmsg_recv(server);

            // print message, for debug purposes
            zmsg_dump(request);

            zframe_t *identity = zmsg_pop(request);
            zframe_t *control = zmsg_pop(request);

            zmsg_t *reply = zmsg_new();

            if (zframe_streq (control, "PING")) {
                zmsg_addstr(reply, "PONG");
            } else if (zframe_streq(control, "TASK")) {
                if (!already_job) {
                    // we can process this job
                    zframe_t *function_name_frame = zmsg_pop(request);
                    zframe_t *function_data_frame = zmsg_pop(request);

                    // prepare message for worker
                    zmsg_t *task = zmsg_new ();
                    zframe_t* identity_copy = zframe_dup(identity);
                    zframe_t* control_copy = zframe_dup(control);
                    zmsg_prepend(task, &identity_copy);
                    zmsg_append(task, &control_copy);
                    zmsg_append(task, &function_name_frame);
                    // TODO: instead of copying data pass a pointer to save memory
                    zmsg_append(task, &function_data_frame);

                    // send a job to a worker thread
                    zmsg_send(&task, task_sender);
                    already_job = true;

                    // reply to Requester that job was accepted
                    zmsg_append(reply, &control);
                    zmsg_addstr(reply, "OK");
                } else {
                    // if job already processed,
                    zmsg_append(reply, &control);
                    zmsg_addstr(reply, "BUSY");
                }
            } else {
                // TODO: replace with some general error response
                zmsg_add (reply, control);
                zmsg_addstr (reply, "Opcode is not found");
            }

            zmsg_destroy(&request);

            // add address of Requester
            zmsg_prepend(reply, &identity);
            zmsg_send(&reply, server);
        }
    }
    // TODO: deregister itself from Redis when exiting a program
    // TODO: destroy sockets on Ctrl+C/Z, add singals handling using lambda function
}
