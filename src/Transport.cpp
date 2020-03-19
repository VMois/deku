#include "Transport.h"

// Returns local IP address
std::string Transport::getLocalIPv4Address() {
    // https://stackoverflow.com/questions/49335001/get-local-ip-address-in-c
    int sock = socket(PF_INET, SOCK_DGRAM, 0);
    sockaddr_in loopback;

    if (sock == -1) {
        throw "Could not create socket";
    }

    std::memset(&loopback, 0, sizeof(loopback));
    loopback.sin_family = AF_INET;
    loopback.sin_addr.s_addr = INADDR_LOOPBACK;   // using loopback ip address
    loopback.sin_port = htons(9);                 // using debug port

    if (connect(sock, reinterpret_cast<sockaddr*>(&loopback), sizeof(loopback)) == -1) {
        close(sock);
        throw "Could not connect to socket";
    }

    socklen_t addrlen = sizeof(loopback);
    if (getsockname(sock, reinterpret_cast<sockaddr*>(&loopback), &addrlen) == -1) {
        close(sock);
        throw "Could not getsockname";
    }

    close(sock);

    char buf[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &loopback.sin_addr, buf, INET_ADDRSTRLEN) == 0x0) 
    {
        throw "Could not inet_ntop";
    } else 
    {
        std::string value(buf, INET_ADDRSTRLEN);
        return value;
    }
}

// Returs random free port
int Transport::getFreePort() {
    // TODO: find a way to check free ports and choose one
    return 3434;
}

void Transport::listenMultiaddr(Multiaddr multiaddr) {
    int opt = 1;
    struct sockaddr_in address;  

    // create a master socket 
    if((master_socket_ = socket(AF_INET , SOCK_STREAM , 0)) == 0)
        throw "Socket creation failed";
    // set master socket to allow multiple connections ,  
    // this is just a good habit, it will work without this  
    if(setsockopt(master_socket_, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, 
        sizeof(opt)) < 0 )
            throw "Setsockopt";
    
    // type of socket created  
    address.sin_family = AF_INET;
    address.sin_port = htons(multiaddr.getPort()); 

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, multiaddr.getIP4Address().c_str(), &address.sin_addr)<=0)  
        throw "Invalid address / Address not supported \n";

    //bind the socket to localhost port 8888  
    if (bind(master_socket_, (struct sockaddr *)&address, sizeof(address)) < 0)
        throw "Bind failed";
    printf("Registered %s:%d address for listening\n", multiaddr.getIP4Address().c_str(), multiaddr.getPort());
}

// listen for incoming connections
void Transport::start() { 
    printf("Start listening...\n");

    int addrlen, new_socket, activity;  
    struct sockaddr_in address;

    // try to specify maximum of 3 pending connections for the master socket  
    if (listen(master_socket_, 5) < 0) {   
        perror("listen");   
        exit(EXIT_FAILURE);   
    }
         
    // set of socket descriptors  
    fd_set readfds;

    while(1)   
    {   
        //clear the socket set  
        FD_ZERO(&readfds);   
     
        //add master socket to set  
        FD_SET(master_socket_, &readfds);

        //wait for an activity on one of the sockets , timeout is NULL ,  
        //so wait indefinitely  
        activity = select(master_socket_ + 1, &readfds, NULL, NULL, NULL);   
       
        if ((activity < 0) && (errno!=EINTR))   
            throw "Select error";   
             
        // If something happened on the master socket ,  
        // then its an incoming connection  
        if (FD_ISSET(master_socket_, &readfds))   
        {   
            if ((new_socket = accept(master_socket_, 
                (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {   
                    throw "Accept error";   
            }
             
            //inform user of socket number - used in send and receive commands  
            printf("New connection, socket fd is %d, ip is : %s , port : %d\n" , new_socket , inet_ntoa(address.sin_addr) , ntohs 
                  (address.sin_port));
            fireEvent("new_connection", new_socket);
        }
    }
}

void Transport::on(std::string event_name, std::function <void(int)> handler) {
    handlers[event_name].push_back(handler);
}

void Transport::fireEvent(std::string event_name, int socket) {
    std::vector<std::function <void(int)>> event = handlers[event_name];
    for (const std::function <void(int)> f : event) {
        std::thread th(f, socket);
        th.detach();
    }
}