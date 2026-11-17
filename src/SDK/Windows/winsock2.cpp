#include "SDK/Windows/winsock2.h"
#if !defined(_WIN32)
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#endif

// The exe's WS2_32.dll import thunks: each is a `jmp [slot]` to its import, so it only calls the slot and returns its
// result.

#if !defined(_WIN32)
int32_t fnvd::imports::gethostname(char* name, int32_t namelen)
{
	return ::gethostname(name, static_cast<size_t>(namelen));
}

// The WS2_32.dll exports the C library has no function of the name for (the C library's sockets are file
// descriptors): closesocket closes the descriptor.
int32_t WINAPI fnvd::imports::closesocket(SOCKET s)
{
	return ::close(static_cast<int32_t>(s));
}

// ioctlsocket's FIONBIO (the SDK's 0x8004667E, the only command the exe passes) sets or clears O_NONBLOCK; any other
// command fails with -1 (SOCKET_ERROR).
int32_t WINAPI fnvd::imports::ioctlsocket(SOCKET s, int32_t cmd, uint32_t* argp)
{
	constexpr int32_t kFionbio = static_cast<int32_t>(0x8004667Eu);
	if (cmd != kFionbio || !argp) {
		errno = EINVAL;
		return -1;
	}
	int32_t iFlags = ::fcntl(static_cast<int32_t>(s), F_GETFL);
	if (iFlags == -1) {
		return -1;
	}
	iFlags = *argp ? (iFlags | O_NONBLOCK) : (iFlags & ~O_NONBLOCK);
	return ::fcntl(static_cast<int32_t>(s), F_SETFL, iFlags) == -1 ? -1 : 0;
}

// The C library's sockets need no start-up: WSAStartup only reports Winsock 2.2, as Windows does, and succeeds.
int32_t WINAPI fnvd::imports::WSAStartup(uint16_t, WSAData* lpWSAData)
{
	std::memset(lpWSAData, 0, sizeof(*lpWSAData));
	lpWSAData->wVersion = 0x202;
	lpWSAData->wHighVersion = 0x202;
	std::strcpy(lpWSAData->szDescription, "WinSock 2.0");
	std::strcpy(lpWSAData->szSystemStatus, "Running");
	return 0;
}

// The last socket error: a call that would block (EAGAIN, EWOULDBLOCK, or a non-blocking connect's EINPROGRESS)
// is the SDK's WSAEWOULDBLOCK, as Windows reports it; any other error is the C library's errno value (the exe only
// tests for WSAEWOULDBLOCK).
int32_t WINAPI fnvd::imports::WSAGetLastError()
{
	int32_t iError = errno;
	if (iError == EAGAIN || iError == EWOULDBLOCK || iError == EINPROGRESS) {
		return WSAEWOULDBLOCK;
	}
	return iError;
}

// No window messages on POSIX, so no socket event can be posted to a window: fails with -1 (SOCKET_ERROR), which
// hkBsdSocket::asyncSelect returns as HK_FAILURE.
int32_t WINAPI fnvd::imports::WSAAsyncSelect(SOCKET, HWND__*, uint32_t, int32_t)
{
	errno = ENOSYS;
	return -1;
}
#endif

int32_t WINAPI closesocket(SOCKET s)
{
    return imp_closesocket(s);
}

SOCKET WINAPI socket(int32_t af, int32_t type, int32_t protocol)
{
    return imp_socket(af, type, protocol);
}

int32_t WINAPI recv(SOCKET s, char* buf, int32_t len, int32_t flags)
{
    return imp_recv(s, buf, len, flags);
}

int32_t WINAPI send(SOCKET s, const char* buf, int32_t len, int32_t flags)
{
    return imp_send(s, buf, len, flags);
}

int32_t WINAPI WSAAsyncSelect_0(SOCKET s, HWND__* hWnd, uint32_t wMsg, int32_t lEvent)
{
    return imp_WSAAsyncSelect(s, hWnd, wMsg, lEvent);
}

int32_t WINAPI ioctlsocket(SOCKET s, int32_t cmd, uint32_t* argp)
{
	return imp_ioctlsocket(s, cmd, argp);
}

int32_t WINAPI WSAStartup(uint16_t wVersionRequested, WSAData* lpWSAData)
{
	return imp_WSAStartup(wVersionRequested, lpWSAData);
}

int32_t WINAPI WSAGetLastError_0()
{
	return imp_WSAGetLastError();
}

uint16_t WINAPI htons_0(int32_t hostshort)
{
	return imp_htons(hostshort);
}

int32_t WINAPI gethostname_0(char* name, int32_t namelen)
{
	return imp_gethostname(name, namelen);
}

int32_t WINAPI listen(SOCKET s, int32_t backlog)
{
	return imp_listen(s, backlog);
}

int32_t WINAPI bind(SOCKET s, const sockaddr* name, int32_t namelen)
{
	return imp_bind(s, name, namelen);
}

int32_t WINAPI setsockopt(SOCKET s, int32_t level, int32_t optname, const char* optval, int32_t optlen)
{
	return imp_setsockopt(s, level, optname, optval, optlen);
}
