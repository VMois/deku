#include "RedisDiscover.h"

RedisDiscover::RedisDiscover() {
    connect();
}

RedisDiscover::~RedisDiscover() {
    redisFree(redis_client_);
}

char* RedisDiscover::getRedisHostNameFromEnv() {
    // TODO: read URL/Hostname from env variable
    return (char*) "redis";
}

void RedisDiscover::connect() {
    struct timeval timeout = { 1, 500000 };
    int port = 6379;  // default Redis port
    char* hostname = getRedisHostNameFromEnv();

    redis_client_ = redisConnectWithTimeout(hostname, port, timeout);
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

void RedisDiscover::notifyPeers() {
    if (redis_client_ == NULL) throw "Redis client is not available";

    printf("Notify peers \n");
    redisReply *reply;
    /* PING server
    reply = (redisReply*) redisCommand(redis_client_, "PING");
    printf("PING: %s\n", reply->str);
    freeReplyObject(reply);*/

    reply = (redisReply*) redisCommand(redis_client_,"LLEN %s", discover_list_);
    printf("List len: %d\n", (int) reply->integer);
    freeReplyObject(reply);

    /* Set a key */
    /*reply = (redisReply*) redisCommand(redis_client_,"SET %s %s", "foo", "hello world");
    printf("SET: %s\n", reply->str);
    freeReplyObject(reply);*/

     /* Try a GET and two INCR
    reply = (redisReply*) redisCommand(redis_client_,"GET foo");
    printf("GET foo: %s\n", reply->str);
    freeReplyObject(reply);*/
}

void RedisDiscover::getPeers() { }