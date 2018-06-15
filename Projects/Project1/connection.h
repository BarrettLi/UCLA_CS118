#include <unistd.h>
#include <stdlib.h>
#include "server.h"

typedef struct{
    char *ptr;
    size_t size;
    size_t length;
} string;

typedef struct{

} http_request;

typedef struct{

} http_response;

typedef struct{
    // client's socket
    int sockfd;

    // status code
    int status_code;

    // local buffer
    string buffer;

    // HTTP request
    http_request *request;

    // HTTP response
    http_response *response;

    // client addr
    struct sockaddr_in addr;

    // request length
    size_t request_len;

    // real path of requested file
    char real_path[PATH_MAX];
} connection;

// accept client's connection
connection* connection_accept(server *serv);

// process the connection
int connection_handler(server *serv, connection *con);

// close the connection
void connection_close(connection *con);