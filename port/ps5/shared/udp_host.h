/* The network link's socket on the bridge hosts (port/src/port_netlink.c, port/progress/ONLINE_ESTUDIO.md): the game
 * image has no sockets, so it asks the host for one non-blocking UDP socket (bridge entries udp_*). Addresses cross
 * the bridge as 6 bytes, the IPv4 address and the port in network order.
 *
 * Included once by each host. A host with BSD sockets (Android 64-bit, the Linux test host) defines UDP_HOST_POSIX
 * first, Windows 64-bit UDP_HOST_WINSOCK; the others get the stand-ins: no socket, the link stays the cartridge's
 * cable. */
#ifndef UDP_HOST_H
#define UDP_HOST_H

#include <stdint.h>
#include <string.h>

#ifdef UDP_HOST_POSIX
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

static int sUdpSock = -1;

/* int (int bind_port): 1 a non-blocking UDP socket on that port (0: any), 0 none */
static int H_udp_open(int port) {
    struct sockaddr_in me;
    if (sUdpSock >= 0)
        close(sUdpSock);
    sUdpSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sUdpSock < 0)
        return 0;
    fcntl(sUdpSock, F_SETFL, fcntl(sUdpSock, F_GETFL, 0) | O_NONBLOCK);
    memset(&me, 0, sizeof(me));
    me.sin_family = AF_INET;
    me.sin_addr.s_addr = htonl(INADDR_ANY);
    me.sin_port = htons((unsigned short)port);
    if (bind(sUdpSock, (struct sockaddr*)&me, sizeof(me)) != 0) {
        close(sUdpSock);
        sUdpSock = -1;
        return 0;
    }
    return 1;
}

/* int (const char* host, int port, uint8_t* addr6): 1 found */
static int H_udp_resolve(const char* host, int port, uint8_t* addr) {
    struct addrinfo hints, *res = NULL;
    uint16_t p = htons((unsigned short)port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(host, NULL, &hints, &res) != 0 || !res)
        return 0;
    memcpy(addr, &((struct sockaddr_in*)res->ai_addr)->sin_addr.s_addr, 4);
    memcpy(addr + 4, &p, 2);
    freeaddrinfo(res);
    return 1;
}

/* void (const uint8_t* addr6, const void* data, int size) */
static void H_udp_send(const uint8_t* addr, const void* data, int size) {
    struct sockaddr_in to;
    if (sUdpSock < 0 || size <= 0)
        return;
    memset(&to, 0, sizeof(to));
    to.sin_family = AF_INET;
    memcpy(&to.sin_addr.s_addr, addr, 4);
    memcpy(&to.sin_port, addr + 4, 2);
    sendto(sUdpSock, data, (size_t)size, 0, (struct sockaddr*)&to, sizeof(to));
}

/* int (void* data, int cap, uint8_t* addr6): the bytes of one datagram, 0 none waiting */
static int H_udp_recv(void* data, int cap, uint8_t* addr) {
    struct sockaddr_in from;
    socklen_t len = sizeof(from);
    ssize_t n;
    if (sUdpSock < 0 || cap <= 0)
        return 0;
    n = recvfrom(sUdpSock, data, (size_t)cap, 0, (struct sockaddr*)&from, &len);
    if (n <= 0)
        return 0;
    memcpy(addr, &from.sin_addr.s_addr, 4);
    memcpy(addr + 4, &from.sin_port, 2);
    return (int)n;
}
#elif defined(UDP_HOST_WINSOCK)
/* Windows 64-bit: the same socket as the 32-bit game's (port/src/port_netlink.c UdpOpen..UdpRecv). The host
   includes winsock2.h before windows.h and links ws2_32. */
static SOCKET sUdpSock = INVALID_SOCKET;

static int H_udp_open(int port) {
    struct sockaddr_in me;
    WSADATA wsa;
    u_long nb = 1;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return 0;
    if (sUdpSock != INVALID_SOCKET)
        closesocket(sUdpSock); /* the lobby opens it again for the room it picked */
    sUdpSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sUdpSock == INVALID_SOCKET)
        return 0;
    ioctlsocket(sUdpSock, FIONBIO, &nb);
    memset(&me, 0, sizeof(me));
    me.sin_family = AF_INET;
    me.sin_addr.s_addr = htonl(INADDR_ANY);
    me.sin_port = htons((unsigned short)port);
    if (bind(sUdpSock, (struct sockaddr*)&me, sizeof(me)) != 0) {
        closesocket(sUdpSock);
        sUdpSock = INVALID_SOCKET;
        return 0;
    }
    return 1;
}

static int H_udp_resolve(const char* host, int port, uint8_t* addr) {
    struct addrinfo hints, *res = NULL;
    uint16_t p = htons((unsigned short)port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(host, NULL, &hints, &res) != 0 || !res)
        return 0;
    memcpy(addr, &((struct sockaddr_in*)res->ai_addr)->sin_addr.s_addr, 4);
    memcpy(addr + 4, &p, 2);
    freeaddrinfo(res);
    return 1;
}

static void H_udp_send(const uint8_t* addr, const void* data, int size) {
    struct sockaddr_in to;
    if (sUdpSock == INVALID_SOCKET || size <= 0)
        return;
    memset(&to, 0, sizeof(to));
    to.sin_family = AF_INET;
    memcpy(&to.sin_addr.s_addr, addr, 4);
    memcpy(&to.sin_port, addr + 4, 2);
    sendto(sUdpSock, (const char*)data, size, 0, (struct sockaddr*)&to, sizeof(to));
}

static int H_udp_recv(void* data, int cap, uint8_t* addr) {
    struct sockaddr_in from;
    int len = sizeof(from), n;
    if (sUdpSock == INVALID_SOCKET || cap <= 0)
        return 0;
    n = recvfrom(sUdpSock, (char*)data, cap, 0, (struct sockaddr*)&from, &len);
    if (n <= 0)
        return 0;
    memcpy(addr, &from.sin_addr.s_addr, 4);
    memcpy(addr + 4, &from.sin_port, 2);
    return n;
}
#else
static int H_udp_open(int port) { (void)port; return 0; }
static int H_udp_resolve(const char* host, int port, uint8_t* addr) { (void)host; (void)port; (void)addr; return 0; }
static void H_udp_send(const uint8_t* addr, const void* data, int size) { (void)addr; (void)data; (void)size; }
static int H_udp_recv(void* data, int cap, uint8_t* addr) { (void)data; (void)cap; (void)addr; return 0; }
#endif

#endif
