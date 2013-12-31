#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include "helper.h"
#include "ffbuffer.h"
#include <libqalculate/qalculate.h>

#define FF_LITTLE_ENDIAN 0
#define FF_BIG_ENDIAN 1

const char *CFG_PATH = "/etc/ffbackup/client.cfg";

void dump_data(void *data, size_t size)
{
    unsigned char *ptr = (unsigned char *)data;
    size_t i;
    for(i = 0; i < size; ++i)
        fprintf(stderr, "%02X ", (int)ptr[i]);
}

int get_byte_order()
{
    uint16_t k = 0x0102;
    unsigned char *ptr = (unsigned char *)&k;
    if(ptr[0] == 0x02)
        return FF_LITTLE_ENDIAN;
    else
        return FF_BIG_ENDIAN;
}

uint16_t ntoh16(uint16_t net)
{
    return ntohs(net);
}

uint16_t hton16(uint16_t host)
{
    return htons(host);
}

uint32_t ntoh32(uint32_t net)
{
    return ntohl(net);
}

uint32_t hton32(uint32_t host)
{
    return htonl(host);
}

uint64_t ntoh64(uint64_t net)
{
    uint64_t u = net;
    if(get_byte_order() == FF_LITTLE_ENDIAN)
    {
        uint8_t *ptr_net = (uint8_t *)&net;
        uint8_t *ptr_u = (uint8_t *)&u;
        int i, j;
        for(i = 0, j = 7; i < 8; ++i, --j)
            ptr_u[i] = ptr_net[j];
    }
    return u;
}

uint64_t hton64(uint64_t host)
{
    uint64_t u = host;
    if(get_byte_order() == FF_LITTLE_ENDIAN)
    {
        uint8_t *ptr_host = (uint8_t *)&host;
        uint8_t *ptr_u = (uint8_t *)&u;
        int i, j;
        for(i = 0, j = 7; i < 8; ++i, --j)
            ptr_u[i] = ptr_host[j];
    }
    return u;
}

char *read_string(int fd)
{
    ffbuffer store;
    char buf[1];
    int ret;
    size_t ffbuffer_length = 0;
    char *pass;
    while(1)
    {
        ret = read(fd, buf, 1);
        if(ret == -1)
        {
            return NULL;
        }
        store.push_back(buf,1);
        if(!buf[0])
            break;
    }
    ffbuffer_length = store.get_size();
    pass = (char *)malloc(ffbuffer_length);
    if(!pass)
    {
        fputs("Malloc error.\n",stderr);
        return NULL;
    }
    store.get(pass, 0, ffbuffer_length);
    return pass;
}

int read_wrapper(int fd, void *buffer, int num)
{
    int ret = 0;
    int pos = 0;
    char *ptr = (char *)buffer;
    while(pos < num)
    {
        ret = read(fd, ptr + pos, num - pos);
        if(ret == -1)
        {
            //Error dealing
            fprintf(stderr, "Read error in read_wrapper()\n");
            return -1;
        }
        pos += ret;
    }
    return 0;
}

int write_wrapper(int fd, const void *buffer, int num)
{
    int ret;
    ret = write(fd, buffer, num);
    fprintf(stderr, "write count: %d\n",ret);
    if(ret == -1)
    {
        //Error dealing
        fprintf(stderr, "Write error in write_wrapper()\n");
        return -1;
    }
    return 0;
}

void shutdown_wrapper(int fd)
{
        int ret;
        ret = shutdown(fd, SHUT_RDWR);
        if(0 == ret)
            return ;
        if(errno == EBADF)
            fprintf(stderr, "Socket is not a valid file descriptor\n");
        else if(errno == ENOTSOCK)
            fprintf(stderr, "Socket is not a socket.\n");
        else if(errno == ENOTCONN)
            fprintf(stderr, "Socket is not connected.\n");
        else
            fprintf(stderr, "Shutdown error.\n");
}

int32_t arrto32(ffbuffer *in)
{
    int32_t ret;
    in->get(&ret, 0, 4);
    return ntoh32(ret);
}

void calculate(ffbuffer *in, ffbuffer *out)
{
    int32_t num_expr;
    int32_t buf;
    in->get(&buf, 6, 4);
    num_expr = ntoh32(buf);
    in->pop_front(10);
    new Calculator();
    CALCULATOR->loadGlobalDefinitions();
    CALCULATOR->loadLocalDefinitions();
    for(int i = 0; i < num_expr; i++)
    {   
        size_t index = 0;
        bool flag;
        index = in->find('\0', &flag);
        char *expression = new char[index + 1];
        fprintf(stderr, "found=%d\n", (int)flag);
        if(flag)
        {
            in->get(expression, 0, index + 1);
            fprintf(stderr, "expression %s\n", expression);
            EvaluationOptions eo; 
            MathStructure result = CALCULATOR->calculate(expression, eo);
            PrintOptions po; 
            result.format(po);
            string result_str = result.print(po);
            out->push_back(result_str.c_str(), result_str.length() + 1);
            fprintf(stderr, "Result:%s\n",result_str.c_str());
        }
        delete[] expression;
        in->pop_front(index + 1);
    }   

}
