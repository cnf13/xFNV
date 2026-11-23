#include "Havok/Source/Common/Serialize/Data/Util/hkDataObjectUtil.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix4.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/MemoryClasses/hkMemoryClasses.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/Attributes/hkAttributes.h"
#include "Havok/Source/Common/Base/Reflection/Registry/hkClassNameRegistry.h"
#include "Havok/Source/Common/Base/Reflection/Registry/hkTypeInfoRegistry.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkCustomAttributes.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Serialize/Data/hkDataArray.h"
#include "Havok/Source/Common/Serialize/Data/hkDataObject.h"
#include "Havok/Source/Common/Serialize/Data/hkDataObjectImpl.h"
#include "Havok/Source/Common/Serialize/Data/hkDataWorld.h"
#include "Havok/Source/Common/Serialize/Packfile/hkPackfileData.h"
#include "Havok/Source/Common/Serialize/Resource/hkObjectResource.h"
#include "Havok/Source/Common/Serialize/Util/hkBuiltinTypeRegistry.h"
#include "Havok/Source/Common/Serialize/Version/hkVersionPatchManager.h"
#include "SDK/MSVC/string.h"

#include <new>

// The function-local static `lut` of hkDataObjectUtil::getTypeFromMemberType, as a file-local global: the hkDataObject
// type a member type maps to, indexed by hkClassMember::Type (TYPE_MAX + 1 entries).
using getTypeFromMemberType_lutArray = const hkDataObject::Type[hkClassMember::TYPE_MAX + 1];
static getTypeFromMemberType_lutArray getTypeFromMemberType_lut{
    hkDataObject::TYPE_VOID,   hkDataObject::TYPE_BYTE,    hkDataObject::TYPE_BYTE,   hkDataObject::TYPE_INT,
    hkDataObject::TYPE_BYTE,   hkDataObject::TYPE_INT,     hkDataObject::TYPE_INT,    hkDataObject::TYPE_INT,
    hkDataObject::TYPE_INT,    hkDataObject::TYPE_INT,     hkDataObject::TYPE_INT,    hkDataObject::TYPE_REAL,
    hkDataObject::TYPE_VEC_4,  hkDataObject::TYPE_VEC_4,   hkDataObject::TYPE_VEC_12, hkDataObject::TYPE_VEC_12,
    hkDataObject::TYPE_VEC_12, hkDataObject::TYPE_VEC_16,  hkDataObject::TYPE_VEC_16, hkDataObject::TYPE_VOID,
    hkDataObject::TYPE_OBJECT, hkDataObject::TYPE_VOID,    hkDataObject::TYPE_ARRAY,  hkDataObject::TYPE_ARRAY,
    hkDataObject::TYPE_VOID,   hkDataObject::TYPE_STRUCT,  hkDataObject::TYPE_ARRAY,  hkDataObject::TYPE_ARRAY_STRUCT,
    hkDataObject::TYPE_OBJECT, hkDataObject::TYPE_CSTRING, hkDataObject::TYPE_INT,    hkDataObject::TYPE_VOID,
    hkDataObject::TYPE_REAL,   hkDataObject::TYPE_CSTRING, hkDataObject::TYPE_VOID
};

namespace hkDataObjectUtil {
    // The data type of a member: its type's entry in getTypeFromMemberType_lut, refined by the subtype when the type is
    // an array. An enum or flags member is refined by its enum's storage type. count sets the tuple bit.
    hkDataObject::Type
    getTypeFromMemberType(hkClassMember::Type mtype, hkClassMember::Type stype, const hkClass* klass, int32_t count) {
        uint32_t type = static_cast<uint32_t>(mtype);
        uint32_t subtype = static_cast<uint32_t>(stype);
        if (type == hkClassMember::TYPE_ENUM || type == hkClassMember::TYPE_FLAGS) {
            type = subtype;
            subtype = 0;
        }
        uint32_t value = static_cast<uint32_t>(getTypeFromMemberType_lut[type]);
        // The switch over the subtype covers the member types up to 0x21; a larger one leaves the array type.
        if (value == hkDataObject::TYPE_ARRAY && subtype <= 0x21) {
            switch (subtype) {
                case hkClassMember::TYPE_UINT8: value = hkDataObject::TYPE_ARRAY_BYTE; break;
                case hkClassMember::TYPE_STRUCT: {
                    // An array of structs is a struct array (TYPE_ARRAY_OBJECT + 1) unless a member is an array, enum
                    // or simple array, which makes it an object array.
                    uint32_t flag = 1;
                    for (int32_t i = 0; i < klass->getNumMembers(); ++i) {
                        uint32_t memberType = klass->getMember(i).m_type.m_storage;
                        if (memberType == hkClassMember::TYPE_ARRAY ||
                            (memberType > hkClassMember::TYPE_STRUCT &&
                             memberType <= hkClassMember::TYPE_HOMOGENEOUSARRAY)) {
                            flag = 0;
                            break;
                        }
                    }
                    value = static_cast<uint32_t>(hkDataObject::TYPE_ARRAY_OBJECT) + flag;
                    break;
                }
                default:
                    value = static_cast<uint32_t>(getTypeFromMemberType_lut[subtype]) | hkDataObject::TYPE_ARRAY;
                    break;
            }
        }
        if (count != 0) {
            value |= hkDataObject::TYPE_TUPLE;
        }
        return static_cast<hkDataObject::Type>(value);
    }
} // namespace hkDataObjectUtil

// Copies src's simple value into the array element value: the integer and real forms through the element's
// assignments, the vector forms through setVec (whose real count is the type's), a data object through a copy in the
// destination world, and a string through the element's assignment.
template <>
void Cloner::copySimpleValue<hkDataArray::Value>(hkDataArray::Value dst, const hkDataArray::Value& src) {
    switch (static_cast<int32_t>(src.getType()) - 1) {
        case 0:
        case 1: dst = src.asInt(); break;
        case 2: dst = src.asReal(); break;
        case 3:
        case 4:
        case 5:
        case 6: {
            int32_t numReals = 4 * static_cast<int32_t>(src.getType()) - 12;
            dst.setVec(src.asVec(numReals), numReals);
            break;
        }
        case 7:
        case 8: {
            hkDataObject object = src.asObject();
            dst = copyObject(object);
            break;
        }
        case 9: dst = src.asString(); break;
    }
}

// Copies src's simple value into the object member value: the integer and real forms through the member's
// assignments, the vector forms through setVec (whose real count is the type's), a data object through a copy in the
// destination world, and a string through the member's assignment.
template <>
void Cloner::copySimpleValue<hkDataObject::Value>(hkDataObject::Value dst, const hkDataObject::Value& src) {
    switch (src.getType()) {
        case hkDataObject::TYPE_BYTE:
        case hkDataObject::TYPE_INT: dst.setInt(src.asInt()); break;
        case hkDataObject::TYPE_REAL: dst.setReal(src.asReal()); break;
        case hkDataObject::TYPE_VEC_4:
        case hkDataObject::TYPE_VEC_8:
        case hkDataObject::TYPE_VEC_12:
        case hkDataObject::TYPE_VEC_16: {
            int32_t numReals = 4 * static_cast<int32_t>(src.getType()) - 12;
            dst.setVec(src.asVec(numReals), numReals);
            break;
        }
        case hkDataObject::TYPE_OBJECT:
        case hkDataObject::TYPE_STRUCT: {
            hkDataObject object = src.asObject();
            dst.setObject(copyObject(object));
            break;
        }
        case hkDataObject::TYPE_CSTRING: dst.setString(src.asString()); break;
        default: break;
    }
}

using hkMapBase_lhkDataObject_nHandle_cvoid_p_cCopier_nHandleOps = hkMapBase<
    hkDataObject::Handle,
    void*,
    Copier::HandleOps>;

using hkMapBase_lhkDataObject_nHandle_chkDataObject_nHandle_cCopier_nHandleOps = hkMapBase<
    hkDataObject::Handle,
    hkDataObject::Handle,
    Copier::HandleOps>;

// getIterator is the SDK's inline body in hkDataObjectUtil.h.
using hkMapBase_lhkDataObject_nHandle_cvoid_p_cCopier_nHandleOps = hkMapBase<
    hkDataObject::Handle,
    void*,
    Copier::HandleOps>;

using hkMapBase_lhkDataObject_nHandle_cvoid_p_cCopier_nHandleOps = hkMapBase<
    hkDataObject::Handle,
    void*,
    Copier::HandleOps>;

using hkMapBase_lhkDataObject_nHandle_cvoid_p_cCopier_nHandleOps = hkMapBase<
    hkDataObject::Handle,
    void*,
    Copier::HandleOps>;

using hkMapBase_lhkDataObject_nHandle_cvoid_p_cCopier_nHandleOps = hkMapBase<
    hkDataObject::Handle,
    void*,
    Copier::HandleOps>;

using hkMapBase_lhkDataObject_nHandle_cvoid_p_cCopier_nHandleOps = hkMapBase<
    hkDataObject::Handle,
    void*,
    Copier::HandleOps>;

using hkMapBase_lhkDataObject_nHandle_cvoid_p_cCopier_nHandleOps = hkMapBase<
    hkDataObject::Handle,
    void*,
    Copier::HandleOps>;

using hkMapBase_lhkDataObject_nHandle_chkDataObject_nHandle_cCopier_nHandleOps = hkMapBase<
    hkDataObject::Handle,
    hkDataObject::Handle,
    Copier::HandleOps>;

using hkMapBase_lhkDataObject_nHandle_chkDataObject_nHandle_cCopier_nHandleOps = hkMapBase<
    hkDataObject::Handle,
    hkDataObject::Handle,
    Copier::HandleOps>;

using hkMapBase_lhkDataObject_nHandle_chkDataObject_nHandle_cCopier_nHandleOps = hkMapBase<
    hkDataObject::Handle,
    hkDataObject::Handle,
    Copier::HandleOps>;

using hkMapBase_lhkDataObject_nHandle_chkDataObject_nHandle_cCopier_nHandleOps = hkMapBase<
    hkDataObject::Handle,
    hkDataObject::Handle,
    Copier::HandleOps>;

// src's native copy as an object resource: a copier over classReg whose copies do not live on the heap.
hkObjectResource* hkDataObjectUtil::toObjectWithRegistry(
    const hkDataObject& src,
    const hkClassNameRegistry* classReg,
    const hkTypeInfoRegistry* typeReg
) {
    Copier copier(classReg, 1);
    return copier.toObject(src, typeReg);
}

