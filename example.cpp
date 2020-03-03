#include "src/discover.h"

int main() {
    Discover network = Discover();
    network.test((char*) "redis");
    return 0;
}