#include "Havok/Source/Common/Serialize/Tagfile/Binary/hkBinaryTagfileReader.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/DebugUtil/Logger/hkOstreamLogger.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/IArchive/hkIArchive.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/System/Io/Reader/hkStreamReader.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Serialize/Data/hkDataArray.h"
#include "Havok/Source/Common/Serialize/Data/hkDataObject.h"
#include "Havok/Source/Common/Serialize/Data/hkDataObjectImpl.h"
#include "Havok/Source/Common/Serialize/Data/hkDataWorld.h"

hkBinaryTagfileReader::hkBinaryTagfileReader() {}

hkBinaryTagfileReader::~hkBinaryTagfileReader() {}

hkDataObject hkBinaryTagfileReader::load(hkStreamReader* stream, hkDataWorld& cont) {
    hkDataObject result;
    Reader v5(stream, cont);
    v5.readObjectTree(&result);
    return result;
}

// The class table starts with a null class, the string table with the empty string and a null string: tags 0 and 1.
Reader::Reader(hkStreamReader* apStream, hkDataWorld& arWorld)
  : m_ia(apStream, false), m_world(arWorld), m_log(nullptr) {
    m_classes.pushBack(nullptr);
    m_prevStrings.pushBack(const_cast<char*>(""));
    m_prevStrings.pushBack(nullptr);
}

// Deletes the log and frees the strings it read (past the first two entries); the tables and the archive free
// themselves.
Reader::~Reader() {
    delete m_log;
    for (int32_t i = 2; i < m_prevStrings.m_size; ++i) {
        hkMemoryRouter::easyFree(*hkMemoryRouter::getInstance().m_heap, m_prevStrings.m_data[i]);
    }
}

const char* Reader::readString() {
    int32_t v2 = readInt<int32_t>();
    int32_t v3 = v2;
    if (v2 <= 0) {
        return m_prevStrings.m_data[-v2];
    }
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    void* v5 = hkMemoryRouter::easyAlloc(*Instance->m_heap, v3 + 1);
    m_ia.readRaw(v5, v3);
    *(static_cast<uint8_t*>(v5) + v3) = 0;
    hkMemoryRouter* v6 = &hkMemoryRouter::getInstance();
    if (m_prevStrings.m_size == (m_prevStrings.m_capacityAndFlags & hkArrayBase<char*>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*v6->m_heap, &m_prevStrings, sizeof(char*));
    }
    m_prevStrings.m_data[m_prevStrings.m_size++] = static_cast<char*>(v5);
    return static_cast<char*>(v5);
}

// Reads ceil(numMembers / 8) bytes (at most 64), unpacked low bit first into one byte per bit; the array is sized to
// the padded bit count for the unpacking, then to numMembers.
void Reader::readBitfield(int32_t numMembers, hkLocalArray<uint8_t>& bout) {
    uint8_t bytes[64];
    const int32_t iPaddedBits = (numMembers + 7) & ~7;
    const int32_t iNumBytes = iPaddedBits / 8;
    // EXE-INLINED none: hkArray<uint8_t>::setSize
    hkMemoryAllocator& rHeap = *hkMemoryRouter::getInstance().m_heap;
    const int32_t iCap = bout.m_capacityAndFlags & hkArrayBase<uint8_t>::CAPACITY_MASK;
    if (iCap < iPaddedBits) {
        int32_t iReq = 2 * iCap;
        if (iPaddedBits >= iReq) {
            iReq = iPaddedBits;
        }
        hkArrayUtil::_reserve(rHeap, &bout, iReq, sizeof(uint8_t));
    }
    bout.m_size = iPaddedBits;
    m_ia.readArrayGeneric(bytes, 1, iNumBytes);
    for (int32_t i = 0; i < iNumBytes; ++i) {
        const uint32_t uiByte = bytes[i];
        bout.m_data[8 * i] = uiByte & 1;
        bout.m_data[8 * i + 1] = (uiByte >> 1) & 1;
        bout.m_data[8 * i + 2] = (uiByte >> 2) & 1;
        bout.m_data[8 * i + 3] = (uiByte >> 3) & 1;
        bout.m_data[8 * i + 4] = (uiByte >> 4) & 1;
        bout.m_data[8 * i + 5] = (uiByte >> 5) & 1;
        bout.m_data[8 * i + 6] = (uiByte >> 6) & 1;
        bout.m_data[8 * i + 7] = (uiByte >> 7) & 1;
    }
    // EXE-INLINED none: hkArray<uint8_t>::setSize
    hkMemoryAllocator& rHeap2 = *hkMemoryRouter::getInstance().m_heap;
    const int32_t iCap2 = bout.m_capacityAndFlags & hkArrayBase<uint8_t>::CAPACITY_MASK;
    if (iCap2 < numMembers) {
        int32_t iReq = 2 * iCap2;
        if (numMembers >= iReq) {
            iReq = numMembers;
        }
        hkArrayUtil::_reserve(rHeap2, &bout, iReq, sizeof(uint8_t));
    }
    bout.m_size = numMembers;
}

