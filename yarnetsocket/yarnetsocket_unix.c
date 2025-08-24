#include "yarnetsocket.h"

#ifdef _YARNETSOCKET_UNIX

#include <unistd.h> 
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <netinet/tcp.h>

void         yarnet_buffer_set(YarnetBuffer *buffer, char *data, size_t data_len) {
    buffer->data = data;
    buffer->data_len = data_len;
}

yarnet_bool  yarnet_address_set(YarnetAddress *address, const char *ip, yarnet_uint16 port) {
    yarnet_uint8 octets[4];
    yarnet_uint8 octet_index = 0;

    const char *octet_p = ip;

    while (*octet_p) {
        char *end;
        long val = strtol(octet_p, &end, 10);

        if (val < 0 || val > 255) {
            return yarnet_false;
        }

        if (end == octet_p) {
            return yarnet_false; /* No digits */
        }

        octets[octet_index++] = (yarnet_uint8)val;

        if (*end == '\0') {
            break;
        }

        if (*end != '.' || octet_index >= 4) {
            return yarnet_false;
        }

        octet_p = end + 1; /* End was pointing at '.' so skip it */
    }

    if (octet_index != 4) {
        return yarnet_false;
    }

    address->host = YARNET_HOST_TO_NET_32(
        (octets[0] << 24) |
        (octets[1] << 16) |
        (octets[2] << 8) |
        octets[3]
    );
    address->port = port;

    return yarnet_true;
}
yarnet_bool  yarnet_address_get(const YarnetAddress *address, char *out_ip, size_t ip_len, size_t *out_port) {
    yarnet_uint32 octet_bytes;
    yarnet_uint8  octets[4];
    yarnet_uint8  i;
    yarnet_uint8  pos;
    yarnet_uint8  len;
    char          buf[3];

    octet_bytes = YARNET_NET_TO_HOST_32(address->host);
    octets[0] = (yarnet_uint8)((octet_bytes >> 24) & 0xFF);
    octets[1] = (yarnet_uint8)((octet_bytes >> 16) & 0xFF);
    octets[2] = (yarnet_uint8)((octet_bytes >> 8) & 0xFF);
    octets[3] = (yarnet_uint8)((octet_bytes >> 0) & 0xFF);

    pos = 0;
    for (i = 0; i < 4; i++) {
        len = 0;

        if (octets[i] >= 100) {
            buf[len++] = '0' + (octets[i] / 100);
            buf[len++] = '0' + ((octets[i] / 10) % 10);
            buf[len++] = '0' + (octets[i] % 10);
        }
        else if (octets[i] >= 10) {
            buf[len++] = '0' + (octets[i] / 10);
            buf[len++] = '0' + (octets[i] % 10);
        }
        else {
            buf[len++] = '0' + octets[i];
        }

        if (pos + len + (i < 3 ? 1 : 0) >= ip_len) {
            return yarnet_false;
        }

        memcpy(out_ip + pos, buf, len);
        pos += len;

        if (i < 3) {
            out_ip[pos++] = '.';
        }
    }

    if (ip_len <= pos) {
        return yarnet_false;
    }

    out_ip[pos] = '\0';

    if (out_port != NULL) {
        *out_port = address->port;
    }

    return yarnet_true;
}

yarnet_bool  yarnet_address_resolve(YarnetAddressList *out_list, const char *hostname, yarnet_uint16 port) {
    struct hostent *host_end;

    host_end = gethostbyname(hostname);
    if (host_end == NULL || host_end->h_addrtype != AF_INET) {
        return yarnet_false;
    }

    for (out_list->count = 0; host_end->h_addr_list[out_list->count] != NULL; out_list->count++) {
        if (out_list->count == YARNET_RESOLVER_MAX_ADDRESS) {
            break;
        }

        memcpy(&out_list->list[out_list->count].host, host_end->h_addr_list[out_list->count], sizeof(yarnet_uint32));
        out_list->list[out_list->count].port = port;
    }

    return yarnet_true;
}

yarnet_bool  yarnet_socket_initialize(void) {
    return yarnet_true;
}

void         yarnet_socket_deinitialize(void) {
    return;
}

YarnetSocket yarnet_socket_create(YarnetSocketType type) {
    switch (type) {
        case YARNET_SOCKET_TYPE_STREAM: {
            return socket(AF_INET, SOCK_STREAM, 0);
        }
        case YARNET_SOCKET_TYPE_DATAGRAM: {
            return socket(AF_INET, SOCK_DGRAM, 0);
        }
    }

    return -1;
}
void         yarnet_socket_close(YarnetSocket sockfd) {
    if (SOCKET_IS_INVALID(sockfd)) {
        return;
    }

    close(sockfd);
}
yarnet_bool  yarnet_socket_shutdown(YarnetSocket sockfd, YarnetSocketShutdown how) {
    if (SOCKET_IS_INVALID(sockfd)) {
        return yarnet_false;
    }

    switch (how) {
        case YARNET_SOCKET_SHUTDOWN_READ: {
            return shutdown(sockfd, 0) != -1;
        }
        case YARNET_SOCKET_SHUTDOWN_WRITE: {
            return shutdown(sockfd, 1) != -1;
        }
        case YARNET_SOCKET_SHUTDOWN_READ_WRITE: {
            return shutdown(sockfd, 2) != -1;
        }
    }

    return yarnet_false;
}

