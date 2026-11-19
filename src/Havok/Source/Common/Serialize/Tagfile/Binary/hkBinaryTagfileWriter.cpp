#include "Havok/Source/Common/Serialize/Tagfile/Binary/hkBinaryTagfileWriter.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/StringMap/hkCachedHashMap.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/System/Io/OArchive/hkOArchive.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Data/hkDataArray.h"
#include "Havok/Source/Common/Serialize/Data/hkDataObject.h"
#include "Havok/Source/Common/Serialize/Data/hkDataObjectImpl.h"
#include "Havok/Source/Common/Serialize/Tagfile/Binary/hkBinaryTagfileCommon.h"
#include "Havok/Source/Common/Serialize/Tagfile/hkTagfileWriter.h"
#include "SDK/MSVC/string.h"

// The ObjectDetails each object scanObject meets for the first time starts from: not written, no remember tag, no flags
// yet, one reference, a null handle. It is copied into the new m_details element.
static Writer::ObjectDetails s_newObjectDetails{0, -1, -1, 1};

hkResult hkBinaryTagfileWriter::save(
    const hkDataObject& obj,
    hkStreamWriter* stream,
    hkTagfileWriter::AddDataObjectListener* userListener
) const {
    Writer v5(stream, userListener);
    v5.m_oa.write32(-0x354FF2E2);
    v5.m_oa.write32(-0x2FEE0532);
    v5.writeInt(1);
    v5.writeInt(0);
    v5.scanObject(obj, 0);
    v5.writeObject(obj);
    stream->flush();
    return HK_SUCCESS;
}

// The flags are grown to a multiple of 8, all false, before the size is set to numMembers.
Writer::MemberWriteFlags::MemberWriteFlags(int32_t numMembers) {
    const int32_t iPadded = (numMembers + 7) & ~7;
    m_flags.setSize(iPadded);
    for (int32_t i = 0; i < iPadded; ++i) {
        m_flags.m_data[i] = false;
    }
    m_flags.setSize(numMembers);
}

void Writer::MemberWriteFlags::toBytes(uint8_t* out) {
    hkBool* v11;
    int32_t m_size = m_flags.m_size;
    int32_t v4 = m_size + 7 & 0xFFFFFFF8u;
    hkMemoryAllocator* m_temp = (&hkMemoryRouter::getInstance())->m_temp;
    int32_t v6 = m_flags.m_capacityAndFlags & hkArrayBase<hkBool>::CAPACITY_MASK;
    if (v6 < v4) {
        int32_t v7 = 2 * v6;
        if (v4 >= v7) {
            v7 = m_size + 7 & 0xFFFFFFF8u;
        }
        hkArrayUtil::_reserve(*m_temp, &m_flags, v7, sizeof(hkBool));
    }
    int32_t v8 = m_flags.m_size;
    if (v4 - v8 > 0) {
        std::memset(&m_flags.m_data[v8], 0, v4 - v8);
    }
    int32_t v9 = 0;
    for (m_flags.m_size = v4; v9 < m_flags.m_size; *out++ = v11[0].m_bool |
             2 *
                 (v11[1].m_bool |
                  2 *
                      (v11[2].m_bool |
                       2 *
                           (v11[3].m_bool |
                            2 * (v11[4].m_bool | 2 * (v11[5].m_bool | 2 * (v11[6].m_bool | 2 * v11[7].m_bool))))))) {
        v11 = &m_flags.m_data[v9];
        v9 += 8;
    }
    hkMemoryAllocator* v12 = (&hkMemoryRouter::getInstance())->m_temp;
    int32_t v13 = m_flags.m_capacityAndFlags & hkArrayBase<hkBool>::CAPACITY_MASK;
    if (v13 < m_size) {
        int32_t v14 = 2 * v13;
        if (m_size >= v14) {
            v14 = m_size;
        }
        hkArrayUtil::_reserve(*v12, &m_flags, v14, sizeof(hkBool));
    }
    m_flags.m_size = m_size;
}

