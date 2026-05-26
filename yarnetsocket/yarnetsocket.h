#ifndef _YARNET_SOCKET_H
#define _YARNET_SOCKET_H

typedef long long yarnet_socket;
typedef char yarnet_address[128]; /* Enough bytes to store any address type = 128 = sizeof(struct sockaddr_storage) */

int yarnet_socket_initialize();
int yarnet_socket_cleanup();

yarnet_socket yarnet_socket_ipv4();
yarnet_socket yarnet_socket_ipv6();
int yarnet_socket_close(yarnet_socket sockfd);

int yarnet_socket_is_invalid(yarnet_socket sockfd);

int yarnet_socket_bind(yarnet_socket sockfd, yarnet_address address);

int yarnet_socket_send(yarnet_socket sockfd, const char *buffer, unsigned long long length, int flags, const yarnet_address address);
int yarnet_socket_recv(yarnet_socket sockfd, char *buffer, unsigned long long length, int flags, yarnet_address address);

int yarnet_socket_wait(yarnet_socket sockfd, unsigned long long timeout_ms);

int yarnet_address_set_ipv4(yarnet_address address, const char *ip, unsigned short port);
int yarnet_address_set_ipv6(yarnet_address address, const char *ip, unsigned short port);
int yarnet_address_get_ip(yarnet_address address, char *buffer, unsigned long long *length); /* length input is the capacity of buffer and then set to the result length as output */
int yarnet_address_get_port(yarnet_address address, unsigned short *port);
int yarnet_address_copy(yarnet_address dest, yarnet_address src);

#endif /* _YARNET_SOCKET_H */