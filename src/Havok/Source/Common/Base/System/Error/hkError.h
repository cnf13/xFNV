#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Object/hkSingleton.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

// Reports c as an error (id 0x2636FE25, hkError.cpp line 40) through hkError's instance.
void _hkErrorMessage(const char* c);

// Havok's error reporter: reports, warnings, assertions and errors go through message, and ids can be switched on and
// off.
class hkError : public hkReferencedObject, public hkSingleton<hkError> {
public:
    enum Message : int32_t {
        MESSAGE_REPORT = 0x0,
        MESSAGE_WARNING = 0x1,
        MESSAGE_ASSERT = 0x2,
        MESSAGE_ERROR = 0x3,
    };

    hkError();
    ~hkError() override;

    virtual int32_t message(
        hkError::Message aiMessage,
        int32_t aiId,
        const char* apDescription,
        const char* apFile,
        int32_t aiLine
    ) = 0;
    virtual void setEnabled(int32_t aiId, hkBool abEnabled) = 0;
    virtual hkBool isEnabled(int32_t aiId) = 0;
    virtual void enableAll() = 0;
    virtual void sectionBegin(int32_t, const char*) {}
    virtual void sectionEnd() {}

    static void hkReferenceCountError(const hkReferencedObject* a1, const char* a2);

    // hkSingleton<hkError>::s_instance.
    static inline hkError* s_instance;
};

// The signature of the function that receives the error reporter's text.
using hkErrorReportFunction = void (*)(const char* apText, void* apErrorReportObject);

// The output stream the report, warning and assert macros format their text into: an hkOstream over a fixed buffer. It
// adds no members and no virtuals.
class hkErrStream : public hkOstream {
public:
    // A stream with no writer (hkOstream()), safe to destroy: the error macros declare the local, then destroy it and
    // construct it again in place over their buffer. hkErrStream adds no members.
    hkErrStream() = default;
    hkErrStream(void* buf, int32_t bufSize);
};