void Writer::MemberWriteFlags::fromBytes(const uint8_t* inb, int32_t n) {
    int32_t m_size = m_flags.m_size;
    int32_t v4 = m_size;
    int32_t v5 = m_size + 7 & 0xFFFFFFF8u;
    hkMemoryAllocator* m_temp = (&hkMemoryRouter::getInstance())->m_temp;
    int32_t v7 = m_flags.m_capacityAndFlags & hkArrayBase<hkBool>::CAPACITY_MASK;
    if (v7 < v5) {
        int32_t v8 = 2 * v7;
        if (v5 >= v8) {
            v8 = m_size + 7 & 0xFFFFFFF8u;
        }
        hkArrayUtil::_reserve(*m_temp, this, v8, sizeof(hkBool));
    }
    int32_t v9 = m_flags.m_size;
    if (v5 - v9 > 0) {
        std::memset(&m_flags.m_data[v9], 0, v5 - v9);
    }
    m_flags.m_size = v5;
    int32_t v10 = 0;
    int32_t v11 = 0;
    if (n > 0) {
        do {
            uint8_t v12 = inb[v11];
            m_flags.m_data[v10].m_bool = v12 & 1;
            v12 >>= 1;
            m_flags.m_data[v10 + 1].m_bool = v12 & 1;
            v12 >>= 1;
            m_flags.m_data[v10 + 2].m_bool = v12 & 1;
            v12 >>= 1;
            m_flags.m_data[v10 + 3].m_bool = v12 & 1;
            v12 >>= 1;
            m_flags.m_data[v10 + 4].m_bool = v12 & 1;
            v12 >>= 1;
            m_flags.m_data[v10 + 5].m_bool = v12 & 1;
            v12 >>= 1;
            m_flags.m_data[v10 + 6].m_bool = v12 & 1;
            m_flags.m_data[v10 + 7].m_bool = (v12 & 2) != 0;
            ++v11;
            v10 += 8;
        }
        while (v11 < n);
        v4 = m_size;
    }
    hkMemoryAllocator* v13 = (&hkMemoryRouter::getInstance())->m_temp;
    int32_t v14 = m_flags.m_capacityAndFlags & hkArrayBase<hkBool>::CAPACITY_MASK;
    if (v14 < v4) {
        int32_t v15 = 2 * v14;
        if (v4 >= v15) {
            v15 = v4;
        }
        hkArrayUtil::_reserve(*v13, this, v15, sizeof(hkBool));
    }
    m_flags.m_size = v4;
}

Writer::ListenerCache::ListenerCache(hkTagfileWriter::AddDataObjectListener* listener) : m_listener(listener) {}

// The cached objects are heap hkDataObjects; their operator delete (the heap's blockFree) is written out.
Writer::ListenerCache::~ListenerCache() {
    for (int32_t i = 0; i < m_cachedObjects.m_size; ++i) {
        hkDataObject* pObject = m_cachedObjects.m_data[i];
        if (pObject) {
            pObject->~hkDataObject();
            hkMemoryRouter::getInstance().m_heap->blockFree(pObject, sizeof(hkDataObject));
        }
    }
}

hkDataObject* Writer::ListenerCache::getCachedDataObject(hkDataObject* apResult, const hkDataObject& h) const {
    int32_t v4;
    hkDataObject::Handle a1;
    if (!m_listener || (h.isNull() || (h.getHandle(&a1), (v4 = m_indexFromHandle.getWithDefault(a1, -1), v4 == -1)))) {
        ::new (apResult) hkDataObject(h);
        return apResult;
    }

    ::new (apResult) hkDataObject(m_cachedObjects.m_data[v4]->m_impl);
    return apResult;
}

// The listener's answer is built in a plain local and destroyed with hkDataObject's destructor on both paths.
hkDataObject* Writer::ListenerCache::callListenerAndCache(hkDataObject* apResult, const hkDataObject& h) {
    if (m_listener && !h.isNull()) {
        hkDataObject::Handle handle;
        h.getHandle(&handle);
        const int32_t index = m_indexFromHandle.getWithDefault(handle, -1);
        if (index != -1) {
            ::new (apResult) hkDataObject(m_cachedObjects.m_data[index]->m_impl);
            return apResult;
        }
        hkDataObject obj;
        m_listener->addDataObjectCallback(&obj, h);
        if (obj != h) {
            m_indexFromHandle.insert(*hkMemoryRouter::getInstance().m_heap, handle, m_cachedObjects.m_size);
            // EXE-INLINED none: hkDataObject::operator new
            void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkDataObject));
            hkDataObject* pCached = pMemory ? ::new (pMemory) hkDataObject(obj) : nullptr;
            // EXE-INLINED none: hkArray<hkDataObject*, hkContainerTempAllocator>::pushBack
            hkMemoryRouter& rRouter = hkMemoryRouter::getInstance();
            if (m_cachedObjects.m_size ==
                (m_cachedObjects.m_capacityAndFlags & hkArrayBase<hkDataObject*>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(*rRouter.m_temp, &m_cachedObjects, sizeof(hkDataObject*));
            }
            m_cachedObjects.m_data[m_cachedObjects.m_size] = pCached;
            m_cachedObjects.m_size += 1;
            ::new (apResult) hkDataObject(obj);
            return apResult;
        }
    }
    ::new (apResult) hkDataObject(h);
    return apResult;
}

