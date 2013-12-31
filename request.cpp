#include "request.h"
#include "helper.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <string.h>
#include <vector>

using namespace std;

Request::Request(int fd)
{
    this->fd = fd;
    version = 0x01;
    command = 0x00;
}

void Request::request_begin()
{
    command = 0x01;
    version = 0x01;
    char req_buf[2];
    req_buf[0] = version;
    req_buf[1] = command;
    int ret = 0;
    //begin and send my command
    ret = write_wrapper(fd, req_buf, 2);
    if(0 != ret)
    {
        shutdown_wrapper(fd);
        return ;
    }
    //read the return code
    ret = read_wrapper(fd, req_buf, 2);
    if(req_buf[1] == 0x00)
    {
        return ;
    }
    else
    {
        //Error dealing
        fprintf(stderr, "Error in request_begin()\n");
        shutdown_wrapper(fd);
        exit(1);
    }
}

void Request::request_start()
{
    command = 0x02;
    version = 0x01;
    vector< string >questions;
    vector< string >results;
    uint32_t n;
    int ret = 0;
    char buf[2];
    buf[0] = version;
    buf[1] = command;
    //输入N条计算表达式子


    //测试用例
    n = 2;
    string tmp_str;
    tmp_str = "1+2+3+4+5";
    questions.push_back(tmp_str);
    tmp_str = "12-2+4+345/326/2+1+(123+34)";
    questions.push_back(tmp_str);
    

    //发送操作代码
    cout << "start" << endl;
    ret = write_wrapper(fd, buf, 2);
    if( 0 != ret )
    {
        cout << "shutdown" << endl;
        shutdown_wrapper(fd);
        exit(1);
    }
    cout << "finish command & version" << endl;
    //发送表达式数量N
    n = hton32(n);
    ret = write_wrapper(fd, &n, 4);
    if( 0 != ret)
    {
        cout << "shutdown" << endl;
        shutdown_wrapper(fd);
        exit(1);
    }
    cout << "finish N transmit" << endl;
    //发送每一条表达式
    for(int i = 0; i < questions.size(); i++)
    {
        ret = write_wrapper(fd, questions[i].c_str(), questions[i].length() + 1);
        if( 0 != ret )
        {
            cout << "shutdown" << endl;
            shutdown_wrapper(fd);
            exit(1);
        }
    }
    cout << "finish N questions" << endl;
    //接收N条答案
    for(int i = 0; i < questions.size(); i++)
    {
        char *tmp = read_string(fd);
        string tmp_str(tmp);
        results.push_back(tmp_str);
    }
    cout << "finish N results" << endl;
    //输出显示
    for(int i = 0; i < results.size(); i++)
    {
        cout << "第" << i <<"条答案: "<< results[i] << endl;
    }
    //接收服务端返回操作代码
    ret = read_wrapper(fd, buf, 2);
    if( 0 != ret )
    {
        shutdown_wrapper(fd);
        exit(1);
    }
    else
    {
        if(buf[1] != 0x00)
        {
            shutdown_wrapper(fd);
            exit(1);
        }
    }

}

void Request::request_finish()
{
    command = 0x03;
    version - 0x01;
    char buf[2];
    int ret = 0;
    buf[0] = version;
    buf[1] = command;
    ret = write_wrapper(fd, buf, 2);
    if( 0 != ret)
    {
        shutdown_wrapper(fd);
        exit(1);
    }
    ret = read_wrapper(fd, buf, 2);
    if( 0x00 != buf[1])
    {
        shutdown_wrapper(fd);
        exit(1);
    }
    shutdown_wrapper(fd);
}
