#include<stdio.h>
#include<stdlib.h>

#include<sys/types.h>
#include<sys/socket.h>

int main(int argc, char **argv){
    int client_socket;
    client_socket = socket(PF_INET, SOCK_STREAM, 0);
    if( client_socket == -1 ){
        printf("Fail on make socket\n");
        exit(-1);
    }

}
