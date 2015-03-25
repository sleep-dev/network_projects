#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

#include<arpa/inet.h>
#include<sys/types.h>
#include<sys/socket.h>

int main(int argc, char **argv){
    int server_fd;

    server_fd = socket(PF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        perror("ERROR opening socket");
        exit(-1);
    }
}
