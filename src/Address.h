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

class Address {
  public:
    std::string getLocalIPv4Address();
};