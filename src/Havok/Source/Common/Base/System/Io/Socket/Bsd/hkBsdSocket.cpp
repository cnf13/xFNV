#include "Havok/Source/Common/Base/System/Io/Socket/Bsd/hkBsdSocket.h"

#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/String/hkStringBuf.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/System/Io/Socket/hkSocket.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "SDK/Windows/winsock2.h"
#include "xfnv/Compiler.h"

void hkBsdNetworkInit() {
    WSAData wsaData;
    if (!g_defaultPlatformInitOnce.m_bool) {
        if (WSAStartup(0x202, &wsaData) == -1) {
            char buf[512];
            hkErrStream ostr(buf, 512);
            ostr << "(Windows)WSAStartup failed with error!";
            if (hkError::s_instance->message(
                    hkError::MESSAGE_ERROR,
                    0x321825F8,
                    buf,
                    ".\\System\\Io\\Socket\\Bsd\\hkBsdSocket.cpp",
                    72
                )) {
                xfnv::DebugTrap();
            }
        }
        g_defaultPlatformInitOnce.m_bool = 1;
    }
}

void hkBsdNetworkQuit() {
    ;
}

// Whether c is a decimal digit: one unsigned compare of c - '0' against 9.
// KEEP: the body writes only one scratch register, and connect keeps the server name in another across the call,
// which clang does not guarantee: it passes that register on to inet_addr or gethostbyname after the call.
hkBool hkIsDigit(int32_t c) {
    return static_cast<uint32_t>(c) - 0x30u <= 9u;
}

// s is an open socket, or -1 for a new one.
hkBsdSocket::hkBsdSocket(socket_t s) : m_socket(s) {
    if (s == static_cast<socket_t>(-1)) {
        createSocket();
    }
}

hkBsdSocket::~hkBsdSocket() {
    hkBsdSocket::close();
}

hkBool hkBsdSocket::isOk() const {
    return m_socket != (-1);
}

// Closes the socket unless it is already closed, then forgets the handle.
void hkBsdSocket::close() {
    if (m_socket != static_cast<socket_t>(-1)) {
        closesocket(m_socket);
        m_socket = static_cast<socket_t>(-1);
    }
}

int32_t hkBsdSocket::read(void* buf, int32_t nbytes) {
    socket_t m_socket = this->m_socket;
    if (m_socket != -1) {
        int32_t result = recv(m_socket, static_cast<char*>(buf), nbytes, 0);
        if (result > 0) {
            return result;
        }
        if (WSAGetLastError_0() != 0x2733) {
            close();
        }
    }
    return 0;
}

int32_t hkBsdSocket::write(const void* buf, int32_t nbytes) {
    socket_t m_socket = this->m_socket;
    if (m_socket != -1) {
        int32_t result = send(m_socket, static_cast<const char*>(buf), nbytes, 0);
        if (result > 0) {
            return result;
        }
        if (WSAGetLastError_0() != 0x2733) {
            close();
        }
    }
    return 0;
}

// Connects to apServerName (a dotted address when it starts with a digit, else a host name to look up) on aiPortNumber,
// creating the socket first if there is none; a connect that would block counts as success.
hkResult hkBsdSocket::connect(const char* servername, int32_t portNumber) {
    sockaddr_in kAddr;
    hkString::memSet(&kAddr, 0, sizeof(kAddr));
    // AF_INET.
    kAddr.sin_family = 2;
    kAddr.sin_port = htons_0(portNumber);
    if (hkIsDigit(*servername).m_bool) {
        kAddr.sin_addr.S_un.S_addr = inet_addr(servername);
    }
    else {
        hostent* pHost = gethostbyname_0(servername);
        if (!pHost) {
            return HK_FAILURE;
        }
        hkString::memCpy(&kAddr.sin_addr, *pHost->h_addr_list, pHost->h_length);
    }
    if (m_socket != static_cast<SOCKET>(-1) || createSocket() == HK_SUCCESS) {
        // Winsock takes the IPv4 address as the generic sockaddr, read for namelen bytes.
        if (::connect(m_socket, reinterpret_cast<const sockaddr*>(&kAddr), sizeof(kAddr)) != -1 ||
            WSAGetLastError_0() == WSAEWOULDBLOCK) {
            return HK_SUCCESS;
        }
        close();
    }
    return HK_FAILURE;
}