// The Copier's handle map lookup. HandleOps' hash is the handle's first word over 16 times the golden ratio, its
// equality both words, an empty slot's first word -1.
void* hkMapBase<hkDataObject::Handle, void*, Copier::HandleOps>::getWithDefault(
    hkDataObject::Handle key,
    void* def
) const {
    if (m_hashMod > 0) {
        for (uint32_t i = static_cast<uint32_t>((reinterpret_cast<uintptr_t>(key.p0) >> 4) * 0x9E3779B1u) & m_hashMod;
             m_elem[i].key.p0 != reinterpret_cast<void*>(static_cast<intptr_t>(-1));
             i = (i + 1) & m_hashMod) {
            if (m_elem[i].key.p0 == key.p0 && m_elem[i].key.p1 == key.p1) {
                return m_elem[i].val;
            }
        }
    }
    return def;
}

int32_t hkMapBase<hkDataObject::Handle, void*, Copier::HandleOps>::insert(
    hkMemoryAllocator& alloc,
    hkDataObject::Handle key,
    void* val
) {
    int32_t numElems = m_numElems & NUM_ELEMS_MASK;
    if (numElems + numElems > m_hashMod) {
        resizeTable(alloc, m_hashMod + m_hashMod + 2);
    }
    uint32_t i;
    int32_t isNewKey = 1;
    for (i = static_cast<uint32_t>((reinterpret_cast<uintptr_t>(key.p0) >> 4) * 0x9E3779B1u) & m_hashMod;
         m_elem[i].key.p0 != reinterpret_cast<void*>(static_cast<intptr_t>(-1));
         i = (i + 1) & m_hashMod) {
        if (m_elem[i].key.p0 == key.p0 && m_elem[i].key.p1 == key.p1) {
            isNewKey = 0;
            break;
        }
    }
    m_numElems += isNewKey;
    m_elem[i].key = key;
    m_elem[i].val = val;
    return isNewKey;
}

void hkMapBase<hkDataObject::Handle, void*, Copier::HandleOps>::reserve(hkMemoryAllocator& alloc, int32_t numElements) {
    int32_t i;
    for (i = 8; i < 2 * numElements; i *= 2) {
        ;
    }
    resizeTable(alloc, i);
}

// Empties every slot: an empty slot's key has p0 all ones (HandleOps' invalid handle).
void hkMapBase<hkDataObject::Handle, void*, Copier::HandleOps>::clear() {
    int32_t v1 = m_hashMod + 1;
    if (v1 > 0) {
        int32_t v2 = 0;
        do {
            m_elem[v2++].key.p0 = reinterpret_cast<void*>(-1);
            --v1;
        }
        while (v1);
    }
    m_numElems &= hkMapBase<hkDataObject::Handle, void*, Copier::HandleOps>::DONT_DEALLOCATE_FLAG;
}

void hkMapBase<hkDataObject::Handle, void*, Copier::HandleOps>::clearAndDeallocate(hkMemoryAllocator& alloc) {
    clear();
    if (m_numElems >= 0) {
        alloc.blockFree(m_elem, sizeof(*m_elem) * (m_hashMod + 1));
    }
}

void hkMapBase<hkDataObject::Handle, void*, Copier::HandleOps>::resizeTable(hkMemoryAllocator& alloc, int32_t newcap) {
    if (newcap <= 8) {
        newcap = 8;
    }
    int32_t dontDeallocate = m_numElems & DONT_DEALLOCATE_FLAG;
    int32_t oldcap = m_hashMod + 1;
    Pair* pOldElem = m_elem;
    m_elem = static_cast<Pair*>(alloc.blockAlloc(sizeof(Pair) * newcap));
    for (int32_t i = 0; i < newcap; ++i) {
        m_elem[i].key.p0 = reinterpret_cast<void*>(static_cast<intptr_t>(-1));
    }
    m_numElems = 0;
    m_hashMod = newcap - 1;
    for (int32_t i = 0; i < oldcap; ++i) {
        if (pOldElem[i].key.p0 != reinterpret_cast<void*>(static_cast<intptr_t>(-1))) {
            insert(alloc, pOldElem[i].key, pOldElem[i].val);
        }
    }
    if (dontDeallocate == 0) {
        alloc.blockFree(pOldElem, sizeof(Pair) * oldcap);
    }
}

// The reference map and the copy stack construct themselves empty; the handle map is emptied here.
Copier::Copier(const hkClassNameRegistry* creg, int32_t allocatedOnHeap) : m_allocatedOnHeap(allocatedOnHeap) {
    if (creg) {
        creg->addReference();
    }
    m_classReg.m_pntr = creg;
    m_copies.m_elem = nullptr;
    m_copies.m_numElems = 0;
    m_copies.m_hashMod = -1;
    m_copies.reserve(*hkMemoryRouter::getInstance().m_heap, 0);
}

// The handle map has no destructor of its own; the other members free themselves.
Copier::~Copier() {
    m_copies.clearAndDeallocate(*hkMemoryRouter::getInstance().m_heap);
}

// An enum or flags member without an enum class is left alone and still succeeds; klass is not read.
hkResult Copier::fillNativeEnumMember(
    void* address,
    const hkClassMember& member,
    const hkDataObject::Value& value,
    const hkClass& klass
) {
    switch (member.m_type.m_storage) {
        case hkClassMember::TYPE_ENUM:
        case hkClassMember::TYPE_FLAGS:
            if (member.m_enum) {
                member.setEnumValue(address, value.asInt());
            }
            return HK_SUCCESS;
    }
    return HK_FAILURE;
}

