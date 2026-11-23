#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/String/hkStringBuf.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/FileSystem/hkFileSystem.h"
#include "Havok/Source/Common/Base/System/Io/Writer/Array/hkArrayStreamWriter.h"
#include "Havok/Source/Common/Base/System/Io/Writer/Buffered/hkBufferedStreamWriter.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

#include <cstdarg>

static void finishLoadedObjecthkOstream(void* p, int32_t finishing);
// Havok SDK signature (hkBase.lib, tied by reloc,target).
static const void* getVtablehkOstream();
static void cleanupLoadedObjecthkOstream(void* p);

hkClass hkOstreamClassType("hkOstream", nullptr, 12, nullptr, 0, nullptr, 0, nullptr, 0, nullptr, nullptr, 0, 0);
hkClass hkOstreamClass(
    "hkOstream",
    &hkReferencedObjectClass,
    12,
    nullptr,
    0,
    nullptr,
    0,
    hkOstream::Members,
    1,
    nullptr,
    nullptr,
    1,
    0
);
hkTypeInfo hkOstreamTypeInfo = {
    "hkOstream",
    finishLoadedObjecthkOstream,
    cleanupLoadedObjecthkOstream,
    getVtablehkOstream(),
    &typeid(hkOstream),
    sizeof(hkOstream)
};

static const void* getVtablehkOstream() {
    return hkGetVtableOf<hkOstream>();
}

static void cleanupLoadedObjecthkOstream(void* p) {
    static_cast<hkOstream*>(p)->~hkOstream();
}

// The type info's finish-loaded-object callback for hkOstream: the null check, then its finish constructor.
static void finishLoadedObjecthkOstream(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag aFlag;
        aFlag.m_finishing = finishing;
        ::new (p) hkOstream(aFlag);
    }
}

hkOstream::MemberArray hkOstream::Members{
    {"writer",
     &hkStreamWriterClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     0,
     0x8,
     nullptr}
};

void writeString(hkStreamWriter* sb, const char* s) {
    if (s) {
        sb->write(s, hkString::strLen(s));
    }
    else {
        sb->write("(null)", 6);
    }
}

hkOstream::hkOstream(const char* apFileName) : m_writer(hkFileSystem::s_instance->openWriter(apFileName)) {}

hkOstream::hkOstream(hkStreamWriter* apWriter) : m_writer(apWriter) {
    if (apWriter != nullptr) {
        apWriter->addReferenceLockUnchecked();
    }
}

hkOstream::hkOstream(hkArray<char, hkContainerHeapAllocator>& arBuf) {
    m_writer = new (hkReferencedObject::newObject(sizeof(hkArrayStreamWriter)))
        hkArrayStreamWriter(&arBuf, hkArrayStreamWriter::ARRAY_BORROW);
}

// A writer on apTrack that borrows it.
hkOstream::hkOstream(hkMemoryTrack* apTrack) {
    m_writer = new (hkReferencedObject::newObject(sizeof(hkMemoryTrackStreamWriter)))
        hkMemoryTrackStreamWriter(apTrack, hkMemoryTrackStreamWriter::TRACK_BORROW);
}

hkOstream::hkOstream(void* apMem, int32_t aiMemSize, bool abIsString) {
    m_writer = new (hkReferencedObject::newObject(sizeof(hkBufferedStreamWriter)))
        hkBufferedStreamWriter(apMem, aiMemSize, static_cast<hkBool>(abIsString));
}

hkOstream::~hkOstream() {
    if (m_writer) {
        m_writer->removeReferenceLockUnchecked();
    }
}

void hkOstream::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
    collector->addReferencedObject("writer", m_writer, 0);
}

const hkClass* hkOstream::getClassType() const {
    return &hkOstreamClassType;
}

hkBool hkOstream::isOk() const {
    return m_writer->isOk();
}

// Writes the pointer as "%p" text to m_writer; returns the stream.
hkOstream& hkOstream::operator<<(const void* apPointer) {
    char acBuffer[10024];
    hkString::snprintf(acBuffer, 10024, "%p", apPointer);
    writeString(this->getStreamWriter(), acBuffer);
    return *this;
}

// Writes "true" or "false" to m_writer; returns the stream.
hkOstream& hkOstream::operator<<(hkBool abValue) {
    const char* v3 = "true";
    if (!abValue.m_bool) {
        v3 = "false";
    }
    writeString(this->getStreamWriter(), v3);
    return *this;
}

