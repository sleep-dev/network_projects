#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

#include<arpa/inet.h>
#include<sys/types.h>
#include<sys/socket.h>

#define DATASIZE 65536
#define bool int
#define PROTO1 1
#define PROTO2 2
#define true 1
#define false 0

struct option{
    char ip[INET_ADDRSTRLEN];
    int port;
    int protocol;
};

struct option opt;

void parse_option(char **argv);
void help();
int phase1(int fd);
void protocol1(int fd);
void protocol2(int fd);

int main(int argc, char **argv){
    parse_option(argv);
    int client_fd;
    struct sockaddr_in server_addr;

    client_fd = socket(PF_INET, SOCK_STREAM, 0);
    if( client_fd < -1 ){
        perror("ERROR opening socket");
        exit(-1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(opt.port);
    server_addr.sin_addr.s_addr = inet_addr(opt.ip);

    if(-1 == (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)))){
        perror("ERROR Connection fail");
        exit(-1);
    }

    phase1(client_fd);
    switch(opt.protocol){
        case PROTO1:
            protocol1(client_fd);
            break;
        case PROTO2:
            protocol2(client_fd);
            break;
        default:
            help();
            break;
    }
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
    checksum = ntohs(checksum);
    trans_id = ntohl(trans_id);


    if(op != 0 || proto != 0){
        printf("Server Error!\n");
        exit(-1);
    }

    checksum += (trans_id >> 16) + (trans_id & 0xffff);
    if(checksum !=  0xffff){
        printf("Checksum is different\n");
        exit(-1);
    }

    //response part
    op = 1;
    proto = opt.protocol;
    
    int calc_checksum;
    calc_checksum = (proto << 8) + op;
    calc_checksum += (trans_id >> 16) + (trans_id & 0xffff);
    calc_checksum = (calc_checksum >> 16) + (calc_checksum & 0xffff);
    calc_checksum = ~calc_checksum;

    checksum = calc_checksum;

    checksum = htons(checksum);
    trans_id = htonl(trans_id);
    send(fd, &op, 1, 0);
    send(fd, &proto, 1, 0);
    send(fd, &checksum, 2, 0);
    send(fd, &trans_id, 4, 0);
    
    return ntohl(trans_id);
}

void protocol1(int fd){
    char buf[DATASIZE];
    char data[DATASIZE];
    int len;
    int read_idx, write_idx;
    bool escape, read_end;
    while(1){
        //read input
        len = read(0, buf, DATASIZE/2);
        if(len == 0) return;

        //send data
        read_idx = 0; write_idx = 0;
        while(read_idx < len){
            data[write_idx++] = buf[read_idx++];
            if(data[write_idx-1] == '\\') data[write_idx++] = '\\';
        }
        send(fd, data, write_idx, 0);
        send(fd, "\\0", 2, 0);

        //recv data
        escape = false; read_end = false;
        while(!read_end){
            len = recv(fd, buf, DATASIZE, 0);
            read_idx = 0; write_idx = 0;
            while(read_idx < len && !read_end){
                if(escape && buf[read_idx] == '0') read_end = true;
                else if (escape && buf[read_idx] == '\\') escape = false;
                else if (buf[read_idx] == '\\') escape = true;
                if(!escape && !read_end) data[write_idx++] = buf[read_idx];
                read_idx++;
            }
            write(1, data, write_idx); 
        }
    }
}

void protocol2(int fd){
    char buf[DATASIZE];
    int n_len, len, recv_len;
    while(1){
        len = read(0, buf, DATASIZE/2);
        if(len == 0) return;
        n_len = htonl(len);
        send(fd, &n_len, 4, 0);
        send(fd, buf, len, 0);

        recv(fd, &n_len, 4, 0);
        len = ntohl(n_len);
        recv_len = 0;
        while(recv_len < len){
            recv_len += recv(fd, buf, len-recv_len, 0);
            write(1, buf, recv_len);      
        }
    }
}

