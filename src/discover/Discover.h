#include "../multiaddr/Multiaddr.h"

class Discover {
  public:
    virtual void notifyPeers(Multiaddr address) = 0;
    virtual std::vector<Multiaddr> getPeers() = 0;
};