// Fills the native array member at address (an hkArray, or the storage of a simple array) from srcArray, by the
// member's subtype: the elements in storage from the heap, which the tracked data frees (HK_MEMORY_CLASS_ARRAY). An
// hkArray member's capacity is then its size, with DONT_DEALLOCATE_FLAG unless the copies live on the heap. An empty
// srcArray leaves the member empty.
hkResult Copier::copyIntoNativeArray(void* address, const hkClassMember& member, const hkDataArray& srcArray) {
    hkArrayBase<void>* pArray = static_cast<hkArrayBase<void>*>(address);
    if (srcArray.getSize() == 0) {
        if (member.m_type.m_storage == hkClassMember::TYPE_ARRAY) {
            pArray->m_capacityAndFlags = hkArrayBase<void>::DONT_DEALLOCATE_FLAG;
        }
        return HK_SUCCESS;
    }
    int32_t owningReference = (member.m_flags.m_storage & hkClassMember::NOT_OWNED) ? 0 : 1;
    hkResult result = HK_SUCCESS;
    // The bytes of one element, for the tracked chunk. Unset on the void subtype's path, which does not read it.
    int32_t elementSize;
    switch (member.m_subtype.m_storage) {
        case hkClassMember::TYPE_VOID: goto setCapacity;
        case hkClassMember::TYPE_BOOL: {
            hkArrayBase<uint8_t>* pDst = static_cast<hkArrayBase<uint8_t>*>(address);
            pDst->m_size = srcArray.getSize();
            pDst->m_data = hkMemoryRouter::getInstance().m_heap->_bufAlloc<uint8_t>(pDst->m_size);
            elementSize = sizeof(uint8_t);
            for (int32_t i = 0; i < srcArray.getSize(); ++i) {
                pDst->m_data[i] = srcArray.access(i).asInt() != 0;
            }
            break;
        }
        case hkClassMember::TYPE_CHAR: {
            hkArrayBase<char>* pDst = static_cast<hkArrayBase<char>*>(address);
            pDst->m_size = srcArray.getSize();
            pDst->m_data = hkMemoryRouter::getInstance().m_heap->_bufAlloc<char>(pDst->m_size);
            elementSize = sizeof(char);
            for (int32_t i = 0; i < srcArray.getSize(); ++i) {
                pDst->m_data[i] = static_cast<char>(srcArray.access(i).asInt());
            }
            break;
        }
        case hkClassMember::TYPE_INT8: {
            hkArrayBase<int8_t>* pDst = static_cast<hkArrayBase<int8_t>*>(address);
            pDst->m_size = srcArray.getSize();
            pDst->m_data = hkMemoryRouter::getInstance().m_heap->_bufAlloc<int8_t>(pDst->m_size);
            elementSize = sizeof(int8_t);
            for (int32_t i = 0; i < srcArray.getSize(); ++i) {
                pDst->m_data[i] = static_cast<int8_t>(srcArray.access(i).asInt());
            }
            break;
        }
        case hkClassMember::TYPE_UINT8: {
            hkArrayBase<uint8_t>* pDst = static_cast<hkArrayBase<uint8_t>*>(address);
            pDst->m_size = srcArray.getSize();
            pDst->m_data = hkMemoryRouter::getInstance().m_heap->_bufAlloc<uint8_t>(pDst->m_size);
            elementSize = sizeof(uint8_t);
            for (int32_t i = 0; i < srcArray.getSize(); ++i) {
                pDst->m_data[i] = static_cast<uint8_t>(srcArray.access(i).asInt());
            }
            break;
        }
        case hkClassMember::TYPE_INT16: {
            hkArrayBase<int16_t>* pDst = static_cast<hkArrayBase<int16_t>*>(address);
            pDst->m_size = srcArray.getSize();
            pDst->m_data = hkMemoryRouter::getInstance().m_heap->_bufAlloc<int16_t>(pDst->m_size);
            elementSize = sizeof(int16_t);
            for (int32_t i = 0; i < srcArray.getSize(); ++i) {
                pDst->m_data[i] = static_cast<int16_t>(srcArray.access(i).asInt());
            }
            break;
        }
        case hkClassMember::TYPE_UINT16: {
            hkArrayBase<uint16_t>* pDst = static_cast<hkArrayBase<uint16_t>*>(address);
            pDst->m_size = srcArray.getSize();
            pDst->m_data = hkMemoryRouter::getInstance().m_heap->_bufAlloc<uint16_t>(pDst->m_size);
            elementSize = sizeof(uint16_t);
            for (int32_t i = 0; i < srcArray.getSize(); ++i) {
                pDst->m_data[i] = static_cast<uint16_t>(srcArray.access(i).asInt());
            }
            break;
        }
        case hkClassMember::TYPE_INT32: {
            hkArrayBase<int32_t>* pDst = static_cast<hkArrayBase<int32_t>*>(address);
            pDst->m_size = srcArray.getSize();
            pDst->m_data = hkMemoryRouter::getInstance().m_heap->_bufAlloc<int32_t>(pDst->m_size);
            elementSize = sizeof(int32_t);
            for (int32_t i = 0; i < srcArray.getSize(); ++i) {
                pDst->m_data[i] = srcArray.access(i).asInt();
            }
            break;
        }
        case hkClassMember::TYPE_UINT32: {
            hkArrayBase<uint32_t>* pDst = static_cast<hkArrayBase<uint32_t>*>(address);
            pDst->m_size = srcArray.getSize();
            pDst->m_data = hkMemoryRouter::getInstance().m_heap->_bufAlloc<uint32_t>(pDst->m_size);
            elementSize = sizeof(uint32_t);
            for (int32_t i = 0; i < srcArray.getSize(); ++i) {
                pDst->m_data[i] = static_cast<uint32_t>(srcArray.access(i).asInt());
            }
            break;
        }
        case hkClassMember::TYPE_INT64: {
            hkArrayBase<int64_t>* pDst = static_cast<hkArrayBase<int64_t>*>(address);
            pDst->m_size = srcArray.getSize();
            pDst->m_data = hkMemoryRouter::getInstance().m_heap->_bufAlloc<int64_t>(pDst->m_size);
            elementSize = sizeof(int64_t);
            for (int32_t i = 0; i < srcArray.getSize(); ++i) {
                pDst->m_data[i] = srcArray.access(i).asInt();
            }
            break;
        }
        case hkClassMember::TYPE_UINT64: {
            // The integer is sign-extended, as for the signed type.
            hkArrayBase<uint64_t>* pDst = static_cast<hkArrayBase<uint64_t>*>(address);
            pDst->m_size = srcArray.getSize();
            pDst->m_data = hkMemoryRouter::getInstance().m_heap->_bufAlloc<uint64_t>(pDst->m_size);
            elementSize = sizeof(uint64_t);
            for (int32_t i = 0; i < srcArray.getSize(); ++i) {
                pDst->m_data[i] = static_cast<uint64_t>(static_cast<int64_t>(srcArray.access(i).asInt()));
            }
            break;
        }
        case hkClassMember::TYPE_REAL: {
            hkArrayBase<float>* pDst = static_cast<hkArrayBase<float>*>(address);
            pDst->m_size = srcArray.getSize();
            pDst->m_data = hkMemoryRouter::getInstance().m_heap->_bufAlloc<float>(pDst->m_size);
            elementSize = sizeof(float);
            for (int32_t i = 0; i < srcArray.getSize(); ++i) {
                pDst->m_data[i] = srcArray.access(i).asReal();
            }
            break;
        }
        case hkClassMember::TYPE_VECTOR4:
        case hkClassMember::TYPE_QUATERNION: {
            hkArrayBase<hkVector4>* pDst = static_cast<hkArrayBase<hkVector4>*>(address);
            pDst->m_size = srcArray.getSize();
            pDst->m_data = hkMemoryRouter::getInstance().m_heap->_bufAlloc<hkVector4>(pDst->m_size);
            elementSize = sizeof(hkVector4);
            for (int32_t i = 0; i < srcArray.getSize(); ++i) {
                // asVec's four reals are read as one hkVector4.
                pDst->m_data[i] = *reinterpret_cast<const hkVector4*>(srcArray.access(i).asVec(4));
            }
            break;
        }
        case hkClassMember::TYPE_MATRIX3: {
            hkArrayBase<hkMatrix3>* pDst = static_cast<hkArrayBase<hkMatrix3>*>(address);
            pDst->m_size = srcArray.getSize();
            pDst->m_data = hkMemoryRouter::getInstance().m_heap->_bufAlloc<hkMatrix3>(pDst->m_size);
            elementSize = sizeof(hkMatrix3);
            for (int32_t i = 0; i < srcArray.getSize(); ++i) {
                // The twelve reals returned by asVec are read as an hkMatrix3.
                const hkMatrix3* pValue = reinterpret_cast<const hkMatrix3*>(srcArray.access(i).asVec(12));
                pDst->m_data[i] = *pValue;
            }
            break;
        }
        case hkClassMember::TYPE_ROTATION:
        case hkClassMember::TYPE_QSTRANSFORM: {
            // A rotation is copied with hkQsTransform::operator= too.
            hkArrayBase<hkQsTransform>* pDst = static_cast<hkArrayBase<hkQsTransform>*>(address);
            pDst->m_size = srcArray.getSize();
            pDst->m_data = hkMemoryRouter::getInstance().m_heap->_bufAlloc<hkQsTransform>(pDst->m_size);
            elementSize = sizeof(hkQsTransform);
            for (int32_t i = 0; i < srcArray.getSize(); ++i) {
                // The const float* holding asVec's twelve reals is read as an hkQsTransform.
                const hkQsTransform* pValue = reinterpret_cast<const hkQsTransform*>(srcArray.access(i).asVec(12));
                pDst->m_data[i] = *pValue;
            }
            break;
        }
        case hkClassMember::TYPE_MATRIX4: {
            hkArrayBase<hkMatrix4>* pDst = static_cast<hkArrayBase<hkMatrix4>*>(address);
            pDst->m_size = srcArray.getSize();
            pDst->m_data = hkMemoryRouter::getInstance().m_heap->_bufAlloc<hkMatrix4>(pDst->m_size);
            elementSize = sizeof(hkMatrix4);
            for (int32_t i = 0; i < srcArray.getSize(); ++i) {
                // The sixteen reals returned by asVec are read as an hkMatrix4.
                const hkMatrix4* pValue = reinterpret_cast<const hkMatrix4*>(srcArray.access(i).asVec(16));
                pDst->m_data[i] = *pValue;
            }
            break;
        }
        case hkClassMember::TYPE_TRANSFORM: {
            hkArrayBase<hkTransform>* pDst = static_cast<hkArrayBase<hkTransform>*>(address);
            pDst->m_size = srcArray.getSize();
            pDst->m_data = hkMemoryRouter::getInstance().m_heap->_bufAlloc<hkTransform>(pDst->m_size);
            elementSize = sizeof(hkTransform);
            for (int32_t i = 0; i < srcArray.getSize(); ++i) {
                // The sixteen reals returned by asVec are read as an hkTransform.
                const hkTransform* pValue = reinterpret_cast<const hkTransform*>(srcArray.access(i).asVec(16));
                pDst->m_data[i].Assign(*pValue);
            }
            break;
        }
        case hkClassMember::TYPE_POINTER: {
            // Each non-null object is found or copied; a copy that fails fails the array, but the copying goes on.
            hkArrayBase<void*>* pDst = static_cast<hkArrayBase<void*>*>(address);
            pDst->m_size = srcArray.getSize();
            pDst->m_data = hkMemoryRouter::getInstance().m_heap->_bufAlloc<void*>(pDst->m_size);
            elementSize = sizeof(void*);
            hkString::memSet(pDst->m_data, 0, pDst->m_size * static_cast<int32_t>(sizeof(void*)));
            for (int32_t i = 0; i < srcArray.getSize(); ++i) {
                hkDataObject object = srcArray.access(i).asObject();
                if (!object.isNull()) {
                    hkVariant copy = findOrCopy(object, owningReference);
                    if (copy.m_object) {
                        pDst->m_data[i] = copy.m_object;
                    }
                    else {
                        result = HK_FAILURE;
                    }
                }
            }
            break;
        }
        case hkClassMember::TYPE_STRUCT: {
            // The element class is the first element's; without one the array is left unallocated.
            const hkClass* pClass;
            {
                hkDataObject object = srcArray.access(0).asObject();
                pClass = getMemberClassAndCheck(member, object);
            }
            if (!pClass) {
                goto setCapacity;
            }
            pArray->m_size = srcArray.getSize();
            int32_t numBytes = pClass->getObjectSize() * pArray->m_size;
            pArray->m_data = hkMemoryRouter::getInstance().m_heap->bufAlloc(numBytes);
            elementSize = pClass->getObjectSize();
            hkString::memSet(pArray->m_data, 0, pClass->getObjectSize() * srcArray.getSize());
            for (int32_t i = 0; i < srcArray.getSize() && result == HK_SUCCESS; ++i) {
                void* pElement = static_cast<char*>(pArray->m_data) + pClass->getObjectSize() * i;
                hkDataObject object = srcArray.access(i).asObject();
                result = fillNativeMembers(pElement, object);
            }
            if (elementSize <= 0) {
                goto setCapacity;
            }
            break;
        }
        case hkClassMember::TYPE_VARIANT: {
            hkArrayBase<hkVariant>* pDst = static_cast<hkArrayBase<hkVariant>*>(address);
            pDst->m_size = srcArray.getSize();
            pDst->m_data = hkMemoryRouter::getInstance().m_heap->_bufAlloc<hkVariant>(pDst->m_size);
            elementSize = sizeof(hkVariant);
            hkString::memSet(pDst->m_data, 0, pDst->m_size * static_cast<int32_t>(sizeof(hkVariant)));
            for (int32_t i = 0; i < srcArray.getSize(); ++i) {
                hkDataObject object = srcArray.access(i).asObject();
                if (!object.isNull()) {
                    hkVariant copy = findOrCopy(object, owningReference);
                    if (copy.m_object) {
                        pDst->m_data[i].m_object = copy.m_object;
                        pDst->m_data[i].m_class = copy.m_class;
                    }
                    else {
                        result = HK_FAILURE;
                    }
                }
            }
            break;
        }
        case hkClassMember::TYPE_CSTRING: {
            hkArrayBase<char*>* pDst = static_cast<hkArrayBase<char*>*>(address);
            pDst->m_size = srcArray.getSize();
            pDst->m_data = hkMemoryRouter::getInstance().m_heap->_bufAlloc<char*>(pDst->m_size);
            for (int32_t i = 0; i < srcArray.getSize() && result == HK_SUCCESS; ++i) {
                result = fillNativeArrayElement(
                    &pDst->m_data[i],
                    static_cast<hkClassMember::Type>(member.m_subtype.m_storage),
                    srcArray.access(i),
                    nullptr,
                    owningReference
                );
            }
            // Falls through into the string pointer case, which allocates and fills the array again without freeing the
            // first storage.
            [[fallthrough]];
        }
        case hkClassMember::TYPE_STRINGPTR: {
            hkArrayBase<hkStringPtr>* pDst = static_cast<hkArrayBase<hkStringPtr>*>(address);
            pDst->m_size = srcArray.getSize();
            pDst->m_data = hkMemoryRouter::getInstance().m_heap->_bufAlloc<hkStringPtr>(pDst->m_size);
            elementSize = sizeof(hkStringPtr);
            for (int32_t i = 0; i < srcArray.getSize() && result == HK_SUCCESS; ++i) {
                result = fillNativeArrayElement(
                    &pDst->m_data[i],
                    static_cast<hkClassMember::Type>(member.m_subtype.m_storage),
                    srcArray.access(i),
                    nullptr,
                    owningReference
                );
            }
            break;
        }
        case hkClassMember::TYPE_ULONG: {
            hkArrayBase<uint32_t>* pDst = static_cast<hkArrayBase<uint32_t>*>(address);
            pDst->m_size = srcArray.getSize();
            pDst->m_data = hkMemoryRouter::getInstance().m_heap->_bufAlloc<uint32_t>(pDst->m_size);
            elementSize = sizeof(uint32_t);
            for (int32_t i = 0; i < srcArray.getSize(); ++i) {
                pDst->m_data[i] = static_cast<uint32_t>(srcArray.access(i).asInt());
            }
            break;
        }
        case hkClassMember::TYPE_HALF: {
            hkArrayBase<hkHalf>* pDst = static_cast<hkArrayBase<hkHalf>*>(address);
            pDst->m_size = srcArray.getSize();
            pDst->m_data = hkMemoryRouter::getInstance().m_heap->_bufAlloc<hkHalf>(pDst->m_size);
            elementSize = sizeof(hkHalf);
            for (int32_t i = 0; i < srcArray.getSize(); ++i) {
                pDst->m_data[i] = hkHalf(srcArray.access(i).asReal());
            }
            break;
        }
        default: return HK_FAILURE;
    }
    m_trackedData.m_pntr->addChunk(pArray->m_data, pArray->m_size * elementSize, HK_MEMORY_CLASS_ARRAY);
setCapacity:
    if (member.m_type.m_storage == hkClassMember::TYPE_ARRAY) {
        int32_t size = pArray->m_size;
        pArray->m_capacityAndFlags = size;
        if (!m_allocatedOnHeap) {
            pArray->m_capacityAndFlags = size | hkArrayBase<void>::DONT_DEALLOCATE_FLAG;
        }
    }
    return result;
}

