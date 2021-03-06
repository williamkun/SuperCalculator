#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#define SERV_PORT     "45612"
#define LISTENQ_MAX	12
#define BUFFER_SIZE 256
#define MAX_NFDS 800


int readline(int fd, void *pbuf, int maxlen);

void handler(int num)
{
    exit(0);
}

//设置相关信号处理
void setup_sigaction(void)
{
    struct sigaction sa;//信号处理结构

    sigemptyset(&sa.sa_mask);//清空此信号集
    sa.sa_handler = handler;//设置其信号处理函数为handler
    sa.sa_flags = 0;//标志位置零

    //sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);
    //依照signum指定的信号设置act中设置的信号处理函数

    //SIGINT:由Interrupt Key产生，通常是CTRL+C或者DELETE。发送给所有ForeGround Group的进程
    sigaction(SIGINT, &sa, NULL);
    //SIGTERM:请求中止进程，kill命令缺省发送
    sigaction(SIGTERM, &sa, NULL);
}

//tcp监听
//成功则返回监听套接字
int tcp_listen(const char *host, const char *serv, socklen_t *len)
{
    //地址结构体
    struct addrinfo *res, *saved, hints;
    //listenfd:监听套接字
    int n, listenfd;
    const int on = 1;

    bzero(&hints, sizeof(hints));
    //ai_flags指定处理地址和名字的方式
    //AI_PASSIVE:套接字用于监听绑定
    hints.ai_flags = AI_PASSIVE;
    //ai_family指定地址族
    //AF_UNSPEC:协议无关
    hints.ai_family = AF_UNSPEC;
    //ai_socktype指定套接字类型
    //SOCK_STREAM:流式套接字
    hints.ai_socktype = SOCK_STREAM;
    //ai_protocol指定协议类型
    //IPPROTO_TCP:tcp协议
    hints.ai_protocol = IPPROTO_TCP;
    //getaddrinfo:将主机名字和服务名字映射到一个地址
    //res:被映射到的地址
    n = getaddrinfo(host, serv, &hints, &res);
    if(n != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(n));
        return -1;
    }
    saved = res;
    while(res)
    {
        //socket()：创建一个监听套接字listenfd
        listenfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if(listenfd >= 0)
        {
            //setsockopt()：设置套接字选项值
            //SOL_SOCKET:选项定义层次
            //SO_REUSEADDR:允许套接口和一个已在使用中的地址捆绑
            //on:存放选项值德缓冲区
            if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1)
                perror("setsockopt");
            //bind():绑定监听套接字
            if(bind(listenfd, res->ai_addr, res->ai_addrlen) == 0)
            {
                //listen():开始监听
                if(listen(listenfd, 128) == 0)
                    break;
            }
            //如果上述步骤有存在不成功
            //close():关闭监听套接字
            close(listenfd);
        }
        res = res->ai_next;
    }
    //创建套接字失败情况
    if(res == NULL)
    {
        perror("tcp_listen");
        //释放内存
        freeaddrinfo(saved);
        return -1;
    }
    else
    {
        //刷新地址长度
        if(len)
            *len = res->ai_addrlen;
        //释放内存
        freeaddrinfo(saved);
        //返回监听套接字
        return listenfd;
    }
}


//设置非阻塞套接字
void set_nonblocking(int sockfd)
{
    //fcntl()：根据操作需要作出相应处理
    //
    //F_GETFL:获取sockfd的选项值
    int flags = fcntl(sockfd, F_GETFL);
    //F_SETFL:设置sockfd为非阻塞套接字
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
    int tcpfd;
    ssize_t count;
    char buffer[BUFFER_SIZE];
    socklen_t len;
    int maxfd;
    char *host, *serv;
    //poll机制
    //fds:待测试的文件描述符和事件
    //struct pollfd fds[MAX_NFDS];
    struct pollfd fds[MAX_NFDS];
    //max_nfds:最大德文件描述符数量
    int max_nfds = MAX_NFDS;
    int nfds;

    host = "::";
    serv = SERV_PORT;
    //serv = "8080";
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

    //设置信号处理函数
    setup_sigaction();

    //创建监听套接字
    tcpfd = tcp_listen(host, serv, NULL);
    if(tcpfd < 0)
        exit(1);

    //设置为非阻塞状态
    set_nonblocking(tcpfd);

    //把tcp监听套接字加入文件poll监听描述符中
    fds[0].fd = tcpfd;
    fds[0].events = POLLIN;
    nfds = 1;

    while(1)
    {
        int n, i, j;
        //addr:记录客户端地址
        struct sockaddr_storage addr;

        //poll():返回已经准备好读、写或出错状态的套接字
        n = poll(fds, nfds, -1);
        //-1:poll()调用失败
        if(n < 0)
            exit(1);
        //如果有tcp监听套接字有数据可读
        if(fds[0].revents & POLLIN)
        {
            int connfd;
            --n;
            while(1)
            {
                len = sizeof(addr);
                //accept():接受连接
                connfd = accept(tcpfd, (struct sockaddr *)&addr, &len);
                //连接失败
                if(connfd < 0)
                {
                    //如果在非阻塞模式下调用了阻塞操作，且该操作没有完成
                    if(errno == EAGAIN || errno == EWOULDBLOCK)
                        break;
                    if(errno != EINTR)
                        perror("accept");
                }
                else
                {
                    //超过poll机制能处理套接字数目
                    if(nfds >= max_nfds)
                    {
                        close(connfd);
                        fprintf(stderr, "too many connections\n");
                        break;
                    }
                    //把数据套接字加入poll文件描述符中并对其可读事件进行监听
                    fds[nfds].fd = connfd;
                    fds[nfds].events = POLLIN;
                    ++nfds;
                    printf("TCP client #%d accpeted.\n", connfd);
                }
            }
        }
        //数据套接字就绪
        for(i = 1; i < max_nfds && n > 0; ++i)
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
                    printf("content: %s\n",buffer);

                    //获取系统当前时间
                    time_t now;
                    struct tm *timenow;
                    time(&now);
                    timenow = localtime(&now);
                    count = write(fds[i].fd, asctime(timenow), strlen(asctime(timenow)) + 1);
                    
                    //count = write(fds[i].fd, buffer, count);
                    printf("write %d byte(s)\n\n", (int)count);
                }
            }
        }

        //优化fds
        nfds = defrag_fds(fds, nfds);
    }
    exit(0);
}


int readline(int fd, void *pbuf, int maxlen)
{
	int n, ret;
	char c, *ptr;

	ptr = pbuf;
	for (n = 0; n < maxlen;n++)
	{
		ret = recv(fd, &c, 1, 0);
		if (ret == 1)
		{
			*ptr++ = c;
			if (c == '\n')
			{
				break;
			}
		}
		else if (ret == 0)
		{
			if (n == 1)
			{
				return 0;//文件尾， 未读到数据
			}
			else
			{
				break;
			}
		}
		else
		{
			if(errno == EINTR);
			return -1;
		}
	}
	*ptr = 0;
	return n;
}
