#include "RedisDiscover.h"

RedisDiscover::RedisDiscover() {
    connect();
}

RedisDiscover::~RedisDiscover() {
    redisFree(redis_client_);
}

std::string RedisDiscover::getRedisHostNameFromEnv() {
    // TODO: read URL/Hostname from env variable
    return "redis";
}

void RedisDiscover::connect() {
    struct timeval timeout = { 1, 500000 };
    int port = 6379;  // default Redis port
    std::string hostname = getRedisHostNameFromEnv();

    redis_client_ = redisConnectWithTimeout(hostname.c_str(), port, timeout);
    if (redis_client_ == NULL || redis_client_->err) {
        if (redis_client_) {
            char err [128];
            strcpy(err, redis_client_->errstr);
            redisFree(redis_client_);
            throw err;
        } else {
            throw "Connection error: can't allocate redis context";
        }
    }
}

std::vector<Multiaddr> RedisDiscover::getPeers() {
    std::vector<Multiaddr> addresses;
    redisReply* reply = (redisReply*) redisCommand(redis_client_, "LRANGE %s 0 -1", discover_list_.c_str());

    if (reply->type == REDIS_REPLY_ARRAY) {
        for (unsigned int j = 0; j < reply->elements; j++) {
            addresses.push_back(Multiaddr(reply->element[j]->str));
        }
    }
    freeReplyObject(reply);
    return addresses;
}

void RedisDiscover::notifyPeers(Multiaddr address) {
    if (redis_client_ == NULL) throw "Redis client is not available";

    // Simple check for duplicates
    std::vector<Multiaddr> peers = getPeers();
     for (const Multiaddr& peer : peers)
        if (peer.toString().compare(address.toString()) == 0)
            return;

    redisReply *reply;
    reply = (redisReply*) redisCommand(redis_client_, "LPUSH %s %s", discover_list_, address.toString().c_str());
    freeReplyObject(reply);
}
