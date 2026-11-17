#pragma once

#include "Havok/Source/Common/Base/System/Io/Socket/hkSocket.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Reconstruction.hpp"
#include "SDK/Windows/winsock2.h"

// Starts the network layer (WSAStartup). hkBsdNetworkQuit stops it: the exe's is an empty function (cdecl, ret 0,
// 0xDA2DA0).
void hkBsdNetworkInit();
void hkBsdNetworkQuit();
// Whether c is a decimal digit (PDB hkIsDigit). The exe takes c in ecx and the hidden result
// pointer apResult in eax (usercall, ret 0, 0xDA2EB0).
hkBool* hkIsDigit(hkBool* apResult, int32_t c);

// A BSD (Winsock) socket. As in hkSocket, the exe returns an hkBool through a hidden pointer and hkResult as an int
// (0 is success).
class hkBsdSocket : public hkSocket {
public:
	// Havok SDK: unsigned. Winsock's SOCKET is a UINT_PTR: the same 4 bytes in the exe, and on x64 the 8 bytes
	// WS2_32.dll's handles (and INVALID_SOCKET, ~0) have.
	using socket_t = SOCKET;

	hkBsdSocket(socket_t a1);
	~hkBsdSocket() override;

	hkBool* isOk(hkBool* arResult) const override;
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
inline constexpr fnvd::Vtable hkBsdSocket_vftable{};
