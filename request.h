#ifndef REQUEST_H
#define REQUEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class Request
{
    public:
        Request(int fd);
        void request_begin();
        void request_start();
        void request_finish();
    private:
        int fd;
        char version;
        char command;
};

#endif
