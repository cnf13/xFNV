#include "Havok/Source/Common/Base/DebugUtil/TraceStream/hkTraceStream.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroupClass.h"

#include <cstdarg>

// Havok SDK signature (hkBase.lib, tied by bytes).
static hkReferencedObject* hkTraceStreamcreate();
static void cleanupLoadedObjecthkTraceStream(void* p);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkTraceStreamTitleTypeInfo = {
    "hkTraceStreamTitle",        nullptr, cleanupLoadedObjecthkTraceStreamTitle, nullptr, &typeid(hkTraceStream::Title),
    sizeof(hkTraceStream::Title)
};

// Reflection of hkTraceStream: Title's 32-character value, the stream's three members (the output stream, the counter
// and the titles), and the type info of the class.
using hkTraceStreamTitleClass_MembersTable = const hkClassMember[1];
static hkTraceStreamTitleClass_MembersTable hkTraceStreamTitleClass_Members{
    {"value",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_CHAR},
     {hkClassMember::TYPE_VOID},
     32,
     {0},
     offsetof(hkTraceStream::Title, m_value),
     nullptr}
};
using hkTraceStreamClass_MembersTable = const hkClassMember[3];
static hkTraceStreamClass_MembersTable hkTraceStreamClass_Members{
    {"stream",
     &hkOstreamClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     offsetof(hkTraceStream, m_stream),
     nullptr},
    {"counter",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     offsetof(hkTraceStream, m_counter),
     nullptr},
    {"titles",
     &hkTraceStreamTitleClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     offsetof(hkTraceStream, m_titles),
     nullptr}
};
hkClass hkTraceStreamTitleClass(
    "hkTraceStreamTitle",
    nullptr,
    sizeof(hkTraceStream::Title),
    nullptr,
    0,
    nullptr,
    0,
    hkTraceStreamTitleClass_Members,
    1,
    nullptr,
    nullptr,
    hkClass::FLAGS_NOT_SERIALIZABLE,
    0
);
hkClass hkTraceStreamClass(
    "hkTraceStream",
    &hkReferencedObjectClass,
    sizeof(hkTraceStream),
    nullptr,
    1,
    nullptr,
    0,
    hkTraceStreamClass_Members,
    3,
    nullptr,
    nullptr,
    hkClass::FLAGS_NOT_SERIALIZABLE,
    0
);
hkTypeInfo hkTraceStreamTypeInfo = {
    "hkTraceStream",          finishLoadedObjecthkTraceStream, cleanupLoadedObjecthkTraceStream,
    getVtablehkTraceStream(), &typeid(hkTraceStream),          sizeof(hkTraceStream)
};

// m_value is untyped storage: populate and depopulate move any singleton's instance pointer through it.
static hkSingletonInitNode hkSingletonRegisterhkTraceStream(
    "hkTraceStream",
    &hkTraceStreamcreate,
    reinterpret_cast<void**>(&hkTraceStream::s_instance)
);

// The singleton's create: the SDK's `new hkTraceStream()`; the allocation stores the memory size.
static hkReferencedObject* hkTraceStreamcreate() {
    hkTraceStream* pStream =
        static_cast<hkTraceStream*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkTraceStream)));
    pStream->m_memSizeAndFlags = sizeof(hkTraceStream);
    return ::new (pStream) hkTraceStream;
}

static void cleanupLoadedObjecthkTraceStream(void* p) {
    static_cast<hkTraceStream*>(p)->~hkTraceStream();
}

const hkClass& hkTraceStream::Title::staticClass() {
    return hkTraceStreamTitleClass;
}

// The title array goes with its member.
hkTraceStream::~hkTraceStream() {}

void hkTraceStream::printf(const char* title, const char* fmt, ...) {
    for (const Title& rTitle : m_titles) {
        if (hkString::strCmp(rTitle.m_value, title) == 0) {
            return;
        }
    }
    ++m_counter;
    // Havok's breakpoint hook: the store changes nothing.
    if (m_counter == 2051) {
        m_counter = 2051;
    }
    if (m_stream) {
        va_list vList;
        va_start(vList, fmt);
        char acBuffer[1024];
        if (fmt[0] == '#') {
            hkString::sprintf(acBuffer, "%4i\t\t", m_counter);
            m_stream->write(acBuffer, hkString::strLen(acBuffer));
            hkString::vsnprintf(acBuffer, 1024, fmt + 1, vList);
        }
        else {
            hkString::vsnprintf(acBuffer, 1024, fmt, vList);
        }
        m_stream->write(acBuffer, hkString::strLen(acBuffer));
        va_end(vList);
    }
}

void hkTraceStream::dontPrintf(const char* title, const char* fmt, ...) {}

const hkClass& hkTraceStream::staticClass() {
    return hkTraceStreamClass;
}