// Reads numItems elements of the array's type. Bytes and reals go through a temp-allocated buffer and setAll; a struct
// array reads its member bitfield, then each present member's swizzled array (or each item's tuple of it) recursively.
// VEC_8 reads nothing.
void Reader::_readArrayItems(hkDataArray& arr, int32_t nelem) {
    switch (arr.getType()) {
        case hkDataObject::TYPE_BYTE: {
            // EXE-INLINED none: hkArray<hkUint8, hkContainerTempAllocator>::hkArray(int)
            XFNV_INLINED_LOCAL(hkArray<uint8_t>, buf, std::nullopt, std::nullopt);
            buf.m_data = nullptr;
            buf.m_size = 0;
            buf.m_capacityAndFlags = static_cast<int32_t>(hkArrayBase<uint8_t>::DONT_DEALLOCATE_FLAG);
            hkMemoryAllocator* pTemp = hkMemoryRouter::getInstance().m_temp;
            uint8_t* pData = nullptr;
            int32_t iNumBytes = nelem;
            if (nelem) {
                pData = static_cast<uint8_t*>(pTemp->bufAlloc(iNumBytes));
            }
            buf.m_data = pData;
            buf.m_size = iNumBytes;
            buf.m_capacityAndFlags =
                iNumBytes ? iNumBytes : static_cast<int32_t>(hkArrayBase<uint8_t>::DONT_DEALLOCATE_FLAG);
            // EXE-INLINED none: hkArrayBase<hkUint8>::reserve
            hkMemoryAllocator* pTemp2 = hkMemoryRouter::getInstance().m_temp;
            const int32_t iCap = buf.m_capacityAndFlags & hkArrayBase<uint8_t>::CAPACITY_MASK;
            if (iCap < nelem) {
                int32_t iReq = 2 * iCap;
                if (nelem >= iReq) {
                    iReq = nelem;
                }
                hkArrayUtil::_reserve(*pTemp2, &buf, iReq, sizeof(uint8_t));
            }
            m_ia.readArrayGeneric(buf.m_data, 1, nelem);
            arr.setAll(buf.m_data, nelem);
            // EXE-INLINED none: hkArray<hkUint8, hkContainerTempAllocator>::~hkArray
            hkMemoryAllocator* pTemp3 = hkMemoryRouter::getInstance().m_temp;
            const int32_t iCapFlags = buf.m_capacityAndFlags;
            buf.m_size = 0;
            if (iCapFlags >= 0) {
                pTemp3->bufFree(
                    buf.m_data,
                    (iCapFlags & hkArrayBase<uint8_t>::CAPACITY_MASK) * static_cast<int32_t>(sizeof(uint8_t))
                );
            }
            return;
        }
        case hkDataObject::TYPE_INT: {
            for (int32_t i = 0; i < nelem; ++i) {
                const int32_t iValue = readInt<int32_t>();
                alignas(8) hkDataArray::Value value;
                arr.access(&value, i)->operator=(iValue);
            }
            return;
        }
        case hkDataObject::TYPE_REAL: {
            // EXE-INLINED none: hkArray<hkReal, hkContainerTempAllocator>::hkArray(int)
            XFNV_INLINED_LOCAL(hkArray<float>, buf, std::nullopt, std::nullopt);
            buf.m_data = nullptr;
            buf.m_size = 0;
            buf.m_capacityAndFlags = static_cast<int32_t>(hkArrayBase<float>::DONT_DEALLOCATE_FLAG);
            hkMemoryAllocator* pTemp = hkMemoryRouter::getInstance().m_temp;
            float* pData = nullptr;
            int32_t iNum = nelem;
            if (nelem) {
                int32_t iNumBytes = nelem * static_cast<int32_t>(sizeof(float));
                pData = static_cast<float*>(pTemp->bufAlloc(iNumBytes));
                iNum = iNumBytes / static_cast<int32_t>(sizeof(float));
            }
            buf.m_data = pData;
            buf.m_size = iNum;
            buf.m_capacityAndFlags = iNum ? iNum : static_cast<int32_t>(hkArrayBase<float>::DONT_DEALLOCATE_FLAG);
            // EXE-INLINED none: hkArray<hkReal, hkContainerTempAllocator>::setSize
            hkMemoryAllocator* pTemp2 = hkMemoryRouter::getInstance().m_temp;
            const int32_t iCap = buf.m_capacityAndFlags & hkArrayBase<float>::CAPACITY_MASK;
            if (iCap < nelem) {
                int32_t iReq = 2 * iCap;
                if (nelem >= iReq) {
                    iReq = nelem;
                }
                hkArrayUtil::_reserve(*pTemp2, &buf, iReq, sizeof(float));
            }
            buf.m_size = nelem;
            m_ia.readArrayGeneric(buf.m_data, 4, nelem);
            arr.setAll(buf.m_data, buf.m_size);
            // EXE-INLINED none: hkArray<hkReal, hkContainerTempAllocator>::~hkArray
            hkMemoryAllocator* pTemp3 = hkMemoryRouter::getInstance().m_temp;
            const int32_t iCapFlags = buf.m_capacityAndFlags;
            buf.m_size = 0;
            if (iCapFlags >= 0) {
                pTemp3->bufFree(
                    buf.m_data, (iCapFlags & hkArrayBase<float>::CAPACITY_MASK) * static_cast<int32_t>(sizeof(float))
                );
            }
            return;
        }
        case hkDataObject::TYPE_VEC_4: {
            // Reads the count before the item loop's test, and deliberately reads that many reals into a four-real
            // buffer, as the game does.
            const int32_t iNumReals = readInt<int32_t>();
            alignas(8) float vec[4];
            vec[0] = 0.0f;
            vec[1] = 0.0f;
            vec[2] = 0.0f;
            vec[3] = 0.0f;
            for (int32_t i = 0; i < nelem; ++i) {
                m_ia.readArrayGeneric(vec, 4, iNumReals);
                hkDataArray::Value value;
                arr.access(&value, i)->setVec(vec, 4);
            }
            return;
        }
        case hkDataObject::TYPE_VEC_12:
        case hkDataObject::TYPE_VEC_16: {
            // Asks for the type again.
            const int32_t iNumReals = 4 * arr.getType() - 12;
            for (int32_t i = 0; i < nelem; ++i) {
                alignas(8) float vec[16];
                for (int32_t k = 0; k < iNumReals; ++k) {
                    alignas(8) float fReal;
                    m_ia.readArrayGeneric((&fReal), 4, 1);
                    vec[k] = fReal;
                }
                alignas(8) hkDataArray::Value value;
                arr.access(&value, i)->setVec(vec, iNumReals);
            }
            return;
        }
        case hkDataObject::TYPE_OBJECT: {
            for (int32_t i = 0; i < nelem; ++i) {
                XFNV_INLINED_LOCAL(hkDataObject, obj, std::nullopt, std::nullopt);
                hkDataObject* pObj = readObject(&obj, -1);
                hkDataArray::Value value;
                arr.access(&value, i)->operator=(*pObj);
                obj.~hkDataObject();
            }
            return;
        }
        case hkDataObject::TYPE_STRUCT: {
            arr.setSize(nelem);
            hkDataClass klass;
            arr.getClass(&klass);
            const int32_t iNumMembers = klass.getNumMembers();

            alignas(8) XFNV_INLINED_LOCAL(hkLocalArray<uint8_t>, bitfield, std::nullopt, std::nullopt);
            bitfield.m_data = nullptr;
            bitfield.m_size = 0;
            bitfield.m_capacityAndFlags = static_cast<int32_t>(hkArrayBase<uint8_t>::DONT_DEALLOCATE_FLAG);
            // EXE-INLINED none: hkAllocateStack<hkUint8> (untyped stack memory, as the exe treats it)
            hkStackMemory& rStack = hkMemoryRouter::getInstance().m_stack;
            void* pBits = rStack.m_next.m_storage;
            rStack.m_next.m_storage = static_cast<char*>(pBits) + 128;
            bitfield.m_data = static_cast<uint8_t*>(pBits);
            bitfield.m_localMemory = static_cast<uint8_t*>(pBits);
            bitfield.m_capacityAndFlags = static_cast<int32_t>(128 | hkArrayBase<uint8_t>::DONT_DEALLOCATE_FLAG);
            readBitfield(iNumMembers, bitfield);

            XFNV_INLINED_LOCAL(hkLocalArray<hkDataClass::MemberInfo>, minfo, std::nullopt, std::nullopt);
            minfo.m_data = nullptr;
            minfo.m_size = 0;
            minfo.m_capacityAndFlags = static_cast<int32_t>(hkArrayBase<hkDataClass::MemberInfo>::DONT_DEALLOCATE_FLAG);
            // EXE-INLINED none: hkAllocateStack<hkDataClass::MemberInfo> (untyped stack memory, as the exe treats it)
            hkStackMemory& rStack2 = hkMemoryRouter::getInstance().m_stack;
            void* pInfos = rStack2.m_next.m_storage;
            rStack2.m_next.m_storage = static_cast<char*>(pInfos) +
                ((static_cast<int32_t>(sizeof(hkDataClass::MemberInfo)) * iNumMembers + 127) & ~127);
            minfo.m_capacityAndFlags = static_cast<int32_t>(
                static_cast<uint32_t>(iNumMembers) | hkArrayBase<hkDataClass::MemberInfo>::DONT_DEALLOCATE_FLAG
            );
            minfo.m_data = static_cast<hkDataClass::MemberInfo*>(pInfos);
            minfo.m_localMemory = static_cast<hkDataClass::MemberInfo*>(pInfos);
            // EXE-INLINED none: hkArray<hkDataClass::MemberInfo>::setSize; the capacity is iNumMembers already, so the
            // exe has no reserve. Each new element is value-initialised by a placement new (its null test kept).
            hkDataClass::MemberInfo* pInfo = minfo.m_data + minfo.m_size;
            for (int32_t iNew = iNumMembers - minfo.m_size; iNew > 0; --iNew, ++pInfo) {
                if (pInfo) {
                    pInfo->m_name = nullptr;
                    pInfo->m_owner = nullptr;
                    pInfo->m_type = hkDataObject::TYPE_VOID;
                    pInfo->m_class = nullptr;
                    pInfo->m_tupleCount = 0;
                }
            }
            minfo.m_size = iNumMembers;
            klass.getAllMemberInfo(minfo);

            for (int32_t m = 0; m < iNumMembers; ++m) {
                const hkDataClass::MemberInfo& mi = minfo.m_data[m];
                if (bitfield.m_data[m] == 0) {
                    continue;
                }
                XFNV_INLINED_LOCAL(hkDataArray, members, std::nullopt, std::nullopt);
                arr.swizzleObjectMember(&members, mi.m_name);
                if (mi.m_tupleCount == 0) {
                    _readArrayItems(members, nelem);
                }
                else {
                    for (int32_t i = 0; i < nelem; ++i) {
                        alignas(8) XFNV_INLINED_LOCAL(hkDataArray, tuple, std::nullopt, std::nullopt);
                        hkDataArray::Value value;
                        members.access(&value, i)->asArray(&tuple);
                        _readArrayItems(tuple, mi.m_tupleCount);
                        tuple.~hkDataArray();
                    }
                }
                members.~hkDataArray();
            }

            // EXE-INLINED none: hkLocalArray<hkDataClass::MemberInfo>::~hkLocalArray
            hkDataClass::MemberInfo* pInfoMemory = minfo.m_localMemory;
            if (pInfoMemory == minfo.m_data) {
                minfo.m_size = 0;
            }
            hkMemoryRouter::getInstance().m_stack.m_next.m_storage =
                static_cast<char*>(static_cast<void*>(pInfoMemory));
            hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
            const int32_t iInfoCap = minfo.m_capacityAndFlags;
            minfo.m_size = 0;
            if ((iInfoCap & static_cast<int32_t>(hkArrayBase<hkDataClass::MemberInfo>::DONT_DEALLOCATE_FLAG)) == 0) {
                pHeap->bufFree(
                    minfo.m_data,
                    (iInfoCap & hkArrayBase<hkDataClass::MemberInfo>::CAPACITY_MASK) *
                        static_cast<int32_t>(sizeof(hkDataClass::MemberInfo))
                );
            }
            minfo.m_data = nullptr;
            minfo.m_capacityAndFlags = static_cast<int32_t>(hkArrayBase<hkDataClass::MemberInfo>::DONT_DEALLOCATE_FLAG);

            // EXE-INLINED none: hkLocalArray<hkUint8>::~hkLocalArray
            uint8_t* pBitMemory = bitfield.m_localMemory;
            if (pBitMemory == bitfield.m_data) {
                bitfield.m_size = 0;
            }
            hkMemoryRouter::getInstance().m_stack.m_next.m_storage = static_cast<char*>(static_cast<void*>(pBitMemory));
            hkMemoryAllocator* pHeap2 = hkMemoryRouter::getInstance().m_heap;
            const int32_t iBitCap = bitfield.m_capacityAndFlags;
            bitfield.m_size = 0;
            if ((iBitCap & static_cast<int32_t>(hkArrayBase<uint8_t>::DONT_DEALLOCATE_FLAG)) == 0) {
                pHeap2->bufFree(bitfield.m_data, iBitCap & hkArrayBase<uint8_t>::CAPACITY_MASK);
            }
            bitfield.m_data = nullptr;
            bitfield.m_capacityAndFlags = static_cast<int32_t>(hkArrayBase<uint8_t>::DONT_DEALLOCATE_FLAG);
            return;
        }
        case hkDataObject::TYPE_CSTRING: {
            for (int32_t i = 0; i < nelem; ++i) {
                const char* pString = readString();
                hkDataArray::Value value;
                arr.access(&value, i)->operator=(pString);
            }
            return;
        }
        default: return;
    }
}

