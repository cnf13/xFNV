#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Io/Reader/hkStreamReader.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"

#include <stdint.h>

// Havok's network socket interface (hkBsdSocket implements it), with a stream reader and a stream writer that read from
// and write to the socket. hkResult is an int (0 is success).
class hkSocket : public hkReferencedObject {
public:
	enum SOCKET_EVENTS : int32_t {
		SOCKET_NOEVENTS = 0x0,
		SOCKET_CAN_READ = 0x1,
		SOCKET_CAN_WRITE = 0x2,
		SOCKET_CONNECT = 0x4,
		SOCKET_CLOSED = 0x8,
		SOCKET_ALLEVENTS = 0xff,
	};

	// The stream reader over the socket: its isOk and read forward to the socket's.
	class ReaderAdapter : public hkStreamReader {
	public:
		// The socket builds its adapters on itself.
		ReaderAdapter(hkSocket* apSocket);
		~ReaderAdapter() override;

		hkBool* isOk(hkBool* arResult) const override;
		int32_t read(void* buf, int32_t nbytes) override;

		hkSocket* m_socket;
	};

	// The stream writer over the socket: its isOk and write forward to the socket's.
	class WriterAdapter : public hkStreamWriter {
	public:
		// The socket builds its adapters on itself.
		WriterAdapter(hkSocket* apSocket);
		~WriterAdapter() override;

		hkBool* isOk(hkBool* arResult) const override;
		int32_t write(const void* buf, int32_t nbytes) override;

		hkSocket* m_socket;
	};

	using s_platformNetInit_t = void (*)();
	// The factory serve() calls to make the server socket (the platform's socket class's create).
	using create_t = hkSocket* (*)();

	hkSocket();
	~hkSocket() override;

	virtual hkBool* isOk(hkBool* arResult) const = 0;
	virtual void close() = 0;
	virtual int32_t read(void* apBuffer, int32_t aiNumBytes) = 0;
	virtual int32_t write(const void* apBuffer, int32_t aiNumBytes) = 0;
	virtual hkResult connect(const char* apServerName, int32_t aiPortNumber) = 0;
	virtual hkResult listen(int32_t aiPortNumber) = 0;

	// The base fails; this is the only virtual function the base implements.
	virtual hkResult asyncSelect(void* notificationHandle, uint32_t message, SOCKET_EVENTS events) {
		return HK_FAILURE;
	}

	virtual hkSocket* pollForNewClient() = 0;

	ReaderAdapter m_reader;
	WriterAdapter m_writer;

	static s_platformNetInit_t s_platformNetInit;
	static inline hkBool s_platformNetInitialized{false};
	static create_t create;
};

hkSocket* hkSocketCreate();
