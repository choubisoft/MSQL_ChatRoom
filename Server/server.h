#ifndef SERVER_H_INCLUDED
#define SERVER_H_INCLUDED

/* Charger la winsock2 si c'est windows */
#ifdef WIN32

    #include <winsock2.h>

/* Charger les bibliothéque nécessaire si c'est Linux */
#elif defined (linux)

    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <resolv.h>
    #include <unistd.h> /* close */
    #include <netdb.h> /* gethostbyname */
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket(s) close(s)
    typedef int SOCKET;
    typedef struct sockaddr_in SOCKADDR_IN;
    typedef struct sockaddr SOCKADDR;
    typedef struct in_addr IN_ADDR;

#else

    #error not defined for this platform

#endif

#define CRLF		"\r\n"

#define MAX_CLIENTS 100

#define BUF_SIZE	1024

#include "client.h"

static void init(void);
static void end(void);
static void chat_Room(int PORT);
static int  init_connection(int PORT);
static void end_connection(int sock);
static int  read_client(SOCKET sock, char *Buffer);
static void write_client(SOCKET sock, const char *Buffer);
static void write_log(char *Buffer);
static int  chat_join(Client client, int actual);
static void chat(Client *clients, Client client, int actual, const char *Buffer, int from_server);
static void chat_leave(Client *clients, int to_remove, int *actual);
static void clear_clients(Client *clients, int actual);
static void chat_list(Client client, int actual);

#endif // SERVER_H_INCLUDED