// Creates the socket, binds it to every local address on aiPortNumber (SO_REUSEADDR set first) and listens with a
// backlog of 2; on a bind or listen failure it closes the socket. Reports the host name and port through hkError.
hkResult hkBsdSocket::listen(int32_t port) {
    if (createSocket()) {
        return HK_FAILURE;
    }
    sockaddr_in kAddr;
    int32_t iOptval;
    // AF_INET, INADDR_ANY.
    kAddr.sin_family = 2;
    kAddr.sin_addr.S_un.S_addr = 0;
    kAddr.sin_port = htons_0(port);
    iOptval = 1;
    // SOL_SOCKET, SO_REUSEADDR. Winsock takes the option value and the address as untyped bytes (char*, and a
    // sockaddr* read for namelen bytes), hence the casts.
    setsockopt(m_socket, 0xFFFF, 4, reinterpret_cast<const char*>(&iOptval), 4);
    if (bind(m_socket, reinterpret_cast<const sockaddr*>(&kAddr), 0x10) == -1 || ::listen(m_socket, 2) == -1) {
        hkSocket* pSocket = this;
        pSocket->close();
        return HK_FAILURE;
    }
    alignas(hkStringBuf) uint8_t acHost[sizeof(hkStringBuf)];
    char acHostName[0x100];
    alignas(hkErrStream) uint8_t acStream[sizeof(hkErrStream)];
    char acBuffer[0x200];
    hkStringBuf* pHost = ::new (acHost) hkStringBuf;
    gethostname_0(acHostName, 0x100);
    *pHost = acHostName;
    hkErrStream* pStream = ::new (acStream) hkErrStream(acBuffer, 0x200);
    (*pStream << "Listening on host[") << *pHost << "] port " << port;
    hkError::s_instance
        ->message(hkError::MESSAGE_REPORT, -1, acBuffer, ".\\System\\Io\\Socket\\Bsd\\hkBsdSocket.cpp", 364);
    pStream->~hkErrStream();
    pHost->~hkStringBuf();
    return HK_SUCCESS;
}

hkResult hkBsdSocket::asyncSelect(void* notificationHandle, uint32_t message, hkSocket::SOCKET_EVENTS events) {
    return static_cast<hkResult>(
        WSAAsyncSelect_0(
            m_socket,
            static_cast<HWND__*>(notificationHandle),
            message,
            (events & 3) | 4 * (events & 12)
        ) != 0
    );
}

// Accepts a waiting client, if select says there is one, reports where it came from, and returns a new socket for it
// with TCP_NODELAY set.
// KEEP: when accept fails the exe still reports the client's port and address from addr, which accept left
// unwritten.
hkSocket* hkBsdSocket::pollForNewClient() {
    SOCKET hSocket = m_socket;
    if (hSocket == static_cast<SOCKET>(-1)) {
        return nullptr;
    }
    fd_set kReadFds;
    fd_set kExceptFds;
    timeval kTimeout;
    FD_ZERO(&kReadFds);
    FD_SET(hSocket, &kReadFds);
    FD_ZERO(&kExceptFds);
    FD_SET(hSocket, &kExceptFds);
    kTimeout.tv_sec = 0;
    kTimeout.tv_usec = 0;
    if (select(static_cast<int32_t>(hSocket + 1), &kReadFds, nullptr, &kExceptFds, &kTimeout) <= 0 ||
        !FD_ISSET(m_socket, &kReadFds)) {
        return nullptr;
    }
    sockaddr_in kAddr;
    int32_t iAddrLen = sizeof(kAddr);
    // Winsock fills the IPv4 address through the generic sockaddr.
    SOCKET hClient = accept(m_socket, reinterpret_cast<sockaddr*>(&kAddr), &iAddrLen);
    hkStringBuf kMessage;
    int32_t iPort = ntohs_0(kAddr.sin_port);
    char* pHost = inet_ntoa_0(kAddr.sin_addr);
    kMessage.printf("Socket got connection from [%s:%d]\n", pHost, iPort);
    char acBuffer[512];
    hkErrStream kStream(acBuffer, 512);
    kStream << kMessage;
    hkError::s_instance
        ->message(hkError::MESSAGE_REPORT, -1, acBuffer, ".\\System\\Io\\Socket\\Bsd\\hkBsdSocket.cpp", 403);
    if (hClient == static_cast<SOCKET>(-1)) {
        return nullptr;
    }
    int32_t iOptval = 1;
    // IPPROTO_TCP, TCP_NODELAY; Winsock takes the option value as untyped bytes.
    setsockopt(hClient, 6, 1, reinterpret_cast<const char*>(&iOptval), 4);
    return new hkBsdSocket(hClient);
}

hkResult hkBsdSocket::setBlocking(hkBool blocking) {
    uint32_t argp;
    argp = blocking.m_bool == 0;
    return static_cast<hkResult>(ioctlsocket(m_socket, -0x7FFB9982, &argp) != 0);
}

hkResult hkBsdSocket::createSocket() {
    close();
    SOCKET v2 = socket(2, 1, 0);
    m_socket = v2;
    return static_cast<hkResult>(v2 == -1);
}