// Fills the members dstObject's class declares from srcObj's members of the same names, then its parent's, and so on. A
// member srcObj lacks is left alone (an hkArray member is marked as not owning its storage); a member type the copier
// cannot fill fails the object. A class the registry does not know leaves the object unfilled.
hkResult Copier::fillNativeMembers(void* dstObject, const hkDataObject& srcObj) {
    hkResult result = HK_SUCCESS;
    const hkClass* klass = m_classReg->getClassByName(srcObj.getClass().getName());
    while (klass) {
        for (int32_t memberIndex = 0; memberIndex < klass->getNumDeclaredMembers(); ++memberIndex) {
            const hkClassMember& member = klass->getDeclaredMember(memberIndex);
            void* address = static_cast<char*>(dstObject) + member.m_offset;
            if (!srcObj.hasMember(member.m_name)) {
                if (member.m_type.m_storage == hkClassMember::TYPE_ARRAY) {
                    static_cast<hkArrayBase<void>*>(address)
                        ->m_capacityAndFlags = hkArrayBase<void>::DONT_DEALLOCATE_FLAG;
                }
                continue;
            }
            // Calls the non-const operator[] on srcObj.
            hkDataObject::Value value = const_cast<hkDataObject&>(srcObj).access(member.m_name);
            int32_t numElements = member.getCstyleArraySize();
            if (numElements == 0) {
                numElements = 1;
            }
            int32_t elementSize = member.getSizeInBytes() / numElements;
            hkClassMember::Type mtype = static_cast<hkClassMember::Type>(member.m_type.m_storage);
            int32_t owningReference = (member.m_flags.m_storage & hkClassMember::NOT_OWNED) ? 0 : 1;
            switch (mtype) {
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
                case hkClassMember::TYPE_STRUCT:
                case hkClassMember::TYPE_VARIANT:
                case hkClassMember::TYPE_CSTRING:
                case hkClassMember::TYPE_ULONG:
                case hkClassMember::TYPE_HALF:
                case hkClassMember::TYPE_STRINGPTR:
                    if (numElements == 1) {
                        result = fillNativeSimpleMember(address, mtype, value, member.getClass(), owningReference);
                    }
                    else {
                        // A C-style array: one element of srcObj's array per slot.
                        hkDataArray array = value.asArray();
                        for (int32_t i = 0; i < array.getSize() && result == HK_SUCCESS; ++i) {
                            result = fillNativeArrayElement(
                                address,
                                mtype,
                                array.access(i),
                                member.getClass(),
                                owningReference
                            );
                            address = static_cast<char*>(address) + elementSize;
                        }
                    }
                    break;
                case hkClassMember::TYPE_POINTER:
                    if (member.m_subtype.m_storage == hkClassMember::TYPE_CHAR) {
                        // A char pointer is a C string.
                        if (numElements == 1) {
                            result = fillNativeSimpleMember(
                                address,
                                hkClassMember::TYPE_CSTRING,
                                value,
                                nullptr,
                                owningReference
                            );
                        }
                        else {
                            hkDataArray array = value.asArray();
                            for (int32_t i = 0; i < array.getSize() && result == HK_SUCCESS; ++i) {
                                result = fillNativeArrayElement(
                                    address,
                                    hkClassMember::TYPE_CSTRING,
                                    array.access(i),
                                    nullptr,
                                    owningReference
                                );
                                address = static_cast<char*>(address) + elementSize;
                            }
                        }
                    }
                    else if (member.m_subtype.m_storage == hkClassMember::TYPE_STRUCT) {
                        if (numElements == 1) {
                            alignas(8) hkDataObject object = value.asObject();
                            result = fillNativeSimpleMember(
                                address,
                                mtype,
                                value,
                                getMemberClassAndCheck(member, object),
                                owningReference
                            );
                        }
                        else {
                            hkDataArray array = value.asArray();
                            for (int32_t i = 0; i < array.getSize() && result == HK_SUCCESS; ++i) {
                                alignas(8) hkDataObject object = array.access(i).asObject();
                                const hkClass* pClass = getMemberClassAndCheck(member, object);
                                result = fillNativeArrayElement(
                                    address,
                                    mtype,
                                    array.access(i),
                                    pClass,
                                    owningReference
                                );
                                address = static_cast<char*>(address) + elementSize;
                            }
                        }
                    }
                    break;
                case hkClassMember::TYPE_ARRAY:
                case hkClassMember::TYPE_SIMPLEARRAY: {
                    // Copies on the heap cannot own storage the tracked data frees: a simple array, or strings and
                    // variants in an array.
                    if (m_allocatedOnHeap &&
                        (mtype == hkClassMember::TYPE_SIMPLEARRAY ||
                         member.m_subtype.m_storage == hkClassMember::TYPE_VARIANT ||
                         member.m_subtype.m_storage == hkClassMember::TYPE_CSTRING)) {
                        return HK_FAILURE;
                    }
                    result = copyIntoNativeArray(address, member, value.asArray());
                    break;
                }
                case hkClassMember::TYPE_ENUM:
                case hkClassMember::TYPE_FLAGS: result = fillNativeEnumMember(address, member, value, *klass); break;
                case hkClassMember::TYPE_HOMOGENEOUSARRAY: {
                    if (m_allocatedOnHeap) {
                        return HK_FAILURE;
                    }
                    // The native homogeneous array: the elements' class, their storage and their count.
                    struct DummyHomogeneousArray {
                        const hkClass* m_class;
                        void* m_data;
                        int32_t m_size;
                    };
                    DummyHomogeneousArray* pDst = static_cast<DummyHomogeneousArray*>(address);
                    hkDataArray array = value.asArray();
                    {
                        alignas(8) hkDataObject object = array.access(0).asObject();
                        pDst->m_class = getMemberClassAndCheck(member, object);
                    }
                    pDst->m_size = array.getSize();
                    int32_t objectSize = pDst->m_class->getObjectSize();
                    alignas(8) int32_t numBytes = pDst->m_size * objectSize;
                    pDst->m_data = hkMemoryRouter::getInstance().m_heap->bufAlloc(numBytes);
                    hkString::memSet(pDst->m_data, 0, numBytes);
                    m_trackedData.m_pntr->addChunk(pDst->m_data, pDst->m_size * objectSize, HK_MEMORY_CLASS_SERIALIZE);
                    int32_t offset = 0;
                    for (int32_t i = 0; i < pDst->m_size && result == HK_SUCCESS; ++i) {
                        void* pElement = static_cast<char*>(pDst->m_data) + offset;
                        alignas(8) hkDataObject object = array.access(i).asObject();
                        result = fillNativeMembers(pElement, object);
                        offset += objectSize;
                    }
                    break;
                }
                default: return HK_FAILURE;
            }
            if (result == HK_FAILURE) {
                return HK_FAILURE;
            }
        }
        klass = klass->getParent();
    }
    return result;
}

