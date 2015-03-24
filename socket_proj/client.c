#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

#include<sys/types.h>
#include<sys/socket.h>

struct option{
    char ip[16];
    int port;
    int protocol;
};

struct option op;

void parse_option(char **argv);
void help();

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
    int option_count = 0;
    while(*argv != NULL){
        if(!strcmp(*argv, "-h")){
            if(!++argv) help();
            strncpy(op.ip, *argv, 15);

            if(gethostname(op.ip, strlen(op.ip))) help();
        }
        else if(!strcmp(*argv, "-p")){
            if(!++argv) help();
            op.port = atoi(*argv);
            if(!op.port) help();
        }
        else if(!strcmp(*argv, "-m")){
            if(!++argv) help();
            op.protocol = atoi(*argv);
            if(!op.protocol) help();
        }
        else{
        }
        argv++;
        option_count++;
    }
    if(option_count != 3) help();
}

void help(){
    // print the help message and exit
    printf("help-function\n");
    exit(-1);
}
