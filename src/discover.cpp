#include "discover.h"


void Discover::test(char *hostname) {
    struct timeval timeout = { 1, 500000 };
    redisContext *c = redisConnectWithTimeout(hostname, 6379, timeout);
    /* PING server */
    redisReply *reply;
    reply = (redisReply*) redisCommand(c, "PING");
    printf("PING: %s\n", reply->str);
    freeReplyObject(reply);

    /* Set a key */
    reply = (redisReply*) redisCommand(c,"SET %s %s", "foo", "hello world");
    printf("SET: %s\n", reply->str);
    freeReplyObject(reply);

     /* Try a GET and two INCR */
    reply = (redisReply*) redisCommand(c,"GET foo");
    printf("GET foo: %s\n", reply->str);
    freeReplyObject(reply);
}