// Copies of objects whose class has no name, and objects already on the copy stack (a cycle of owning references
// on the heap), are null. A new copy is zeroed, owned by the tracked data, recorded for obj's handle before its
// members are filled, and dropped (null) when they fail. Owning references to a referenced object are counted while
// the copies live on the heap.
hkVariant Copier::findOrCopy(const hkDataObject& obj, int32_t owningReference) {
    int32_t trackCopy = 0;
    if (m_allocatedOnHeap && owningReference) {
        trackCopy = 1;
    }
    if (obj.isNull()) {
        return hkVariant{nullptr, nullptr};
    }
    bool noClassName = true;
    if (!obj.getClass().isNull()) {
        noClassName = obj.getClass().getName() == nullptr;
    }
    if (noClassName) {
        return hkVariant{nullptr, nullptr};
    }
    if (trackCopy) {
        const hkDataObject::Handle handle = obj.getHandle();
        int32_t index = -1;
        for (int32_t i = 0; i < m_copyStack.m_size; ++i) {
            if (m_copyStack.m_data[i].p0 == handle.p0 && m_copyStack.m_data[i].p1 == handle.p1) {
                index = i;
                break;
            }
        }
        if (index != -1) {
            return hkVariant{nullptr, nullptr};
        }
        const hkDataObject::Handle pushed = obj.getHandle();
        m_copyStack.pushBack(pushed);
    }
    const hkClass* pClass = m_classReg->getClassByName(obj.getClass().getName());
    void* pObject = m_copies.getWithDefault(obj.getHandle(), nullptr);
    if (!pObject) {
        // The versions are read and dropped (the SDK's version check).
        obj.getClass().getVersion();
        pClass->getDescribedVersion();
        int32_t objectSize = pClass->getObjectSize();
        pObject = hkMemoryRouter::getInstance().m_heap->blockAlloc(objectSize);
        m_trackedData.m_pntr->addChunk(pObject, objectSize, HK_MEMORY_CLASS_SERIALIZE);
        hkString::memSet(pObject, 0, pClass->getObjectSize());
        hkDataObject::Handle key = obj.getHandle();
        m_copies.insert(*hkMemoryRouter::getInstance().m_heap, key, pObject);
        if (fillNativeMembers(pObject, obj) == HK_FAILURE) {
            pObject = nullptr;
            pClass = nullptr;
        }
    }
    if (pObject && trackCopy) {
        if (pClass->hasVtable().m_bool) {
            if (hkReferencedObjectClass.isSuperClass(*pClass).m_bool) {
                int32_t refIndex = m_refCount.m_map.findOrInsertKey(
                    *hkMemoryRouter::getInstance().m_heap,
                    m_refCount.ToMapInt(pObject),
                    0
                );
                ++m_refCount.m_map.m_elem[refIndex].val;
            }
        }
    }
    if (trackCopy) {
        --m_copyStack.m_size;
    }
    return hkVariant{pObject, pClass};
}

// obj's native copy as an object resource over the class and type registries. The copies are tracked in new packfile
// data while copying; each referenced object gets the reference count counted for it, and every copy is finished
// through typeReg (a virtual class typeReg cannot finish is warned about). The data then stops tracking: the copies
// are the resource's. Null when obj cannot be copied.
hkObjectResource* Copier::toObject(const hkDataObject& obj, const hkTypeInfoRegistry* typeReg) {
    hkPackfileData* pData = new hkPackfileData;
    // The SDK's hkRefPtr::setAndDontIncrementRefCount.
    if (m_trackedData.m_pntr && m_trackedData.m_pntr != pData) {
        m_trackedData.m_pntr->removeReference();
    }
    m_trackedData.m_pntr = pData;
    hkVariant contents = findOrCopy(obj, 1);
    const hkDataWorld* pWorld = obj.getClass().getWorld();
    if (!contents.m_object) {
        m_trackedData.m_pntr->m_destructorsEnabled = 0;
        if (m_trackedData.m_pntr) {
            m_trackedData.m_pntr->removeReference();
        }
        m_trackedData.m_pntr = nullptr;
        return nullptr;
    }
    // A counted object gets its size and its count.
    int32_t i;
    for (i = 0; i <= m_copies.m_hashMod; ++i) {
        if (m_copies.m_elem[i].key.p0 != reinterpret_cast<void*>(static_cast<intptr_t>(-1))) {
            break;
        }
    }
    while (i <= m_copies.m_hashMod) {
        hkDataObject::Handle handle = m_copies.m_elem[i].key;
        hkDataObject object = pWorld->findObject(handle);
        const hkClass* pClass = m_classReg->getClassByName(object.getClass().getName());
        hkReferencedObject* pCopy = static_cast<hkReferencedObject*>(m_copies.m_elem[i].val);
        uint16_t refCount = static_cast<uint16_t>(m_refCount.getWithDefault(pCopy, 0));
        if (static_cast<int16_t>(refCount) > 0) {
            pCopy->m_memSizeAndFlags = static_cast<uint16_t>(pClass->getObjectSize());
            pCopy->m_referenceCount = static_cast<int16_t>(refCount);
        }
        for (++i; i <= m_copies.m_hashMod; ++i) {
            if (m_copies.m_elem[i].key.p0 != reinterpret_cast<void*>(static_cast<intptr_t>(-1))) {
                break;
            }
        }
    }
    for (i = 0; i <= m_copies.m_hashMod; ++i) {
        if (m_copies.m_elem[i].key.p0 != reinterpret_cast<void*>(static_cast<intptr_t>(-1))) {
            break;
        }
    }
    while (i <= m_copies.m_hashMod) {
        hkDataObject::Handle handle = m_copies.m_elem[i].key;
        hkDataObject object = pWorld->findObject(handle);
        const hkClass* pClass = m_classReg->getClassByName(object.getClass().getName());
        void* pCopy = m_copies.m_elem[i].val;
        const hkTypeInfo* pTypeInfo = typeReg->finishLoadedObject(pCopy, pClass->getName());
        if (pClass->hasVtable().m_bool && !pTypeInfo) {
            char acBuffer[0x200];
            hkErrStream kStream(acBuffer, 0x200);
            kStream << "Cannot finish virtual object of class " << pClass->getName()
                    << ". Please make sure the class is registered.";
            hkError::s_instance
                ->message(hkError::MESSAGE_WARNING, 0x7C303834, acBuffer, ".\\Data\\Util\\hkDataObjectUtil.cpp", 0x133);
        }
        for (++i; i <= m_copies.m_hashMod; ++i) {
            if (m_copies.m_elem[i].key.p0 != reinterpret_cast<void*>(static_cast<intptr_t>(-1))) {
                break;
            }
        }
    }
    hkObjectResource* pResource = new hkObjectResource(contents);
    pResource->setClassNameRegistry(m_classReg.m_pntr);
    pResource->setTypeInfoRegistry(typeReg);
    // Stops tracking allocations: the resource owns the copies.
    hkPackfileData* pTracked = m_trackedData.m_pntr;
    pTracked->m_destructorsEnabled = 0;
    pTracked->m_memory.m_size = 0;
    pTracked->m_chunks.m_size = 0;
    if (m_trackedData.m_pntr) {
        m_trackedData.m_pntr->removeReference();
    }
    m_trackedData.m_pntr = nullptr;
    return pResource;
}

// fillNativeSimpleMember's twin for an array element: the same stores, from an hkDataArray::Value.
hkResult Copier::fillNativeArrayElement(
    void* address,
    hkClassMember::Type mtype,
    const hkDataArray::Value& value,
    const hkClass* structClass,
    int32_t owningReference
) {
    switch (mtype) {
        case hkClassMember::TYPE_BOOL: *static_cast<uint8_t*>(address) = value.asInt() != 0; return HK_SUCCESS;
        case hkClassMember::TYPE_CHAR:
            *static_cast<char*>(address) = static_cast<char>(value.asInt());
            return HK_SUCCESS;
        case hkClassMember::TYPE_INT8:
            *static_cast<int8_t*>(address) = static_cast<int8_t>(value.asInt());
            return HK_SUCCESS;
        case hkClassMember::TYPE_UINT8:
            *static_cast<uint8_t*>(address) = static_cast<uint8_t>(value.asInt());
            return HK_SUCCESS;
        case hkClassMember::TYPE_INT16:
            *static_cast<int16_t*>(address) = static_cast<int16_t>(value.asInt());
            return HK_SUCCESS;
        case hkClassMember::TYPE_UINT16:
            *static_cast<uint16_t*>(address) = static_cast<uint16_t>(value.asInt());
            return HK_SUCCESS;
        case hkClassMember::TYPE_INT32: *static_cast<int32_t*>(address) = value.asInt(); return HK_SUCCESS;
        case hkClassMember::TYPE_UINT32:
        case hkClassMember::TYPE_ULONG:
            *static_cast<uint32_t*>(address) = static_cast<uint32_t>(value.asInt());
            return HK_SUCCESS;
        case hkClassMember::TYPE_INT64:
        case hkClassMember::TYPE_UINT64: *static_cast<int64_t*>(address) = value.asInt(); return HK_SUCCESS;
        case hkClassMember::TYPE_REAL: *static_cast<float*>(address) = value.asReal(); return HK_SUCCESS;
        case hkClassMember::TYPE_HALF: {
            *static_cast<hkHalf*>(address) = hkHalf(value.asReal());
            return HK_SUCCESS;
        }
        case hkClassMember::TYPE_POINTER: {
            if (!structClass) {
                return HK_SUCCESS;
            }
            hkDataObject object = value.asObject();
            if (!object.isNull()) {
                hkVariant copy = findOrCopy(object, owningReference);
                if (!copy.m_object) {
                    return HK_FAILURE;
                }
                *static_cast<void**>(address) = copy.m_object;
            }
            return HK_SUCCESS;
        }
        case hkClassMember::TYPE_STRUCT: {
            if (!structClass) {
                return HK_SUCCESS;
            }
            hkDataObject object = value.asObject();
            bool bFailed = fillNativeMembers(address, object) == HK_FAILURE;
            return bFailed ? HK_FAILURE : HK_SUCCESS;
        }
        case hkClassMember::TYPE_VECTOR4:
        case hkClassMember::TYPE_QUATERNION:
            // asVec's four reals are read as one hkVector4.
            *static_cast<hkVector4*>(address) = *reinterpret_cast<const hkVector4*>(value.asVec(4));
            return HK_SUCCESS;
        case hkClassMember::TYPE_MATRIX3:
            // The twelve reals returned by asVec are read as an hkMatrix3.
            *static_cast<hkMatrix3*>(address) = *reinterpret_cast<const hkMatrix3*>(value.asVec(12));
            return HK_SUCCESS;
        case hkClassMember::TYPE_ROTATION:
        case hkClassMember::TYPE_QSTRANSFORM:
            // A rotation is copied with hkQsTransform::operator= too.
            // The const float* holding asVec's twelve reals is read as an hkQsTransform.
            *static_cast<hkQsTransform*>(address) = *reinterpret_cast<const hkQsTransform*>(value.asVec(12));
            return HK_SUCCESS;
        case hkClassMember::TYPE_MATRIX4:
            // The sixteen reals returned by asVec are read as an hkMatrix4.
            *static_cast<hkMatrix4*>(address) = *reinterpret_cast<const hkMatrix4*>(value.asVec(16));
            return HK_SUCCESS;
        case hkClassMember::TYPE_TRANSFORM:
            // The sixteen reals returned by asVec are read as an hkTransform.
            static_cast<hkTransform*>(address)->Assign(*reinterpret_cast<const hkTransform*>(value.asVec(16)));
            return HK_SUCCESS;
        case hkClassMember::TYPE_CSTRING: {
            if (m_allocatedOnHeap) {
                return HK_FAILURE;
            }
            if (!value.asString()) {
                *static_cast<char**>(address) = nullptr;
                return HK_SUCCESS;
            }
            char* pString = hkString::strDup(value.asString());
            hkArray<void*, hkContainerHeapAllocator>& memory = m_trackedData.m_pntr->m_memory;
            memory.pushBack(pString);
            *static_cast<char**>(address) = pString;
            return HK_SUCCESS;
        }
        case hkClassMember::TYPE_STRINGPTR: {
            char* pString = nullptr;
            if (value.asString()) {
                pString = hkString::strDup(value.asString());
                hkArray<void*, hkContainerHeapAllocator>& memory = m_trackedData.m_pntr->m_memory;
                memory.pushBack(pString);
            }
            hkStringPtr* pDst = static_cast<hkStringPtr*>(address);
            pDst->m_stringAndFlag = pString;
            if (m_allocatedOnHeap) {
                pDst->m_stringAndFlag = reinterpret_cast<const char*>(
                    reinterpret_cast<uintptr_t>(pString) | hkStringPtr::OWNED_FLAG
                );
            }
            return HK_SUCCESS;
        }
        case hkClassMember::TYPE_VARIANT: {
            if (m_allocatedOnHeap) {
                return HK_FAILURE;
            }
            hkDataObject object = value.asObject();
            if (!object.isNull()) {
                hkVariant copy = findOrCopy(object, owningReference);
                if (!copy.m_object) {
                    return HK_FAILURE;
                }
                hkVariant* pDst = static_cast<hkVariant*>(address);
                pDst->m_object = copy.m_object;
                pDst->m_class = copy.m_class;
            }
            return HK_SUCCESS;
        }
        default: return HK_FAILURE;
    }
}

