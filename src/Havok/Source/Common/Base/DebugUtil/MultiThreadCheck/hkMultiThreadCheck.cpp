#include "Havok/Source/Common/Base/DebugUtil/MultiThreadCheck/hkMultiThreadCheck.h"

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Thread/CriticalSection/hkCriticalSection.h"

static int64_t getMyThreadId();
static void cleanupLoadedObjecthkMultiThreadCheck(void* p);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkMultiThreadCheckTypeInfo = {
    "hkMultiThreadCheck",
    nullptr,
    cleanupLoadedObjecthkMultiThreadCheck,
    nullptr,
    &typeid(hkMultiThreadCheck),
    sizeof(hkMultiThreadCheck)
};

// Reflection of hkMultiThreadCheck: the AccessType and ReadMode enums and the three members, none of them serialized.
using hkMultiThreadCheckAccessTypeEnumItemsTable = const hkClassEnum::Item[3];
static hkMultiThreadCheckAccessTypeEnumItemsTable hkMultiThreadCheckAccessTypeEnumItems{
    {hkMultiThreadCheck::HK_ACCESS_IGNORE, "HK_ACCESS_IGNORE"},
    {hkMultiThreadCheck::HK_ACCESS_RO, "HK_ACCESS_RO"},
    {hkMultiThreadCheck::HK_ACCESS_RW, "HK_ACCESS_RW"}
};
using hkMultiThreadCheckReadModeEnumItemsTable = const hkClassEnum::Item[2];
static hkMultiThreadCheckReadModeEnumItemsTable hkMultiThreadCheckReadModeEnumItems{
    {hkMultiThreadCheck::THIS_OBJECT_ONLY, "THIS_OBJECT_ONLY"},
    {hkMultiThreadCheck::RECURSIVE, "RECURSIVE"}
};
using hkMultiThreadCheckEnumsTable = const hkClassEnum[2];
static hkMultiThreadCheckEnumsTable hkMultiThreadCheckEnums{
    {"AccessType", hkMultiThreadCheckAccessTypeEnumItems, 3, nullptr, {0}},
    {"ReadMode", hkMultiThreadCheckReadModeEnumItems, 2, nullptr, {0}}
};
using hkMultiThreadCheckClass_MembersTable = const hkClassMember[3];
static hkMultiThreadCheckClass_MembersTable hkMultiThreadCheckClass_Members{
    {"threadId",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT32},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     offsetof(hkMultiThreadCheck, m_threadId),
     nullptr},
    {"markCount",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT16},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     offsetof(hkMultiThreadCheck, m_markCount),
     nullptr},
    {"markBitStack",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT16},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     offsetof(hkMultiThreadCheck, m_markBitStack),
     nullptr}
};
hkClass hkMultiThreadCheckClass(
    "hkMultiThreadCheck",
    nullptr,
    sizeof(hkMultiThreadCheck),
    nullptr,
    0,
    hkMultiThreadCheckEnums,
    2,
    hkMultiThreadCheckClass_Members,
    3,
    nullptr,
    nullptr,
    0,
    0
);

static int64_t getMyThreadId() {
    return GetCurrentThreadId();
}

static void cleanupLoadedObjecthkMultiThreadCheck(void* p) {
    ;
}

void hkMultiThreadCheck::disableChecks() {
    m_markCount |= 0x8000u;
}

void hkMultiThreadCheck::markForReadImpl(ReadMode mode) {
    if ((m_markCount & 0x8000) == 0) {
        uint32_t m_threadId = this->m_threadId;
        if (this->m_threadId == -63 || (m_threadId == -31 || m_threadId == -15)) {
            EnterCriticalSection(s_criticalSection);
            this->m_threadId = mode != RECURSIVE ? -63 : -31;
        }
        else {
            EnterCriticalSection(s_criticalSection);
        }
        m_markBitStack *= 2;
        ++m_markCount;
        LeaveCriticalSection(s_criticalSection);
    }
}

void hkMultiThreadCheck::markForWriteImpl() {
    if ((m_markCount & 0x8000) == 0) {
        _RTL_CRITICAL_SECTION* v2 = s_criticalSection;
        EnterCriticalSection(s_criticalSection);
        uint32_t v3 = getMyThreadId();
        uint16_t m_markBitStack = this->m_markBitStack;
        ++m_markCount;
        bool v5 = m_threadId == -15;
        this->m_markBitStack = 2 * m_markBitStack | 1;
        if (v5) {
            m_threadId = v3;
        }
        LeaveCriticalSection(v2);
    }
}

bool hkMultiThreadCheck::isMarkedForWriteImpl() {
    if ((m_markCount & 0x8000) != 0) {
        return true;
    }
    _RTL_CRITICAL_SECTION* v3 = s_criticalSection;
    EnterCriticalSection(s_criticalSection);
    if (m_threadId == static_cast<uint32_t>(getMyThreadId())) {
        LeaveCriticalSection(v3);
        return true;
    }

    LeaveCriticalSection(v3);
    return false;
}

void hkMultiThreadCheck::unmarkForReadImpl() {
    if ((m_markCount & 0x8000) == 0) {
        _RTL_CRITICAL_SECTION* v2 = s_criticalSection;
        EnterCriticalSection(s_criticalSection);
        m_markBitStack >>= 1;
        if (!--m_markCount) {
            m_threadId = -15;
        }
        LeaveCriticalSection(v2);
    }
}

void hkMultiThreadCheck::unmarkForWriteImpl() {
    if ((m_markCount & 0x8000) == 0) {
        _RTL_CRITICAL_SECTION* v2 = s_criticalSection;
        EnterCriticalSection(s_criticalSection);
        m_markBitStack >>= 1;
        if (!--m_markCount) {
            m_threadId = -15;
        }
        LeaveCriticalSection(v2);
    }
}

// Where the checks are off (the top bit of m_markCount), marks the object unmarked (-15, UNMARKED) with no marks.
void hkMultiThreadCheck::enableChecks() {
    if (m_markCount & 0x8000) {
        m_threadId = -15;
        m_markCount = 0;
    }
}

void hkMultiThreadCheck::reenableChecks() {
    m_markCount &= ~0x8000u;
}

void hkMultiThreadCheck::accessCheck(hkMultiThreadCheck::AccessType type) {
    ;
}

// The engine constructs the check through its constructors; nothing here needs replacing.
void hkMultiThreadCheck::staticInit() {
    hkCriticalSection* pSection = new hkCriticalSection(1000);
    s_criticalSection = pSection ? &pSection->m_section : nullptr;
}

void hkMultiThreadCheck::staticQuit() {
    if (s_criticalSection) {
        // s_criticalSection is the m_section of the hkCriticalSection staticInit allocated, its first member.
        delete reinterpret_cast<hkCriticalSection*>(s_criticalSection);
        s_criticalSection = nullptr;
    }
}

void hkMultiThreadCheck::accessCheckWithParent(
    const hkMultiThreadCheck* parentLock,
    hkMultiThreadCheck::AccessType parentType,
    const hkMultiThreadCheck& lock,
    hkMultiThreadCheck::AccessType type
) {
    ;
}

const hkClass& hkMultiThreadCheck::staticClass() {
    return hkMultiThreadCheckClass;
}
