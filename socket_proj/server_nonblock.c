#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

#include<arpa/inet.h>
#include<sys/types.h>
#include<sys/socket.h>

#define DATASIZE 65536
#define CLIENT_NUM 1024
#define PROTO1 1
#define PROTO2 2
#define bool int
#define true 1
#define false 0

struct client{
    int phase;
    int trans_id;
    char write_char;
    bool escape;
};

int phase1_send(int client_fd);
int phase1_recv(int client_fd, int trans_id);
int parse_option(char **argv);
void help();
int protocol1(int client_fd, struct client * cli);
int protocol2(int client_fd);


int main(int argc, char **argv){
    int server_fd, max_fd = 0, client_fd, fd_num;
    int portno;
    int addr_len;
    int i;

    struct client cli[CLIENT_NUM];

    struct sockaddr_in server_addr, client_addr;
    fd_set  readfds, allfds;

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

    FD_ZERO(&readfds);
    FD_SET(server_fd, &readfds);
    max_fd = server_fd;

    while(1){
        allfds = readfds;
        printf("Select Wait %d\n", max_fd);
        fd_num = select(max_fd+1, &allfds, (fd_set *)0, (fd_set *)0, NULL);
        if(FD_ISSET(server_fd, &allfds)){
            //ACCEPT the Client
            addr_len = sizeof(client_addr);
            client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
            FD_SET(client_fd, &readfds);

            if(client_fd > max_fd) max_fd = client_fd;
            printf("Accept OK\n");

            memset(&cli[client_fd], 0, sizeof(struct client));
            cli[client_fd].trans_id = phase1_send(client_fd);
            cli[client_fd].phase = 1;

            continue;
        }

        for(i = 3; i <= max_fd; i++){
            client_fd = i;
            if(FD_ISSET(client_fd, &allfds)){
                if(!cli[client_fd].phase){
                    printf("CONNECTION ERROR");
                    exit(-1);
                }
                switch(cli[client_fd].phase){
                    case 1:
                        printf("%d client phase1\n", client_fd);
                        if((cli[client_fd].phase = phase1_recv(client_fd, cli[client_fd].trans_id)) == -1){
                            printf("client error, close the socket\n");
                            close(client_fd);
                            break;
                        }
                        break;
                    case 210:
                    case 211:
                        printf("%d client phase2 protocol1\n", client_fd);
                        if((cli[client_fd].phase = protocol1(client_fd, &cli[client_fd])) == -1){
                            printf("client end\n");
                            close(client_fd);
                        }
                        break;
                    default:
                        break;
                }
            }
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


int phase1_send(int fd){
    unsigned char op = 0, proto = 0;
    unsigned short checksum;
    unsigned int trans_id = rand();

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
    return trans_id; 
}

int phase1_recv(int fd, int trans_id){
    unsigned char op = 0, proto = 0;
    unsigned short checksum;
    unsigned int recv_id;

    recv(fd, &op, 1, 0);
    recv(fd, &proto, 1, 0);
    recv(fd, &checksum, 2, 0);
    recv(fd, &recv_id, 4, 0);
    recv_id = ntohl(recv_id);

    checksum = ntohs(checksum);
    checksum += (trans_id >> 16) + (trans_id & 0xffff);
    checksum += (op << 8) + proto;

    if(recv_id != trans_id || checksum^0xffff) return -1;

    return 200 + proto*10;
}


int protocol1(int fd, struct client *cli){
    char buf[DATASIZE];
    char write_char;
    int read_idx, write_idx;
    int recv_len;
    bool read_end, escape, first_check;

    read_end = false; 
    escape = cli->escape;
    write_char = cli->write_char;
    if(cli->phase == 210) first_check = true;
    else first_check = false;

    read_idx = 0; write_idx = 0;
    recv_len = recv(fd, buf, DATASIZE, 0);

    if(!recv_len) return -1;
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

    send(fd, buf, write_idx, 0);

    if(read_end){
        send(fd, "\\0", 2, 0);
        return 210;
    }
    cli->write_char = write_char;
    cli->escape = escape;
    return 211;
}


int protocol2(int fd){
    char *buf;
    int read_idx, write_idx;
    int len, recv_len, send_len;
    int check;
    char write_char;
    bool first_check = true;

    int total_len = 0;

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
    


