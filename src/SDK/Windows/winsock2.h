#pragma once

#include "Reconstruction.hpp"
#include "SDK/Windows/windef.h"

// The Windows SDK v6.0A <winsock2.h> declarations the exe's code uses (WS2_32.dll), with the SDK's names and 32-bit
// layouts, so no Windows header is needed. Global, as Windows declares them. Where POSIX declares a name of its own
// (gethostname, select and its sets), the POSIX side takes the C library's (this header's platform conditions, Linux
// plan section 6).

struct HWND__;

// WS2_32.dll socket operations: the exe's import slots and its import thunks, each of which only calls its slot
// (defined and registered in winsock2.cpp). SOCKET is the SDK's socket handle, a UINT_PTR (4 bytes in the exe, 8 on
// x64); WSAAsyncSelect takes the window as the SDK's HWND.
typedef uintptr_t SOCKET;
// What the socket imports the C library provides on POSIX (socket, recv, send, listen, bind, setsockopt, connect,
// accept) take and return there: the descriptor is an int (-1 converts to SOCKET's ~0, the SDK's INVALID_SOCKET),
// recv and send take a size_t length and return an ssize_t. On Windows, the SDK's SOCKET and int.
#if defined(_WIN32)
using ImportSocket = SOCKET;
using ImportLength = int32_t;
using ImportCount = int32_t;
#else
using ImportSocket = int32_t;
using ImportLength = size_t;
using ImportCount = intptr_t;
#endif
using imp_closesocket_t = int32_t(WINAPI*)(SOCKET s);
using imp_socket_t = ImportSocket(WINAPI*)(int32_t af, int32_t type, int32_t protocol);
using imp_recv_t = ImportCount(WINAPI*)(ImportSocket s, char* buf, ImportLength len, int32_t flags);
using imp_send_t = ImportCount(WINAPI*)(ImportSocket s, const char* buf, ImportLength len, int32_t flags);
using imp_WSAAsyncSelect_t = int32_t(WINAPI*)(SOCKET s, HWND__* hWnd, uint32_t wMsg, int32_t lEvent);
namespace fnvd::imports {
    FNVD_IMPORT(imp_closesocket_t, closesocket, 0xFDF33C);
    FNVD_IMPORT(imp_socket_t, socket, 0xFDF340);
    FNVD_IMPORT(imp_recv_t, recv, 0xFDF348);
    FNVD_IMPORT(imp_send_t, send, 0xFDF34C);
    FNVD_IMPORT(imp_WSAAsyncSelect_t, WSAAsyncSelect, 0xFDF38C);
} // namespace fnvd::imports
FNVD_IMPORT_AS(imp_closesocket_t, imp_closesocket, 0xFDF33C, fnvd::imports::closesocket);
FNVD_IMPORT_AS(imp_socket_t, imp_socket, 0xFDF340, fnvd::imports::socket);
FNVD_IMPORT_AS(imp_recv_t, imp_recv, 0xFDF348, fnvd::imports::recv);
FNVD_IMPORT_AS(imp_send_t, imp_send, 0xFDF34C, fnvd::imports::send);
FNVD_IMPORT_AS(imp_WSAAsyncSelect_t, imp_WSAAsyncSelect, 0xFDF38C, fnvd::imports::WSAAsyncSelect);
int32_t WINAPI closesocket(SOCKET s);
SOCKET WINAPI socket(int32_t af, int32_t type, int32_t protocol);
int32_t WINAPI recv(SOCKET s, char* buf, int32_t len, int32_t flags);
int32_t WINAPI send(SOCKET s, const char* buf, int32_t len, int32_t flags);
int32_t WINAPI WSAAsyncSelect_0(SOCKET s, HWND__* hWnd, uint32_t wMsg, int32_t lEvent);
// The calling thread's last Winsock error (WS2_32.dll WSAGetLastError, stdcall, ret 0); the import thunk 0xEE6D50
// only jumps to the slot 0xFDF344. IDA name; not in the PDB.
using imp_WSAGetLastError_t = int32_t(WINAPI*)();
namespace fnvd::imports {
    FNVD_IMPORT(imp_WSAGetLastError_t, WSAGetLastError, 0xFDF344);
} // namespace fnvd::imports
FNVD_IMPORT_AS(imp_WSAGetLastError_t, imp_WSAGetLastError, 0xFDF344, fnvd::imports::WSAGetLastError);
int32_t WINAPI WSAGetLastError_0();
// The SDK's generic and IPv4 socket addresses (16 bytes each); sin_port and sin_addr are in network byte order.
struct sockaddr {
    uint16_t sa_family;
    char sa_data[14];
};
// The SDK's IPv4 address (4 bytes): the four bytes, the two words and the network-order long in one union.
// hkBsdSocket::pollForNewClient passes the 4 bytes at sockaddr::sa_data[2] to inet_ntoa.
struct in_addr {
    union {
        struct {
            uint8_t s_b1;
            uint8_t s_b2;
            uint8_t s_b3;
            uint8_t s_b4;
        } S_un_b;
        struct {
            uint16_t s_w1;
            uint16_t s_w2;
        } S_un_w;
        uint32_t S_addr;
    } S_un;
};
struct sockaddr_in {
    int16_t sin_family;
    uint16_t sin_port;
    in_addr sin_addr;
    char sin_zero[8];
};
// WS2_32.dll WSAStartup's version and implementation description (the SDK's WSAData, 0x190 bytes).
// hkBsdNetworkInit passes one to WSAStartup with version 0x202 and only keeps the 0x190 bytes WSAStartup
// fills (its local ends at +0x190). The SDK's 64-bit WSAData has the counts and the vendor pointer before the
// strings.
struct WSAData {
    uint16_t wVersion;
    uint16_t wHighVersion;
#if FNVD_64BIT
    uint16_t iMaxSockets;
    uint16_t iMaxUdpDg;
    char* lpVendorInfo;
    char szDescription[257];
    char szSystemStatus[129];
#else
    char szDescription[257];
    char szSystemStatus[129];
    uint16_t iMaxSockets;
    uint16_t iMaxUdpDg;
    char* lpVendorInfo;
#endif
};
// WS2_32.dll ioctlsocket and WSAStartup: the import slots and the thunks 0xEE6D80 and 0xEE6D86, each of which only
// jumps to its slot.
using imp_ioctlsocket_t = int32_t(WINAPI*)(SOCKET s, int32_t cmd, uint32_t* argp);
using imp_WSAStartup_t = int32_t(WINAPI*)(uint16_t wVersionRequested, WSAData* lpWSAData);
namespace fnvd::imports {
    FNVD_IMPORT(imp_ioctlsocket_t, ioctlsocket, 0xFDF360);
    FNVD_IMPORT(imp_WSAStartup_t, WSAStartup, 0xFDF364);
} // namespace fnvd::imports
FNVD_IMPORT_AS(imp_ioctlsocket_t, imp_ioctlsocket, 0xFDF360, fnvd::imports::ioctlsocket);
FNVD_IMPORT_AS(imp_WSAStartup_t, imp_WSAStartup, 0xFDF364, fnvd::imports::WSAStartup);
int32_t WINAPI ioctlsocket(SOCKET s, int32_t cmd, uint32_t* argp);
int32_t WINAPI WSAStartup(uint16_t wVersionRequested, WSAData* lpWSAData);
// WS2_32.dll gethostbyname's host entry (the SDK's hostent): the name, the alias list, the address family and its
// length, and the address list. hkBsdSocket::connect copies h_length bytes from *h_addr_list. On POSIX,
// gethostbyname is the C library's, whose family and length are ints.
struct hostent {
    char* h_name;
    char** h_aliases;
#if defined(_WIN32)
    int16_t h_addrtype;
    int16_t h_length;
#else
    int32_t h_addrtype;
    int32_t h_length;
#endif
    char** h_addr_list;
};
// Windows: u_short htons(u_short). The exe's thunk htons_0 (0xEE6D74, `jmp [0xFDF35C]`) hands the import its
// caller's whole pushed dword (connect pushes edx whole at 0xDA2EE0); uint16_t would be zero-extended.
using imp_htons_t = uint16_t(WINAPI*)(int32_t hostshort);
using imp_gethostname_t = int32_t(WINAPI*)(char* name, int32_t namelen);
using imp_listen_t = int32_t(WINAPI*)(ImportSocket s, int32_t backlog);
using imp_bind_t = int32_t(WINAPI*)(ImportSocket s, const sockaddr* name, int32_t namelen);
using imp_setsockopt_t =
    int32_t(WINAPI*)(ImportSocket s, int32_t level, int32_t optname, const char* optval, int32_t optlen);
