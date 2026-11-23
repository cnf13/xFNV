#include "Havok/Source/Common/Base/System/Error/hkError.h"

#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/System/Io/Writer/Buffered/hkBufferedStreamWriter.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "xfnv/Compiler.h"

// m_value is untyped storage: populate and depopulate move any singleton's instance pointer through it.
static hkSingletonInitNode
    hkSingletonRegisterhkError("hkError", nullptr, reinterpret_cast<void**>(&hkError::s_instance));

// The writer takes the last bytes of buf, its size rounded up to 16 (the SDK's HK_NEXT_MULTIPLE_OF), and is handed
// to hkOstream's constructor, which takes the stream's reference on it.
static const int32_t kErrStreamWriterSize = static_cast<int32_t>(
    (sizeof(hkBufferedStreamWriter) + 15) & ~static_cast<size_t>(15)
);

void _hkErrorMessage(const char* c) {
    char buf[512];
    hkErrStream ostr(buf, 512);
    ostr << c;
    if (hkError::s_instance->message(hkError::MESSAGE_ERROR, 0x2636FE25, buf, ".\\System\\Error\\hkError.cpp", 40)) {
        xfnv::DebugTrap();
    }
}

hkError::hkError() {}

hkError::~hkError() {}

void hkError::hkReferenceCountError(const hkReferencedObject* object, const char* methodName) {
    char buf[512];
    hkErrStream ostr(buf, 512);
    int32_t m_referenceCount = object->m_referenceCount;
    ostr << "Reference count error on object " << static_cast<const void*>(object) << " with ref count of "
         << m_referenceCount << " in " << methodName << ".\n"
         << " * Are you calling delete instead of removeReference?\n"
         << " * Have you called removeReference too many times?\n"
         << " * In a multithreaded environment, what is the hkReferencedObject lock mode you "
            "use (see setLockMode())?\n"
         << " * Is this a valid object?\n"
         << " * Do you have more than 32768 references? (unlikely)\n";
    if (hkError::s_instance->message(hkError::MESSAGE_ERROR, 0x2C66F2D8, buf, ".\\System\\Error\\hkError.cpp", 33)) {
        xfnv::DebugTrap();
    }
}

hkErrStream::hkErrStream(void* buf, int32_t bufSize)
  : hkOstream(new (static_cast<char*>(buf) + (bufSize - kErrStreamWriterSize))
                  hkBufferedStreamWriter(buf, bufSize - kErrStreamWriterSize, static_cast<hkBool>(true))) {}
