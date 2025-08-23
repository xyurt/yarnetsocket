#include "yarnetsocket.h"

#ifdef _YARNETSOCKET_WIN32

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
    octets[2] = (yarnet_uint8)((octet_bytes >>  8) & 0xFF);
    octets[3] = (yarnet_uint8)((octet_bytes >>  0) & 0xFF);

    pos = 0;
    for (i = 0; i < 4; i++) {
        len = 0;

        if (octets[i] >= 100) {
            buf[len++] = '0' + (octets[i] / 100);
            buf[len++] = '0' + ((octets[i] / 10) % 10);
            buf[len++] = '0' + (octets[i] % 10);
        } else if (octets[i] >= 10) {
            buf[len++] = '0' + (octets[i] / 10);
            buf[len++] = '0' + (octets[i] % 10);
        } else {
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
    WSADATA wsa_data;
    return WSAStartup(MAKEWORD(2, 2), &wsa_data) == 0;
}

void         yarnet_socket_deinitialize(void) {
    WSACleanup();
}

YarnetSocket yarnet_socket_create(YarnetSocketType type) {
    switch (type) {
        case YARNET_SOCKET_TYPE_STREAM: {
            return socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        }
        case YARNET_SOCKET_TYPE_DATAGRAM: {
            return socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        }
    }

    return INVALID_SOCKET;
}
void         yarnet_socket_close(YarnetSocket sockfd) {
    if (SOCKET_IS_INVALID(sockfd)) {
        return;
    }

    closesocket(sockfd);
}
yarnet_bool  yarnet_socket_shutdown(YarnetSocket sockfd, YarnetSocketShutdown how) {
    if (SOCKET_IS_INVALID(sockfd)) {
        return yarnet_false;
    }

    switch (how) {
        case YARNET_SOCKET_SHUTDOWN_READ: {
            return shutdown(sockfd, 0) != SOCKET_ERROR;
        }
        case YARNET_SOCKET_SHUTDOWN_WRITE: {
            return shutdown(sockfd, 1) != SOCKET_ERROR;
        }
        case YARNET_SOCKET_SHUTDOWN_READ_WRITE: {
            return shutdown(sockfd, 2) != SOCKET_ERROR;
        }
    }

    return yarnet_false;
}

yarnet_bool  yarnet_socket_get_local_address(YarnetSocket sockfd, YarnetAddress *out_address) {
    struct sockaddr_in socket_address;
    int                address_length;

    if (SOCKET_IS_INVALID(sockfd)) {
        return yarnet_false;
    }

    address_length = sizeof(struct sockaddr_in);
    if (getsockname(sockfd, (struct sockaddr *)&socket_address, &address_length) == SOCKET_ERROR) {
        return yarnet_false;
    }

    out_address->host = socket_address.sin_addr.S_un.S_addr;
    out_address->port = YARNET_NET_TO_HOST_16(socket_address.sin_port);

    return yarnet_true;
}
yarnet_bool  yarnet_socket_get_remote_address(YarnetSocket sockfd, YarnetAddress *out_address) {
    struct sockaddr_in socket_address;
    int                address_length;

    if (SOCKET_IS_INVALID(sockfd)) {
        return yarnet_false;
    }

    address_length = sizeof(struct sockaddr_in);
    if (getpeername(sockfd, (struct sockaddr *)&socket_address, &address_length) == SOCKET_ERROR) {
        return yarnet_false;
    }

    out_address->host = socket_address.sin_addr.S_un.S_addr;
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
    socket_address.sin_addr.S_un.S_addr = address->host;
    socket_address.sin_port = YARNET_HOST_TO_NET_16(address->port);

    if (bind(sockfd, (const struct sockaddr *)&socket_address, sizeof(struct sockaddr_in)) == SOCKET_ERROR) {
        return yarnet_false;
    }

    return yarnet_true;
}

yarnet_bool  yarnet_socket_receive(YarnetSocket sockfd, YarnetAddress *out_address, YarnetBuffer *out_buffers, size_t out_buffer_count, size_t *out_receive_length) {
    struct sockaddr_in socket_address;
    int                address_length;
    yarnet_uint32      receive_length;
    yarnet_uint32      receive_flags;

    if (SOCKET_IS_INVALID(sockfd)) {
        return yarnet_false;
    }

    address_length = sizeof(struct sockaddr_in);
    memset(&socket_address, 0, address_length);
    receive_flags = 0;
    if (WSARecvFrom(
        (SOCKET)sockfd, 
        (LPWSABUF)out_buffers, (DWORD)out_buffer_count,
        (LPDWORD)&receive_length,
        (LPDWORD)&receive_flags,
        (out_address ? (struct sockaddr *)&socket_address : NULL), (out_address ? (LPINT)&address_length : NULL),
        NULL, NULL) == SOCKET_ERROR) {

        *out_receive_length = 0;

        switch (WSAGetLastError()) {
            case WSAEWOULDBLOCK: {
                return yarnet_true;
            }
        }
        return yarnet_false;
    }

    *out_receive_length = receive_length;

    if (out_address) {
        out_address->host = socket_address.sin_addr.S_un.S_addr;
        out_address->port = YARNET_NET_TO_HOST_16(socket_address.sin_port);
    }

    return yarnet_true;
}
yarnet_bool  yarnet_socket_send(YarnetSocket sockfd, const YarnetAddress *address, const YarnetBuffer *buffers, size_t buffer_count, size_t *out_sent_length) {
    struct sockaddr_in socket_address;
    yarnet_uint32      send_length;

    if (SOCKET_IS_INVALID(sockfd)) {
        return yarnet_false;
    }

    if (address) {
        socket_address.sin_family = AF_INET;
        socket_address.sin_addr.S_un.S_addr = address->host;
        socket_address.sin_port = YARNET_HOST_TO_NET_16(address->port);
    }

    if (WSASendTo(
        (SOCKET)sockfd,
        (LPWSABUF)buffers, (DWORD)buffer_count,
        (LPDWORD)&send_length,
        (DWORD)0,
        (address ? (const struct sockaddr *)&socket_address : NULL), (address ? sizeof(struct sockaddr_in) : (int)0),
        NULL, NULL) == SOCKET_ERROR) {

        *out_sent_length = 0;

        switch (WSAGetLastError()) {
            case WSAEWOULDBLOCK: {
                return yarnet_true;
            }
        }
        return yarnet_false;
    }

    *out_sent_length = send_length;

    return yarnet_true;
}

yarnet_bool  yarnet_socket_get_option(YarnetSocket sockfd, YarnetSocketOption option, int *out_value) {
    int optval; 
    int optlen;

    if (SOCKET_IS_INVALID(sockfd)) {
        return yarnet_false;
    }

    optlen = sizeof(optval);

    switch (option) {
        case YARNET_SOCKET_OPTION_BROADCAST: {
            if (getsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (char *)&optval, &optlen) == SOCKET_ERROR) {
                return yarnet_false;
            }

            *out_value = optval;
            return yarnet_true;
        }

        case YARNET_SOCKET_OPTION_RCVBUF: {
            if (getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char *)&optval, &optlen) == SOCKET_ERROR) {
                return yarnet_false;
            }
            *out_value = optval;
            return yarnet_true;
        }

        case YARNET_SOCKET_OPTION_SNDBUF: {
            if (getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char *)&optval, &optlen) == SOCKET_ERROR) {
                return yarnet_false;
            }
            *out_value = optval;
            return yarnet_true;
        }

        case YARNET_SOCKET_OPTION_REUSEADDR: {
            if (getsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, &optlen) == SOCKET_ERROR) {
                return yarnet_false;
            }
            *out_value = optval;
            return yarnet_true;
        }

        case YARNET_SOCKET_OPTION_RCVTIMEO: {
            if (getsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&optval, &optlen) == SOCKET_ERROR) {
                return yarnet_false;
            }
            *out_value = optval;
            return yarnet_true;
        }

        case YARNET_SOCKET_OPTION_SNDTIMEO: {
            if (getsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&optval, &optlen) == SOCKET_ERROR) {
                return yarnet_false;
            }
            *out_value = optval;
            return yarnet_true;
        }

        case YARNET_SOCKET_OPTION_NODELAY: {
            if (getsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&optval, &optlen) == SOCKET_ERROR) {
                return yarnet_false;
            }
            *out_value = optval;
            return yarnet_true;
        }

        case YARNET_SOCKET_OPTION_TTL: {
            if (getsockopt(sockfd, IPPROTO_IP, 4, (char *)&optval, &optlen) == SOCKET_ERROR) {
                return yarnet_false;
            }
            *out_value = optval;
            return yarnet_true;
        }

        case YARNET_SOCKET_OPTION_KEEPALIVE: {
            if (getsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (char *)&optval, &optlen) == SOCKET_ERROR) {
                return yarnet_false;
            }
            *out_value = optval;
            return yarnet_true;
        }

        case YARNET_SOCKET_OPTION_DONTROUTE: {
            if (getsockopt(sockfd, SOL_SOCKET, SO_DONTROUTE, (char *)&optval, &optlen) == SOCKET_ERROR) {
                return yarnet_false;
            }
            *out_value = optval;
            return yarnet_true;
        }
    }

    return yarnet_false;
}
yarnet_bool  yarnet_socket_set_option(YarnetSocket sockfd, YarnetSocketOption option, int value) {
    if (SOCKET_IS_INVALID(sockfd)) {
        return yarnet_false;
    }

    switch (option) {
        case YARNET_SOCKET_OPTION_NONBLOCK: {
            return ioctlsocket(sockfd, FIONBIO, (u_long *)&value) != SOCKET_ERROR;
        }

        case YARNET_SOCKET_OPTION_BROADCAST: {
            return setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (const char *)&value, sizeof(value)) != SOCKET_ERROR;
        }

        case YARNET_SOCKET_OPTION_RCVBUF: {
            return setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (const char *)&value, sizeof(value)) != SOCKET_ERROR;
        }

        case YARNET_SOCKET_OPTION_SNDBUF: {
            return setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (const char *)&value, sizeof(value)) != SOCKET_ERROR;
        }

        case YARNET_SOCKET_OPTION_REUSEADDR: {
            return setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&value, sizeof(value)) != SOCKET_ERROR;
        }

        case YARNET_SOCKET_OPTION_RCVTIMEO: {
            return setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&value, sizeof(value)) != SOCKET_ERROR;
        }

        case YARNET_SOCKET_OPTION_SNDTIMEO: { 
            return setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&value, sizeof(value)) != SOCKET_ERROR; 
        }

        case YARNET_SOCKET_OPTION_NODELAY: {
            return setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (const char *)&value, sizeof(value)) != SOCKET_ERROR;
        }

        case YARNET_SOCKET_OPTION_TTL: {
            return setsockopt(sockfd, IPPROTO_IP, 4, (const char *)&value, sizeof(value)) != SOCKET_ERROR;
        }

        case YARNET_SOCKET_OPTION_KEEPALIVE: {
            return setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (const char *)&value, sizeof(value)) != SOCKET_ERROR;
        }

        case YARNET_SOCKET_OPTION_DONTROUTE: {
            return setsockopt(sockfd, SOL_SOCKET, SO_DONTROUTE, (const char *)&value, sizeof(value)) != SOCKET_ERROR;
        }
    }

    return yarnet_false;
}