namespace fnvd::imports {
    FNVD_IMPORT(imp_htons_t, htons, 0xFDF35C);
#if defined(_WIN32)
    FNVD_IMPORT(imp_gethostname_t, gethostname, 0xFDF368);
#else
    // POSIX's gethostname, which takes the length as a size_t (defined in winsock2.cpp, which includes <unistd.h>).
    int32_t gethostname(char* name, int32_t namelen);
#endif
    FNVD_IMPORT(imp_listen_t, listen, 0xFDF36C);
    FNVD_IMPORT(imp_bind_t, bind, 0xFDF370);
    FNVD_IMPORT(imp_setsockopt_t, setsockopt, 0xFDF374);
} // namespace fnvd::imports
FNVD_IMPORT_AS(imp_htons_t, imp_htons, 0xFDF35C, fnvd::imports::htons);
FNVD_IMPORT_AS(imp_gethostname_t, imp_gethostname, 0xFDF368, fnvd::imports::gethostname);
FNVD_IMPORT_AS(imp_listen_t, imp_listen, 0xFDF36C, fnvd::imports::listen);
FNVD_IMPORT_AS(imp_bind_t, imp_bind, 0xFDF370, fnvd::imports::bind);
FNVD_IMPORT_AS(imp_setsockopt_t, imp_setsockopt, 0xFDF374, fnvd::imports::setsockopt);
// IDA name; not in the PDB. u_short in the SDK: the exe passes the caller's whole dword on; uint16_t would be
// zero-extended.
uint16_t WINAPI htons_0(int32_t hostshort);
int32_t WINAPI gethostname_0(char* name, int32_t namelen);
int32_t WINAPI listen(SOCKET s, int32_t backlog);
int32_t WINAPI bind(SOCKET s, const sockaddr* name, int32_t namelen);
int32_t WINAPI setsockopt(SOCKET s, int32_t level, int32_t optname, const char* optval, int32_t optlen);
// The SDK's WSAEWOULDBLOCK (10035): a non-blocking socket's operation could not complete at once.
constexpr int32_t WSAEWOULDBLOCK = 10035;
// WS2_32.dll import thunks not reconstructed yet, each only a `jmp [slot]` to its import: inet_addr (0xEE6D6E,
// slot 0xFDF358), gethostbyname (0xEE6D68, 0xFDF354), connect (0xEE6D62, 0xFDF350), select (0xEE6DBC, 0xFDF37C),
// __WSAFDIsSet (0xEE6DB6, 0xFDF380), accept (0xEE6DB0, 0xFDF384), ntohs (0xEE6DAA, 0xFDF388) and inet_ntoa
// (0xEE6DA4, 0xFDF378). Inside a class with a member of the same name, call them qualified (`::connect`).
// The standalone side of the thunks below: the WS2_32.dll exports themselves (their slots have no src declaration).
using imp_inet_addr_t = uint32_t(WINAPI*)(const char* cp);
using imp_gethostbyname_t = hostent*(WINAPI*)(const char* name);
using imp_connect_t = int32_t(WINAPI*)(ImportSocket s, const sockaddr* name, int32_t namelen);
using imp_accept_t = ImportSocket(WINAPI*)(ImportSocket s, sockaddr* addr, int32_t* addrlen);
using imp_ntohs_t = uint16_t(WINAPI*)(uint16_t netshort);
using imp_inet_ntoa_t = char*(WINAPI*)(in_addr in);
namespace fnvd::imports {
    FNVD_IMPORT(imp_inet_addr_t, inet_addr, 0xFDF358);
    FNVD_IMPORT(imp_gethostbyname_t, gethostbyname, 0xFDF354);
    FNVD_IMPORT(imp_connect_t, connect, 0xFDF350);
    FNVD_IMPORT(imp_accept_t, accept, 0xFDF384);
    FNVD_IMPORT(imp_ntohs_t, ntohs, 0xFDF388);
    FNVD_IMPORT(imp_inet_ntoa_t, inet_ntoa, 0xFDF378);
} // namespace fnvd::imports
inline uint32_t inet_addr(const char* cp) {
    return fnvd::imports::inet_addr(cp);
}
inline hostent* gethostbyname_0(const char* name) {
    return fnvd::imports::gethostbyname(name);
}
inline int32_t connect(SOCKET s, const sockaddr* name, int32_t namelen) {
    return fnvd::imports::connect(s, name, namelen);
}
inline SOCKET accept(SOCKET s, sockaddr* addr, int32_t* addrlen) {
    return fnvd::imports::accept(s, addr, addrlen);
}
inline uint16_t ntohs_0(uint16_t netshort) {
    return fnvd::imports::ntohs(netshort);
}
// The address goes by value, as its 4 bytes.
inline char* inet_ntoa_0(in_addr in) {
    return fnvd::imports::inet_ntoa(in);
}
// select and its sets, which POSIX declares too (<sys/select.h>, which the C library's own headers include): the
// SDK's on Windows, the C library's elsewhere (this header's platform condition, Linux plan section 6). hkBsdSocket
// fills and tests the sets through FD_ZERO, FD_SET and FD_ISSET, which both define.
#if defined(_WIN32)
// The SDK's winsock fd_set: a count and up to 64 sockets. hkBsdSocket's pollForNewClient sets fd_count = 1
// and fd_array[0] = the socket, then waits on it.
struct fd_set {
    uint32_t fd_count;
    SOCKET fd_array[64];
};
// WS2_32.dll select's timeout (the SDK's timeval, 8 bytes): seconds and microseconds. hkBsdSocket::pollForNewClient
// zeroes both and passes one to select.
struct timeval {
    int32_t tv_sec;
    int32_t tv_usec;
};
// The SDK's set macros, as functions: FD_ZERO empties the set; FD_SET appends a socket the set does not hold yet,
// while there is room for it; FD_ISSET asks WS2_32.dll.
inline void FD_ZERO(fd_set* apSet) {
    apSet->fd_count = 0;
}
inline void FD_SET(SOCKET aSocket, fd_set* apSet) {
    uint32_t i = 0;
    while (i < apSet->fd_count && apSet->fd_array[i] != aSocket) {
        ++i;
    }
    if (i == apSet->fd_count && apSet->fd_count < 64) {
        apSet->fd_array[i] = aSocket;
        ++apSet->fd_count;
    }
}
using imp_select_t =
    int32_t(WINAPI*)(int32_t nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, const timeval* timeout);
using imp___WSAFDIsSet_t = int32_t(WINAPI*)(SOCKET fd, fd_set* set);
namespace fnvd::imports {
    FNVD_IMPORT(imp_select_t, select, 0xFDF37C);
    FNVD_IMPORT(imp___WSAFDIsSet_t, __WSAFDIsSet, 0xFDF380);
} // namespace fnvd::imports
inline int32_t select(int32_t nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, const timeval* timeout) {
    return fnvd::imports::select(nfds, readfds, writefds, exceptfds, timeout);
}
inline int32_t __WSAFDIsSet(SOCKET fd, fd_set* set) {
    return fnvd::imports::__WSAFDIsSet(fd, set);
}
inline int32_t FD_ISSET(SOCKET aSocket, fd_set* apSet) {
    return __WSAFDIsSet(aSocket, apSet);
}
#else
#include <sys/select.h>
#endif