// The integer forms store value's integer at the member's width (a bool as 0 or 1, a 64-bit one sign-extended), the
// real and vector forms copy value's reals, a half the top half of the real's bits. An object pointer or variant is
// the found or new copy of value's object; a struct is filled in place. A string is a copy the tracked data frees
// (an hkStringPtr owns it when the copies live on the heap, where a C string or variant member fails).
hkResult Copier::fillNativeSimpleMember(
    void* address,
    hkClassMember::Type mtype,
    const hkDataObject::Value& value,
    const hkClass* structClass,
    int32_t owningReference
) {
    switch (mtype) {
        case hkClassMember::TYPE_BOOL: *static_cast<uint8_t*>(address) = value.asInt() != 0; return HK_SUCCESS;
        case hkClassMember::TYPE_CHAR:
            *static_cast<char*>(address) = static_cast<char>(value.asInt());
            return HK_SUCCESS;
        case hkClassMember::TYPE_INT8:
            *static_cast<int8_t*>(address) = static_cast<int8_t>(value.asInt());
            return HK_SUCCESS;
        case hkClassMember::TYPE_UINT8:
            *static_cast<uint8_t*>(address) = static_cast<uint8_t>(value.asInt());
            return HK_SUCCESS;
        case hkClassMember::TYPE_INT16:
            *static_cast<int16_t*>(address) = static_cast<int16_t>(value.asInt());
            return HK_SUCCESS;
        case hkClassMember::TYPE_UINT16:
            *static_cast<uint16_t*>(address) = static_cast<uint16_t>(value.asInt());
            return HK_SUCCESS;
        case hkClassMember::TYPE_INT32: *static_cast<int32_t*>(address) = value.asInt(); return HK_SUCCESS;
        case hkClassMember::TYPE_UINT32:
        case hkClassMember::TYPE_ULONG:
            *static_cast<uint32_t*>(address) = static_cast<uint32_t>(value.asInt());
            return HK_SUCCESS;
        case hkClassMember::TYPE_INT64:
        case hkClassMember::TYPE_UINT64: *static_cast<int64_t*>(address) = value.asInt(); return HK_SUCCESS;
        case hkClassMember::TYPE_REAL: *static_cast<float*>(address) = value.asReal(); return HK_SUCCESS;
        case hkClassMember::TYPE_HALF: {
            *static_cast<hkHalf*>(address) = hkHalf(value.asReal());
            return HK_SUCCESS;
        }
        case hkClassMember::TYPE_POINTER: {
            if (!structClass) {
                return HK_SUCCESS;
            }
            hkDataObject object = value.asObject();
            if (!object.isNull()) {
                hkVariant copy = findOrCopy(object, owningReference);
                if (!copy.m_object) {
                    return HK_FAILURE;
                }
                *static_cast<void**>(address) = copy.m_object;
            }
            return HK_SUCCESS;
        }
        case hkClassMember::TYPE_STRUCT: {
            if (!structClass) {
                return HK_SUCCESS;
            }
            hkDataObject object = value.asObject();
            bool bFailed = fillNativeMembers(address, object) == HK_FAILURE;
            return bFailed ? HK_FAILURE : HK_SUCCESS;
        }
        case hkClassMember::TYPE_VECTOR4:
        case hkClassMember::TYPE_QUATERNION:
            // asVec's four reals are read as one hkVector4.
            *static_cast<hkVector4*>(address) = *reinterpret_cast<const hkVector4*>(value.asVec(4));
            return HK_SUCCESS;
        case hkClassMember::TYPE_MATRIX3:
            // The twelve reals returned by asVec are read as an hkMatrix3.
            *static_cast<hkMatrix3*>(address) = *reinterpret_cast<const hkMatrix3*>(value.asVec(12));
            return HK_SUCCESS;
        case hkClassMember::TYPE_ROTATION:
        case hkClassMember::TYPE_QSTRANSFORM:
            // A rotation is copied with hkQsTransform::operator= too.
            // The const float* holding asVec's twelve reals is read as an hkQsTransform.
            *static_cast<hkQsTransform*>(address) = *reinterpret_cast<const hkQsTransform*>(value.asVec(12));
            return HK_SUCCESS;
        case hkClassMember::TYPE_MATRIX4:
            // The sixteen reals returned by asVec are read as an hkMatrix4.
            *static_cast<hkMatrix4*>(address) = *reinterpret_cast<const hkMatrix4*>(value.asVec(16));
            return HK_SUCCESS;
        case hkClassMember::TYPE_TRANSFORM:
            // The sixteen reals returned by asVec are read as an hkTransform.
            static_cast<hkTransform*>(address)->Assign(*reinterpret_cast<const hkTransform*>(value.asVec(16)));
            return HK_SUCCESS;
        case hkClassMember::TYPE_CSTRING: {
            if (m_allocatedOnHeap) {
                return HK_FAILURE;
            }
            if (!value.asString()) {
                *static_cast<char**>(address) = nullptr;
                return HK_SUCCESS;
            }
            char* pString = hkString::strDup(value.asString());
            hkArray<void*, hkContainerHeapAllocator>& memory = m_trackedData.m_pntr->m_memory;
            memory.pushBack(pString);
            *static_cast<char**>(address) = pString;
            return HK_SUCCESS;
        }
        case hkClassMember::TYPE_STRINGPTR: {
            char* pString = nullptr;
            if (value.asString()) {
                pString = hkString::strDup(value.asString());
                hkArray<void*, hkContainerHeapAllocator>& memory = m_trackedData.m_pntr->m_memory;
                memory.pushBack(pString);
            }
            hkStringPtr* pDst = static_cast<hkStringPtr*>(address);
            pDst->m_stringAndFlag = pString;
            if (m_allocatedOnHeap) {
                pDst->m_stringAndFlag = reinterpret_cast<const char*>(
                    reinterpret_cast<uintptr_t>(pString) | hkStringPtr::OWNED_FLAG
                );
            }
            return HK_SUCCESS;
        }
        case hkClassMember::TYPE_VARIANT: {
            if (m_allocatedOnHeap) {
                return HK_FAILURE;
            }
            hkDataObject object = value.asObject();
            if (!object.isNull()) {
                hkVariant copy = findOrCopy(object, owningReference);
                if (!copy.m_object) {
                    return HK_FAILURE;
                }
                hkVariant* pDst = static_cast<hkVariant*>(address);
                pDst->m_object = copy.m_object;
                pDst->m_class = copy.m_class;
            }
            return HK_SUCCESS;
        }
        default: return HK_FAILURE;
    }
}

// The native class of the member's object: null for a null object or a class the registry lacks. A member with a
// class gets the object's class only where that derives from the member's (or from the class its hk.DataObjectType
// attribute names); otherwise the member's (or the attribute's) class.
const hkClass* Copier::getMemberClassAndCheck(const hkClassMember& member, const hkDataObject& obj) {
    if (obj.isNull()) {
        return nullptr;
    }
    const hkClass* pMemberClass = member.getClass();
    const char* pClassName = obj.getClass().getName();
    const hkClass* pObjectClass = pClassName ? m_classReg->getClassByName(pClassName) : nullptr;
    if (!pMemberClass) {
        return pObjectClass;
    }
    if (!pObjectClass) {
        return nullptr;
    }
    if (hkString::strCmp(pMemberClass->getName(), pClassName) == 0) {
        return pMemberClass;
    }
    if (pMemberClass->isSuperClass(*pObjectClass).m_bool) {
        return pObjectClass;
    }
    const hkVariant* pAttribute = member.getAttribute("hk.DataObjectType");
    if (!pAttribute) {
        return pMemberClass;
    }
    const hkClass* pTypeClass = m_classReg->getClassByName(
        static_cast<const hkDataObjectTypeAttribute*>(pAttribute->m_object)->m_typeName
    );
    return pTypeClass->isSuperClass(*pObjectClass).m_bool ? pObjectClass : pTypeClass;
}

int32_t hkMapBase<hkDataObject::Handle, hkDataObject::Handle, Copier::HandleOps>::insert(
    hkMemoryAllocator& alloc,
    hkDataObject::Handle key,
    hkDataObject::Handle val
) {
    int32_t numElems = m_numElems & NUM_ELEMS_MASK;
    if (numElems + numElems > m_hashMod) {
        resizeTable(alloc, m_hashMod + m_hashMod + 2);
    }
    uint32_t i;
    int32_t isNewKey = 1;
    for (i = static_cast<uint32_t>((reinterpret_cast<uintptr_t>(key.p0) >> 4) * 0x9E3779B1u) & m_hashMod;
         m_elem[i].key.p0 != reinterpret_cast<void*>(static_cast<intptr_t>(-1));
         i = (i + 1) & m_hashMod) {
        if (m_elem[i].key.p0 == key.p0 && m_elem[i].key.p1 == key.p1) {
            isNewKey = 0;
            break;
        }
    }
    m_numElems += isNewKey;
    m_elem[i].key = key;
    m_elem[i].val = val;
    return isNewKey;
}

