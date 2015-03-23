#include<stdio.h>
#include<stdlib.h>

#include<sys/types.h>
#include<sys/socket.h>

int main(int argc, char **argv){
    int client_fd;
    struct sockaddr_in serv_addr;

    client_fd = socket(PF_INET, SOCK_STREAM, 0);
    if( client_fd == -1 ){
        printf("Fail on make socket\n");
        exit(-1);
    }
}

int phase1(int fd){

}
