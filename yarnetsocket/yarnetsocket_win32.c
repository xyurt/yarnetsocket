#include "yarnetsocket.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <string.h>

int yarnet_socket_initialize() {
	WSADATA wsadata;
	return WSAStartup(MAKEWORD(2, 2), &wsadata) == 0;
}
int yarnet_socket_cleanup() {
	return WSACleanup() == 0;
}

yarnet_socket yarnet_socket_ipv4() {
	return socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
}
yarnet_socket yarnet_socket_ipv6() {
	return socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
}
int yarnet_socket_close(yarnet_socket sockfd) {
	if (yarnet_socket_is_invalid(sockfd)) {
		return 0;
	}
	return closesocket(sockfd) == 0;
}

int yarnet_socket_is_invalid(yarnet_socket sockfd) {
	return sockfd == INVALID_SOCKET;
}

int yarnet_socket_bind(yarnet_socket sockfd, yarnet_address address) {
	struct sockaddr_storage *address_p;
	int result;
	struct sockaddr_storage bind_address;
	int name_len;
	address_p = (struct sockaddr_storage *)address;
	if (yarnet_socket_is_invalid(sockfd) || address == NULL || (address_p->ss_family != AF_INET && address_p->ss_family != AF_INET6)) {
		return 0;
	}
	result = bind(sockfd, (const struct sockaddr *)address_p, address_p->ss_family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6)) == 0;
	if (result) {
		name_len = sizeof(struct sockaddr_storage);
		if (getsockname(sockfd, (struct sockaddr *)&bind_address, &name_len) == 0) {
			if (address_p->ss_family == AF_INET) {
				((struct sockaddr_in *)address_p)->sin_port = ((struct sockaddr_in *)&bind_address)->sin_port;
			}
			else {
				((struct sockaddr_in6 *)address_p)->sin6_port = ((struct sockaddr_in6 *)&bind_address)->sin6_port;
			}
		}
	}
	return result;
}

int yarnet_socket_send(yarnet_socket sockfd, const char *buffer, unsigned long long length, int flags, const yarnet_address address) {
	const struct sockaddr_storage *address_p;
	address_p = (const struct sockaddr_storage *)address;
	if (yarnet_socket_is_invalid(sockfd) || (buffer == NULL && length != 0) || address == NULL || (address_p->ss_family != AF_INET && address_p->ss_family != AF_INET6)) {
		return -1;
	}
	return sendto(sockfd, buffer, (int)length, flags, (const struct sockaddr *)address_p, address_p->ss_family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6));
}
int yarnet_socket_recv(yarnet_socket sockfd, char *buffer, unsigned long long length, int flags, yarnet_address address) {
	int from_len;
	if (yarnet_socket_is_invalid(sockfd) || (buffer == NULL && length != 0)) {
		return -1;
	}
	from_len = sizeof(yarnet_address);
	return recvfrom(sockfd, buffer, (int)length, flags, (struct sockaddr *)address, address == NULL ? NULL : &from_len);
}

int yarnet_socket_wait(yarnet_socket sockfd, unsigned long long timeout_ms) {
	struct timeval tv;
	fd_set read_set;
	int result;
	if (yarnet_socket_is_invalid(sockfd)) {
		return -1;
	}
	tv.tv_sec = timeout_ms / 1000;
	tv.tv_usec = (timeout_ms % 1000) * 1000;
	FD_ZERO(&read_set);
	FD_SET(sockfd, &read_set);
	result = select(0, &read_set, NULL, NULL, &tv);
	if (result != 0 && result != 1) {
		return -1;
	}
	return FD_ISSET(sockfd, &read_set);
}

int yarnet_address_set_ipv4(yarnet_address address, const char *ip, unsigned short port) {
	struct sockaddr_in *address_p;
	if (address == NULL || ip == NULL) {
		return 0;
	}
	address_p = (const struct sockaddr_in *)address;
	address_p->sin_family = AF_INET;
	address_p->sin_port = htons(port);
	return inet_pton(AF_INET, ip, &address_p->sin_addr) == 1;
}
int yarnet_address_set_ipv6(yarnet_address address, const char *ip, unsigned short port) {
	struct sockaddr_in6 *address_p;
	if (address == NULL || ip == NULL) {
		return 0;
	}
	address_p = (const struct sockaddr_in6 *)address;
	address_p->sin6_family = AF_INET6;
	address_p->sin6_port = htons(port);
	return inet_pton(AF_INET6, ip, &address_p->sin6_addr) == 1;
}
int yarnet_address_get_ip(yarnet_address address, char *buffer, unsigned long long *length) {
	const struct sockaddr_storage *address_p;
	char ip_buffer[INET6_ADDRSTRLEN];
	unsigned long long buffer_length;
	address_p = (const struct sockaddr_storage *)address;
	if (length == NULL || buffer == NULL) {
		return 0;
	}
	if (*length > 0) {
		buffer[0] = '\0';
	}
	if (address == NULL) {
		return 0;
	}
	if (address_p->ss_family == AF_INET) {
		if (inet_ntop(AF_INET, &((struct sockaddr_in *)address_p)->sin_addr, ip_buffer, sizeof(ip_buffer)) == NULL) {
			return 0;
		}
	}
	else if (address_p->ss_family == AF_INET6) {
		if (inet_ntop(AF_INET6, &((struct sockaddr_in6 *)address_p)->sin6_addr, ip_buffer, sizeof(ip_buffer)) == NULL) {
			return 0;
		}
	}
	else {
		return 0;
	}
	buffer_length = strlen(ip_buffer);
	memcpy(buffer, ip_buffer, *length > buffer_length ? buffer_length : *length);
	if (buffer_length != *length) {
		buffer[*length] = '\0';
	}
	*length = buffer_length;
	return 1;
}
int yarnet_address_get_port(yarnet_address address, unsigned short *port) {
	const struct sockaddr_storage *address_p;
	if (address == NULL || port == NULL) {
		return 0;
	}
	address_p = (const struct sockaddr_storage *)address;
	if (address_p->ss_family == AF_INET) {
		*port = ntohs(((struct sockaddr_in *)address_p)->sin_port);
	}
	else if (address_p->ss_family == AF_INET6) {
		*port = ntohs(((struct sockaddr_in6 *)address_p)->sin6_port);
	}
	else {
		return 0;
	}
	return 1;
}
int yarnet_address_copy(yarnet_address dest, yarnet_address src) {
	if (dest == NULL || src == NULL) {
		return 0;
	}
	memcpy(dest, src, sizeof(yarnet_address));
	return 1;
}