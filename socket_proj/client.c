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

struct option opt;

void parse_option(char **argv);
void help();
int phase1(int fd);
void phase2(int fd);

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
    server_addr.sin_port = htons(opt.port);
    server_addr.sin_addr.s_addr = inet_addr(opt.ip);

    if(-1 == (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)))){
        printf("Connection Fail\n");
        exit(-1);
    }

    phase1(client_fd);
    while(1)    phase2(client_fd);
}

void parse_option(char **argv){
    argv++;
    int option_count = 0;
    while(*argv != NULL){
        if(!strcmp(*argv, "-h")){
            if(!++argv) help();
            struct sockaddr_in sa;
            if(!inet_pton(AF_INET, *argv, &(sa.sin_addr))) help();
            inet_ntop(AF_INET, &(sa.sin_addr), opt.ip, INET_ADDRSTRLEN);
        }
        else if(!strcmp(*argv, "-p")){
            if(!++argv) help();
            opt.port = atoi(*argv);
            if(!opt.port) help();
        }
        else if(!strcmp(*argv, "-m")){
            if(!++argv) help();
            opt.protocol = atoi(*argv);
            if(!opt.protocol) help();
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

int phase1(int fd){
    //recv part
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
    printf("%x\n", calc_checksum);
    
    if(calc_checksum != checksum){
        printf("Checksum is different\n");
        exit(-1);
    }

    //response part

    op = 1;
    proto = opt.protocol;
    
    calc_checksum = (proto << 8) + op;
    calc_checksum += (trans_id >> 16) + (trans_id & 0xffff);
    calc_checksum = (calc_checksum >> 16) + (calc_checksum & 0xffff);
    calc_checksum ^= 0xffff;

    checksum = calc_checksum;

    send(fd, &op, 1, 0);
    send(fd, &proto, 1, 0);
    send(fd, &checksum, 2, 0);
    send(fd, &trans_id, 4, 0);
    
    return ntohl(trans_id);
}

void phase2(int fd){
    char buf[1040];
    int len;
    printf("Input(max 1024 // EOF - ctrl-D) : ");
    fflush(stdout);

    len = read(1, buf, 1024);
    printf("\n");
    fflush(stdout);

    if(opt.protocol == 1){
        //protocol 1 send part
        buf[len] = '\\';
        buf[len+1] = '0';
        send(fd, buf, len+2, 0);

        //protocol 1 recv part

        len = recv(fd, buf, 1040, 0);
        buf[len] = 0;
    }
    else{
        char test[10];
        int length = 10;
        length = htonl(length);
        send(fd, &length, 4, 0);
        send(fd, "aaaabbbbaa", 10, 0);
        recv(fd, &length, 4, 0);
        length = ntohl(length);
        recv(fd, test, length, 0);
    }
    printf("server data : %s\n", buf); 
}