// A tuple reads the member's tuple count of items into a new array, an array reads its count (and, for an array of
// structs with no class, the class's tag) first; any other member is read by its type.
void Reader::readBinaryValue(hkDataObject::Value value, const hkDataClass::MemberInfo& minfo) {
    const hkDataObject::Type eType = minfo.m_type;
    if ((eType & hkDataObject::TYPE_TUPLE) != 0) {
        hkDataObject obj(value.m_impl);
        hkDataArray arr(m_world.newArray(obj, minfo));
        _readArrayItems(arr, minfo.m_tupleCount);
        return;
    }
    if ((eType & hkDataObject::TYPE_ARRAY) != 0) {
        hkDataClass::MemberInfo info = minfo;
        const int32_t iCount = readInt<int32_t>();
        if (value.getType() == hkDataObject::TYPE_ARRAY_STRUCT && !minfo.m_class) {
            const int32_t iClassTag = readInt<int32_t>();
            info.m_class = m_classes.m_data[iClassTag];
        }
        hkDataObject obj(value.m_impl);
        hkDataArray arr(m_world.newArray(obj, info));
        arr.setSize(iCount);
        _readArrayItems(arr, iCount);
        return;
    }
    switch (eType) {
        case hkDataObject::TYPE_BYTE: {
            uint8_t ucByte;
            m_ia.readArrayGeneric(&ucByte, 1, 1);
            value.setInt(ucByte);
            break;
        }
        case hkDataObject::TYPE_INT: {
            const int64_t iValue = readInt<int64_t>();
            value.setInt64(static_cast<int32_t>(iValue), static_cast<int32_t>(iValue >> 32));
            break;
        }
        case hkDataObject::TYPE_REAL: {
            float fValue = m_ia.readFloat32();
            value.setReal(fValue);
            break;
        }
        case hkDataObject::TYPE_VEC_4:
        case hkDataObject::TYPE_VEC_12:
        case hkDataObject::TYPE_VEC_16: {
            float vec[16];
            const int32_t iNumReals = 4 * eType - 12;
            for (int32_t i = 0; i < iNumReals; ++i) {
                float fReal;
                m_ia.readArrayGeneric((&fReal), 4, 1);
                vec[i] = fReal;
            }
            value.setVec(vec, iNumReals);
            break;
        }
        case hkDataObject::TYPE_OBJECT:
        case hkDataObject::TYPE_STRUCT: {
            XFNV_INLINED_LOCAL(hkDataObject, obj, std::nullopt, std::nullopt);
            readObject(&obj, -1);
            value.setObject(obj);
            obj.~hkDataObject();
            break;
        }
        case hkDataObject::TYPE_CSTRING: {
            value.setString(readString());
            break;
        }
        default: break;
    }
}

