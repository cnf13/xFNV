#include "Havok/Source/Common/Base/System/Error/hkError.h"

#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/System/Io/Writer/Buffered/hkBufferedStreamWriter.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"

// m_value is untyped storage: populate and depopulate move any singleton's instance pointer through it.
static hkSingletonInitNode hkSingletonRegisterhkError("hkError", nullptr,
	reinterpret_cast<void**>(&hkError::s_instance));

// The writer takes the last bytes of buf, its size rounded up to 16 (the SDK's HK_NEXT_MULTIPLE_OF), and is handed
// to hkOstream's constructor, which takes the stream's reference on it.
static const int32_t kErrStreamWriterSize = static_cast<int32_t>((sizeof(hkBufferedStreamWriter) + 15) & ~size_t(15));

void _hkErrorMessage(const char* c) {
	char buf[512];
	hkErrStream apThis(buf, 512);
	apThis.operator<<(c);
	if (hkError::s_instance->message(hkError::MESSAGE_ERROR, 0x2636FE25, buf, ".\\System\\Error\\hkError.cpp", 40)) {
		__builtin_debugtrap();
	}
}

hkError::hkError() {}

hkError::~hkError() {}

void hkError::hkReferenceCountError(const hkReferencedObject* a1, const char* a2) {
	char buf[512];
	hkErrStream apThis(buf, 512);
	int32_t m_referenceCount = a1->m_referenceCount;
	hkOstream* v2 = &apThis.operator<<("Reference count error on object ");
	hkOstream* v3 = &v2->operator<<(static_cast<const void*>(a1));
	hkOstream* v4 = &v3->operator<<(" with ref count of ");
	hkOstream* v5 = &v4->operator<<(m_referenceCount);
	hkOstream* v6 = &v5->operator<<(" in ");
	hkOstream* v7 = &v6->operator<<(a2);
	hkOstream* v8 = &v7->operator<<(".\n");
	hkOstream* v9 = &v8->operator<<(" * Are you calling delete instead of removeReference?\n");
	hkOstream* v10 = &v9->operator<<(" * Have you called removeReference too many times?\n");
	hkOstream* v11 = &v10->operator<<(" * In a multithreaded environment, what is the hkReferencedObject lock mode you "
									  "use (see setLockMode())?\n");
	hkOstream* v12 = &v11->operator<<(" * Is this a valid object?\n");
	v12->operator<<(" * Do you have more than 32768 references? (unlikely)\n");
	if (hkError::s_instance->message(hkError::MESSAGE_ERROR, 0x2C66F2D8, buf, ".\\System\\Error\\hkError.cpp", 33)) {
		__builtin_debugtrap();
	}
}

hkErrStream::hkErrStream(void* buf, int32_t bufSize)
	: hkOstream(new (static_cast<char*>(buf) + (bufSize - kErrStreamWriterSize))
			  hkBufferedStreamWriter(buf, bufSize - kErrStreamWriterSize, static_cast<hkBool>(1))) {}