yarnet_bool  yarnet_socket_get_local_address(YarnetSocket sockfd, YarnetAddress *out_address) {
    struct sockaddr_in socket_address;
    socklen_t          address_length;

    if (SOCKET_IS_INVALID(sockfd)) {
        return yarnet_false;
    }

    address_length = sizeof(struct sockaddr_in);
    if (getsockname(sockfd, (struct sockaddr *)&socket_address, &address_length) == -1) {
        return yarnet_false;
    }

    out_address->host = socket_address.sin_addr.s_addr;
    out_address->port = YARNET_NET_TO_HOST_16(socket_address.sin_port);

    return yarnet_true;
}
yarnet_bool  yarnet_socket_get_remote_address(YarnetSocket sockfd, YarnetAddress *out_address) {
    struct sockaddr_in socket_address;
    socklen_t          address_length;

    if (SOCKET_IS_INVALID(sockfd)) {
        return yarnet_false;
    }

    address_length = sizeof(struct sockaddr_in);
    if (getpeername(sockfd, (struct sockaddr *)&socket_address, &address_length) == -1) {
        return yarnet_false;
    }

    out_address->host = socket_address.sin_addr.s_addr;
    out_address->port = YARNET_NET_TO_HOST_16(socket_address.sin_port);

    return yarnet_true;
}

yarnet_bool  yarnet_socket_bind(YarnetSocket sockfd, const YarnetAddress *address) {
    struct sockaddr_in socket_address;

    if (SOCKET_IS_INVALID(sockfd)) {
        return yarnet_false;
    }

    memset(&socket_address, 0, sizeof(struct sockaddr_in));

    socket_address.sin_family = AF_INET;
    socket_address.sin_addr.s_addr = address->host;
    socket_address.sin_port = YARNET_HOST_TO_NET_16(address->port);

    if (bind(sockfd, (const struct sockaddr *)&socket_address, sizeof(struct sockaddr_in)) == -1) {
        return yarnet_false;
    }

    return yarnet_true;
}

yarnet_bool  yarnet_socket_receive(YarnetSocket sockfd, YarnetAddress *out_address, YarnetBuffer *out_buffers, size_t out_buffer_count, size_t *out_receive_length) {
    struct sockaddr_in socket_address;
    socklen_t          address_length;
    struct msghdr      msg;
    ssize_t            ret;

    if (SOCKET_IS_INVALID(sockfd)) {
        return yarnet_false;
    }

    address_length = sizeof(struct sockaddr_in);
    memset(&socket_address, 0, address_length);
    memset(&msg, 0, sizeof(struct msghdr));

    if (out_address) {
        msg.msg_name = &socket_address;
        msg.msg_namelen = address_length;
    }

    msg.msg_iov = (struct iovec *)out_buffers;
    msg.msg_iovlen = out_buffer_count;

    ret = recvmsg(sockfd, &msg, 0);
    if (ret < 0) {
        *out_receive_length = 0;

        return yarnet_false;
    }

    *out_receive_length = (size_t)ret;

    if (out_address) {
        out_address->host = socket_address.sin_addr.s_addr;
        out_address->port = YARNET_NET_TO_HOST_16(socket_address.sin_port);
    }

    return yarnet_true;
}
yarnet_bool  yarnet_socket_send(YarnetSocket sockfd, const YarnetAddress *address, const YarnetBuffer *buffers, size_t buffer_count, size_t *out_sent_length) {
    struct sockaddr_in socket_address;
    struct msghdr      msg;
    ssize_t            ret;

    if (SOCKET_IS_INVALID(sockfd)) {
        return yarnet_false;
    }

    memset(&msg, 0, sizeof(msg));

    if (address) {
        memset(&socket_address, 0, sizeof(socket_address));
        socket_address.sin_family = AF_INET;
        socket_address.sin_addr.s_addr = address->host;
        socket_address.sin_port = YARNET_HOST_TO_NET_16(address->port);

        msg.msg_name = &socket_address;
        msg.msg_namelen = sizeof(socket_address);
    }

    msg.msg_iov = (struct iovec *)buffers;
    msg.msg_iovlen = buffer_count;

    ret = sendmsg(sockfd, &msg, 0);
    if (ret < 0) {
        *out_sent_length = 0;

        return yarnet_false;
    }

    *out_sent_length = (size_t)ret;
    return yarnet_true;
}

