#pragma once
#include <string>
#include <stdio.h> 
#include <iostream>     
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

std::string getLocalEndpointAddress();