// Writes the one byte to m_writer; returns the stream.
hkOstream& hkOstream::operator<<(char acValue) {
    this->getStreamWriter()->write(&acValue, 1);
    return *this;
}

// Writes the string to m_writer; returns the stream.
hkOstream& hkOstream::operator<<(const char* apString) {
    writeString(this->getStreamWriter(), apString);
    return *this;
}

hkOstream& hkOstream::operator<<(int16_t asValue) {
    char acBuffer[10024];
    hkString::snprintf(acBuffer, 0x2728, "%i", asValue);
    writeString(this->getStreamWriter(), acBuffer);
    return *this;
}

hkOstream& hkOstream::operator<<(uint16_t ausValue) {
    char acBuffer[10024];
    hkString::snprintf(acBuffer, 0x2728, "%u", ausValue);
    writeString(this->getStreamWriter(), acBuffer);
    return *this;
}

// Formats the value with "%i" into a local buffer and writes it to m_writer; returns the
// stream.
hkOstream& hkOstream::operator<<(int32_t aiValue) {
    char acBuffer[10024];
    hkString::snprintf(acBuffer, 0x2728, "%i", aiValue);
    writeString(this->getStreamWriter(), acBuffer);
    return *this;
}

// Formats the value with "%u" into a local buffer and writes it to m_writer; returns the
// stream.
hkOstream& hkOstream::operator<<(uint32_t auiValue) {
    char acBuffer[10024];
    hkString::snprintf(acBuffer, 0x2728, "%u", auiValue);
    writeString(this->getStreamWriter(), acBuffer);
    return *this;
}

hkOstream& hkOstream::operator<<(float afValue) {
    char acBuffer[10024];
    hkString::snprintf(acBuffer, 0x2728, "%f", afValue);
    writeString(this->getStreamWriter(), acBuffer);
    return *this;
}

hkOstream& hkOstream::operator<<(int64_t aiValue) {
    char acBuffer[10024];
    hkString::snprintf(acBuffer, 0x2728, "%lli", aiValue);
    writeString(this->getStreamWriter(), acBuffer);
    return *this;
}

hkOstream& hkOstream::operator<<(uint64_t auiValue) {
    char acBuffer[10024];
    hkString::snprintf(acBuffer, 0x2728, "%llu", auiValue);
    writeString(this->getStreamWriter(), acBuffer);
    return *this;
}

// Writes the string's characters to m_writer, or "(null)" for a null string; returns the stream.
hkOstream& hkOstream::operator<<(const hkStringPtr& arString) {
    const char* pcString = arString.cString();
    if (pcString) {
        m_writer->write(pcString, arString.getLength());
    }
    else {
        m_writer->write("(null)", 6);
    }
    return *this;
}

// Writes the string's characters to m_writer; returns the stream.
hkOstream& hkOstream::operator<<(const hkStringBuf& arString) {
    hkStreamWriter* m_writer = this->getStreamWriter();
    if (arString.m_string.begin()) {
        m_writer->write(arString.m_string.begin(), arString.m_string.m_size - 1);
    }
    else {
        m_writer->write("(null)", 6);
    }
    return *this;
}

int32_t hkOstream::write(const char* buf, int32_t nbytes) {
    return this->getStreamWriter()->write(buf, nbytes);
}

// Formats into a 10024-byte stack buffer and writes the text to m_writer.
void hkOstream::printf(const char* fmt, ...) {
    char acBuffer[0x2728];
    va_list kArgs;
    va_start(kArgs, fmt);
    hkString::vsnprintf(acBuffer, 0x2728, fmt, kArgs);
    writeString(m_writer, acBuffer);
    va_end(kArgs);
}

void hkOstream::flush() {
    m_writer->flush();
}

void hkOstream::setStreamWriter(hkStreamWriter* newWriter) {
    newWriter->addReference();
    m_writer->removeReference();
    m_writer = newWriter;
}

const hkClass& hkOstream::staticClass() {
    return hkOstreamClass;
}

// Empties the track.
void hkMemoryTrackStreamWriter::clear() {
    m_track->clear();
}

hkMemoryTrackStreamWriter::~hkMemoryTrackStreamWriter() {
    if (m_ownerShip == TRACK_TAKE && m_track) {
        m_track->~hkMemoryTrack();
        hkMemoryRouter::getInstance().m_heap->blockFree(m_track, sizeof(hkMemoryTrack));
    }
}

const hkClass* hkArrayStreamWriter::getClassType() const {
    return &hkArrayStreamWriterClass;
}
