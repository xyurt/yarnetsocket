#ifndef _YARNETSOCKET_H
#define _YARNETSOCKET_H

/*
    _YARNETSOCKET_WIN32 -> Windows
    _YARNETSOCKET_UNIX  -> Unix
*/

#if defined(_WIN32) || defined(_WIN64)
    #define _YARNETSOCKET_WIN32
#elif defined(__unix__) || defined(__unix) || defined(__APPLE__) || defined(__MACH__)
    #define _YARNETSOCKET_UNIX
#else
    #error "Unsupported platform"
#endif

typedef unsigned char  yarnet_uint8;
typedef unsigned short yarnet_uint16;
typedef unsigned long  yarnet_uint32;

typedef unsigned char yarnet_bool;
#define yarnet_false ((yarnet_bool)(0))
#define yarnet_true  ((yarnet_bool)(1))

/* Headers */
#include <stdlib.h>
#include <string.h>

#if defined(_YARNETSOCKET_WIN32) 
    #define _WINSOCK_DEPRECATED_NO_WARNINGS
    #include <winsock2.h>
#elif defined(_YARNETSOCKET_UNIX)
    #include <arpa/inet.h>
#endif /* _YARNETSOCKET_WIN32 && _YARNETSOCKET_UNIX */

#define YARNET_HOST_TO_NET_16 htons
#define YARNET_HOST_TO_NET_32 htonl
#define YARNET_NET_TO_HOST_16 ntohs
#define YARNET_NET_TO_HOST_32 ntohl

#define YARNET_RESOLVER_MAX_ADDRESS 16

/* Type definitions */
#if defined(_YARNETSOCKET_WIN32) 
    typedef SOCKET YarnetSocket;
#elif defined(_YARNETSOCKET_UNIX)
    typedef int YarnetSocket;
#endif /* _YARNETSOCKET_WIN32 && _YARNETSOCKET_UNIX */

/* Macros */
#if defined(_YARNETSOCKET_WIN32) 
    #define SOCKET_IS_INVALID(yarnet_socket) ((yarnet_socket) == INVALID_SOCKET)
    #define YARNET_INVALID_SOCKET INVALID_SOCKET
#elif defined(_YARNETSOCKET_UNIX)
    #define SOCKET_IS_INVALID(yarnet_socket) ((yarnet_socket) < 0)
    #define YARNET_INVALID_SOCKET -1
#endif /* _YARNETSOCKET_WIN32 && _YARNETSOCKET_UNIX */

/* Structs */
typedef struct YarnetAddress {
    yarnet_uint32 host;
    yarnet_uint16 port;
} YarnetAddress;

#if defined(_YARNETSOCKET_WIN32) 
    typedef struct YarnetBuffer {
        size_t data_len;
        void  *data;
    } YarnetBuffer;
#elif defined(_YARNETSOCKET_UNIX)
    typedef struct YarnetBuffer {
        void *data;
        size_t data_len;
    } YarnetBuffer;
#endif /* _YARNETSOCKET_WIN32 && _YARNETSOCKET_UNIX */

typedef struct YarnetAddressList {
    YarnetAddress  list[YARNET_RESOLVER_MAX_ADDRESS];
    size_t         count;
} YarnetAddressList;

/* Enums */
typedef enum YarnetSocketType {
    YARNET_SOCKET_TYPE_DATAGRAM = 1,
    YARNET_SOCKET_TYPE_STREAM   = 2
} YarnetSocketType;

typedef enum YarnetSocketShutdown {
    YARNET_SOCKET_SHUTDOWN_READ        = 1,
    YARNET_SOCKET_SHUTDOWN_WRITE       = 2,
    YARNET_SOCKET_SHUTDOWN_READ_WRITE  = 3
} YarnetSocketShutdown;

typedef enum YarnetSocketOption {
   YARNET_SOCKET_OPTION_NONBLOCK       = 1,
   YARNET_SOCKET_OPTION_BROADCAST      = 2,
   YARNET_SOCKET_OPTION_RCVBUF         = 3,
   YARNET_SOCKET_OPTION_SNDBUF         = 4,
   YARNET_SOCKET_OPTION_REUSEADDR      = 5,
   YARNET_SOCKET_OPTION_RCVTIMEO       = 6,
   YARNET_SOCKET_OPTION_SNDTIMEO       = 7,
   YARNET_SOCKET_OPTION_ERROR          = 8,
   YARNET_SOCKET_OPTION_NODELAY        = 9,
   YARNET_SOCKET_OPTION_TTL            = 10,
   YARNET_SOCKET_OPTION_KEEPALIVE      = 11,
   YARNET_SOCKET_OPTION_DONTROUTE      = 12
} YarnetSocketOption;

typedef enum YarnetSocketWait {
    YARNET_SOCKET_WAIT_NONE       = 0,
    YARNET_SOCKET_WAIT_READ       = 1,
    YARNET_SOCKET_WAIT_WRITE      = 2,
    YARNET_SOCKET_WAIT_READ_WRITE = 3
} YarnetSocketWait;

/* Buffer */
void         yarnet_buffer_set(YarnetBuffer *buffer, char *data, size_t data_len);

/* Address */
yarnet_bool  yarnet_address_set(YarnetAddress *address, const char *ip, yarnet_uint16 port);
yarnet_bool  yarnet_address_get(const YarnetAddress *address, char *out_ip, size_t ip_len, size_t *out_port);
yarnet_bool  yarnet_address_resolve(YarnetAddressList *out_list, const char *hostname, yarnet_uint16 port);

/* Socket */
yarnet_bool  yarnet_socket_initialize(void);
void         yarnet_socket_deinitialize(void);

YarnetSocket yarnet_socket_create(YarnetSocketType type);
void         yarnet_socket_close(YarnetSocket sockfd);
yarnet_bool  yarnet_socket_shutdown(YarnetSocket sockfd, YarnetSocketShutdown how);

yarnet_bool  yarnet_socket_get_remote_address(YarnetSocket sockfd, YarnetAddress *out_address);
yarnet_bool  yarnet_socket_get_local_address(YarnetSocket sockfd, YarnetAddress *out_address);

yarnet_bool  yarnet_socket_bind(YarnetSocket sockfd, const YarnetAddress *address);

yarnet_bool  yarnet_socket_receive(YarnetSocket sockfd, YarnetAddress *out_address, YarnetBuffer *out_buffers, size_t out_buffer_count, size_t *out_receive_length);
yarnet_bool  yarnet_socket_send(YarnetSocket sockfd, const YarnetAddress *address, const YarnetBuffer *buffers, size_t buffer_count, size_t *out_sent_length);

yarnet_bool  yarnet_socket_get_option(YarnetSocket sockfd, YarnetSocketOption option, int *out_value);
yarnet_bool  yarnet_socket_set_option(YarnetSocket sockfd, YarnetSocketOption option, int value);

yarnet_bool  yarnet_socket_listen(YarnetSocket sockfd, int backlog);
YarnetSocket yarnet_socket_accept(YarnetSocket sockfd, YarnetAddress *out_address);
yarnet_bool  yarnet_socket_connect(YarnetSocket sockfd, const YarnetAddress *address);

yarnet_bool  yarnet_socket_wait(YarnetSocket sockfd, YarnetSocketWait how, YarnetSocketWait *out_how, int timeout_ms);


#endif /* _YARNETSOCKET_H */