yarnet_bool  yarnet_socket_listen(YarnetSocket sockfd, int backlog) {
    if (SOCKET_IS_INVALID(sockfd)) {
        return yarnet_false;
    }

    return listen(sockfd, backlog) != SOCKET_ERROR;
}
YarnetSocket yarnet_socket_accept(YarnetSocket sockfd, YarnetAddress *out_address) {
    struct sockaddr_in socket_address;
    int                address_length;
    SOCKET             client_socket;

    if (SOCKET_IS_INVALID(sockfd)) {
        return INVALID_SOCKET;
    }

    address_length = sizeof(struct sockaddr_in);
    client_socket = accept(sockfd, (struct sockaddr *)&socket_address, &address_length);
    if (client_socket == SOCKET_ERROR) {
        return INVALID_SOCKET;
    }

    if (out_address) {
        out_address->host = socket_address.sin_addr.S_un.S_addr;
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
    socket_address.sin_addr.S_un.S_addr = address->host;
    socket_address.sin_port = YARNET_HOST_TO_NET_16(address->port);

    if (connect(sockfd, (const struct sockaddr *)&socket_address, sizeof(struct sockaddr_in)) == SOCKET_ERROR) {
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
            result = select(0, &sets[0], NULL, NULL, timeout_ms >= 0 ? &tv : NULL);
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
            result = select(0, NULL, &sets[1], NULL, timeout_ms >= 0 ? &tv : NULL);
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
            result = select(0, &sets[0], &sets[1], NULL, timeout_ms >= 0 ? &tv : NULL);
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

#endif /* _YARNETSOCKET_WIN32 */