#include "Havok/Source/Common/Serialize/Util/hkStructureLayout.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

namespace hkSerializerHostDetection {
    union EndianChecker {
        uint8_t c[4];
        uint32_t i;
    };
    // The word 1 whose first byte tells the host's byte order.
    static const EndianChecker endianChecker{{1, 0, 0, 0}};
} // namespace hkSerializerHostDetection

hkStructureLayout::LayoutRules hkStructureLayout::HostLayoutRules = {
    sizeof(void*), hkSerializerHostDetection::endianChecker.i == 1, 0, 1
};

hkStructureLayout::hkStructureLayout() : m_rules(hkStructureLayout::HostLayoutRules) {}

hkStructureLayout::hkStructureLayout(const LayoutRules& rules) : m_rules(rules) {}

void hkStructureLayout::computeMemberOffsetsInplace(
    hkClass& klass,
    hkPointerMap<const hkClass*, int32_t>& classesDone,
    int32_t recurse
) const {
    if (!classesDone.isValid(classesDone.findKey(&klass))) {
        hkStructureLayout::computeClassMemberOffsetsInplace(klass, m_rules, classesDone, recurse);
    }
}

int32_t hkStructureLayout::getMemberSize(
    const hkClassMember& member,
    const hkStructureLayout::LayoutRules& rules,
    hkClassMember::Type type
) {
    int32_t result;
    int32_t m_bytesInPointer;
    const hkClassMember* v6;
    int32_t v3 = 0;
    switch (type) {
        case hkClassMember::TYPE_BOOL:
        case hkClassMember::TYPE_CHAR:
        case hkClassMember::TYPE_INT8:
        case hkClassMember::TYPE_UINT8:
        case hkClassMember::TYPE_INT16:
        case hkClassMember::TYPE_UINT16:
        case hkClassMember::TYPE_INT32:
        case hkClassMember::TYPE_UINT32:
        case hkClassMember::TYPE_INT64:
        case hkClassMember::TYPE_UINT64:
        case hkClassMember::TYPE_REAL:
        case hkClassMember::TYPE_VECTOR4:
        case hkClassMember::TYPE_QUATERNION:
        case hkClassMember::TYPE_MATRIX3:
        case hkClassMember::TYPE_ROTATION:
        case hkClassMember::TYPE_QSTRANSFORM:
        case hkClassMember::TYPE_MATRIX4:
        case hkClassMember::TYPE_TRANSFORM:
        case hkClassMember::TYPE_ENUM:
        case hkClassMember::TYPE_FLAGS:
        case hkClassMember::TYPE_HALF: {
            return member.getSizeInBytes();
        }
        case hkClassMember::TYPE_ZERO: {
            return hkStructureLayout::getMemberSize(
                member, rules, static_cast<hkClassMember::Type>(member.m_subtype.m_storage)
            );
        }
        case hkClassMember::TYPE_POINTER:
        case hkClassMember::TYPE_FUNCTIONPOINTER:
        case hkClassMember::TYPE_CSTRING:
        case hkClassMember::TYPE_ULONG:
        case hkClassMember::TYPE_STRINGPTR: {
            m_bytesInPointer = rules.m_bytesInPointer;
            v6 = &member;
            goto LABEL_4;
        }
        case hkClassMember::TYPE_ARRAY: {
            v3 = 4;
            goto LABEL_9;
        }
        case hkClassMember::TYPE_INPLACEARRAY: {
            goto LABEL_16;
        }
        case hkClassMember::TYPE_STRUCT: {
            const hkClass* StructClass;
            v6 = &member;
            StructClass = &member.getStructClass();
            m_bytesInPointer = StructClass->getObjectSize();
        LABEL_4:;
            if (v6->getCstyleArraySize()) {
                return m_bytesInPointer * v6->getCstyleArraySize();
            }

            return m_bytesInPointer;
        }
        case hkClassMember::TYPE_SIMPLEARRAY: {
        LABEL_9:;
            result = v3 + rules.m_bytesInPointer + 4;
            break;
        }
        case hkClassMember::TYPE_HOMOGENEOUSARRAY: {
            result = 2 * rules.m_bytesInPointer + 4;
            break;
        }
        case hkClassMember::TYPE_VARIANT: {
            result = 2 * rules.m_bytesInPointer;
            break;
        }
        default: {
            char buf[512];
            {
                hkErrStream apThis(buf, 512);
                apThis.operator<<("Unknown class member type in structureLayout::getMemberSize().");
                if (hkError::s_instance->message(
                        hkError::MESSAGE_ERROR, 0x50A18B58, buf, ".\\Util\\hkStructureLayout.cpp", 195
                    )) {
                    __builtin_debugtrap();
                }
            }
        LABEL_16:;
            result = 0;
            break;
        }
    }
    return result;
}

