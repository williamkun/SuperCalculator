#ifndef RESPONSE_H
#define RESPONSE_H

#include <stdio.h>
#include <stdlib.h>

class Response
{
    public:
        Response(int fd);
        void response_request();
        void response_start();
        void response_finish();
    private:
        int fd; 
        char error_code;
        char version;
        char command;
};
#endif
