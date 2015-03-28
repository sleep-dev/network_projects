#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

#include<arpa/inet.h>
#include<sys/types.h>
#include<sys/socket.h>

void client_main(int client_fd);
int phase1(int client_fd);
void phase2(int client_fd, int mode);

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

void client_main(int client_fd){
    printf("%d\n", phase1(client_fd));
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

    checksum += (trans_id >> 16) + (trans_id & 0xffff);
    checksum += (op << 8) + proto;

    if(recv_id != trans_id || checksum^0xffff) return -1;

    return proto;
}

void phase2(int fd, int mode){
}
