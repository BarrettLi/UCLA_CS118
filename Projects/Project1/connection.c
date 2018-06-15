#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "connection.h"

connection* connection_accept(server *serv){
    struct sockaddr_in addr;
    connection *con;
    int sockfd;
    socklen_t addr_len = sizeof(addr);

    // accept new connections
    sockfd = accept(serv->sockfd, (struct sockaddr *) &addr, &addr_len);

    if (sockfd < 0){
        perror("accept");
        return NULL;
    }

    con = malloc(sizeof(*con));

    con->sockfd = sockfd;
    con->status_code = 0;
    con->request_len = 0;
    con->real_path[0] = '\0';
}

