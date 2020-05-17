# Deku

C++ library for microservices. Inspired by [cote](https://github.com/dashersw/cote) and [Dask Distributed](https://distributed.dask.org/en/latest/). Powered by [ZeroMQ](https://zeromq.org). Targeted platform is **Linux**.

**IMPORTANT! The library is not intended to be used in production. Yet...**

Project for "Basics of programming 2" course at Budapest University of Technology and Economics (BME).

Simplified example:

- responder.cpp

```c++
Responder res = Responder();

res.on("echo", [] (const std::stringstream& input, std::stringstream& output) {
    // input sstream is a data that Requester send (input.str() == "hello world")
    // output sstream is a data that will be returned back to Requester as a response
    output.write(input.str().data(), input.str().size());
});

// starts main thread and worker to process requests and jobs, blocking
res.start();
```

- requester.cpp

```c++
// starts Agent in separate thread to process messages
Requester req = Requester();

// send a string "hello world" to any Responder that available and supports "echo" task
std::stringstream reply = req.send("echo", "hello world");
std::cout << reply.str() << std::endl;
// "hello world"
```

## Table of Contents

1. [Goal](#goal)
2. [High level overview](#overview)
3. [Specification](#spec)
4. [Implementation](#impl)
    1. [Project structure](#structure)
    2. [External libraries](#external)
    3. [Class structure](#class)
    4. [Agent class](#agent)
5. [Known issues](#issues)
6. [Development](#dev)
7. [Future](#future)

## Goal

Creating even a primitive distributed system is hard. This library aims to provide a high-level API to be able to build a simple system that is capable of distributing jobs between many computers. It is designed to be used with Docker + cloud (think, GCP + Kubernetes).

In addition, this is also a project for my C++ university course. While working on *Deku*, I have learned a ton of new stuff about the language and distributed systems.

To make a development process easier, I will assume that the network is secure, network has infinite capacity and all participants (nodes) are acting honestly.

<a name="overview"></a>

## High level overview

**Deku** will provide the ability to create two types of **nodes** in the network (naming is taken from *cote* library):

- Responder
- Requester

Generally, *Responder* will provide certain functions that *Requester* can excecute. *Responder* will process the job(s) sent by *Requester* and return results back. Functions are developed by library users.

The number of Responders and Requesters can be dynamically adjusted. New nodes can be added to the network and removed. *Deku* will handle the discovery of new nodes and communication between them. My priority goal is to support usage of library in the cloud. Because, cloud providers do not support UDP broadcasting, currently, the only way of discoverying new nodes in *Deku* is to use [Redis](https://redis.io) in-memory database.

From a graph theory perspective, Responders and Requesters are two **independent** sets in the directed bipartite graph. *Responders* communicate **only** with *Requesters* and vice-versa. Responders cannot start communication with Requesters. It reduces the number of connections needed and simplifies the design.

![simple network diagram](images/high_overview.png)

<a name="spec"></a>

## Specification

Here you can find the design description of the library. Code is not discussed here. If you are interested in implementation, please, check [Implementation](#impl) section.

### Responder

*Responder* is designed to be a non-blocking server. While jobs are excecuting inside **workers** in the background, other incoming messages can be handled by **main thread**. When job is ready, worker will communicate it to the main thread and main thread will send the results back to *Requester*. Currently, only single worker is supported (one job at a time), but it can be extended pretty easly.

![Responder overview diagram](images/responder_spec.png)

### Requester

*Requester* is asynchronous client that is using similar idea as *Responder*. It creates a separate **single** worker in the background that can send and receive messages independently. Main client is communicating with worker to send or receive messages. Currently, one message at the time is supported. This can be extended.

Because *Deku* doesn't have a centralised scheduler, independent *Requesters* are responsible for scheduling their own jobs. Current algorithm is using approach described in [ZeroMQ guide](http://zguide.zeromq.org/page:all#toc112). *Requester* will try to send a message to each known *Responder* one by one until it succeceed or job expired.

### Protocol

*Requester* and *Responder* are using a set of pre-defined operational codes (**opcodes**) to communicate over the network. Based on value of opcode, they will take a different action. A simplified list of opcodes with short desciption is specified below.

| Opcode(s)    | Description | States             |
|:----------:|:-----------:|:------------------:|
| PING/PONG  | check if Responder is alive by sending probe request ||
| TASK       | accepting and rejecting jobs from Requester |   OK, BUSY, RESULT, ERROR |

<a name="impl"></a>

## Implementation

Here implementation details of the project are discussed.

Deku is written in **C++14** and was tested only on **Linux** (in Docker Compose, manually). Initially, I was thinking to use a raw sockets to develop the library, but after some time I realized that there are so many different edge cases and issues you can encounter. Instead, I decided to use an amazing networking and threading library called [ZeroMQ](https://zeromq.org). ZeroMQ not only simplifies the development of the project, but also allows, in future, to make *Deku* stable, performant and easy to use.

<a name="structure"></a>

### Project structure

Root files structure:

- `build/` - executable files
- `examples/` - some examples of using the library
- `include/` - headers or source code of external libraries
- `libs/`- compiled external libraries (*.a or *.so extensions)
- `src/` - source code (*.cpp and *.h extensions)
- `Makefile` - collection of commands to build a C++ project

<a name="external"></a>

### External libraries

External libraries used:

- `hiredis`, C library to connect to Redis
- `czmq`, high-level C Binding for ZeroMQ

<a name="class"></a>

### Class structure

*Deku* has two main classes:

- Responder
- Requester

Both *Responder* and *Requester* are depending on *RedisDiscovery* class which provides the ability to connect to, register or fetch the available nodes from the Redis.

In addition, *Requester* depends on *Agent* class (in `agent/` folder). *Agent* is responsible for sending and receiving requests, and it works inside the separate thread.

Generally, class structure is very simple.

<a name="agent"></a>

### How it works

*Responder's* implementation is straight-forward and doesn't require a discussion. Please, check a [Specification section](#spec) and code.

*Requester* and *Agent* are fairly sophisticated and require a little bit of explanation. Implementation and flow of `Agent` class were taken and reworked from the [ZeroMQ guide (Freelance pattern section)](http://zguide.zeromq.org/page:all#toc112) (description is taken from the guide + some parts rewritten):

- Multithreaded API. The *Requester API* consists of two parts, a synchronous *Requester* class that runs in the application thread, and an asynchronous *Agent* class that runs as a background thread. The *Requester* and *Agent* classes talk to each other with messages over an **inproc** socket (with help of `zactor`). The agent in effect acts like a mini-broker, talking to servers in the background, so that when we make a request, it can make a best effort to reach a server it believes is available. *Agent* checks if server (Responder) is not busy and if it supports the task Requester wants to excecute.

- Tickless poll timer, the Agent uses a tickless timer, which calculates the poll delay based on the next timeout we're expecting. A proper implementation would keep an ordered list of timeouts. We just check all timeouts and calculate the poll delay until the next one.

- Discovery, every `DISCOVERY_INTERVAL`, *Agent* fetches the servers from Redis and updates the known servers (check `Agent.discover_servers()`).

All other details, can be found in the code. Thanks.

<a name="issues"></a>

## Known issues

As for now, *Deku* is very raw project with a lot of small and big issues. Some of them:

- Agent (hence Requester) can process only one request at the time.
- if Redis doesn't exist Responder and Requester will fail on a start time.
- no exception classes specific for Deku are created. I am using `std::logic_error` now but this is not a good solution.
- even after some fixes there is possibility that if Requester will start too early before Responder, it will not be able to send a message on time and it will expire. You will get and exception with `EXPIRED` message.

<a name="dev"></a>

## Development

Here you can find information on how to setup a project and compile it by yourself. First of all, clone this repo.

### Docker (only library compiling)

The easiest way is to use Docker. Please, check `Dockerfile` where you can find all steps. Comment the last line (which is used for examples) and uncomment `make build_lib`. After building an image, you will find a compiled dynamic library in `libs/libdeku.a`. To use a library, you will need to install `czmq` and `hiredis` C libraries (check `Dockerfile`, simple `apt-get install`) and link them in your project. For an example of using it in the project check next section.

Docker version used during the development: **18.06.1-ce**

## Docker Compose (running an example)

Folder for examples is `examples/`. Replace `make build_lib` with `make build_example_echo` (or `make build_example_file`). Run `docker-compose up --build`. It will compile, build and launch two instances (Requester and Responder) that will excecute example program. Enjoy!

Docker-compose version used during the development: **1.25.4**

<a name="future"></a>

## Future

Currently, *Deku* is raw and buggy. BUT, design, in my opinion, is simple and promising. With ZeroMQ, *Deku* will have a powerful networking and threading background. By adding multi-request support, enhancing API for users and making *Deku* more stable will give this library a chance to exist and possibly be used for some useful purposes :)
