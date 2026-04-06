#pragma once

#include <string>
#include "protocol.h"
#include "context.h"

class packet
{
public:
    int            fd;
    requestcontext ctx;

    packet()
        : fd(-1)
        , ctx{}
    {}

    packet(int clientfd, const requestcontext& context)
        : fd(clientfd)
        , ctx(context)
    {}
};