// Returns the slot index of key, or m_hashMod + 1 when it is absent. The Iterator is no pointer but carries the index
// (HK_MAP_INDEX_TO_ITERATOR).
hkMapBase<hkDataObject::Handle, hkDataObject::Handle, Copier::HandleOps>::Iterator
hkMapBase<hkDataObject::Handle, hkDataObject::Handle, Copier::HandleOps>::findKey(hkDataObject::Handle key) const {
    int32_t hashMod = m_hashMod;
    if (hashMod > 0) {
        for (uint32_t i = static_cast<uint32_t>((reinterpret_cast<uintptr_t>(key.p0) >> 4) * 0x9E3779B1u) & hashMod;
             m_elem[i].key.p0 != reinterpret_cast<void*>(static_cast<intptr_t>(-1));
             i = (i + 1) & hashMod) {
            if (m_elem[i].key.p0 == key.p0 && m_elem[i].key.p1 == key.p1) {
                return reinterpret_cast<Iterator>(static_cast<uintptr_t>(i));
            }
        }
    }
    return reinterpret_cast<Iterator>(static_cast<uintptr_t>(hashMod + 1));
}

int32_t hkMapBase<hkDataObject::Handle, hkDataObject::Handle, Copier::HandleOps>::get(
    hkDataObject::Handle key,
    hkDataObject::Handle* out
) const {
    // The slot index findKey's Iterator carries (HK_MAP_ITERATOR_TO_INDEX).
    int32_t v4 = static_cast<int32_t>(reinterpret_cast<uintptr_t>(findKey(key)));
    if (v4 > m_hashMod) {
        return 1;
    }
    *out = m_elem[v4].val;
    return 0;
}

// The SDK's reserve: the table's capacity doubled from 8 until it holds twice numElements.
void hkMapBase<hkDataObject::Handle, hkDataObject::Handle, Copier::HandleOps>::reserve(
    hkMemoryAllocator& alloc,
    int32_t numElements
) {
    int32_t iCapacity;
    for (iCapacity = 8; iCapacity < 2 * numElements; iCapacity *= 2) {}
    resizeTable(alloc, iCapacity);
}

// The SDK's clearAndDeallocate: every slot emptied, then the table freed unless the map does not own it.
void hkMapBase<hkDataObject::Handle, hkDataObject::Handle, Copier::HandleOps>::clearAndDeallocate(
    hkMemoryAllocator& alloc
) {
    for (int32_t i = 0; i < m_hashMod + 1; ++i) {
        m_elem[i].key.p0 = reinterpret_cast<void*>(static_cast<intptr_t>(-1));
    }
    m_numElems &= DONT_DEALLOCATE_FLAG;
    if (m_numElems >= 0) {
        alloc.blockFree(m_elem, sizeof(Pair) * (m_hashMod + 1));
    }
}

void hkMapBase<hkDataObject::Handle, hkDataObject::Handle, Copier::HandleOps>::resizeTable(
    hkMemoryAllocator& alloc,
    int32_t newcap
) {
    if (newcap <= 8) {
        newcap = 8;
    }
    int32_t dontDeallocate = m_numElems & DONT_DEALLOCATE_FLAG;
    int32_t oldcap = m_hashMod + 1;
    Pair* pOldElem = m_elem;
    m_elem = static_cast<Pair*>(alloc.blockAlloc(sizeof(Pair) * newcap));
    for (int32_t i = 0; i < newcap; ++i) {
        m_elem[i].key.p0 = reinterpret_cast<void*>(static_cast<intptr_t>(-1));
    }
    m_numElems = 0;
    m_hashMod = newcap - 1;
    for (int32_t i = 0; i < oldcap; ++i) {
        if (pOldElem[i].key.p0 != reinterpret_cast<void*>(static_cast<intptr_t>(-1))) {
            insert(alloc, pOldElem[i].key, pOldElem[i].val);
        }
    }
    if (dontDeallocate == 0) {
        alloc.blockFree(pOldElem, sizeof(Pair) * oldcap);
    }
}

// A null object gives a null object; an object copied before gives the destination world's object of the handle
// recorded for it (the map's get gives 0 when it has the key); any other is created in the destination world from the
// destination class of its class's name, recorded, and its members copied.
hkDataObject Cloner::copyObject(const hkDataObject& src) {
    if (src.isNull()) {
        return hkDataObject(static_cast<hkDataObjectImpl*>(nullptr));
    }
    hkDataObject::Handle copiedHandle;
    if (!m_copied.get(src.getHandle(), &copiedHandle)) {
        return hkDataObject(m_dstWorld->findObject(copiedHandle));
    }
    const char* name = src.getClass().getName();
    hkDataClass dstClass(findDestClass(name));
    hkDataObject copy(m_dstWorld->newObject(dstClass));
    const hkDataObject::Handle copyKey = copy.getHandle();
    const hkDataObject::Handle srcKey = src.getHandle();
    hkMemoryAllocator& heap = *hkMemoryRouter::getInstance().m_heap;
    m_copied.insert(heap, srcKey, copyKey);
    copyObjectMembers(copy, src);
    return hkDataObject(copy);
}

// The SDK's findDestClass: the destination world's class called name; when it has none, name's source class and its
// parents, up to the first one the destination world has, are described into a temp array of Cinfo and created there
// base first, then the classes of the source class's members likewise, and the destination world is asked again.
// The source classes' destructors are empty functions.
hkDataClassImpl* Cloner::findDestClass(const char* classname) {
    hkDataClassImpl* pDstClass = m_dstWorld->findClass(classname);
    if (pDstClass) {
        return pDstClass;
    }
    // The SDK's hkArray<hkDataClass::Cinfo, hkContainerTempAllocator> cinfos, then cinfos.reserve(10).
    hkArrayBase<hkDataClass::Cinfo> cinfos;
    cinfos.m_data = nullptr;
    cinfos.m_size = 0;
    cinfos.m_capacityAndFlags = hkArrayBase<hkDataClass::Cinfo>::DONT_DEALLOCATE_FLAG;
    cinfos._reserve(hkMemoryRouter::getInstance().temp(), 10);
    const char* pName = classname;
    while (pName && !m_dstWorld->findClass(pName)) {
        hkDataClass srcClass(m_srcWorld->findClass(pName));
        hkDataClass::Cinfo& rCinfo = cinfos._expandOne(*hkMemoryRouter::getInstance().m_temp);
        srcClass.getCinfo(rCinfo);
        if (srcClass.getParent().isNull()) {
            pName = nullptr;
        }
        else {
            pName = srcClass.getParent().getName();
        }
    }
    for (int32_t i = cinfos.m_size - 1; i >= 0; --i) {
        m_dstWorld->newClass(cinfos.m_data[i]);
    }
    hkDataClassImpl* pSrcClass = m_srcWorld->findClass(classname);
    for (int32_t i = 0; i < pSrcClass->getNumMembers(); ++i) {
        hkDataClass::MemberInfo info;
        std::memset(&info, 0, sizeof(info));
        pSrcClass->getMemberInfo(i, info);
        if (info.m_class) {
            findDestClass(info.m_class->getName());
        }
    }
    pDstClass = m_dstWorld->findClass(classname);
    // The SDK's ~hkArray<hkDataClass::Cinfo, hkContainerTempAllocator>, inline: each element's members array goes
    // back to the heap, then the storage to the temp allocator.
    hkMemoryAllocator* pTemp = hkMemoryRouter::getInstance().m_temp;
    for (hkDataClass::Cinfo& rCinfo : cinfos) {
        hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
        hkArray<hkDataClass::Cinfo::Member>& rMembers = rCinfo.members;
        int32_t iMembersCapacity = rMembers.m_capacityAndFlags;
        rMembers.m_size = 0;
        if (iMembersCapacity >= 0) {
            pHeap->bufFree(
                rMembers.m_data,
                sizeof(hkDataClass::Cinfo::Member) *
                    (iMembersCapacity & hkArrayBase<hkDataClass::Cinfo::Member>::CAPACITY_MASK)
            );
        }
        rMembers.m_data = nullptr;
        rMembers.m_capacityAndFlags = hkArrayBase<hkDataClass::Cinfo::Member>::DONT_DEALLOCATE_FLAG;
    }
    cinfos.m_size = 0;
    if (cinfos.m_capacityAndFlags >= 0) {
        pTemp->bufFree(cinfos.m_data, sizeof(hkDataClass::Cinfo) * cinfos.getCapacity());
    }
    return pDstClass;
}

void Cloner::copyValue(hkDataObject& dstObj, const hkDataObject& srcObj, const hkDataClass::MemberInfo& minfo) {
    if (minfo.m_type & hkDataObject::TYPE_TUPLE) {
        hkDataArray srcArray = srcObj.access(minfo.m_name).asArray();
        hkDataArrayImpl* pImpl = dstObj.getClass().getWorld()->newArray(dstObj, minfo);
        hkDataArray dstArray(pImpl);
        dstArray.setSize(minfo.m_tupleCount);
        copyArray(dstArray, srcArray, minfo.m_tupleCount);
    }
    else if (minfo.m_type & hkDataObject::TYPE_ARRAY) {
        hkDataArray srcArray = srcObj.access(minfo.m_name).asArray();
        hkDataArrayImpl* pImpl = dstObj.getClass().getWorld()->newArray(dstObj, minfo);
        hkDataArray dstArray(pImpl);
        dstArray.setSize(srcArray.getSize());
        copyArray(dstArray, srcArray, srcArray.getSize());
    }
    else {
        hkDataObject::Value value = dstObj.access(minfo.m_name);
        copySimpleValue<hkDataObject::Value>(value, srcObj.access(minfo.m_name));
    }
}

