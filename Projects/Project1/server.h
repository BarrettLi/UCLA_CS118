#include <stdio.h>
#include <stdlib.h>
// server struct
typedef struct{
    // socket
    int sockfd;

    // port number
    short port;
} server;