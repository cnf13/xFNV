#include "Havok/Source/Common/Base/System/Io/Socket/hkSocket.h"

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Io/Socket/Bsd/hkBsdSocket.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

hkSocket::s_platformNetInit_t hkSocket::s_platformNetInit{&hkBsdNetworkInit};
hkSocket::create_t hkSocket::create{&hkSocketCreate};

hkSocket::ReaderAdapter::ReaderAdapter(hkSocket* apSocket) : m_socket(apSocket) {}

hkSocket::ReaderAdapter::~ReaderAdapter() {}

hkBool* hkSocket::ReaderAdapter::isOk(hkBool* arResult) const {
	m_socket->isOk(arResult);
	return arResult;
}

int32_t hkSocket::ReaderAdapter::read(void* buf, int32_t nbytes) {
	int32_t v3 = 0;
	if (nbytes <= 0) {
		return nbytes;
	}
	while (1) {
		int32_t v5 = m_socket->read(static_cast<char*>(buf) + v3, nbytes - v3);
		v3 += v5;
		if (!v5) {
			break;
		}
		if (v3 >= nbytes) {
			return nbytes;
		}
	}
	return v3;
}

hkSocket::WriterAdapter::WriterAdapter(hkSocket* apSocket) : m_socket(apSocket) {}

hkSocket::WriterAdapter::~WriterAdapter() {}

hkBool* hkSocket::WriterAdapter::isOk(hkBool* arResult) const {
	m_socket->isOk(arResult);
	return arResult;
}

// Writes all aiNumBytes to the socket, stopping at a write of 0 bytes.
int32_t hkSocket::WriterAdapter::write(const void* buf, int32_t nbytes) {
	int32_t iWritten = 0;
	if (nbytes <= 0) {
		return nbytes;
	}
	while (1) {
		int32_t iCount = m_socket->write(static_cast<const char*>(buf) + iWritten, nbytes - iWritten);
		iWritten += iCount;
		if (!iCount) {
			break;
		}
		if (iWritten >= nbytes) {
			return nbytes;
		}
	}
	return iWritten;
}

// The first socket made starts the platform's network layer.
hkSocket::hkSocket() : m_reader(this), m_writer(this) {
	if (!hkSocket::s_platformNetInitialized.m_bool) {
		if (hkSocket::s_platformNetInit) {
			hkSocket::s_platformNetInit();
			hkSocket::s_platformNetInitialized.m_bool = 1;
		}
	}
}

hkSocket::~hkSocket() {}

hkSocket* hkSocketCreate() {
	hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
	hkBsdSocket* v1 = static_cast<hkBsdSocket*>(Instance->m_heap->blockAlloc(sizeof(hkBsdSocket)));
	v1->m_memSizeAndFlags = sizeof(hkBsdSocket);
	// No socket yet (Havok SDK: the constructor's default, socket_t(-1)): the constructor creates one.
	return ::new (v1) hkBsdSocket(static_cast<hkBsdSocket::socket_t>(-1));
}
