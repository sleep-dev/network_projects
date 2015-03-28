#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

#include<arpa/inet.h>
#include<sys/types.h>
#include<sys/socket.h>

#define DATASIZE 65536
#define bool int
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
int phase2(int fd);

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
    while(phase2(client_fd));
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

    checksum += (trans_id >> 16) + (trans_id & 0xffff);
    if(checksum ^ 0xffff){
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

    send(fd, &op, 1, 0);
    send(fd, &proto, 1, 0);
    send(fd, &checksum, 2, 0);
    send(fd, &trans_id, 4, 0);
    
    return ntohl(trans_id);
}

int phase2(int fd){
    char buf[DATASIZE];
    char data[DATASIZE];
    int i = 0, len = 0, recv_len = 0, send_len = 0;
    //printf("Input(max 1024 // EOF - ctrl-D) : ");
    fflush(stdout);

    len = read(0, buf, DATASIZE/2 - 1);
    if(len == 0) return 0; 

    //printf("\n");
    fflush(stdout);



    if(opt.protocol == 1){
        //protocol 1 send part
        while(i < len){
            data[send_len++] = buf[i++];
            if(data[send_len-1] == '\\') data[send_len++] = '\\';
        }

        data[send_len] = '\\';
        data[send_len+1] = '0';
        send(fd, data, send_len+2, 0);

        //protocol 1 recv part
        len = 0;
        i = 0;
        bool read_end = false;
        bool slash_check = false;
        while(!read_end){
            recv_len += recv(fd, buf + recv_len, DATASIZE, 0);
            while(i < recv_len && !read_end){
                if(slash_check && buf[i] == '0')read_end = true;
                else if(slash_check && buf[i] == '\\'){
                    slash_check = false;
                    data[len++] = '\\';
                }else if(buf[i] == '\\') slash_check = true;
                else data[len++] = buf[i];
                i++;
            }
        }
    }
    else{
        //protocol 2 send part
        send_len = htonl(len);
        send(fd, &send_len, 4, 0);
        send(fd, buf, len, 0); 

        //protocol2 recv part
        recv(fd, &recv_len, 4, 0);
        recv_len = ntohl(recv_len);
        len = 0;
        while(len != recv_len)
            len += recv(fd, data+len, recv_len - len, 0);
    }
    //printf("server data : "); 
    write(1, data, len);
    return 1;
}