yarnet_bool  yarnet_socket_get_option(YarnetSocket sockfd, YarnetSocketOption option, int *out_value) {
    struct timeval tv;
    int optval;
    socklen_t optlen;

    if (SOCKET_IS_INVALID(sockfd)) {
        return yarnet_false;
    }

    optlen = sizeof(optval);

    switch (option) {
        case YARNET_SOCKET_OPTION_BROADCAST: {
            if (getsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (void *)&optval, &optlen) == -1) {
                return yarnet_false;
            }

            *out_value = optval;
            return yarnet_true;
        }

        case YARNET_SOCKET_OPTION_RCVBUF: {
            if (getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (void *)&optval, &optlen) == -1) {
                return yarnet_false;
            }
            *out_value = optval;
            return yarnet_true;
        }

        case YARNET_SOCKET_OPTION_SNDBUF: {
            if (getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (void *)&optval, &optlen) == -1) {
                return yarnet_false;
            }
            *out_value = optval;
            return yarnet_true;
        }

        case YARNET_SOCKET_OPTION_REUSEADDR: {
            if (getsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&optval, &optlen) == -1) {
                return yarnet_false;
            }
            *out_value = optval;
            return yarnet_true;
        }

        case YARNET_SOCKET_OPTION_RCVTIMEO: {
            optlen = sizeof(struct timeval);
            if (getsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (void *)&tv, &optlen) == -1) {
                return yarnet_false;
            }
            *out_value = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
            return yarnet_true;
        }

        case YARNET_SOCKET_OPTION_SNDTIMEO: {
            optlen = sizeof(struct timeval);
            if (getsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (void *)&tv, &optlen) == -1) {
                return yarnet_false;
            }
            *out_value = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
            return yarnet_true;
        }

        case YARNET_SOCKET_OPTION_NODELAY: {
            if (getsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (void *)&optval, &optlen) == -1) {
                return yarnet_false;
            }
            *out_value = optval;
            return yarnet_true;
        }

        case YARNET_SOCKET_OPTION_TTL: {
            if (getsockopt(sockfd, IPPROTO_IP, IP_TTL, (void *)&optval, &optlen) == -1) {
                return yarnet_false;
            }
            *out_value = optval;
            return yarnet_true;
        }

        case YARNET_SOCKET_OPTION_KEEPALIVE: {
            if (getsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (void *)&optval, &optlen) == -1) {
                return yarnet_false;
            }
            *out_value = optval;
            return yarnet_true;
        }

        case YARNET_SOCKET_OPTION_DONTROUTE: {
            if (getsockopt(sockfd, SOL_SOCKET, SO_DONTROUTE, (void *)&optval, &optlen) == -1) {
                return yarnet_false;
            }
            *out_value = optval;
            return yarnet_true;
        }
    }

    return yarnet_false;
}
yarnet_bool  yarnet_socket_set_option(YarnetSocket sockfd, YarnetSocketOption option, int value) {
    int            flags;
    struct timeval tv;

    if (SOCKET_IS_INVALID(sockfd)) {
        return yarnet_false;
    }

    switch (option) {
        case YARNET_SOCKET_OPTION_NONBLOCK: {
            flags = fcntl(sockfd, F_GETFL, 0);
            if (flags == -1) {
                return yarnet_false;
            }
            if (value) {
                flags |= O_NONBLOCK;
            }
            else {
                flags &= ~O_NONBLOCK;
            }
            return fcntl(sockfd, F_SETFL, flags) != -1;
        }

        case YARNET_SOCKET_OPTION_BROADCAST: {
            return setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (const char *)&value, sizeof(value)) != -1;
        }

        case YARNET_SOCKET_OPTION_RCVBUF: {
            return setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (const char *)&value, sizeof(value)) != -1;
        }

        case YARNET_SOCKET_OPTION_SNDBUF: {
            return setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (const char *)&value, sizeof(value)) != -1;
        }

        case YARNET_SOCKET_OPTION_REUSEADDR: {
            return setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&value, sizeof(value)) != -1;
        }

        case YARNET_SOCKET_OPTION_RCVTIMEO: {
            tv.tv_sec = value / 1000;
            tv.tv_usec = (value % 1000) * 1000;
            return setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != -1;
        }

        case YARNET_SOCKET_OPTION_SNDTIMEO: {
            tv.tv_sec = value / 1000;
            tv.tv_usec = (value % 1000) * 1000;
            return setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) != -1;
        }

        case YARNET_SOCKET_OPTION_TTL: {
            return setsockopt(sockfd, IPPROTO_IP, IP_TTL, (const char *)&value, sizeof(value)) != -1;
        }

        case YARNET_SOCKET_OPTION_KEEPALIVE: {
            return setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (const char *)&value, sizeof(value)) != -1;
        }

        case YARNET_SOCKET_OPTION_DONTROUTE: {
            return setsockopt(sockfd, SOL_SOCKET, SO_DONTROUTE, (const char *)&value, sizeof(value)) != -1;
        }
        case YARNET_SOCKET_OPTION_NODELAY: {
            return setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (const char *)&value, sizeof(value)) != -1;
        }
    }

    return yarnet_false;
}