// Reads one object: a reference to a remembered object (5), a null object (6), or a new object of the class under the
// tag read next (3; 4 also remembers it), whose members the bitfield marks as present are read into it. The bitfield's
// and the member infos' local arrays come from the thread's stack memory.
hkDataObject* Reader::readObject(hkDataObject* apResult, int32_t aiType) {
    int32_t iType = aiType;
    if (aiType == -1) {
        iType = readInt<int32_t>();
    }
    if (iType == 5) {
        int32_t iIndex = readInt<int32_t>();
        ::new (apResult) hkDataObject(m_rememberedObjects.m_data[iIndex]);
        return apResult;
    }
    if (iType == 6) {
        ::new (apResult) hkDataObject(nullptr);
        return apResult;
    }
    int32_t iClassTag = readInt<int32_t>();
    hkDataClass klass(m_classes.m_data[iClassTag]);
    const int32_t numMembers = klass.getNumMembers();
    hkDataObject obj(m_world.newObject(klass));
    if (iType != 3) {
        if (iType == 4) {
            hkDataObjectImpl* pImpl = obj.m_impl;
            // EXE-INLINED none: hkArray<hkDataObjectImpl*>::pushBack
            hkMemoryRouter& router = hkMemoryRouter::getInstance();
            if (m_rememberedObjects.m_size ==
                (m_rememberedObjects.m_capacityAndFlags & hkArrayBase<hkDataObjectImpl*>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(
                    *router.m_heap, &m_rememberedObjects, static_cast<int32_t>(sizeof(hkDataObjectImpl*))
                );
            }
            m_rememberedObjects.m_data[m_rememberedObjects.m_size] = pImpl;
            ++m_rememberedObjects.m_size;
        }
        else {
            char buf[512];
            hkErrStream errStream(buf, 512);
            errStream.operator<<("corrupt file");
            if (hkError::s_instance->message(
                    hkError::MESSAGE_ERROR, 0x484994D5, buf, ".\\Tagfile\\Binary\\hkBinaryTagfileReader.cpp", 376
                )) {
                __builtin_debugtrap();
            }
        }
    }

    // EXE-INLINED none: hkLocalArray<uint8_t>::hkLocalArray(128)
    XFNV_INLINED_LOCAL(hkLocalArray<uint8_t>, bitfield, std::nullopt, std::nullopt);
    bitfield.m_data = nullptr;
    bitfield.m_size = 0;
    bitfield.m_capacityAndFlags = hkArrayBase<uint8_t>::DONT_DEALLOCATE_FLAG;
    {
        // EXE-INLINED none: hkAllocateStack<uint8_t> (untyped memory from the thread's stack allocator)
        hkStackMemory& stack = hkMemoryRouter::getInstance().m_stack;
        char* pBlock = stack.m_next.m_storage;
        stack.m_next.m_storage = pBlock + 128;
        bitfield.m_data = static_cast<uint8_t*>(static_cast<void*>(pBlock));
        bitfield.m_localMemory = static_cast<uint8_t*>(static_cast<void*>(pBlock));
        bitfield.m_capacityAndFlags = 128 | hkArrayBase<uint8_t>::DONT_DEALLOCATE_FLAG;
    }
    readBitfield(numMembers, bitfield);

    // EXE-INLINED none: hkLocalArray<hkDataClass::MemberInfo>::hkLocalArray(numMembers)
    XFNV_INLINED_LOCAL(hkLocalArray<hkDataClass::MemberInfo>, members, std::nullopt, std::nullopt);
    members.m_data = nullptr;
    members.m_size = 0;
    members.m_capacityAndFlags = hkArrayBase<hkDataClass::MemberInfo>::DONT_DEALLOCATE_FLAG;
    {
        // EXE-INLINED none: hkAllocateStack<hkDataClass::MemberInfo> (untyped memory, the byte count rounded up to 128)
        hkStackMemory& stack = hkMemoryRouter::getInstance().m_stack;
        char* pBlock = stack.m_next.m_storage;
        stack.m_next.m_storage =
            pBlock + ((static_cast<int32_t>(sizeof(hkDataClass::MemberInfo)) * numMembers + 127) & ~127);
        members.m_capacityAndFlags = numMembers | hkArrayBase<hkDataClass::MemberInfo>::DONT_DEALLOCATE_FLAG;
        members.m_data = static_cast<hkDataClass::MemberInfo*>(static_cast<void*>(pBlock));
        members.m_localMemory = static_cast<hkDataClass::MemberInfo*>(static_cast<void*>(pBlock));
    }
    // EXE-INLINED none: hkArray<hkDataClass::MemberInfo>::setSize
    {
        hkDataClass::MemberInfo* pInfo = members.m_data + members.m_size;
        for (int32_t iLeft = numMembers - members.m_size; iLeft > 0; --iLeft, ++pInfo) {
            // EXE-INLINED none: hkDataClass::MemberInfo::MemberInfo
            if (pInfo) {
                pInfo->m_name = nullptr;
                pInfo->m_owner = nullptr;
                pInfo->m_type = hkDataObject::TYPE_VOID;
                pInfo->m_class = nullptr;
                pInfo->m_tupleCount = 0;
            }
        }
        members.m_size = numMembers;
    }
    klass.getAllMemberInfo(members);
    for (int32_t i = 0; i < numMembers; ++i) {
        const hkDataClass::MemberInfo& info = members.m_data[i];
        if (bitfield.m_data[i]) {
            hkDataObject::Value value;
            readBinaryValue(*obj.access(&value, info.m_name), info);
        }
    }
    ::new (apResult) hkDataObject(obj);
    members.~hkLocalArray<hkDataClass::MemberInfo>();
    bitfield.~hkLocalArray<uint8_t>();
    return apResult;
}

// Reads a class description (name, version, parent tag, members) and registers it with the world, remembering the new
// class under the next class tag; fails, reading no more, when the world is a native one (type 0).
hkResult Reader::readClass() {
    // EXE-INLINED none: hkDataClass::Cinfo::Cinfo
    XFNV_INLINED_LOCAL(hkDataClass::Cinfo, info, std::nullopt, std::nullopt);
    info.members.m_data = nullptr;
    info.members.m_size = 0;
    info.members.m_capacityAndFlags = hkArrayBase<hkDataClass::Cinfo::Member>::DONT_DEALLOCATE_FLAG;
    info.name = readString();
    info.version = readInt<int32_t>();
    if (!m_world.getType().m_storage) {
        // EXE-INLINED none: hkDataClass::Cinfo::~Cinfo
        hkMemoryAllocator* heap = hkMemoryRouter::getInstance().m_heap;
        info.members.m_size = 0;
        if (info.members.m_capacityAndFlags >= 0) {
            heap->bufFree(
                info.members.m_data,
                (info.members.m_capacityAndFlags & hkArrayBase<hkDataClass::Cinfo::Member>::CAPACITY_MASK) *
                    static_cast<int32_t>(sizeof(hkDataClass::Cinfo::Member))
            );
        }
        return HK_FAILURE;
    }
    int32_t parentTag = readInt<int32_t>();
    if (parentTag >= 0 && m_classes.m_data[parentTag]) {
        info.parent = m_classes.m_data[parentTag]->getName();
    }
    else {
        info.parent = nullptr;
    }
    int32_t numMembers = readInt<int32_t>();
    {
        // EXE-INLINED none: hkArray<hkDataClass::Cinfo::Member>::setSize
        hkMemoryAllocator* heap = hkMemoryRouter::getInstance().m_heap;
        int32_t capacity = info.members.m_capacityAndFlags & hkArrayBase<hkDataClass::Cinfo::Member>::CAPACITY_MASK;
        if (capacity < numMembers) {
            int32_t doubled = 2 * capacity;
            hkArrayUtil::_reserve(
                *heap, &info.members, numMembers < doubled ? doubled : numMembers,
                static_cast<int32_t>(sizeof(hkDataClass::Cinfo::Member))
            );
        }
        info.members.m_size = numMembers;
    }
    for (int32_t i = 0; i < info.members.m_size; ++i) {
        hkDataClass::Cinfo::Member& member = info.members.m_data[i];
        member.name = readString();
        member.type = static_cast<hkDataObject::Type>(readInt<int32_t>());
        member.tupleCount = (member.type & hkDataObject::TYPE_TUPLE) ? readInt<int32_t>() : 0;
        int32_t basicType = member.type & hkDataObject::TYPE_MASK_BASIC_TYPES;
        member.className =
            (basicType == hkDataObject::TYPE_STRUCT || basicType == hkDataObject::TYPE_OBJECT) ? readString() : nullptr;
        if (member.className) {
            m_world.findClass(member.className);
        }
    }
    hkDataClassImpl* klass = m_world.newClass(info);
    {
        // EXE-INLINED none: hkArray<hkDataClassImpl*>::pushBack
        hkMemoryRouter& router = hkMemoryRouter::getInstance();
        if (m_classes.m_size == (m_classes.m_capacityAndFlags & hkArrayBase<hkDataClassImpl*>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*router.m_heap, &m_classes, static_cast<int32_t>(sizeof(hkDataClassImpl*)));
        }
        m_classes.m_data[m_classes.m_size] = klass;
        ++m_classes.m_size;
    }
    // EXE-INLINED none: hkDataClass::Cinfo::~Cinfo
    hkMemoryAllocator* heap = hkMemoryRouter::getInstance().m_heap;
    info.members.m_size = 0;
    if (info.members.m_capacityAndFlags >= 0) {
        heap->bufFree(
            info.members.m_data,
            (info.members.m_capacityAndFlags & hkArrayBase<hkDataClass::Cinfo::Member>::CAPACITY_MASK) *
                static_cast<int32_t>(sizeof(hkDataClass::Cinfo::Member))
        );
    }
    return HK_SUCCESS;
}

hkDataObject* Reader::readObjectTree(hkDataObject* apResult) {
    uint32_t v5;
    XFNV_INLINED_LOCAL(hkErrStream, apThis, std::nullopt, std::nullopt);
    uint32_t array;
    char buf[512];
    m_ia.readArrayGeneric(&array, 4, 1);
    m_ia.readArrayGeneric(&v5, 4, 1);
    if (array == -0x354FF2E2 && v5 == -0x2FEE0532) {
        int32_t v3 = readInt<int32_t>();
        if (v3 != -1) {
            while (true) {
                switch (v3) {
                    case 1: {
                        if (!readInt<int32_t>()) {
                            goto LABEL_8;
                        }
                        ::new (&apThis) hkErrStream(buf, 512);
                        apThis.operator<<("Unrecognised tagfile version.");
                        hkError::s_instance->message(
                            hkError::MESSAGE_WARNING, 0x2AB6036F, buf, ".\\Tagfile\\Binary\\hkBinaryTagfileReader.cpp",
                            473
                        );
                        goto LABEL_12;
                    }
                    case 2: {
                        if (readClass() == HK_FAILURE) {
                            break;
                        }
                    LABEL_8:;
                        v3 = readInt<int32_t>();
                        if (v3 == -1) {
                            break;
                        }
                        continue;
                    }
                    case 3:
                    case 4:
                    case 6: {
                        readObject(apResult, v3);
                        return apResult;
                    }
                    default: {
                        goto LABEL_13;
                    }
                }
                break;
            }
        }
    }
    else {
        ::new (&apThis) hkErrStream(buf, 512);
        apThis.operator<<("This does not look like a binary tagfile (magic number mismatch)");
        hkError::s_instance->message(
            hkError::MESSAGE_WARNING, 0x2AB6036F, buf, ".\\Tagfile\\Binary\\hkBinaryTagfileReader.cpp", 453
        );
    LABEL_12:;
        XFNV_DESTRUCT(hkOstream)(&apThis);
    }
LABEL_13:;
    ::new (apResult) hkDataObject(nullptr);
    return apResult;
}
