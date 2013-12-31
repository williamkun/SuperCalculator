#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <strings.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <errno.h>
#include <netdb.h>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFFER_SIZE 256
#define MAX_NFDS 800

void handler(int num)
{
    exit(0);
}

/* setup signal handler for SIGINT and SIGTERM */
void setup_sigaction(void)
{
    struct sigaction sa;

    sigemptyset(&sa.sa_mask);
    sa.sa_handler = handler;
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

/**
 * create a new socket and bind to host:serv, finally listen()
 * this function also set the SO_REUSEADDR socket option
 *
 * len: the length of address is returned
 * via this parameter after the call (if len is not NULL)
 *
 * On success, a file descriptor for the new socket is returned
 * On error, -1 is returned
 */
int tcp_listen(const char *host, const char *serv, socklen_t *len)
{
    struct addrinfo *res, *saved, hints;
    int n, listenfd;
    const int on = 1;

    bzero(&hints, sizeof(hints));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    n = getaddrinfo(host, serv, &hints, &res);
    if(n != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(n));
        return -1;
    }
    saved = res;
    while(res)
    {
        listenfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if(listenfd >= 0)
        {
            if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1)
                perror("setsockopt");
            if(bind(listenfd, res->ai_addr, res->ai_addrlen) == 0)
            {
                if(listen(listenfd, 128) == 0)
                    break;
            }
            close(listenfd);
        }
        res = res->ai_next;
    }
    if(res == NULL)
    {
        perror("tcp_listen");
        freeaddrinfo(saved);
        return -1;
    }
    else
    {
        if(len)
            *len = res->ai_addrlen;
        freeaddrinfo(saved);
        return listenfd;
    }
}

/* this function is similar to tcp_listen() */
int udp_server(const char *host, const char *serv)
{
    int n, sockfd;
    struct addrinfo hints, *res, *saved;

    bzero(&hints, sizeof(hints));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    n = getaddrinfo(host, serv, &hints, &res);
    if(n != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(n));
        return -1;
    }
    saved = res;
    while(res)
    {
        sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if(sockfd >= 0)
        {
            if(bind(sockfd, res->ai_addr, res->ai_addrlen) == 0)
                break;
        }
        res = res->ai_next;
    }
    if(res == NULL)
    {
        perror("udp_server");
        sockfd = -1;
    }
    freeaddrinfo(saved);
    return sockfd;
}

void print_address(struct sockaddr *addr, socklen_t len)
{
    int n;
    char ip[NI_MAXHOST], port[NI_MAXSERV];

    n = getnameinfo(addr, len, ip, sizeof(ip), port, sizeof(port),
            NI_NUMERICHOST | NI_NUMERICSERV);
    if(n != 0)
    {
        fprintf(stderr, "getnameinfo: %s\n", gai_strerror(n));
        return;
    }
    printf("Client [%s]:%s\n", ip, port);
}

void set_nonblocking(int sockfd)
{
    int flags = fcntl(sockfd, F_GETFL);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

nfds_t defrag_fds(struct pollfd *fds, nfds_t nfds)
{
    nfds_t i, j;

    for(i = 0; i < nfds; ++i)
    {
        if(fds[i].fd == -1)
            break;
    }
    if(i == nfds)
        return nfds;

    for(j = i + 1; j < nfds; ++j)
    {
        if(fds[j].fd != -1)
        {
            fds[i] = fds[j];
            fds[j].fd = -1;
            ++i;
        }
    }
    return i;
}

int main(int argc, char *argv[])
{
    int tcpfd, udpfd;
    ssize_t count;
    char buffer[BUFFER_SIZE];
    socklen_t len;
    int maxfd;
    char *host, *serv;
    struct pollfd fds[MAX_NFDS];
    int max_nfds = MAX_NFDS;
    int nfds;

    host = "::";
    serv = "8080";
    switch(argc)
    {
        case 3:
            serv = argv[2];
        case 2:
            host = argv[1];
        case 1:
            break;
        default:
            fprintf(stderr, "usage: %s <host_name> <service_name>\n", argv[0]);
            exit(1);
    }

    setup_sigaction();

    tcpfd = tcp_listen(host, serv, NULL);
    if(tcpfd < 0)
        exit(1);

    udpfd = udp_server(host, serv);
    if(udpfd < 0)
        exit(1);

    set_nonblocking(tcpfd);
    set_nonblocking(udpfd);

    fds[0].fd = tcpfd;
    fds[0].events = POLLIN;
    fds[1].fd = udpfd;
    fds[1].events = POLLIN;
    nfds = 2;

    while(1)
    {
        int n, i, j;
        struct sockaddr_storage addr;

        n = poll(fds, nfds, -1);
        if(n < 0)
            exit(1);

        if(fds[1].revents & POLLIN)
        {
            --n;
            len = sizeof(addr);
            count = recvfrom(udpfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&addr, &len);
            if(count < 0)
                perror("recvfrom");
            else if(count == 0)
                printf("[warning] received nothing.\n");
            else
            {
                printf("[log] receive %d byte(s).\n", (int)count);
                count = sendto(udpfd, buffer, count, 0, (struct sockaddr *)&addr, len);
                printf("[log] send %d byte(s).\n", (int)count);
            }
            printf("\n");
        }

        if(fds[0].revents & POLLIN)
        {
            int connfd;

            --n;
            while(1)
            {
                len = sizeof(addr);
                connfd = accept(tcpfd, (struct sockaddr *)&addr, &len);
                if(connfd < 0)
                {
                    if(errno == EAGAIN || errno == EWOULDBLOCK)
                        break;
                    if(errno != EINTR)
                        perror("accept");
                }
                else
                {
                    if(nfds >= max_nfds)
                    {
                        close(connfd);
                        fprintf(stderr, "too many connections\n");
                        break;
                    }
                    fds[nfds].fd = connfd;
                    fds[nfds].events = POLLIN;
                    ++nfds;
                    printf("TCP client #%d accpeted.\n", connfd);
                }
            }
        }

        for(i = 2; i < max_nfds && n > 0; ++i)
        {
            if(fds[i].revents & (POLLIN | POLLERR))
            {
                --n;
                count = read(fds[i].fd, buffer, BUFFER_SIZE);
                if(count <= 0)
                {
                    close(fds[i].fd);
                    printf("#%d closed.\n", fds[i].fd);
                    fds[i].fd = -1;
                }
                else
                {
                    printf("read %d byte(s)\n", (int)count);
                    count = write(fds[i].fd, buffer, count);
                    printf("write %d byte(s)\n\n", (int)count);
                }
            }
        }

        nfds = defrag_fds(fds, nfds);
    }
    exit(0);
}
