#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>
#include "request.h"

using namespace std;

#define HELLO_WORLD_SERVER_PORT    6666 
#define BUFFER_SIZE 1024
#define FILE_NAME_MAX_SIZE 512

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Usage: ./%s ServerIPAddress\n",argv[0]);
        exit(1);
    }

    struct sockaddr_in client_addr;


    bzero(&client_addr,sizeof(client_addr)); 
    client_addr.sin_family = AF_INET; 
    client_addr.sin_addr.s_addr = htons(INADDR_ANY);
    client_addr.sin_port = htons(0); 

    int client_socket = socket(AF_INET,SOCK_STREAM,0);
    if( client_socket < 0)
    {
        fprintf(stderr, "Create socket failed.\n");
        exit(1);
    }


    if( bind(client_socket,(struct sockaddr*)&client_addr,sizeof(client_addr)))
    {
        fprintf(stderr, "Client bind port failed!\n"); 
        exit(1);
    }

    struct sockaddr_in server_addr;
    bzero(&server_addr,sizeof(server_addr));
    server_addr.sin_family = AF_INET;

    if(inet_aton(argv[1],&server_addr.sin_addr) == 0) 
    {
        fprintf(stderr, "Server ip address error!\n");
        exit(1);
    }

    server_addr.sin_port = htons(HELLO_WORLD_SERVER_PORT);
    socklen_t server_addr_length = sizeof(server_addr);

    if(connect(client_socket,(struct sockaddr*)&server_addr, server_addr_length) < 0)
    {
        fprintf(stderr, "Can Not Connect To %s!\n",argv[1]);
        exit(1);
    }

    //发送请求
    Request req(client_socket);
    req.request_begin();
    cout << "begin over" << endl;
    req.request_start();
    cout << "start over" << endl;
    req.request_finish();
    cout << "finish over" << endl;
    return 0;
}
