#include <stdio.h> 
#include <iostream>     
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <string> 
#include <cstring>
#include <arpa/inet.h>
#pragma once
#include <unistd.h>
#include <map>
#include <functional>
#include <thread> 

#include "multiaddr/Multiaddr.h"

class Transport {
  std::map<std::string, std::vector<std::function <void(int)>>> handlers;
  std::vector<int> client_sockets_;
  int master_socket_;

  public:
    Transport(): master_socket_(0) {};
    // ~Transport();  TODO: clean master_socket
    std::string getLocalIPv4Address();
    int getFreePort();

    void start();
    void on(std::string event_name, 
            std::function <void(int)> handler);
    
    void fireEvent(std::string event_name, int socket);
    void listenMultiaddr(Multiaddr address);
};