int32_t hkStructureLayout::calculateMemberAlignment(
    const hkClassMember& member,
    hkClassMember::Type type,
    int32_t bytesInPointer
) {
    int32_t result = -1;
    switch (type) {
        case hkClassMember::TYPE_VOID:
        case hkClassMember::TYPE_BOOL:
        case hkClassMember::TYPE_CHAR:
        case hkClassMember::TYPE_INT8:
        case hkClassMember::TYPE_UINT8:
        case hkClassMember::TYPE_INT16:
        case hkClassMember::TYPE_UINT16:
        case hkClassMember::TYPE_INT32:
        case hkClassMember::TYPE_UINT32:
        case hkClassMember::TYPE_INT64:
        case hkClassMember::TYPE_UINT64:
        case hkClassMember::TYPE_REAL:
        case hkClassMember::TYPE_VECTOR4:
        case hkClassMember::TYPE_QUATERNION:
        case hkClassMember::TYPE_MATRIX3:
        case hkClassMember::TYPE_ROTATION:
        case hkClassMember::TYPE_QSTRANSFORM:
        case hkClassMember::TYPE_MATRIX4:
        case hkClassMember::TYPE_TRANSFORM:
        case hkClassMember::TYPE_ENUM:
        case hkClassMember::TYPE_FLAGS:
        case hkClassMember::TYPE_HALF: {
            result = member.getAlignment();
            break;
        }
        case hkClassMember::TYPE_ZERO: {
            result = hkStructureLayout::calculateMemberAlignment(
                member, static_cast<hkClassMember::Type>(member.m_subtype.m_storage), bytesInPointer
            );
            break;
        }
        case hkClassMember::TYPE_POINTER:
        case hkClassMember::TYPE_FUNCTIONPOINTER:
        case hkClassMember::TYPE_ARRAY:
        case hkClassMember::TYPE_SIMPLEARRAY:
        case hkClassMember::TYPE_HOMOGENEOUSARRAY:
        case hkClassMember::TYPE_VARIANT:
        case hkClassMember::TYPE_CSTRING:
        case hkClassMember::TYPE_ULONG:
        case hkClassMember::TYPE_STRINGPTR: {
            result = bytesInPointer;
            break;
        }
        case hkClassMember::TYPE_STRUCT: {
            int32_t v4 = 1;
            const hkClass* StructClass = &member.getStructClass();
            for (int32_t i = 0; i < StructClass->getNumMembers(); ++i) {
                const hkClassMember* v7 = &StructClass->getMember(i);
                int32_t v8 = hkStructureLayout::calculateMemberAlignment(
                    *v7, static_cast<hkClassMember::Type>(v7->m_type.m_storage), bytesInPointer
                );
                if (v8 > v4) {
                    v4 = v8;
                }
            }
            result = v4;
            break;
        }
        default: {
            break;
        }
    }
    if ((member.m_flags.get() & 384) != 0 && 8 * ((member.m_flags.get() & 384) == 256) + 8 > result) {
        return 8 * ((member.m_flags.get() & 384) == 256) + 8;
    }
    return result;
}

// The class's hierarchy is laid out base first: interfaces take a pointer each, members go at their alignment, and
// each class's size is rounded up to the largest alignment so far (an empty class takes 1 byte).
void hkStructureLayout::computeClassMemberOffsetsInplace(
    hkClass& klass,
    const LayoutRules& rules,
    hkPointerMap<const hkClass*, int32_t>& classesDone,
    int32_t recurse
) {
    classesDone.insert(&klass, 0);
    if (recurse) {
        int32_t iNumMembers = klass.getNumMembers();
        for (int32_t i = 0; i < iNumMembers; ++i) {
            // Calls the non-const getMember, as the game does.
            hkClassMember& member = klass.getMember(i);
            // EXE-INLINED none: hkClassMember::hasClass
            if ((member.m_class) &&
                (classesDone.m_map.findKey(hkPointerMap<const hkClass*, int32_t>::ToMapInt(&member.getStructClass())) >
                 classesDone.m_map.m_hashMod))
            // EXE-INLINED none: hkPointerMap::hasKey
            {
                computeClassMemberOffsetsInplace(
                    const_cast<hkClass&>(member.getStructClass()), rules, classesDone, recurse
                );
            }
        }
    }
    hkArray<hkClass*, hkContainerTempAllocator> hierarchy;
    for (hkClass* pClass = &klass; pClass; pClass = const_cast<hkClass*>(pClass->getParent())) {
        hierarchy._insertAt(*hkMemoryRouter::getInstance().m_temp, 0, pClass);
    }
    int32_t iBytesInPointer = rules.m_bytesInPointer;
    int32_t iOffset = 0;
    int32_t iStructAlign = 1;
    for (int32_t iClass = 0; iClass < hierarchy.m_size; ++iClass) {
        hkClass* pCurrent = hierarchy.m_data[iClass];
        for (int32_t i = 0; i < pCurrent->getNumDeclaredInterfaces(); ++i) {
            if (iOffset % iBytesInPointer) {
                iOffset += iBytesInPointer - iOffset % iBytesInPointer;
            }
            iOffset += iBytesInPointer;
            if (iBytesInPointer > iStructAlign) {
                iStructAlign = iBytesInPointer;
            }
        }
        for (int32_t i = 0; i < pCurrent->getNumDeclaredMembers(); ++i) {
            hkClassMember& member = const_cast<hkClassMember&>(pCurrent->getDeclaredMember(i));
            if (iOffset == 0 && iClass != 0 && !rules.m_emptyBaseClassOptimization) {
                iOffset = 1;
            }
            int32_t iAlign = calculateMemberAlignment(
                member, static_cast<hkClassMember::Type>(member.m_type.m_storage), rules.m_bytesInPointer
            );
            if (iAlign > iStructAlign) {
                iStructAlign = iAlign;
            }
            if (iOffset % iAlign) {
                iOffset += iAlign - iOffset % iAlign;
            }
            // EXE-INLINED none: hkClassMember::setOffset
            if (member.m_offset != iOffset) {
                member.m_offset = static_cast<uint16_t>(iOffset);
            }
            iOffset += getMemberSize(member, rules, static_cast<hkClassMember::Type>(member.m_type.m_storage));
        }
        int32_t iSize = iOffset;
        if (iOffset % iStructAlign) {
            iSize = iStructAlign - iOffset % iStructAlign + iOffset;
        }
        int32_t iObjectSize = iSize ? iSize : 1;
        if (pCurrent->getObjectSize() != iObjectSize) {
            pCurrent->setObjectSize(iObjectSize);
        }
        if (!rules.m_reusePaddingOptimization) {
            iOffset = iSize;
        }
    }
}
