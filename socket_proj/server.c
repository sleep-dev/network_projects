#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

#include<arpa/inet.h>
#include<sys/types.h>
#include<sys/socket.h>

#define DATASIZE 65536
#define PROTO1 1
#define PROTO2 2
#define bool int
#define true 1
#define false 0

void client_main(int client_fd);
int phase1(int client_fd);
int parse_option(char **argv);
void help();
void protocol1(int client_fd);
void protocol2(int client_fd);

int main(int argc, char **argv){
    int server_fd;
    int portno;

    struct sockaddr_in server_addr, client_addr;

    portno = parse_option(argv);

    server_fd = socket(PF_INET, SOCK_STREAM, 0);
    if(server_fd < 0){
        perror("ERROR opening socket");
        exit(-1);
    }


    memset(&server_addr, 0, sizeof(server_addr));   
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(portno);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0){
        perror("ERROR binding socket");
        exit(-1);
    }
    if(listen(server_fd, 10) < 0){
        perror("ERROR Listening socket");
        exit(-1);
    }

    while(1){
        int len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &len);
        int pid;

        if((pid = fork()) == -1){
            close(client_fd);
            continue;
        }
        else if(pid > 0){
            close(client_fd);
            continue;
        }
        else if(pid == 0){
            client_main(client_fd);
            close(client_fd);
            break;
        }
    }
}

int parse_option(char **argv){
    argv++;
    int option_count = 0;
    int port = 0;
    while(*argv != NULL){
        if(!strcmp(*argv, "-p")){
            if(!++argv) help();
            port = atoi(*argv);
            if(!port) help();
        }
        else help();
        argv++;
        option_count++;
    }
    if(option_count != 1) help();
    
    return port;
}

void help(){
    printf("help-function\n");
    exit(-1);
}

void client_main(int client_fd){
    int protocol = phase1(client_fd);
    switch(protocol){
        case PROTO1:
            protocol1(client_fd);
            break;
        case PROTO2:
            protocol2(client_fd);
            break;
        default:
            break;
    }
    printf("client end\n");
}

int phase1(int fd){
    unsigned char op = 0, proto = 0;
    unsigned short checksum;
    unsigned int calc_checksum, recv_id, trans_id = rand();

    checksum = (trans_id >> 16) + (trans_id & 0xffff);
    checksum = (checksum >> 16) + (checksum & 0xffff);
    checksum ^= 0xffff;
    checksum = htons(checksum);
    trans_id = htonl(trans_id);

    send(fd, &op, 1, 0);
    send(fd, &proto, 1, 0);
    send(fd, &checksum, 2, 0);
    send(fd, &trans_id, 4, 0);
    trans_id = ntohl(trans_id);

    recv(fd, &op, 1, 0);
    recv(fd, &proto, 1, 0);
    recv(fd, &checksum, 2, 0);
    recv(fd, &recv_id, 4, 0);
    recv_id = ntohl(recv_id);

    checksum = ntohs(checksum);
    checksum += (trans_id >> 16) + (trans_id & 0xffff);
    checksum += (op << 8) + proto;

    if(recv_id != trans_id || checksum^0xffff) return -1;

    return proto;
}

void protocol1(int fd){
    char buf[DATASIZE];
    char write_char;
    int read_idx, write_idx;
    int recv_len;

    bool read_end, escape = false, first_check = true;
    int read_sum = 0;
    int write_sum = 0;
    
    while(1){
        read_end = false; escape = false;
        while(!read_end){
            read_idx = 0; write_idx = 0;
            recv_len = recv(fd, buf, DATASIZE, 0);
            if(!recv_len) return;

            while(read_idx < recv_len && !read_end){
                if (escape && buf[read_idx] == '0') read_end = true;
                else if (escape && buf[read_idx] == '\\') escape = false;
                else if (buf[read_idx] == '\\') escape = true;
                
                if(!escape && !read_end && (write_char != buf[read_idx] || first_check)){
                    write_char = buf[read_idx];
                    buf[write_idx++] = write_char;
                    if(write_char == '\\') buf[write_idx++] = '\\';
                    first_check = false;
                }
                read_idx++;
            }
            read_sum += recv_len;
            write_sum += write_idx;

            send(fd, buf, write_idx, 0);
        }
        send(fd, "\\0", 2, 0);
    }
                    
}


void protocol2(int fd){
    char *buf;
    int read_idx, write_idx;
    int len, recv_len, send_len;
    int check;
    char write_char;
    bool first_check = true;

    int total_len = 0;

    while(1){
        if(!recv(fd, &recv_len, 4, 0)) return;
        recv_len = ntohl(recv_len);
        buf = malloc(recv_len);

        read_idx = 0; write_idx = 0; len = 0;

        while(len != recv_len){
            check = recv(fd, buf + len, recv_len - len, 0);
            if(!check) return;
            len += check;
        }

        for(; read_idx < recv_len; read_idx++){
            if(write_char != buf[read_idx] || first_check){
                write_char = buf[read_idx];
                buf[write_idx++] = write_char;
                first_check = false;
            }
        }

        send_len = write_idx;
        len = htonl(send_len);

        send(fd, &len, 4, 0);
        send(fd, buf, send_len, 0); 
        free(buf);
    }
}
    