// "" is entered as index 0 of the class and string maps; their keys are the strings' addresses (hkUlong words, hence
// the pointer-to-integer reinterpret_casts).
Writer::Writer(hkStreamWriter* a1, hkTagfileWriter::AddDataObjectListener* a2)
  : m_oa(a1, hkBool(false)), m_writtenStrings(0u), m_doneClasses(0u), m_currentTag(0), m_listenerCache(a2) {
    m_doneClasses.insert(reinterpret_cast<hkUlong>(""), 0);
    m_writtenStrings.insert(reinterpret_cast<hkUlong>(""), 0);
}

// A tuple writes its items; an array its size, then, for an array of structs whose member has no class of its own, the
// index of the array's class, then its items; any other value by its basic type. The array, the array's class and the
// object are plain locals.
void Writer::writeBinaryValue(const hkDataObject::Value& value, const hkDataClass::MemberInfo& minfo) {
    if (value.getType() & hkDataObject::TYPE_TUPLE) {
        XFNV_INLINED_LOCAL(hkDataArray, array, std::nullopt, std::nullopt);
        value.asArray(&array);
        _writeArrayItems(array, array.getSize());
        array.~hkDataArray();
        return;
    }
    if (value.getType() & hkDataObject::TYPE_ARRAY) {
        XFNV_INLINED_LOCAL(hkDataArray, array, std::nullopt, std::nullopt);
        value.asArray(&array);
        const int32_t numItems = array.getSize();
        writeInt(numItems);
        if (array.getType() == hkDataObject::TYPE_STRUCT && minfo.m_class == nullptr) {
            hkDataClass klass;
            array.getClass(&klass);
            // The untyped string map keys a name by its address (hkUlong); its value is the class index writeClass
            // stored, read back as an int and sign-extended.
            writeInt(static_cast<int32_t>(m_doneClasses.getWithDefault(reinterpret_cast<hkUlong>(klass.getName()), 0)));
        }
        _writeArrayItems(array, numItems);
        array.~hkDataArray();
        return;
    }
    switch (value.getType()) {
        case hkDataObject::TYPE_BYTE: m_oa.write8u(value.asInt()); return;
        case hkDataObject::TYPE_INT: writeInt(value.asInt()); return;
        case hkDataObject::TYPE_REAL: m_oa.writeFloat32(value.asReal()); return;
        case hkDataObject::TYPE_VEC_4:
        case hkDataObject::TYPE_VEC_12:
        case hkDataObject::TYPE_VEC_16: {
            // TYPE_VEC_4, 12 and 16 are 4, 6 and 7: 4 * type - 12 is the number of reals.
            const int32_t numReals = 4 * value.getType() - 12;
            m_oa.writeArrayFloat32(value.asVec(numReals), numReals);
            return;
        }
        case hkDataObject::TYPE_OBJECT:
        case hkDataObject::TYPE_STRUCT: {
            XFNV_INLINED_LOCAL(hkDataObject, object, std::nullopt, std::nullopt);
            writeObject(*value.asObject(&object));
            object.~hkDataObject();
            return;
        }
        case hkDataObject::TYPE_CSTRING: writeString(value.asString()); return;
        default: return;
    }
}

// The SDK's writeInt: i zigzag-encoded (doubled, negatives as -2i + 1) as a varint, 7 bits a byte, lowest first, the
// high bit set on every byte but the last.
void Writer::writeInt(int64_t i) {
    uint64_t u = i < 0 ? (((uint64_t{0} - static_cast<uint64_t>(i)) << 1) | 1) : (static_cast<uint64_t>(i) << 1);
    uint8_t b = static_cast<uint8_t>(u & 0x7f);
    u >>= 7;
    while (u != 0) {
        m_oa.write8u(b | 0x80);
        b = static_cast<uint8_t>(u & 0x7f);
        u >>= 7;
    }
    // The last byte is passed with i's bits 8-31 in the upper bytes, as the game does.
    m_oa.write8u(static_cast<int32_t>((static_cast<uint32_t>(i) & 0xFFFFFF00u) | b));
}

// The SDK's writeString: null as -1; a string written before as minus its 1-based index; else its length and bytes,
// then its index recorded. The map's keys are the strings' addresses (hkUlong words, hence the pointer-to-integer
// reinterpret_casts).
void Writer::writeString(const char* s) {
    if (!s) {
        writeInt(-1);
        return;
    }
    const int32_t prev =
        static_cast<int32_t>(m_writtenStrings.getWithDefault(reinterpret_cast<hkUlong>(s), static_cast<hkUlong>(-1)));
    if (prev == -1) {
        const int32_t len = hkString::strLen(s);
        writeInt(len);
        m_oa.writeRaw(s, len);
        // EXE-INLINED none: hkStringMap::getSize
        m_writtenStrings.insert(reinterpret_cast<hkUlong>(s), static_cast<hkUlong>(m_writtenStrings.m_numElems + 1));
    }
    else {
        writeInt(-prev);
    }
}

