#include "Response.h"
#include "helper.h"

#include <libqalculate/qalculate.h>

#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>

using namespace std;

Response::Response(int fd)
{
    this->fd = fd;
    error_code = 0x00;
    version = 0x01;
    command = 0x00;
}

void Response::response_request()
{
    error_code = 0x00;
    version = 0x01;
    char req_buf[2];
    int ret = 0;
    ret = read_wrapper(fd, req_buf, 2);
    if(0 != ret)
    {
        shutdown_wrapper(fd);
        return ;
    }
    if(req_buf[1] == 0x01)
    {
        fprintf(stderr, "Begin to send in response_request()\n");
        req_buf[0] = version;
        req_buf[1] = error_code;
        ret = write_wrapper(fd, req_buf, 2);
        if(0 != ret)
        {
            shutdown_wrapper(fd);
            return ;
        }
    }
    else
    {
        //Error dealing
        fprintf(stderr, "Error command in response_request()\n");
        shutdown_wrapper(fd);
    }
}

void Response::response_start()
{
    uint32_t num_expr;
    uint32_t i = 0;
    char buf[2];
    char *expression;
    int ret = read_wrapper(fd, buf, 2);
    vector< string > res_vec;
    if( 0 != ret)
    {
        shutdown_wrapper(fd);
        return ;
    }
    if(buf[1] == 0x02)
    {
        fprintf(stderr, "read Num in response_start()\n");
        ret = read_wrapper(fd, &num_expr, 4);
        num_expr = ntoh32(num_expr);
        fprintf(stderr, "Num is %d\n",num_expr);
    }
    else
    {
        fprintf(stderr, "Error command in response_start()\n");
        shutdown_wrapper(fd);
        return ;
    }
    new Calculator();
    CALCULATOR->loadGlobalDefinitions();
    CALCULATOR->loadLocalDefinitions();
    for( i = 0; i < num_expr; i++)
    {
        expression = read_string(fd);
        if(!expression)
        {
            shutdown_wrapper(fd);
            return ;
        }
        EvaluationOptions eo;
        MathStructure result = CALCULATOR->calculate(expression, eo);
        PrintOptions po;
        result.format(po);
        string result_str = result.print(po); 
        fprintf(stderr, "Result:%s\n",result_str.c_str());
        res_vec.push_back(result_str);
        free(expression);
    }
    for(int i = 0; i < res_vec.size(); i++)
    {
        ret = write_wrapper(fd, res_vec[i].c_str(), res_vec[i].length() + 1);
        if( 0 != ret )
        {
            shutdown_wrapper(fd);
            return ;
        }
    }
    version = 0x01;
    error_code = 0x00;
    buf[0] = version;
    buf[1] = error_code;
    write_wrapper(fd, buf, 2);
}

void Response::response_finish()
{
    char buf[2];
    int ret = read_wrapper(fd, buf, 2);
    if(0 != ret)
    {
        shutdown_wrapper(fd);
        return ;
    }
    if(buf[1] == 0x03)
    {
        version = 0x01;
        error_code = 0x00;
        buf[0] = version;
        buf[1] = error_code;
        ret = write_wrapper(fd, buf, 2); 
        if(0 != ret)
        {
            shutdown_wrapper(fd);
            return ;
        }
    }
    else
    {
        //Error dealing
        fprintf(stderr, "Error command in response_finish()\n");
        shutdown_wrapper(fd);
        return;
    }
}
