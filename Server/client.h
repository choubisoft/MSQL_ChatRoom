#ifndef CLIENT_H_INCLUDED
#define CLIENT_H_INCLUDED


#include "server.h"

typedef struct {
    SOCKET sock;
    char nickName[BUF_SIZE];
}Client;

#endif // CLIENT_H_INCLUDED
