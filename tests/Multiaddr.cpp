#define CATCH_CONFIG_MAIN
#include "../include/catch2.h"
#include "../src/multiaddr/Multiaddr.h"

TEST_CASE("Multiaddr class", "[Multiaddr]" ) {
    Multiaddr correct_ip4_tcp("/ip4/172.16.2.3/tcp/1234");
    Multiaddr correct_ip4_udp("/ip4/172.140.3.30/udp/4321");

    REQUIRE(correct_ip4_tcp.toString() == "/ip4/172.16.2.3/tcp/1234");
    REQUIRE(correct_ip4_tcp.getIP4Address() == "172.16.2.3");
    REQUIRE(correct_ip4_udp.getIP4Address() == "172.140.3.30");

    REQUIRE(correct_ip4_tcp.getPort() == 1234);
    // not working in C++ because if constructor thows an error object doesn't exist
    // REQUIRE_THROWS_WITH(Multiaddr("ip4/127.0.0.1/udp/1234"), );
}