// Writes klass's parent first, then klass once (its name, version, parent's index and declared members), then the
// classes of its members; returns klass's index in m_doneClasses, 0 for a null class. The map's keys are the class
// names' addresses (hkUlong words, hence the pointer-to-integer reinterpret_casts). The member array is an
// hkArray<MemberInfo>::Temp of numMembers default members.
int32_t Writer::writeClass(const hkDataClass& klass) {
    // EXE-INLINED 0xDBA670
    if (!klass.m_impl) {
        return 0;
    }
    int32_t prev = static_cast<int32_t>(
        m_doneClasses.getWithDefault(reinterpret_cast<hkUlong>(klass.getName()), static_cast<hkUlong>(-1))
    );
    if (prev >= 0) {
        return prev;
    }
    hkDataClass parent;
    // Passes the reference returned by getParent on, not the local's address again.
    const int32_t parentIndex = writeClass(klass.getParent(parent));
    // Writing the parent may have written klass.
    prev = static_cast<int32_t>(
        m_doneClasses.getWithDefault(reinterpret_cast<hkUlong>(klass.getName()), static_cast<hkUlong>(-1))
    );
    if (prev != -1) {
        return prev;
    }
    // EXE-INLINED none: hkStringMap::getSize
    const int32_t classIndex = m_doneClasses.m_numElems;
    m_doneClasses.insert(reinterpret_cast<hkUlong>(klass.getName()), static_cast<hkUlong>(classIndex));
    const int32_t numMembers = klass.getNumDeclaredMembers();

    // EXE-INLINED none: hkArray<hkDataClass::MemberInfo, hkContainerTempAllocator>::hkArray(int)
    hkArrayBase<hkDataClass::MemberInfo> minfo;
    minfo.m_data = nullptr;
    minfo.m_size = 0;
    minfo.m_capacityAndFlags = hkArrayBase<hkDataClass::MemberInfo>::DONT_DEALLOCATE_FLAG;
    hkMemoryAllocator* pTemp = hkMemoryRouter::getInstance().m_temp;
    hkDataClass::MemberInfo* pData = nullptr;
    int32_t numAllocated = numMembers;
    if (numMembers != 0) {
        pData = pTemp->_bufAlloc<hkDataClass::MemberInfo>(numAllocated);
    }
    minfo.m_data = pData;
    minfo.m_size = numAllocated;
    minfo.m_capacityAndFlags = numAllocated != 0
        ? numAllocated
        : static_cast<int32_t>(hkArrayBase<hkDataClass::MemberInfo>::DONT_DEALLOCATE_FLAG);
    // EXE-INLINED none: hkDataClass::MemberInfo::MemberInfo (placement new, with its null test)
    for (int32_t i = 0; i < numAllocated; ++i) {
        hkDataClass::MemberInfo* pInfo = &pData[i];
        if (pInfo) {
            pInfo->m_name = nullptr;
            pInfo->m_owner = nullptr;
            pInfo->m_type = hkDataObject::TYPE_VOID;
            pInfo->m_class = nullptr;
            pInfo->m_tupleCount = 0;
        }
    }
    klass.getAllDeclaredMemberInfo(minfo);

    writeInt(hkBinaryTagfile::TAG_METADATA);
    writeString(klass.getName());
    writeInt(klass.getVersion());
    writeInt(parentIndex);
    writeInt(numMembers);
    for (int32_t i = 0; i < numMembers; ++i) {
        const hkDataClass::MemberInfo& mi = minfo.m_data[i];
        writeString(mi.m_name);
        writeInt(mi.m_type);
        if (mi.m_type & hkDataObject::TYPE_TUPLE) {
            writeInt(minfo.m_data[i].m_tupleCount);
        }
        const int32_t basicType = mi.m_type & hkDataObject::TYPE_MASK_BASIC_TYPES;
        if (basicType == hkDataObject::TYPE_STRUCT || basicType == hkDataObject::TYPE_OBJECT) {
            if (mi.m_class) {
                // The SDK's hkDataClass(const_cast<hkDataClassImpl*>(mi.m_class)): the constructor takes a non-const
                // implementation.
                hkDataClass memberClass(const_cast<hkDataClassImpl*>(mi.m_class));
                writeString(memberClass.getName());
            }
            else {
                writeInt(-1);
            }
        }
    }
    for (int32_t i = 0; i < numMembers; ++i) {
        if (minfo.m_data[i].m_class) {
            // As above: the SDK's const_cast for hkDataClass's constructor.
            hkDataClass memberClass(const_cast<hkDataClassImpl*>(minfo.m_data[i].m_class));
            writeClass(memberClass);
        }
    }

    // EXE-INLINED none: hkArray<hkDataClass::MemberInfo, hkContainerTempAllocator>::~hkArray
    hkMemoryAllocator* pTempFree = hkMemoryRouter::getInstance().m_temp;
    minfo.m_size = 0;
    if ((minfo.m_capacityAndFlags & hkArrayBase<hkDataClass::MemberInfo>::DONT_DEALLOCATE_FLAG) == 0) {
        pTempFree->_bufFree<hkDataClass::MemberInfo>(minfo.m_data, minfo.getCapacity());
    }
    return classIndex;
}