// Each element is read from src before dst's element is fetched. A struct array copies member by member through
// swizzled member arrays; a tuple or array member's element count is dst's element array's size for an array member,
// else the tuple count. The member infos live in a temp-allocated array.
void Cloner::copyArray(hkDataArray& dst, const hkDataArray& src, int32_t arraySize) {
    switch (src.getType()) {
        case hkDataObject::TYPE_BYTE:
        case hkDataObject::TYPE_INT:
            for (int32_t i = 0; i < arraySize; ++i) {
                int32_t iValue = src.access(i).asInt();
                dst.access(i) = iValue;
            }
            break;
        case hkDataObject::TYPE_REAL:
            for (int32_t i = 0; i < arraySize; ++i) {
                float fValue = src.access(i).asReal();
                dst.access(i) = fValue;
            }
            break;
        case hkDataObject::TYPE_VEC_4:
        case hkDataObject::TYPE_VEC_8:
        case hkDataObject::TYPE_VEC_12:
        case hkDataObject::TYPE_VEC_16: {
            int32_t iNumReals = 4 * static_cast<int32_t>(src.getType()) - 12;
            for (int32_t i = 0; i < arraySize; ++i) {
                const float* pValue = src.access(i).asVec(iNumReals);
                dst.access(i).setVec(pValue, iNumReals);
            }
            break;
        }
        case hkDataObject::TYPE_OBJECT:
            dst.setSize(arraySize);
            for (int32_t i = 0; i < arraySize; ++i) {
                alignas(8) hkDataObject object = src.access(i).asObject();
                alignas(8) hkDataObject copy = copyObject(object);
                dst.access(i) = copy;
            }
            break;
        case hkDataObject::TYPE_STRUCT: {
            dst.setSize(arraySize);
            alignas(8) hkDataClass klass = src.getClass();
            int32_t iNumMembers = klass.getNumMembers();
            // The SDK's hkArray<hkDataClass::MemberInfo, hkContainerTempAllocator> minfo(numMembers), inline: the temp
            // allocator may return more than asked, and every element it returned is constructed.
            hkArrayBase<hkDataClass::MemberInfo> minfo;
            minfo.m_data = nullptr;
            minfo.m_size = 0;
            minfo.m_capacityAndFlags = hkArrayBase<hkDataClass::MemberInfo>::DONT_DEALLOCATE_FLAG;
            hkMemoryAllocator* pTemp = hkMemoryRouter::getInstance().m_temp;
            int32_t iAllocated = iNumMembers;
            hkDataClass::MemberInfo* pData = nullptr;
            if (iNumMembers != 0) {
                pData = pTemp->_bufAlloc<hkDataClass::MemberInfo>(iAllocated);
            }
            minfo.m_data = pData;
            minfo.m_size = iAllocated;
            minfo.m_capacityAndFlags = iAllocated != 0
                ? iAllocated
                : static_cast<int32_t>(hkArrayBase<hkDataClass::MemberInfo>::DONT_DEALLOCATE_FLAG);
            for (hkDataClass::MemberInfo& rMemberInfo : minfo) {
                new (&rMemberInfo) hkDataClass::MemberInfo();
            }
            for (int32_t i = minfo.m_size; i < iNumMembers; ++i) {
                new (&minfo.m_data[i]) hkDataClass::MemberInfo();
            }
            minfo.m_size = iNumMembers;
            klass.getAllMemberInfo(minfo);
            for (int32_t m = 0; m < iNumMembers; ++m) {
                const hkDataClass::MemberInfo& mi = minfo.m_data[m];
                if (!(mi.m_type & hkDataObject::TYPE_MASK_BASIC_TYPES)) {
                    continue;
                }
                alignas(8) hkDataArray dstMembers = dst.swizzleObjectMember(mi.m_name);
                alignas(8) hkDataArray srcMembers = src.swizzleObjectMember(mi.m_name);
                if (mi.m_type & (hkDataObject::TYPE_ARRAY | hkDataObject::TYPE_TUPLE)) {
                    for (int32_t i = 0; i < arraySize; ++i) {
                        int32_t iSize;
                        if (mi.m_type & hkDataObject::TYPE_ARRAY) {
                            iSize = dstMembers.access(i).asArray().getSize();
                        }
                        else {
                            iSize = mi.m_tupleCount;
                        }
                        alignas(8) hkDataArray dstArray = dstMembers.access(i).asArray();
                        alignas(8) hkDataArray srcArray = srcMembers.access(i).asArray();
                        copyArray(dstArray, srcArray, iSize);
                    }
                }
                else {
                    for (int32_t i = 0; i < arraySize; ++i) {
                        copySimpleValue<hkDataArray::Value>(dstMembers.access(i), srcMembers.access(i));
                    }
                }
            }
            // The SDK's ~hkArray, inline: the storage goes back to the temp allocator unless the array does not own it.
            pTemp = hkMemoryRouter::getInstance().m_temp;
            minfo.m_size = 0;
            if (minfo.m_capacityAndFlags >= 0) {
                pTemp->_bufFree<hkDataClass::MemberInfo>(
                    minfo.m_data,
                    minfo.m_capacityAndFlags & hkArrayBase<hkDataClass::MemberInfo>::CAPACITY_MASK
                );
            }
            minfo.m_data = nullptr;
            minfo.m_capacityAndFlags = hkArrayBase<hkDataClass::MemberInfo>::DONT_DEALLOCATE_FLAG;
            break;
        }
        case hkDataObject::TYPE_CSTRING:
            for (int32_t i = 0; i < arraySize; ++i) {
                const char* pValue = src.access(i).asString();
                dst.access(i) = pValue;
            }
            break;
        default: break;
    }
}

// Fills dstInfos with dstClass's member infos; each member whose source member has a class and which has none is given
// the destination class of that class's name. The source infos live in a temp array.
void Cloner::getClassMemberInfos(
    hkArray<hkDataClass::MemberInfo, hkContainerTempAllocator>& dstInfos,
    const hkDataClass& dstClass,
    const hkDataClass& srcClass
) {
    hkArray<hkDataClass::MemberInfo, hkContainerTempAllocator> srcInfos;
    int32_t iNumSrc = srcClass.getNumMembers();
    srcInfos.setSize(iNumSrc);
    srcClass.getAllMemberInfo(srcInfos);
    int32_t iNumDst = dstClass.getNumMembers();
    dstInfos.setSize(iNumDst);
    dstClass.getAllMemberInfo(dstInfos);
    for (int32_t i = 0; i < srcClass.getNumMembers(); ++i) {
        const hkDataClassImpl* pSrcMemberClass = srcInfos.m_data[i].m_class;
        if (pSrcMemberClass) {
            hkDataClass::MemberInfo& rDstInfo = dstInfos.m_data[i];
            if (!rDstInfo.m_class) {
                rDstInfo.m_class = findDestClass(pSrcMemberClass->getName());
            }
        }
    }
}

// Copies each member of dstObj's class, its class completed by getClassMemberInfos, from srcObj through copyValue. The
// member infos live in a temp array.
void Cloner::copyObjectMembers(hkDataObject& dstObj, const hkDataObject& srcObj) {
    hkDataClass dstClass = dstObj.getClass();
    using MemberInfoArray = hkArray<hkDataClass::MemberInfo, hkContainerTempAllocator>;
    MemberInfoArray infos;
    getClassMemberInfos(infos, dstClass, srcObj.getClass());
    for (const hkDataClass::MemberInfo& info : infos) {
        copyValue(dstObj, srcObj, info);
    }
}

// obj's native copy as the packfile data that owns it, the copies tracked by class name, with one reference for the
// caller; the copier's copies live on the heap.
hkResource* hkDataObjectUtil::toResourceWithRegistry(const hkDataObject& src, const hkClassNameRegistry* classReg) {
    Copier copier(classReg, 0);
    if (src.isNull()) {
        return nullptr;
    }
    hkPackfileData* pData = new hkPackfileData;
    // The SDK's hkRefPtr::setAndDontIncrementRefCount.
    if (copier.m_trackedData.m_pntr && copier.m_trackedData.m_pntr != pData) {
        copier.m_trackedData.m_pntr->removeReference();
    }
    copier.m_trackedData.m_pntr = pData;
    hkVariant contents = copier.findOrCopy(src, 0);
    if (!contents.m_object || !contents.m_class) {
        if (copier.m_trackedData.m_pntr) {
            copier.m_trackedData.m_pntr->removeReference();
        }
        copier.m_trackedData.m_pntr = nullptr;
        return nullptr;
    }
    const hkDataWorld* pWorld = src.getClass().getWorld();
    for (int32_t i = static_cast<int32_t>(reinterpret_cast<uintptr_t>(copier.m_copies.getIterator()));
         i <= copier.m_copies.m_hashMod;) {
        hkDataObject::Handle handle = copier.m_copies.m_elem[i].key;
        hkDataObject object = pWorld->findObject(handle);
        const hkClass* pClass = copier.m_classReg->getClassByName(object.getClass().getName());
        void* pCopy = copier.m_copies.m_elem[i].val;
        copier.m_trackedData.m_pntr->m_trackedObjects.insert(pCopy, pClass->getName());
        for (++i; i <= copier.m_copies.m_hashMod; ++i) {
            if (copier.m_copies.m_elem[i].key.p0 != reinterpret_cast<void*>(static_cast<intptr_t>(-1))) {
                break;
            }
        }
    }
    copier.m_trackedData.m_pntr->setContentsWithName(contents.m_object, contents.m_class->getName());
    hkPackfileData* pResult = copier.m_trackedData.m_pntr;
    pResult->addReference();
    if (copier.m_trackedData.m_pntr) {
        copier.m_trackedData.m_pntr->removeReference();
    }
    copier.m_trackedData.m_pntr = nullptr;
    return pResult;
}

hkResource* hkDataObjectUtil::toResource(const hkDataObject& src) {
    return toResourceWithRegistry(src, hkBuiltinTypeRegistry::s_instance->getClassNameRegistry());
}

// src's native copy as an object resource over hkBuiltinTypeRegistry's two registries.
hkObjectResource* hkDataObjectUtil::toObject(const hkDataObject& src) {
    hkBuiltinTypeRegistry* pBuiltins = hkBuiltinTypeRegistry::s_instance;
    hkTypeInfoRegistry* pTypeReg = hkBuiltinTypeRegistry::s_instance->getTypeInfoRegistry();
    hkClassNameRegistry* pClassReg = pBuiltins->getClassNameRegistry();
    return hkDataObjectUtil::toObjectWithRegistry(src, pClassReg, pTypeReg);
}

// A Cloner from src to dst copies src's contents; its handle map is freed after.
hkResult hkDataObjectUtil::deepCopyWorld(hkDataWorld& dst, const hkDataWorld& src) {
    Cloner cloner(&dst, &src);
    hkDataObject contents = src.getContents();
    cloner.copyObject(contents);
    return HK_SUCCESS;
}
