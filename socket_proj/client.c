#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

#include<arpa/inet.h>
#include<sys/types.h>
#include<sys/socket.h>

struct option{
    char ip[INET_ADDRSTRLEN];
    int port;
    int protocol;
};

struct option op;

void parse_option(char **argv);
void help();
void phase1(int fd);

int main(int argc, char **argv){
    parse_option(argv);
    int client_fd;
    struct sockaddr_in server_addr;

    client_fd = socket(PF_INET, SOCK_STREAM, 0);
    if( client_fd == -1 ){
        printf("Fail on make socket\n");
        exit(-1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(op.port);
    server_addr.sin_addr.s_addr = inet_addr(op.ip);

    if(-1 == (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)))){
        printf("Connection Fail\n");
        exit(-1);
    }

    phase1(client_fd);
}

void parse_option(char **argv){
    argv++;
    int option_count = 0;
    while(*argv != NULL){
        if(!strcmp(*argv, "-h")){
            if(!++argv) help();
            struct sockaddr_in sa;
            if(!inet_pton(AF_INET, *argv, &(sa.sin_addr))) help();
            inet_ntop(AF_INET, &(sa.sin_addr), op.ip, INET_ADDRSTRLEN);
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
        else help();
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

void phase1(int fd){
    unsigned char op;
    unsigned char proto;
    unsigned short checksum;
    unsigned int trans_id;

    recv(fd, &op, 1, 0);
    recv(fd, &proto, 1, 0);
    recv(fd, &checksum, 2, 0);
    recv(fd, &trans_id, 4, 0);

    if(op != 0 || proto != 0){
        printf("Server Error!\n");
        exit(-1);
    }

    unsigned int calc_checksum = 0;
    calc_checksum = (trans_id >> 16) + (trans_id & 0xffff);
    calc_checksum = (calc_checksum >> 16) + (calc_checksum & 0xffff);
    calc_checksum ^= 0xffff;
    
    if(calc_checksum != checksum){
        printf("Checksum is different\n");
        exit(-1);
    }
}