// Takes the object the listener cache gives for objIn; a null one is skipped, one met before gets one more reference.
// Else (a2 zero) it is filed in m_detailIndexFromObject and m_details; its class is written, and each member worth
// writing gets its flag and, for an object, a struct or an array of them, its objects scanned in turn (an array of
// structs through each object member of the struct). With a2 zero the flags go to m_bitfieldBuf, where the object's
// details point.
void Writer::scanObject(const hkDataObject& objIn, int32_t a2) {
    alignas(8) hkDataObject obj;
    m_listenerCache.callListenerAndCache(&obj, objIn);
    if (obj.isNull()) {
        return;
    }
    hkDataObject::Handle handle;
    int32_t detailIndex = m_detailIndexFromObject.getWithDefault(*obj.getHandle(&handle), -1);
    if (detailIndex != -1) {
        m_details.m_data[detailIndex].refCount += 1;
        return;
    }
    if (a2 == 0) {
        detailIndex = m_detailIndexFromObject.m_numElems &
            hkMapBase<hkDataObject::Handle, int32_t, Writer::HandleOps>::NUM_ELEMS_MASK;
        hkDataObject::Handle key = *obj.getHandle(&handle);
        m_detailIndexFromObject.insert(*hkMemoryRouter::getInstance().m_heap, key, detailIndex);
        // EXE-INLINED none: hkArray<Writer::ObjectDetails, hkContainerTempAllocator>::pushBack
        hkMemoryRouter& rRouter = hkMemoryRouter::getInstance();
        if (m_details.m_size == (m_details.m_capacityAndFlags & hkArrayBase<ObjectDetails>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*rRouter.m_temp, &m_details, sizeof(ObjectDetails));
        }
        ObjectDetails* pNew = &m_details.m_data[m_details.m_size];
        m_details.m_size += 1;
        *pNew = s_newObjectDetails;
        m_details.m_data[m_details.m_size - 1].handle = *obj.getHandle(&handle);
    }
    alignas(8) hkDataClass klass;
    obj.getClass(&klass);
    writeClass(klass);
    Writer::MemberWriteFlags flags(klass.getNumMembers());
    for (int32_t it = obj.getMemberIterator(); obj.isValid(it); it = obj.getNextMember(it)) {
        hkDataObject::Value value;
        obj.getMemberValue(&value, it);
        if (!worthWriting(value)) {
            continue;
        }
        flags.m_flags.m_data[klass.getMemberIndexByName(obj.getMemberName(it))].m_bool = 1;
        const int32_t basicType = value.getType() & hkDataObject::TYPE_MASK_BASIC_TYPES;
        if (basicType != hkDataObject::TYPE_OBJECT && basicType != hkDataObject::TYPE_STRUCT) {
            continue;
        }
        switch (value.getType()) {
            case hkDataObject::TYPE_OBJECT:
            case hkDataObject::TYPE_STRUCT: {
                hkDataObject member;
                scanObject(*value.asObject(&member), 0);
                break;
            }
            case hkDataObject::TYPE_ARRAY_STRUCT:
            case hkDataObject::TYPE_TUPLE_STRUCT: {
                alignas(8) hkDataArray array;
                hkDataClass structClass;
                value.asArray(&array)->getClass(&structClass);
                writeClass(structClass);
                for (int32_t i = 0; i < structClass.getNumMembers(); ++i) {
                    // EXE-INLINED none: hkDataClass::MemberInfo::MemberInfo
                    hkDataClass::MemberInfo minfo;
                    minfo.m_name = nullptr;
                    minfo.m_owner = nullptr;
                    minfo.m_type = hkDataObject::TYPE_VOID;
                    minfo.m_class = nullptr;
                    minfo.m_tupleCount = 0;
                    structClass.getMemberInfo(i, minfo);
                    if (minfo.m_type != hkDataObject::TYPE_OBJECT) {
                        continue;
                    }
                    hkDataArray whole;
                    hkDataArray objects;
                    value.asArray(&whole)->swizzleObjectMember(&objects, minfo.m_name);
                    const int32_t numObjects = objects.getSize();
                    for (int32_t j = 0; j < numObjects; ++j) {
                        hkDataArray::Value item;
                        alignas(8) hkDataObject member;
                        scanObject(*objects.access(&item, j)->asObject(&member), 0);
                    }
                }
                break;
            }
            case hkDataObject::TYPE_ARRAY_OBJECT:
            case hkDataObject::TYPE_TUPLE_OBJECT: {
                hkDataArray sizeArray;
                const int32_t numObjects = value.asArray(&sizeArray)->getSize();
                for (int32_t j = 0; j < numObjects; ++j) {
                    hkDataArray array;
                    hkDataArray::Value item;
                    alignas(8) hkDataObject member;
                    scanObject(*value.asArray(&array)->access(&item, j)->asObject(&member), 0);
                }
                break;
            }
            default: break;
        }
    }
    if (a2 == 0) {
        m_details.m_data[detailIndex].bitfieldStartIndex = m_bitfieldBuf.m_size;
        const int32_t numBytes = ((flags.m_flags.m_size + 7) & ~7) / 8;
        flags.toBytes(m_bitfieldBuf._expandBy(*hkMemoryRouter::getInstance().m_temp, numBytes));
    }
}

