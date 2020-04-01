#include "RedisDiscover.h"

RedisDiscover::RedisDiscover() {
    redis_client_ = NULL;
    tasks_list_ = "deku_tasks";
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

std::vector<std::string> RedisDiscover::getTasks() {
    std::vector<std::string> tasks;

    redisReply* reply = (redisReply*) redisCommand(redis_client_, "LRANGE %s 0 -1", tasks_list_.c_str());

    if (reply->type == REDIS_REPLY_ARRAY) {
        for (unsigned int j = 0; j < reply->elements; j++) {
            tasks.push_back(reply->element[j]->str);
        }
    }
    freeReplyObject(reply);
    return tasks;
}

std::vector<Multiaddr> RedisDiscover::getAddresses(std::string task_name) {
    std::vector<Multiaddr> addresses;
    redisReply* reply = (redisReply*) redisCommand(redis_client_, "LRANGE %s 0 -1", task_name.c_str());

    if (reply->type == REDIS_REPLY_ARRAY) {
        for (unsigned int j = 0; j < reply->elements; j++) {
            addresses.push_back(Multiaddr(reply->element[j]->str));
        }
    }
    freeReplyObject(reply);
    return addresses;
}

void RedisDiscover::notifyPeers(Multiaddr address, std::vector<std::string> new_tasks) {
    if (redis_client_ == NULL) throw "Redis client is not available";
    redisReply *reply = NULL;

    for (const std::string task : new_tasks) {
        std::vector<std::string> tasks = getTasks();
        std::vector<std::string>::iterator tasks_it = std::find(tasks.begin(), tasks.end(), task);

        // add task if not found
        if (tasks_it == tasks.end()) {
            reply = (redisReply*) redisCommand(redis_client_, "LPUSH %s %s", tasks_list_.c_str(), task.c_str());
            freeReplyObject(reply);
        }
        
        std::vector<Multiaddr> peers = getAddresses(task);
        std::vector<Multiaddr>::iterator peers_it = std::find(peers.begin(), peers.end(), address);

        // add address if not exists
        if (peers_it == peers.end()) {
            reply = (redisReply*) redisCommand(redis_client_, "LPUSH %s %s", task.c_str(), address.toString().c_str());
            freeReplyObject(reply);
        }
    }
}

void RedisDiscover::notifyService(Multiaddr address, std::vector<std::string> tasks) {
    connect();

    std::chrono::seconds duration(5);

    while(1) {
        notifyPeers(address, tasks);
        std::this_thread::sleep_for(duration);
    }
}
