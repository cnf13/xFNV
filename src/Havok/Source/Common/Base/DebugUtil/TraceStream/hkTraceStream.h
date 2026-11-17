#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Object/hkSingleton.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"

#include <stdint.h>

class hkOstream;

// Havok's debug trace output: printf writes to m_stream unless the title is in m_titles. Only the destructor is
// overridden; it frees m_titles, then the hkReferencedObject part.
class hkTraceStream : public hkReferencedObject, public hkSingleton<hkTraceStream> {
public:
    // A title whose output is switched off: up to 31 characters and the terminator.
    struct Title {
        // Havok SDK signature (hkBase.lib, tied by target).
        static const hkClass& staticClass();

        char m_value[32];
    };

    hkTraceStream() : m_stream(nullptr), m_counter(0) {}

    // The finish constructor sets the reference count to 1; the loaded fields are kept.
    hkTraceStream(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag), m_titles(aFlag) {
        m_referenceCount = 1;
    }

    ~hkTraceStream() override;

    // Writes the formatted text to m_stream unless apTitle is one of m_titles; a format starting with '#' gets the
    // counter first.
    void printf(const char* title, const char* fmt, ...);
    // The release build's printf: does nothing.
    void dontPrintf(const char* title, const char* fmt, ...);

    // The reflection class hkTraceStreamClass.
    static const hkClass& staticClass();

    hkOstream* m_stream;
    int32_t m_counter;
    hkArray<Title> m_titles;

    // hkSingleton<hkTraceStream>::s_instance.
    static inline hkTraceStream* s_instance;
};

extern hkClass hkTraceStreamClass;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkTraceStreamTitleTypeInfo;
