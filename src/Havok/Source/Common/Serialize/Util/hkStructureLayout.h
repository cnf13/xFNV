#pragma once

#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"

class hkClass;

// The layout rules of a target platform: the pointer size, the byte order and the padding optimisations.
class hkStructureLayout {
public:
    struct LayoutRules {
        // Whether all four bytes equal other's, built in apResult.
        hkBool IsEqual(const hkStructureLayout::LayoutRules& other) const;

        uint8_t m_bytesInPointer;
        uint8_t m_littleEndian;
        uint8_t m_reusePaddingOptimization;
        uint8_t m_emptyBaseClassOptimization;
    };

    hkStructureLayout();
    hkStructureLayout(const LayoutRules& rules);

    // Lays out klass's members under m_rules unless classesDone holds klass already.
    void computeMemberOffsetsInplace(
        hkClass& klass,
        hkPointerMap<const hkClass*, int32_t>& classesDone,
        int32_t recurse
    ) const;

    // The size of a member of the given type under rules.
    static int32_t getMemberSize(const hkClassMember& member, const LayoutRules& rules, hkClassMember::Type type);
    // The alignment of a member of the given type with pointers of bytesInPointer bytes.
    static int32_t
    calculateMemberAlignment(const hkClassMember& member, hkClassMember::Type type, int32_t bytesInPointer);
    // Lays out klass's members under rules, and first those of the struct classes it names when recurse is set;
    // classesDone collects the classes done. Helper of computeMemberOffsetsInplace.
    static void computeClassMemberOffsetsInplace(
        hkClass& klass,
        const LayoutRules& rules,
        hkPointerMap<const hkClass*, int32_t>& classesDone,
        int32_t recurse
    );

    LayoutRules m_rules;

    static hkStructureLayout::LayoutRules HostLayoutRules;
    // The rules of each compiler and platform: bytes in a pointer, little endian, reuse of padding, empty base class
    // optimization.
    static constexpr hkStructureLayout::LayoutRules MsvcWin32LayoutRules{4, 1, 0, 1};
    static constexpr hkStructureLayout::LayoutRules MsvcXboxLayoutRules{4, 1, 0, 1};
    static constexpr hkStructureLayout::LayoutRules MsvcAmd64LayoutRules{8, 1, 0, 1};
    static constexpr hkStructureLayout::LayoutRules CwPs2LayoutRules{4, 1, 0, 1};
    static constexpr hkStructureLayout::LayoutRules CwNgcLayoutRules{4, 0, 0, 1};
    static constexpr hkStructureLayout::LayoutRules CwPspLayoutRules{4, 1, 0, 1};
    static constexpr hkStructureLayout::LayoutRules CwWiiLayoutRules{4, 0, 1, 1};
    static constexpr hkStructureLayout::LayoutRules Gcc32Ps2LayoutRules{4, 1, 1, 1};
    static constexpr hkStructureLayout::LayoutRules Gcc151PspLayoutRules{4, 1, 1, 1};
    static constexpr hkStructureLayout::LayoutRules Gcc295LinuxLayoutRules{4, 1, 0, 0};
    static constexpr hkStructureLayout::LayoutRules Gcc33LinuxLayoutRules{4, 1, 1, 1};
    static constexpr hkStructureLayout::LayoutRules Gcc33Amd64LinuxLayoutRules{8, 1, 1, 1};
    static constexpr hkStructureLayout::LayoutRules Sn31Ps2LayoutRules{4, 1, 0, 0};
    static constexpr hkStructureLayout::LayoutRules Sn10PspLayoutRules{4, 1, 0, 1};
    static constexpr hkStructureLayout::LayoutRules Sn393NgcLayoutRules{4, 0, 0, 0};
    static constexpr hkStructureLayout::LayoutRules Xbox360LayoutRules{4, 0, 0, 1};
    static constexpr hkStructureLayout::LayoutRules GccPs3LayoutRules{4, 0, 1, 1};
    static constexpr hkStructureLayout::LayoutRules Gcc40MacPpcLayoutRules{4, 0, 1, 1};
    static constexpr hkStructureLayout::LayoutRules Gcc40MacIntelLayoutRules{4, 1, 1, 1};
};
