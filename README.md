# Deku

C++ library for microservices. Inspired by [cote](https://github.com/dashersw/cote) and [libp2p](https://github.com/libp2p/specs). Project for "Basic of programming 2" course at Budapest University of Technology and Economics (BME). Targeted platform is **Linux** (Docker). **Work in Progress**.

## Table of Contents

1. [Goal](#goal)
2. [High level overview](#hoverview)
3. [Project structure](#structure)
4. [Development](#dev)

## Goal

There are a few goals for this project:

1. Learn C++ and it's advanced features.

2. Learn low-level networking or simply - socket programming. That is why, for example, [boost](https://www.boost.org) libraries are not used.

3. Learn the basics of distributed systems by implementing a simple one.

To make a development process easier, I will assume that the network is secure and all participants (nodes) are acting honestly.

**The library is not intended to be used in production.**

<a name="hoverview"></a>

## High level overview

**Deku** (library name) will provide ability to create two types of **nodes** in the network:

- Responder
- Requester

Generally, *Responder* will provide certain functions that *Requester* can call. *Responder* will process the results and send them back to *Requester*. Functions are developed by library users.

The number of Responders and Requesters can be dynamically adjusted. New nodes can be added to the network and removed. *Deku* will handle the discovery of other nodes and communication between them. The current plan is to use [Redis](https://redis.io) in-memory storage for the discovery purposes.

From a graph theory perspective, Responders and Requesters are two **independent** sets in the directed bipartite graph. *Responders* communicate **only** with *Requesters* and vice-versa. In addition, Responders cannot start communication with Requesters. It reduces the number of connections and simplifies the design.

![simple network diagram](high_overview.png)


<a name="structure"></a>

## Project structure

Root file structure overview:

- `build/` - executable files
- `include/` - headers or source code of external libraries
- `libs/`- compiled external libraries (*.a or *.so extensions)
- `src/` - source code (*.cpp and *.h extensions)
- `tests/` - unit tests

<a name="dev"></a>

## Development

External libraries used:

- `hiredis`, to connect to Redis memory storage
- `msgpack-c`, to serialize/deserialize data
- `stdlog` (1.x version), logging library
- `catch2`, testing framework
