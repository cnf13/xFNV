#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"

#include <stdint.h>

class hkMemoryTrack;
class hkStatisticsCollector;
class hkStringBuf;

void writeString(hkStreamWriter* sb, const char* s);

// Havok's text output stream: formats values and writes them to an hkStreamWriter (m_writer). hkErrStream derives from
// it. The operator<< overloads for signed and unsigned char and char pointers and for double forward to the char, const
// char* and float ones.
class hkOstream : public hkReferencedObject {
public:
    // The member table hkOstreamClass describes the stream with.
    using MemberArray = const hkClassMember[1];

    // A stream with no writer: the error macros declare their hkErrStream local, then construct it again in place after
    // running ~hkOstream, which releases m_writer when set, so the writer starts null.
    hkOstream() : m_writer(nullptr) {}
    hkOstream(const char* apFileName);
    hkOstream(hkStreamWriter* apWriter);
    hkOstream(hkArray<char>& arBuf);
    hkOstream(hkMemoryTrack* apTrack);
    // The finish constructor: the base's default constructor (a reference count of 1); m_writer stays as loaded.
    hkOstream(hkFinishLoadedObjectFlag) {}
    hkOstream(void* apMem, int32_t aiMemSize, bool abIsString);
    ~hkOstream() override;

    // Overrides of the base's two slots, so they add none.
    void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
    const hkClass* getClassType() const override;

    hkBool isOk() const;
    hkOstream& operator<<(const void* apPointer);
    hkOstream& operator<<(hkBool abValue);
    hkOstream& operator<<(char acValue);
    hkOstream& operator<<(const char* apString);
    hkOstream& operator<<(int16_t asValue);
    hkOstream& operator<<(uint16_t ausValue);
    hkOstream& operator<<(int32_t aiValue);
    hkOstream& operator<<(uint32_t auiValue);
    hkOstream& operator<<(float afValue);
    hkOstream& operator<<(int64_t aiValue);
    hkOstream& operator<<(uint64_t auiValue);
    hkOstream& operator<<(const hkStringPtr& arString);
    hkOstream& operator<<(const hkStringBuf& arString);
    // Writes the bytes to the writer; returns how many were written.
    int32_t write(const char* buf, int32_t nbytes);
    void printf(const char* fmt, ...);
    void flush();
    hkStreamWriter* getStreamWriter() {
        return m_writer;
    }
    void setStreamWriter(hkStreamWriter* newWriter);

    static const hkClass& staticClass();

    hkStreamWriter* m_writer;

    static MemberArray Members;
};

// hkOstream is spelled hkOfstream when it writes to a file.
using hkOfstream = hkOstream;

extern hkClass hkOstreamClassType;
extern hkTypeInfo hkOstreamTypeInfo;
extern hkClass hkArrayStreamWriterClass;
extern hkTypeInfo hkTraceStreamTypeInfo;
extern hkClass hkOstreamClass;
