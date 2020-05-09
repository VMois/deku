# Deku

C++ library for microservices. Inspired by [cote](https://github.com/dashersw/cote) and [Dask Distributed](https://distributed.dask.org/en/latest/). Powered by [ZeroMQ](https://zeromq.org). Targeted platform is **Linux**.

Project for "Basics of programming 2" course at Budapest University of Technology and Economics (BME).

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

Generally, *Responder* will provide certain functions that *Requester* can call. *Responder* will process the results and send them back to *Requester*. Functions are developed by library users.

The number of Responders and Requesters can be dynamically adjusted. New nodes can be added to the network and removed. *Deku* will handle the discovery of other nodes and communication between them. The current plan is to use [Redis](https://redis.io) in-memory storage for the discovery purposes.

From a graph theory perspective, Responders and Requesters are two **independent** sets in the directed bipartite graph. *Responders* communicate **only** with *Requesters* and vice-versa. Responders cannot start communication with Requesters. It reduces the number of connections needed and simplifies the design.

![simple network diagram](high_overview.png)

<a name="spec"></a>

## Specification

Here you can find the design description of the library. Code is not discussed here. If you are interested in implementation, please, check [Implementation](#impl) section.

...

<a name="impl"></a>

## Implementation

Here implementation details of the project are discussed. Deku is written in **C++14** and was tested only on **Linux**.

<a name="structure"></a>

### Project structure

Root files structure:

- `build/` - executable files
- `include/` - headers or source code of external libraries
- `libs/`- compiled external libraries (*.a or *.so extensions)
- `src/` - source code (*.cpp and *.h extensions)
- `tests/` - unit tests

<a name="external"></a>

### External libraries

External libraries used:

- `hiredis`, C library to connect to Redis in-memory storage
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
