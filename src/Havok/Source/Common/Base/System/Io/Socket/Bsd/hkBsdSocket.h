#pragma once

#include "Havok/Source/Common/Base/System/Io/Socket/hkSocket.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "SDK/Windows/winsock2.h"

// Starts the network layer (WSAStartup). hkBsdNetworkQuit stops it and does nothing.
void hkBsdNetworkInit();
void hkBsdNetworkQuit();
// Whether c is a decimal digit.
hkBool hkIsDigit(int32_t c);

// A BSD (Winsock) socket. As in hkSocket, hkResult is returned as an int (0 is success).
class hkBsdSocket : public hkSocket {
public:
    // Winsock's SOCKET is a UINT_PTR: the 8 bytes WS2_32.dll's handles (and INVALID_SOCKET, ~0) have (4 in the exe).
    using socket_t = SOCKET;

    hkBsdSocket(socket_t s);
    ~hkBsdSocket() override;

    hkBool isOk() const override;
    void close() override;
    int32_t read(void* buf, int32_t nbytes) override;
    int32_t write(const void* buf, int32_t nbytes) override;
    hkResult connect(const char* servername, int32_t portNumber) override;
    hkResult listen(int32_t port) override;
    hkResult asyncSelect(void* notificationHandle, uint32_t message, SOCKET_EVENTS events) override;
    hkSocket* pollForNewClient() override;

    // Switches the socket between blocking and non-blocking.
    hkResult setBlocking(hkBool blocking);
    // Creates the socket when there is none.
    hkResult createSocket();

    socket_t m_socket;
};

inline hkBool g_defaultPlatformInitOnce{false};