// Writes numItems items of arr by its element type: a tuple's items as arrays (recursively), numbers, vectors (a
// vector-4 array as 3 or 4 reals: 4 when any w is not 0), objects, strings, and a struct array member by member: first
// the bits of the members that hold a value worth writing, then those members' arrays.
void Writer::_writeArrayItems(const hkDataArray& arr, int32_t numItems) {
    if ((arr.getType() & hkDataObject::TYPE_TUPLE) != 0) {
        for (int32_t i = 0; i < numItems; ++i) {
            alignas(8) hkDataArray::Value item;
            alignas(8) hkDataArray tuple;
            arr.access(&item, i)->asArray(&tuple);
            _writeArrayItems(tuple, tuple.getSize());
        }
        return;
    }
    switch (arr.getType()) {
        case hkDataObject::TYPE_BYTE: {
            for (int32_t i = 0; i < numItems; ++i) {
                alignas(8) hkDataArray::Value item;
                m_oa.write8u(arr.access(&item, i)->asInt());
            }
            return;
        }
        case hkDataObject::TYPE_INT: {
            for (int32_t i = 0; i < numItems; ++i) {
                alignas(8) hkDataArray::Value item;
                writeInt(arr.access(&item, i)->asInt());
            }
            return;
        }
        case hkDataObject::TYPE_REAL: {
            for (int32_t i = 0; i < numItems; ++i) {
                alignas(8) hkDataArray::Value item;
                float f = arr.access(&item, i)->asReal();
                m_oa.writeFloat32(f);
            }
            return;
        }
        case hkDataObject::TYPE_VEC_4: {
            // Three reals per item, four when any item's w is not 0 (a NaN counts as not 0).
            int32_t numReals = 3;
            for (int32_t i = 0; i < numItems; ++i) {
                alignas(8) hkDataArray::Value item;
                if (0.0f != arr.access(&item, i)->asVec(4)[3]) {
                    numReals = 4;
                    break;
                }
            }
            writeInt(numReals);
            for (int32_t i = 0; i < numItems; ++i) {
                alignas(8) hkDataArray::Value item;
                m_oa.writeArrayFloat32(arr.access(&item, i)->asVec(4), numReals);
            }
            return;
        }
        case hkDataObject::TYPE_VEC_12:
        case hkDataObject::TYPE_VEC_16: {
            // 12 or 16 reals: the type is read again.
            int32_t numReals = 4 * (arr.getType() - hkDataObject::TYPE_REAL);
            for (int32_t i = 0; i < numItems; ++i) {
                alignas(8) hkDataArray::Value item;
                m_oa.writeArrayFloat32(arr.access(&item, i)->asVec(numReals), numReals);
            }
            return;
        }
        case hkDataObject::TYPE_STRUCT: {
            alignas(8) hkDataClass klass;
            int32_t numMembers = arr.getClass(&klass)->getNumMembers();

            using minfo_t = hkArray<hkDataClass::MemberInfo, hkContainerTempAllocator>;
            alignas(8) minfo_t minfo(numMembers);
            hkDataClass klass2;
            arr.getClass(&klass2)->getAllMemberInfo(minfo);

            alignas(8) Writer::MemberWriteFlags flags(numMembers);
            // A member is written when any item's value in it is worth writing.
            for (int32_t i = 0; i < numMembers; ++i) {
                if (minfo.m_data[i].m_type != hkDataObject::TYPE_VOID) {
                    alignas(8) hkDataArray member;
                    arr.swizzleObjectMember(&member, minfo.m_data[i].m_name);
                    for (int32_t j = 0; j < member.getSize(); ++j) {
                        alignas(8) hkDataArray::Value item;
                        if (worthWriting(*member.access(&item, j))) {
                            flags.m_flags.m_data[i].m_bool = 1;
                            break;
                        }
                    }
                }
            }

            {
                using bytes_t = hkArray<uint8_t, hkContainerTempAllocator>;
                bytes_t bytes;
                int32_t numBytes = ((flags.m_flags.m_size + 7) & ~7) / 8;
                // EXE-INLINED none: hkArray<uint8_t, hkContainerTempAllocator>::setSize
                hkMemoryAllocator* pTemp2 = hkMemoryRouter::getInstance().m_temp;
                int32_t capacity = bytes.m_capacityAndFlags & hkArrayBase<uint8_t>::CAPACITY_MASK;
                if (capacity < numBytes) {
                    int32_t newCapacity = 2 * capacity;
                    if (numBytes >= newCapacity) {
                        newCapacity = numBytes;
                    }
                    hkArrayUtil::_reserve(*pTemp2, &bytes, newCapacity, sizeof(uint8_t));
                }
                bytes.m_size = numBytes;
                flags.toBytes(bytes.m_data);
                m_oa.writeRaw(bytes.m_data, bytes.m_size);
            }

            for (int32_t i = 0; i < numMembers; ++i) {
                if (flags.m_flags.m_data[i].m_bool) {
                    alignas(8) hkDataArray member;
                    arr.swizzleObjectMember(&member, minfo.m_data[i].m_name);
                    _writeArrayItems(member, numItems);
                }
            }

            // EXE-INLINED none: hkArray<hkBool, hkContainerTempAllocator>::~hkArray
            hkMemoryAllocator* pTemp4 = hkMemoryRouter::getInstance().m_temp;
            flags.m_flags.m_size = 0;
            if (flags.m_flags.m_capacityAndFlags >= 0) {
                pTemp4->bufFree(
                    flags.m_flags.m_data, flags.m_flags.m_capacityAndFlags & hkArrayBase<hkBool>::CAPACITY_MASK
                );
            }
            flags.m_flags.m_data = nullptr;
            flags.m_flags.m_capacityAndFlags = hkArrayBase<hkBool>::DONT_DEALLOCATE_FLAG;
            return;
        }
        case hkDataObject::TYPE_OBJECT: {
            for (int32_t i = 0; i < numItems; ++i) {
                alignas(8) hkDataArray::Value item;
                alignas(8) hkDataObject obj;
                arr.access(&item, i)->asObject(&obj);
                writeObject(obj);
            }
            return;
        }
        case hkDataObject::TYPE_CSTRING: {
            for (int32_t i = 0; i < numItems; ++i) {
                alignas(8) hkDataArray::Value item;
                writeString(arr.access(&item, i)->asString());
            }
            return;
        }
        default: return;
    }
}

