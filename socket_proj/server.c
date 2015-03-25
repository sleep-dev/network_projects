#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

#include<arpa/inet.h>
#include<sys/types.h>
#include<sys/socket.h>

int main(int argc, char **argv){
    int server_fd;
    int portno;

    struct sockaddr_in server_addr, client_addr;

    server_fd = socket(PF_INET, SOCK_STREAM, 0);
    if(server_fd < 0){
        perror("ERROR opening socket");
        exit(-1);
    }

    portno = 12345;

    memset(&server_addr, 0, sizeof(server_addr));   
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(portno);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0){
        perror("ERROR binding socket");
        exit(-1);
    }

}
