#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Serialize/Resource/hkResource.h"

#include <stdint.h>

// One section's header in a binary packfile: its tag and the offsets, from the section's start, of its data, fixup,
// export and import tables. The headers are 0x30 bytes each, with m_exportsOffset and m_importsOffset at +0x24 and
// +0x28.
class hkPackfileSectionHeader {
public:
    // The reflection's member table.
    using MembersArray = const hkClassMember[9];

    // Appends one export (name, address in the section) per entry of the section's export
    // table: each entry is the data's offset, then its name.
    void getExports(const void* sectionBegin, hkArray<hkResource::Export>& exportsOut) const;
    // Appends one import (name, address of the slot in the section) per entry of the section's import table,
    // m_importsOffset up to m_endOffset: each entry is the slot's offset (-1 ends the table), then its name.
    void getImports(const void* sectionBegin, hkArray<hkResource::Import>& importsOut) const;

    // Havok SDK signature (hkSerialize.lib, tied by target).
    static const hkClass& staticClass();

    char m_sectionTag[19];
    char m_nullByte;
    int32_t m_absoluteDataStart;
    int32_t m_localFixupsOffset;
    int32_t m_globalFixupsOffset;
    int32_t m_virtualFixupsOffset;
    int32_t m_exportsOffset;
    int32_t m_importsOffset;
    int32_t m_endOffset;

    static MembersArray Members;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkPackfileSectionHeaderTypeInfo;
