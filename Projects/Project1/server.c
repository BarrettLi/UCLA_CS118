#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>

#define DEFAULT_PORT 8888
// server struct
typedef struct{
    // socket
    int sockfd;

    // port number
    short port;
} server;

// initialize server struct
static server* server_init(){
    server *serv;
    serv = malloc(sizeof(*serv));
    memset(serv, 0, sizeof(*serv));
    return serv;
}

// free server struct
static void server_free(server *serv){
    if (!serv) return;
    free(serv);
}

// bind port and listen
static void bind_and_listen(server *serv){
    struct sockaddr_in serv_addr;

    // create a socket
    serv->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(serv->sockfd < 0){
        perror("socket");
        exit(1);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(serv->port);

    // bind()
    int yes=1;
    setsockopt(serv->sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    if (bind(serv->sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
        perror("bind");
        exit(1);
    }

    // listen(), waiting for the connections
    if (listen(serv->sockfd, 5) < 0){
        perror("listen");
        exit(1);
    }
}

// start server
static void start_server(server *serv){
    // set port number
    serv->port = DEFAULT_PORT;
    // printf("port: %d\n", serv->port);

    // bind and listen
    bind_and_listen(serv);
}

// connection accept
static int connection_accept(server *serv){
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    // socket accept
    int sockfd = accept(serv->sockfd, (struct sockaddr *) &addr, &addr_len);
    if (sockfd < 0) {
        perror("accept");
        return -1;
    }
    return sockfd;
}

static char* string_append(char *str1, char* str2){
    char *new_str;
    if (strlen(str1) == 0){
        new_str = malloc(strlen(str2)+1);
        new_str[0] = '\0';
        strcat(new_str,str2);
    }
    else if((new_str = malloc(strlen(str1)+strlen(str2)+1)) != NULL){
        new_str[0] = '\0';
        strcat(new_str, str1);
        strcat(new_str, str2);
    }
    return new_str;
}

// extract the filename from the http_request
char * http_request_parser(char *http_request){
    int start=5, end;
    
    for (int i = start;;i++){
        if(http_request[i] == ' '){
            end = i;
            break;
        }
    }

    if(start == end){
        return NULL;
    }

    int name_len = end-start;
    char *filename;
    filename = malloc(name_len + 1);
    memcpy(filename, &http_request[start], name_len);
    filename[name_len] = '\0';
    return filename;
}

char * space_replace(char *fname){
    char *name;
    int index = 0;
    name = malloc(strlen(fname));
    for (int i=0; i <= strlen(fname); i++){
        if(strncmp(&fname[i], "%20", 3) == 0){
            // replace space
            name[index]=' ';
            index++;
            i += 2;
        }
        else{
            name[index] = fname[i];
            index++;
        }
    }
    name[index]='\0';
    return name;
}

char *file_name(char *fname){
    DIR *Dir;
    struct dirent *DirEnt;

    Dir = opendir(".");
    while((DirEnt = readdir(Dir))){
        if(strcasecmp(DirEnt->d_name, fname)==0){
            fname = DirEnt->d_name;
            break;
        }
    }
    return fname;
}

// sent error 404 message
void send_error(int sockfd){
    char *err_message = "HTTP/1.1 404 Not Found\r\n\r\n<html><h1>Error 404: File Not Found!</h1></html>";
    send(sockfd, err_message, strlen(err_message), 0);
    close(sockfd);
    return;
}

// get file type
char * get_file_type(char *fname){
    char * extension;
    int start=strlen(fname);
    int end=start;
    for(int i=0; i <= end; i++){
        if(fname[i] == '.'){
            start = i + 1;
        }
    }

    if (start == end){
        // no extension specified
        // MIME type of binary file:
        // https://stackoverflow.com/questions/6783921/which-mime-type-to-use-for-a-binary-file-thats-specific-to-my-program
        return "Content-Type: application/octet-stream\r\n";
    }

    
    extension = malloc(end-start+1);
    memcpy(extension, &fname[start], end-start);
    extension[end-start] = '\0';

    // printf("file extension is: %s\n", extension);

    if(strcasecmp(extension, "html") == 0|| strcasecmp(extension, "htm")==0){
        return "Content-Type: text/html\r\n";
    }
    else if(strcasecmp(extension, "txt")==0){
        return "Content-Type: text/txt\r\n";
    }
    else if(strcasecmp(extension, "jpg")==0 || strcasecmp(extension, "jpeg")==0){
        return "Content-Type: image/jpeg\r\n";
    }
    else if (strcasecmp(extension, "gif")==0){
        return "Content-Type: image/gif\r\n";
    }
    return NULL;
}

// build and send response message
void build_and_send_response(int sockfd, char *fname, int fsize, char *content){
    char * status_line = "HTTP/1.1 200 OK\r\n";
    char * server = "Server: Yuhai Li\r\n";

    char * content_type = get_file_type(fname);
    char len[10];
    sprintf(len, "%d", fsize);
    char content_length[30] = "Content-Length: ";
    strcat(content_length, len);
    strcat(content_length, "\r\n");
    char *terminate = "\r\n\0";

    // send header
    send(sockfd, status_line, strlen(status_line), 0);
    send(sockfd, server, strlen(server), 0);
    send(sockfd, content_type, strlen(content_type), 0);
    send(sockfd, content_length, strlen(content_length), 0);
    send(sockfd, terminate, strlen(terminate), 0);

    // printf("%s\n", status_line);
    // printf("%s\n", server);
    // printf("%s\n", content_type);
    // printf("%s\n", content_length);

    // send body
    send(sockfd, content, fsize, 0);
}

// build connections and process the request
static void process(server *serv){

    while (1){
        int sockfd = connection_accept(serv);

        char buf[512];
        char *http_request = "\0";
        int nbytes;
        while((nbytes = recv(sockfd, buf, sizeof(buf), 0)) > 0){
            http_request = string_append(http_request, buf);
            // Part A: print http request to console
            printf("%s", http_request);
            if(nbytes < 512){
                // complete reading request
                break;
            }
        }

        // extract and parse the filename
        char *fname;
        fname = http_request_parser(http_request);
        if(!fname){
            send_error(sockfd);
            continue;
        }
        fname = space_replace(fname);

        // read file
        FILE *fp;
        int fsize;

        fname = file_name(fname);
        fp = fopen(fname, "r");

        // no such file
        if (!fp) {
            send_error(sockfd);
            // printf("Invalid filename: %s\n", fname);
            continue;
        }

        // get file size
        fseek(fp, 0, SEEK_END);
        fsize = ftell(fp);
        fseek(fp, 0, SEEK_SET);
    
        // read file content
        char *content;
        content = malloc(fsize+1);
        if (fread(content, fsize, 1, fp) > 0) {
            content[fsize]='\0';
        }
    
        // print file content to console
        // printf("File content:\n");
        // printf("%s", content);
        // printf("\n");
        fclose(fp);

        // build and send response
        build_and_send_response(sockfd, fname, fsize, content);

        close(sockfd);
        free(content);

    }
}

int main(int argc, char** argv){
    // initialize server
    server *server;
    server = server_init();

    // start server
    start_server(server);

    // waiting for connections and process the request
    process(server);

    // free the server
    server_free(server);

    return 0;
}