#include "Havok/Source/Common/Serialize/Packfile/Binary/hkPackfileSectionHeader.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroupClass.h"
#include "Havok/Source/Common/Serialize/Resource/hkResource.h"

// The NUL-terminated string at apBase + *apOffset; *apOffset moves
// past it and its terminator, rounded up to 4 bytes (the export and import tables' names).
static const char* readAlignedString(const char* apBase, int32_t* apOffset);
// The int32_t at apBase + *apOffset; *apOffset moves past it (the
// export and import tables' offsets, beside readAlignedString).
static int32_t readInt32(const char* apBase, int32_t* apOffset);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkPackfileSectionHeaderTypeInfo = {
    "hkPackfileSectionHeader",
    nullptr,
    cleanupLoadedObjecthkPackfileSectionHeader,
    nullptr,
    &typeid(hkPackfileSectionHeader),
    sizeof(hkPackfileSectionHeader)
};
hkPackfileSectionHeader::MembersArray hkPackfileSectionHeader::Members{
    {"sectionTag", nullptr, nullptr, {hkClassMember::TYPE_CHAR}, {hkClassMember::TYPE_VOID}, 19, {0}, 0x0, nullptr},
    {"nullByte", nullptr, nullptr, {hkClassMember::TYPE_CHAR}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x13, nullptr},
    {"absoluteDataStart",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x14,
     nullptr},
    {"localFixupsOffset",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x18,
     nullptr},
    {"globalFixupsOffset",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x1C,
     nullptr},
    {"virtualFixupsOffset",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x20,
     nullptr},
    {"exportsOffset", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x24, nullptr},
    {"importsOffset", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x28, nullptr},
    {"endOffset", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x2C, nullptr}
};
hkClass hkPackfileSectionHeaderClass(
    "hkPackfileSectionHeader",
    nullptr,
    0x30,
    nullptr,
    0,
    nullptr,
    0,
    hkPackfileSectionHeader::Members,
    9,
    nullptr,
    nullptr,
    0,
    0
);

static const char* readAlignedString(const char* apBase, int32_t* apOffset) {
    int32_t i;
    const char* result = apBase + *apOffset;
    int32_t v3 = 0;
    if (*result) {
        do {
            ++v3;
        }
        while (result[v3]);
    }
    for (i = v3 + 1; (i & 3) != 0; ++i) {
        ;
    }
    *apOffset += i;
    return result;
}

static int32_t readInt32(const char* apBase, int32_t* apOffset) {
    int32_t iOffset = *apOffset;
    *apOffset += 4;
    return *reinterpret_cast<const int32_t*>(apBase + iOffset);
}

// The export table: (data offset, name) pairs from sectionBegin + m_exportsOffset up to the imports, -1 ending it
// early; each pushed as (name, sectionBegin + offset).
void hkPackfileSectionHeader::getExports(
    const void* sectionBegin,
    hkArray<hkResource::Export, hkContainerHeapAllocator>& exportsOut
) const {
    const char* pTable = static_cast<const char*>(sectionBegin) + m_exportsOffset;
    int32_t iPos = 0;
    if (m_importsOffset - m_exportsOffset > 0) {
        do {
            int32_t iDataOffset = readInt32(pTable, &iPos);
            if (iDataOffset == -1) {
                break;
            }
            const char* pName = readAlignedString(pTable, &iPos);
            hkMemoryRouter& rRouter = hkMemoryRouter::getInstance();
            // EXE-INLINED none: hkArray::expandOne
            if (exportsOut.m_size == (exportsOut.m_capacityAndFlags & hkArrayBase<hkResource::Export>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(*rRouter.m_heap, &exportsOut, sizeof(hkResource::Export));
            }
            hkResource::Export& rExport = exportsOut.m_data[exportsOut.m_size++];
            rExport.name = pName;
            rExport.data = const_cast<char*>(static_cast<const char*>(sectionBegin)) + iDataOffset;
        }
        while (iPos < m_importsOffset - m_exportsOffset);
    }
}

// The import table: (location offset, name) pairs from sectionBegin + m_importsOffset up to the end, -1 ending it
// early; each pushed as (name, sectionBegin + offset).
void hkPackfileSectionHeader::getImports(
    const void* sectionBegin,
    hkArray<hkResource::Import, hkContainerHeapAllocator>& importsOut
) const {
    const char* pTable = static_cast<const char*>(sectionBegin) + m_importsOffset;
    int32_t iPos = 0;
    if (m_endOffset - m_importsOffset > 0) {
        do {
            int32_t iLocationOffset = readInt32(pTable, &iPos);
            if (iLocationOffset == -1) {
                break;
            }
            const char* pName = readAlignedString(pTable, &iPos);
            hkMemoryRouter& rRouter = hkMemoryRouter::getInstance();
            // EXE-INLINED none: hkArray::expandOne
            if (importsOut.m_size == (importsOut.m_capacityAndFlags & hkArrayBase<hkResource::Import>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(*rRouter.m_heap, &importsOut, sizeof(hkResource::Import));
            }
            hkResource::Import& rImport = importsOut.m_data[importsOut.m_size++];
            rImport.name = pName;
            // The section is raw loaded memory: the pointer slot at that offset is untyped bytes there.
            rImport.location =
                reinterpret_cast<void**>(const_cast<char*>(static_cast<const char*>(sectionBegin)) + iLocationOffset);
        }
        while (iPos < m_endOffset - m_importsOffset);
    }
}

const hkClass& hkPackfileSectionHeader::staticClass() {
    return hkPackfileSectionHeaderClass;
}
