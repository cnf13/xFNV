#include "Havok/Source/Common/Base/System/Error/hkDefaultError.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/Checking/hkLeakDetectAllocator.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

// The disabled-id map and the section stack start empty through their own constructors.
hkDefaultError::hkDefaultError(hkErrorReportFunction errorReportFunction, void* errorReportObject)
  : m_errorFunction(errorReportFunction), m_errorObject(errorReportObject) {}

// The section stack and the disabled-id map are freed by their own destructors.
hkDefaultError::~hkDefaultError() {}

int32_t
hkDefaultError::message(hkError::Message msg, int32_t id, const char* description, const char* file, int32_t line) {
    int32_t v6 = id;
    if ((id == -1) && m_sectionIds.m_size) {
        v6 = m_sectionIds.m_data[m_sectionIds.m_size - 1];
    }

    if (!isEnabled(v6).m_bool) {
        return 0;
    }
    const char* v9 = "";
    hkBool bIsError;
    bIsError.m_bool = 0;
    switch (msg) {
        case hkError::MESSAGE_REPORT: {
            v9 = "Report";
            break;
        }
        case hkError::MESSAGE_WARNING: {
            v9 = "Warning";
            break;
        }
        case hkError::MESSAGE_ASSERT: {
            v9 = "Assert";
            bIsError.m_bool = 1;
            break;
        }
        case hkError::MESSAGE_ERROR: {
            v9 = "Error";
            bIsError.m_bool = 1;
            break;
        }
        default: {
            break;
        }
    }
    showMessage(v9, v6, description, file, line, bIsError);
    return msg == hkError::MESSAGE_ASSERT || msg == hkError::MESSAGE_ERROR;
}

void hkDefaultError::setEnabled(int32_t id, hkBool enabled) {
    if (enabled.m_bool) {
        m_disabledAssertIds.remove(id);
    }
    else {
        m_disabledAssertIds.insert(id, 1);
    }
}

hkBool hkDefaultError::isEnabled(int32_t aiId) {
    return m_disabledAssertIds.getWithDefault(aiId, 0) == 0;
}

void hkDefaultError::enableAll() {
    m_disabledAssertIds.m_map.clear();
}

void hkDefaultError::sectionBegin(int32_t id, const char* sectionName) {
    m_sectionIds.pushBack(id);
}

void hkDefaultError::sectionEnd() {
    --m_sectionIds.m_size;
}

void hkDefaultError::showMessage(
    const char* what,
    int32_t id,
    const char* desc,
    const char* file,
    int32_t line,
    hkBool stackTrace
) {
    char apcBuffer[2048];
    if (id || file) {
        hkString::snprintf(apcBuffer, 2048, "%s(%d): [0x%08X] %s : %s\n", file, line, id, what, desc);
    }
    else {
        hkString::snprintf(apcBuffer, 2048, "%s\n", desc);
    }
    apcBuffer[2047] = 0;
    m_errorFunction(apcBuffer, m_errorObject);
    if (stackTrace.m_bool) {
        hkUlong trace[20];
        hkStackTracer* pTracer = hkStackTracer::s_instance;
        if (!pTracer) {
            hkStackTracer* pNewTracer = hkStackTracer::create();
            hkSingleton<hkStackTracer>::replaceInstance(pNewTracer);
            pTracer = pNewTracer;
        }
        int32_t iNumTrace = pTracer->getStackTrace(trace, 20);
        if (iNumTrace > 2) {
            m_errorFunction("Stack trace is:\n", m_errorObject);
            // The first two frames are in this file.
            hkStackTracer::s_instance->dumpStackTrace(trace + 2, iNumTrace - 2, m_errorFunction, m_errorObject);
        }
    }
}