yarnet_bool  yarnet_socket_listen(YarnetSocket sockfd, int backlog) {
    if (SOCKET_IS_INVALID(sockfd)) {
        return yarnet_false;
    }

    return listen(sockfd, backlog) != -1;
}
YarnetSocket yarnet_socket_accept(YarnetSocket sockfd, YarnetAddress *out_address) {
    struct sockaddr_in socket_address;
    socklen_t          address_length;
    int                client_socket;

    if (SOCKET_IS_INVALID(sockfd)) {
        return -1;
    }

    address_length = sizeof(struct sockaddr_in);
    client_socket = accept(sockfd, (struct sockaddr *)&socket_address, &address_length);
    if (client_socket == -1) {
        return -1;
    }

    if (out_address) {
        out_address->host = socket_address.sin_addr.s_addr;
        out_address->port = YARNET_NET_TO_HOST_16(socket_address.sin_port);
    }

    return client_socket;
}
yarnet_bool  yarnet_socket_connect(YarnetSocket sockfd, const YarnetAddress *address) {
    struct sockaddr_in socket_address;

    if (SOCKET_IS_INVALID(sockfd)) {
        return yarnet_false;
    }

    socket_address.sin_family = AF_INET;
    socket_address.sin_addr.s_addr = address->host;
    socket_address.sin_port = YARNET_HOST_TO_NET_16(address->port);

    if (connect(sockfd, (const struct sockaddr *)&socket_address, sizeof(struct sockaddr_in)) == -1) {
        return yarnet_false;
    }

    return yarnet_true;
}

yarnet_bool  yarnet_socket_wait(YarnetSocket sockfd, YarnetSocketWait how, YarnetSocketWait *out_how, int timeout_ms) {
    fd_set          sets[2]; /* [0]: readfds, [1]: writefds */
    struct timeval  tv;
    int             result;
    yarnet_bool     read_set;
    yarnet_bool     write_set;

    if (SOCKET_IS_INVALID(sockfd)) {
        return yarnet_false;
    }

    if (timeout_ms >= 0) {
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
    }

    read_set = yarnet_false;
    write_set = yarnet_false;

    if (out_how) {
        *out_how = YARNET_SOCKET_WAIT_NONE;
    }

    switch (how) {
        case YARNET_SOCKET_WAIT_READ: {
            FD_ZERO(&sets[0]);
            FD_SET(sockfd, &sets[0]);
            result = select(sockfd + 1, &sets[0], NULL, NULL, timeout_ms >= 0 ? &tv : NULL);
            if (result <= 0) {
                return yarnet_false;
            }
            read_set = FD_ISSET(sockfd, &sets[0]);
            if (out_how && read_set) {
                *out_how = YARNET_SOCKET_WAIT_READ;
            }
            return read_set;
        }
        case YARNET_SOCKET_WAIT_WRITE: {
            FD_ZERO(&sets[1]);
            FD_SET(sockfd, &sets[1]);
            result = select(sockfd + 1, NULL, &sets[1], NULL, timeout_ms >= 0 ? &tv : NULL);
            if (result <= 0) {
                return yarnet_false;
            }
            write_set = FD_ISSET(sockfd, &sets[1]);
            if (out_how && write_set) {
                *out_how = YARNET_SOCKET_WAIT_WRITE;
            }
            return write_set;
        }
        case YARNET_SOCKET_WAIT_READ_WRITE: {
            FD_ZERO(&sets[0]);
            FD_ZERO(&sets[1]);
            FD_SET(sockfd, &sets[0]);
            FD_SET(sockfd, &sets[1]);
            result = select(sockfd + 1, &sets[0], &sets[1], NULL, timeout_ms >= 0 ? &tv : NULL);
            if (result <= 0) {
                return yarnet_false;
            }
            read_set = FD_ISSET(sockfd, &sets[0]);
            write_set = FD_ISSET(sockfd, &sets[1]);
            if (out_how && (read_set || write_set)) {
                if (read_set && write_set) {
                    *out_how = YARNET_SOCKET_WAIT_READ_WRITE;
                }
                else if (read_set) {
                    *out_how = YARNET_SOCKET_WAIT_READ;
                }
                else {
                    *out_how = YARNET_SOCKET_WAIT_WRITE;
                }
            }

            return read_set || write_set;
        }
    }

    return yarnet_false;
}


#endif /* _YARNETSOCKET_UNIX */
