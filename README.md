# Deku

C++ library for microservices. Inspired by [cote](https://github.com/dashersw/cote) and [Dask Distributed](https://distributed.dask.org/en/latest/). Powered by [ZeroMQ](https://zeromq.org). Targeted platform is **Linux**.

Project for "Basics of programming 2" course at Budapest University of Technology and Economics (BME).

Simplified example:

- responder.cpp

```c++
Responder res = Responder();

// return the same data that was received
res.on("echo", [] (const std::stringstream& input, std::stringstream& output) {
    output.write(input.str().data(), input.str().size());
});

res.start(); // blocking
```

- requester.cpp

```c++
Requester req = Requester();
std::stringstream reply = req.send("echo", "hello world");
std::cout << reply.str() << std::endl;
// == "hello world"
```

## Table of Contents

1. [Goal](#goal)
2. [High level overview](#overview)
3. [Specification](#spec)
4. [Implementation](#impl)
    1. [Project structure](#structure)
    2. [External libraries](#external)
5. [Development](#dev)
6. [Credits](#creds)
7. [Why "Deku"?](#why)

## Goal

There are a few goals for this project:

1. Learn C++ and it's advanced features.

2. Learn low-level networking or simply - socket programming. ~~That is why, for example, [boost](https://www.boost.org) libraries are not used.~~. Change of mind happend. Because, reliable networking is very complex even for simple distributed application, I decided to use [ZeroMQ](https://zeromq.org). I am still working with sockets but in a little bit different way :)

3. Learn the basics of distributed systems by implementing a simple one.

To make a development process easier, I will assume that the network is secure and all participants (nodes) are acting honestly.

**The library is not intended to be used in production.**

<a name="overview"></a>

## High level overview

**Deku** will provide the ability to create two types of **nodes** in the network (naming is taken from *cote* library):

- Responder
- Requester

Generally, *Responder* will provide certain functions that *Requester* can excecute. *Responder* will process the job(s) sent by *Requester* and return results back. Functions are developed by library users.

The number of Responders and Requesters can be dynamically adjusted. New nodes can be added to the network and removed. *Deku* will handle the discovery of new nodes and communication between them. Currently, the only supported way of discoverying is to use [Redis](https://redis.io) in-memory storage.

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

Because *Deku* doesn't have a centralised scheduler, independent *Requesters* are responsible for scheduling their own jobs. Current algorithm is using approach described in [ZeroMQ guide](http://zguide.zeromq.org/page:all#toc112). *Requester* will try each known *Responder* one by one until it succeceed or job expired.

### Protocol

*Requester* and *Responder* are using a set of pre-defined operational codes (**opcodes**) to communicate over the network. Based on value of opcode, they will take a different action. A simplified list of opcodes with short desciption is specified below.

| Opcode(s)    | Description | States             |
|:----------:|:-----------:|:------------------:|
| PING/PONG  | check if Responder is alive by sending probe request ||
| TASK       | accepting and rejecting jobs from Requester |   OK, BUSY, RESULT |

<a name="impl"></a>

## Implementation

Here implementation details of the project are discussed. Deku is written in **C++14** and was tested only on **Linux**. **ZeroMQ** is the backbone networking and threading library.

<a name="structure"></a>

### Project structure

Root files structure:

- `build/` - executable files
- `examples/` - example of using the library
- `include/` - headers or source code of external libraries
- `libs/`- compiled external libraries (*.a or *.so extensions)
- `src/` - source code (*.cpp and *.h extensions)
- `Makefile` - collection of commands to build a C++ project

<a name="external"></a>

### External libraries

External libraries used:

- `hiredis`, C library to connect to Redis
- `czmq`, high-level C Binding for ZeroMQ

<a name="dev"></a>

## Development

Here you can find information how to setup a project and compile it by yourself.

<a name="creds"></a>

## Credits

...

<a name="why"></a>

## Why "Deku"?

...
