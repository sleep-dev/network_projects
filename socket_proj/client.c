#include<stdio.h>
#include<stdlib.h>

#include<sys/types.h>
#include<sys/socket.h>

struct option{
    char ip[16];
    int port;
    int protocol;
};

struct option op;

void parse_option(char **argv);

int main(int argc, char **argv){
    parse_option(argv);
    int client_fd;

    client_fd = socket(PF_INET, SOCK_STREAM, 0);
    if( client_fd == -1 ){
        printf("Fail on make socket\n");
        exit(-1);
    }
}

void parse_option(char **argv){
    argv++;
    while(*argv != NULL){
        printf("%s\n", *argv);
        argv++;
    }
}