// Writes the object the listener cache gives for objIn; a null one is TAG_OBJECT_NULL, one remembered before
// TAG_OBJECT_BACKREF and its tag. Else its class is written, then TAG_OBJECT (TAG_OBJECT_REMEMBER with a new tag when
// it is referenced more than once), the class's index, the member flags scanObject left in m_bitfieldBuf and the values
// of the members they mark.
void Writer::writeObject(const hkDataObject& objIn) {
    hkDataObject obj;
    m_listenerCache.getCachedDataObject(&obj, objIn);
    if (obj.isNull()) {
        writeInt(hkBinaryTagfile::TAG_OBJECT_NULL);
        return;
    }
    hkDataObject::Handle handle;
    const int32_t detailIndex = m_detailIndexFromObject.getWithDefault(*obj.getHandle(&handle), -1);
    ObjectDetails* pDetails = &m_details.m_data[detailIndex];
    if (pDetails->rememberTag >= 0) {
        writeInt(hkBinaryTagfile::TAG_OBJECT_BACKREF);
        writeInt(pDetails->rememberTag);
        return;
    }
    hkDataClass klass;
    obj.getClass(&klass);
    const int32_t classIndex = writeClass(klass);
    if (pDetails->refCount > 1) {
        writeInt(hkBinaryTagfile::TAG_OBJECT_REMEMBER);
        pDetails->rememberTag = m_currentTag;
        m_currentTag += 1;
    }
    else {
        writeInt(hkBinaryTagfile::TAG_OBJECT);
    }
    pDetails->wasWritten = 1;
    writeInt(classIndex);
    const int32_t numMembers = klass.getNumMembers();
    Writer::MemberWriteFlags flags(numMembers);

    // EXE-INLINED none: hkArray<hkUint8, hkContainerTempAllocator>::hkArray(buffer, size, capacity)
    // The object's flag bytes, an array over m_bitfieldBuf's storage that does not own it.
    hkArrayBase<uint8_t> bits;
    bits.m_data = &m_bitfieldBuf.m_data[pDetails->bitfieldStartIndex];
    bits.m_size = ((flags.m_flags.m_size + 7) & ~7) / 8;
    bits.m_capacityAndFlags = static_cast<int32_t>(bits.m_size | hkArrayBase<uint8_t>::DONT_DEALLOCATE_FLAG);
    m_oa.writeRaw(bits.m_data, bits.m_size);
    flags.fromBytes(bits.m_data, bits.m_size);
    // EXE-INLINED none: hkArray<hkUint8, hkContainerTempAllocator>::~hkArray
    hkMemoryAllocator* pTempBits = hkMemoryRouter::getInstance().m_temp;
    bits.m_size = 0;
    if (bits.m_capacityAndFlags >= 0) {
        pTempBits->bufFree(bits.m_data, bits.m_capacityAndFlags & hkArrayBase<uint8_t>::CAPACITY_MASK);
    }

    // EXE-INLINED none: hkArray<hkDataClass::MemberInfo, hkContainerTempAllocator>::hkArray(int)
    hkArrayBase<hkDataClass::MemberInfo> minfo;
    minfo.m_data = nullptr;
    minfo.m_size = 0;
    minfo.m_capacityAndFlags = hkArrayBase<hkDataClass::MemberInfo>::DONT_DEALLOCATE_FLAG;
    hkMemoryAllocator* pTemp = hkMemoryRouter::getInstance().m_temp;
    hkDataClass::MemberInfo* pData = nullptr;
    int32_t numAllocated = numMembers;
    if (numMembers != 0) {
        pData = pTemp->_bufAlloc<hkDataClass::MemberInfo>(numAllocated);
    }
    minfo.m_data = pData;
    minfo.m_size = numAllocated;
    minfo.m_capacityAndFlags = numAllocated != 0
        ? numAllocated
        : static_cast<int32_t>(hkArrayBase<hkDataClass::MemberInfo>::DONT_DEALLOCATE_FLAG);
    // EXE-INLINED none: hkDataClass::MemberInfo::MemberInfo (placement new, with its null test)
    for (int32_t i = 0; i < numAllocated; ++i) {
        hkDataClass::MemberInfo* pInfo = &pData[i];
        if (pInfo) {
            pInfo->m_name = nullptr;
            pInfo->m_owner = nullptr;
            pInfo->m_type = hkDataObject::TYPE_VOID;
            pInfo->m_class = nullptr;
            pInfo->m_tupleCount = 0;
        }
    }
    klass.getAllMemberInfo(minfo);

    for (int32_t i = 0; i < numMembers; ++i) {
        if (flags.m_flags.m_data[i].m_bool) {
            hkDataObject::Value value;
            // Calls the const operator[] on the cached object.
            static_cast<const hkDataObject&>(obj).access(&value, minfo.m_data[i].m_name);
            writeBinaryValue(value, minfo.m_data[i]);
        }
    }

    // EXE-INLINED none: hkArray<hkDataClass::MemberInfo, hkContainerTempAllocator>::~hkArray
    hkMemoryAllocator* pTempInfo = hkMemoryRouter::getInstance().m_temp;
    minfo.m_size = 0;
    if ((minfo.m_capacityAndFlags & hkArrayBase<hkDataClass::MemberInfo>::DONT_DEALLOCATE_FLAG) == 0) {
        pTempInfo->_bufFree<hkDataClass::MemberInfo>(
            minfo.m_data, minfo.m_capacityAndFlags & hkArrayBase<hkDataClass::MemberInfo>::CAPACITY_MASK
        );
    }
    minfo.m_data = nullptr;
    minfo.m_capacityAndFlags = hkArrayBase<hkDataClass::MemberInfo>::DONT_DEALLOCATE_FLAG;
}
