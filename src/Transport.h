#pragma once
#include <stdio.h> 
#include <iostream>     
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <string> 
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <algorithm>
#include <cassert>

#include <zmq.hpp>

class Transport {
  std::function <void(zmq::socket_t&)> new_data_function_;

  void worker(zmq::context_t &context);

  public:
    // TODO: clean zmq
    // ~Transport();
    
    std::string getLocalIPv4Address();
    int getFreePort();

    void on_new_data(std::function <void(zmq::socket_t&)> function);
    void listen(zmq::context_t &context, std::string address);
};
