#include "Havok/Source/Common/Visualize/Process/hkInspectProcess.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/Registry/hkVtableClassRegistry.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/System/Io/Writer/Array/hkArrayStreamWriter.h"
#include "Havok/Source/Common/Serialize/Serialize/hkDisplaySerializeIStream.h"
#include "Havok/Source/Common/Serialize/Util/hkStructureLayout.h"
#include "Havok/Source/Common/Visualize/Serialize/hkDisplaySerializeOStream.h"
#include "Havok/Source/Common/Visualize/Serialize/hkObjectSerialize.h"
#include "Havok/Source/Common/Visualize/hkProcess.h"
#include "Havok/Source/Common/Visualize/hkProcessContext.h"
#include "Havok/Source/Common/Visualize/hkVisualDebugger.h"

using hkInspectProcessObjectPairArray = hkArray<hkInspectProcess::ObjectPair>;
using hkInspectProcessGlobalFixupArray = hkArray<hkObjectSerialize::GlobalFixup>;
using hkInspectProcessCharArray = hkArray<char>;
using hkInspectProcessUInt16Array = hkArray<uint16_t>;

// The commands the process consumes, a writable byte array whose address is handed out.
using hkInspectProcessCommands = uint8_t[5];
static hkInspectProcessCommands s_inspectCommands{33, 34, 35, 36, 37};

void myTrackedObjectCallback(void* a1, const hkClass* a2, hkBool a3, hkUlong a4, void* a5) {
    if (a3.m_bool) {
        static_cast<hkInspectProcess*>(a5)->addTopLevelObject(a1, *a2, a4);
    }
    else {
        static_cast<hkInspectProcess*>(a5)->removeTopLevelObject(a1);
    }
}

void* getMemberAddressFromPath(
    hkClassMember::Type type,
    void* address,
    const hkArray<uint16_t>& path,
    int32_t index,
    const hkClassMember* member
) {
    char* pAddress = static_cast<char*>(address);
    for (;;) {
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
            case hkClassMember::TYPE_ENUM:
            case hkClassMember::TYPE_ULONG:
            case hkClassMember::TYPE_FLAGS:
            case hkClassMember::TYPE_HALF: return pAddress;
            case hkClassMember::TYPE_MATRIX3:
            case hkClassMember::TYPE_ROTATION:
            case hkClassMember::TYPE_QSTRANSFORM:
            case hkClassMember::TYPE_MATRIX4:
            case hkClassMember::TYPE_TRANSFORM:
                // A column of the matrix.
                ++index;
                pAddress += path.m_data[index] * static_cast<int32_t>(sizeof(hkVector4));
                type = hkClassMember::TYPE_VECTOR4;
                break;
            case hkClassMember::TYPE_ARRAY:
            case hkClassMember::TYPE_SIMPLEARRAY: {
                // An element of the array: the member's bytes start with its data pointer (reflected data read
                // untyped).
                ++index;
                const int32_t element = path.m_data[index];
                pAddress = *reinterpret_cast<char**>(pAddress) + member->getArrayMemberSize() * element;
                type = static_cast<hkClassMember::Type>(member->m_subtype.m_storage);
                break;
            }
            case hkClassMember::TYPE_STRUCT: {
                if (!member || !member->m_class) {
                    return nullptr;
                }
                const hkClass* pStruct = &member->getStructClass();
                if (!pStruct) {
                    return nullptr;
                }
                ++index;
                member = &pStruct->getMember(path.m_data[index]);
                pAddress += member->m_offset;
                type = static_cast<hkClassMember::Type>(member->m_type.m_storage);
                break;
            }
            default: return nullptr;
        }
    }
}

hkInspectProcess::hkInspectProcess(const hkArray<hkProcessContext*>& contexts) : hkProcess(true) {
    m_vdb = contexts.m_size > 0 ? contexts.m_data[0]->m_owner : nullptr;
    if (m_vdb) {
        m_vdb->addReference();
        m_vdb->addTrackedObjectCallback(&myTrackedObjectCallback, this);
    }
}

hkInspectProcess::~hkInspectProcess() {
    if (m_vdb) {
        for (int32_t i = 0; i < m_vdb->m_trackedObjects.m_size; ++i) {
            removeTopLevelObject(m_vdb->m_trackedObjects.m_data[i].m_ptr);
        }
        m_vdb->removeTrackedObjectCallback(&myTrackedObjectCallback);
        m_vdb->removeReference();
    }
}

int32_t hkInspectProcess::getProcessTag() {
    return m_tag;
}

void hkInspectProcess::getConsumableCommands(uint8_t*& commands, int32_t& numCommands) {
    commands = s_inspectCommands;
    numCommands = 5;
}

void hkInspectProcess::consumeCommand(uint8_t command) {
    switch (command) {
        case HK_REQUEST_OBJECT: {
            // Reads the object's id, its class id and the request's flags (1: update it every step, 2: write the
            // objects it points to).
            uint64_t id;
            m_inStream->readArrayGeneric(&id, 8, 1);
            uint64_t classId;
            m_inStream->readArrayGeneric(&classId, 8, 1);
            uint8_t flags;
            m_inStream->readArrayGeneric(&flags, 1, 1);
            if (!id || !classId) {
                return;
            }
            // The client's ids are the addresses of the object and its class.
            void* ptr = reinterpret_cast<void*>(static_cast<hkUlong>(id));
            const hkClass* pClass = reinterpret_cast<const hkClass*>(static_cast<hkUlong>(classId));
            XFNV_INLINED_LOCAL(hkInspectProcessObjectPairArray, recursionStack, std::nullopt, std::nullopt);
            // EXE-INLINED none: hkArray<hkInspectProcess::ObjectPair>::hkArray
            recursionStack.m_data = nullptr;
            recursionStack.m_size = 0;
            recursionStack.m_capacityAndFlags = hkArrayBase<ObjectPair>::DONT_DEALLOCATE_FLAG;
            const bool bRecurse = pClass == &hkClassClass || (flags & 2) != 0;
            writeObject(ptr, *pClass, bRecurse, recursionStack);
            // EXE-INLINED none: hkArray<hkInspectProcess::ObjectPair>::indexOf
            const int32_t size = m_autoUpdateList.m_size;
            int32_t index = 0;
            for (; index < size; ++index) {
                if (m_autoUpdateList.m_data[index].obj == ptr) {
                    break;
                }
            }
            const bool bListed = index != size;
            if (flags & 1) {
                if (!bListed) {
                    // EXE-INLINED none: hkArray<hkInspectProcess::ObjectPair>::pushBack
                    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
                    if (m_autoUpdateList.m_size ==
                        static_cast<int32_t>(
                            m_autoUpdateList.m_capacityAndFlags & hkArrayBase<ObjectPair>::CAPACITY_MASK
                        )) {
                        hkArrayUtil::_reserveMore(*pHeap, &m_autoUpdateList, sizeof(ObjectPair));
                    }
                    ObjectPair& pair = m_autoUpdateList.m_data[m_autoUpdateList.m_size++];
                    pair.klass = pClass;
                    pair.obj = ptr;
                }
            }
            else if (bListed) {
                // EXE-INLINED none: hkArray<hkInspectProcess::ObjectPair>::removeAt
                --m_autoUpdateList.m_size;
                if (m_autoUpdateList.m_size != index) {
                    m_autoUpdateList.m_data[index] = m_autoUpdateList.m_data[m_autoUpdateList.m_size];
                }
            }
            // EXE-INLINED none: ~hkArray<hkInspectProcess::ObjectPair>
            hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
            recursionStack.m_size = 0;
            if (recursionStack.m_capacityAndFlags >= 0) {
                pHeap->bufFree(
                    recursionStack.m_data,
                    static_cast<int32_t>(
                        (recursionStack.m_capacityAndFlags & hkArrayBase<ObjectPair>::CAPACITY_MASK) *
                        sizeof(ObjectPair)
                    )
                );
            }
            return;
        }
        case HK_UPDATE_MEMBER: {
            // The object's id, its class id, the member path (one index per level) and the member's new bytes.
            uint64_t id;
            m_inStream->readArrayGeneric(&id, 8, 1);
            uint64_t classId;
            m_inStream->readArrayGeneric(&classId, 8, 1);
            uint16_t numMembers;
            m_inStream->readArrayGeneric(&numMembers, 2, 1);
            const int32_t pathLength = numMembers;
            XFNV_INLINED_LOCAL(hkInspectProcessUInt16Array, memberPath, std::nullopt, std::nullopt);
            {
                // EXE-INLINED none: hkArray<hkUint16>::hkArray(pathLength)
                memberPath.m_data = nullptr;
                memberPath.m_size = 0;
                memberPath.m_capacityAndFlags = hkArrayBase<uint16_t>::DONT_DEALLOCATE_FLAG;
                hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
                int32_t count = pathLength;
                uint16_t* pData = nullptr;
                if (count) {
                    int32_t numBytes = count * static_cast<int32_t>(sizeof(uint16_t));
                    pData = static_cast<uint16_t*>(pHeap->bufAlloc(numBytes));
                    count = numBytes / static_cast<int32_t>(sizeof(uint16_t));
                }
                memberPath.m_data = pData;
                memberPath.m_size = count;
                memberPath.m_capacityAndFlags =
                    count ? count : static_cast<int32_t>(hkArrayBase<uint16_t>::DONT_DEALLOCATE_FLAG);
            }
            {
                // EXE-INLINED none: hkArray<hkUint16>::setSize
                hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
                const int32_t capacity =
                    static_cast<int32_t>(memberPath.m_capacityAndFlags & hkArrayBase<uint16_t>::CAPACITY_MASK);
                if (capacity < pathLength) {
                    const int32_t doubled = 2 * capacity;
                    hkArrayUtil::_reserve(
                        *pHeap, &memberPath, pathLength < doubled ? doubled : pathLength, sizeof(uint16_t)
                    );
                }
                memberPath.m_size = pathLength;
            }
            for (int32_t i = 0; i < pathLength; ++i) {
                uint16_t memberIndex;
                m_inStream->readArrayGeneric(&memberIndex, 2, 1);
                memberPath.m_data[i] = memberIndex;
            }
            int32_t dataSize;
            m_inStream->readArrayGeneric(&dataSize, 4, 1);
            XFNV_INLINED_LOCAL(hkInspectProcessCharArray, data, std::nullopt, std::nullopt);
            {
                // EXE-INLINED none: hkArray<char>::hkArray(dataSize)
                data.m_data = nullptr;
                data.m_size = 0;
                data.m_capacityAndFlags = hkArrayBase<char>::DONT_DEALLOCATE_FLAG;
                hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
                int32_t count = dataSize;
                char* pData = nullptr;
                if (count) {
                    int32_t numBytes = count;
                    pData = static_cast<char*>(pHeap->bufAlloc(numBytes));
                    count = numBytes;
                }
                data.m_data = pData;
                data.m_size = count;
                data.m_capacityAndFlags = count ? count : static_cast<int32_t>(hkArrayBase<char>::DONT_DEALLOCATE_FLAG);
            }
            {
                // EXE-INLINED none: hkArray<char>::setSize
                hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
                const int32_t capacity =
                    static_cast<int32_t>(data.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK);
                if (capacity < dataSize) {
                    const int32_t doubled = 2 * capacity;
                    hkArrayUtil::_reserve(*pHeap, &data, dataSize < doubled ? doubled : dataSize, sizeof(char));
                }
            }
            char* pBytes = data.m_data;
            data.m_size = dataSize;
            m_inStream->readRaw(pBytes, dataSize);
            if (id && classId && memberPath.m_size > 0 && dataSize) {
                // The client's ids are the addresses of the object and its class.
                hkClass* pClass = reinterpret_cast<hkClass*>(static_cast<hkUlong>(classId));
                const hkClassMember& member = pClass->getMember(memberPath.m_data[0]);
                void* pTarget = getMemberAddressFromPath(
                    static_cast<hkClassMember::Type>(member.m_type.m_storage),
                    reinterpret_cast<char*>(static_cast<hkUlong>(id)) + member.m_offset, memberPath, 0, &member
                );
                if (pTarget) {
                    hkString::memCpy(pTarget, data.m_data, dataSize);
                }
            }
            {
                // EXE-INLINED none: ~hkArray<char>
                hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
                data.m_size = 0;
                if (data.m_capacityAndFlags >= 0) {
                    pHeap->bufFree(
                        data.m_data, static_cast<int32_t>(data.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK)
                    );
                }
                data.m_data = nullptr;
                data.m_capacityAndFlags = hkArrayBase<char>::DONT_DEALLOCATE_FLAG;
            }
            // EXE-INLINED none: ~hkArray<hkUint16>
            hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
            memberPath.m_size = 0;
            if (memberPath.m_capacityAndFlags >= 0) {
                pHeap->bufFree(
                    memberPath.m_data,
                    static_cast<int32_t>(
                        (memberPath.m_capacityAndFlags & hkArrayBase<uint16_t>::CAPACITY_MASK) * sizeof(uint16_t)
                    )
                );
            }
            return;
        }
        default: return;
    }
}

void hkInspectProcess::init() {
    if (m_vdb) {
        m_outStream->write32u(13);
        m_outStream->write8u(HK_SETUP);
        // The client knows classes by their addresses.
        m_outStream->write64u(reinterpret_cast<hkUlong>(&hkClassClass));
        m_outStream->write8u(hkStructureLayout::HostLayoutRules.m_bytesInPointer);
        m_outStream->write8u(hkStructureLayout::HostLayoutRules.m_littleEndian);
        m_outStream->write8u(hkStructureLayout::HostLayoutRules.m_reusePaddingOptimization);
        m_outStream->write8u(hkStructureLayout::HostLayoutRules.m_emptyBaseClassOptimization);
        const hkArray<hkVisualDebuggerTrackedObject>& tracked = m_vdb->m_trackedObjects;
        for (int32_t i = 0; i < tracked.m_size; ++i) {
            addTopLevelObject(tracked.m_data[i].m_ptr, *tracked.m_data[i].m_class, tracked.m_data[i].m_tag);
        }
    }
}

void hkInspectProcess::step(float frameTimeInMs) {
    XFNV_INLINED_LOCAL(hkInspectProcessObjectPairArray, recursionStack, std::nullopt, std::nullopt);
    // EXE-INLINED none: hkArray<hkInspectProcess::ObjectPair>::hkArray
    recursionStack.m_data = nullptr;
    recursionStack.m_size = 0;
    recursionStack.m_capacityAndFlags = hkArrayBase<ObjectPair>::DONT_DEALLOCATE_FLAG;
    for (int32_t i = 0; i < m_autoUpdateList.m_size; ++i) {
        // EXE-INLINED none: hkArray<hkInspectProcess::ObjectPair>::setSize(0): the heap is fetched for its reserve,
        // which never runs (a capacity is never below 0).
        hkMemoryRouter::getInstance();
        recursionStack.m_size = 0;
        writeObject(m_autoUpdateList.m_data[i].obj, *m_autoUpdateList.m_data[i].klass, false, recursionStack);
    }
    // EXE-INLINED none: ~hkArray<hkInspectProcess::ObjectPair>
    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
    recursionStack.m_size = 0;
    if (recursionStack.m_capacityAndFlags >= 0) {
        pHeap->bufFree(
            recursionStack.m_data,
            static_cast<int32_t>(
                (recursionStack.m_capacityAndFlags & hkArrayBase<ObjectPair>::CAPACITY_MASK) * sizeof(ObjectPair)
            )
        );
    }
}

int32_t hkInspectProcess::addTopLevelObject(void* ptr, const hkClass& klass, hkUlong tag) {
    m_outStream->write32u(25);
    // The most derived class from the debugger's registry when the class is virtual.
    const hkClass* pClass = &klass;
    if (m_vdb->m_classReg) {
        hkBool bHasVtable;
        if (klass.hasVtable(&bHasVtable)->m_bool) {
            pClass = m_vdb->m_classReg->getClassFromVirtualInstance(ptr);
            if (!pClass) {
                pClass = &klass;
            }
        }
    }
    m_outStream->write8u(HK_ADD_TOPLEVEL);
    // The client knows objects and classes by their addresses.
    m_outStream->write64u(reinterpret_cast<hkUlong>(ptr));
    m_outStream->write64u(reinterpret_cast<hkUlong>(pClass));
    m_outStream->write64u(tag);
    return 25;
}

int32_t hkInspectProcess::removeTopLevelObject(void* ptr) {
    m_outStream->write32u(9);
    m_outStream->write8u(HK_REMOVE_TOPLEVEL);
    // The client knows objects by their addresses.
    m_outStream->write64u(reinterpret_cast<hkUlong>(ptr));
    const int32_t size = m_autoUpdateList.m_size;
    int32_t index = 0;
    for (; index < size; ++index) {
        if (m_autoUpdateList.m_data[index].obj == ptr) {
            break;
        }
    }
    if (index < size) {
        // EXE-INLINED none: hkArray<hkInspectProcess::ObjectPair>::removeAt
        --m_autoUpdateList.m_size;
        if (m_autoUpdateList.m_size != index) {
            m_autoUpdateList.m_data[index] = m_autoUpdateList.m_data[m_autoUpdateList.m_size];
        }
    }
    return 9;
}

int32_t hkInspectProcess::writeObject(
    const void* ptr,
    const hkClass& klass,
    hkBool recurse,
    hkArray<ObjectPair>& recursionStack
) {
    hkStructureLayout layout(hkStructureLayout::MsvcWin32LayoutRules);
    XFNV_INLINED_LOCAL(hkInspectProcessGlobalFixupArray, globalFixups, std::nullopt, std::nullopt);
    // EXE-INLINED none: hkArray<hkObjectSerialize::GlobalFixup>::hkArray
    globalFixups.m_data = nullptr;
    globalFixups.m_size = 0;
    globalFixups.m_capacityAndFlags = hkArrayBase<hkObjectSerialize::GlobalFixup>::DONT_DEALLOCATE_FLAG;
    // The most derived class from the debugger's registry when the class is virtual.
    const hkClass* pClass = &klass;
    if (m_vdb->m_classReg) {
        hkBool bHasVtable;
        if (klass.hasVtable(&bHasVtable)->m_bool) {
            pClass = m_vdb->m_classReg->getClassFromVirtualInstance(ptr);
            if (!pClass) {
                pClass = &klass;
            }
        }
    }
    if (recurse.m_bool) {
        // EXE-INLINED none: hkArray<hkInspectProcess::ObjectPair>::indexOf
        const int32_t size = recursionStack.m_size;
        int32_t index = 0;
        for (; index < size; ++index) {
            if (recursionStack.m_data[index].obj == ptr) {
                break;
            }
        }
        if (index < size) {
            // Written already.
            hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
            globalFixups.m_size = 0;
            if (globalFixups.m_capacityAndFlags >= 0) {
                pHeap->bufFree(
                    globalFixups.m_data,
                    static_cast<int32_t>(
                        (globalFixups.m_capacityAndFlags & hkArrayBase<hkObjectSerialize::GlobalFixup>::CAPACITY_MASK) *
                        sizeof(hkObjectSerialize::GlobalFixup)
                    )
                );
            }
            return 0;
        }
        // EXE-INLINED none: hkArray<hkInspectProcess::ObjectPair>::pushBack
        hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
        if (recursionStack.m_size ==
            static_cast<int32_t>(recursionStack.m_capacityAndFlags & hkArrayBase<ObjectPair>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*pHeap, &recursionStack, sizeof(ObjectPair));
        }
        ObjectPair& pair = recursionStack.m_data[recursionStack.m_size++];
        pair.obj = ptr;
        pair.klass = pClass;
    }
    XFNV_INLINED_LOCAL(hkInspectProcessCharArray, buffer, std::nullopt, std::nullopt);
    // EXE-INLINED none: hkArray<char>::hkArray
    buffer.m_data = nullptr;
    buffer.m_size = 0;
    buffer.m_capacityAndFlags = hkArrayBase<char>::DONT_DEALLOCATE_FLAG;
    int32_t result;
    {
        hkArrayStreamWriter writer(&buffer, hkArrayStreamWriter::ARRAY_BORROW);
        // The object's id is its address.
        const int32_t bytesWritten =
            hkObjectSerialize::writeObject(&writer, layout, reinterpret_cast<hkUlong>(ptr), ptr, *pClass, globalFixups);
        if (bytesWritten == buffer.m_size && bytesWritten >= 1) {
            int32_t total = bytesWritten + 1;
            m_outStream->write32u(total);
            m_outStream->write8u(HK_ADD_OBJECT);
            m_outStream->writeRaw(buffer.m_data, buffer.m_size);
            if (recurse.m_bool) {
                // The objects it points to: a global fixup's ids are the addresses of the object and its class.
                for (int32_t i = 0; i < globalFixups.m_size; ++i) {
                    const hkObjectSerialize::GlobalFixup& fixup = globalFixups.m_data[i];
                    total += writeObject(
                        reinterpret_cast<const void*>(static_cast<hkUlong>(fixup.toID)),
                        *reinterpret_cast<const hkClass*>(static_cast<hkUlong>(fixup.toClassID)), true, recursionStack
                    );
                }
            }
            result = total;
        }
        else {
            result = -1;
        }
    }
    // EXE-INLINED none: ~hkArray<char>
    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
    buffer.m_size = 0;
    if (buffer.m_capacityAndFlags >= 0) {
        pHeap->bufFree(
            buffer.m_data, static_cast<int32_t>(buffer.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK)
        );
    }
    buffer.m_data = nullptr;
    buffer.m_capacityAndFlags = hkArrayBase<char>::DONT_DEALLOCATE_FLAG;
    // EXE-INLINED none: ~hkArray<hkObjectSerialize::GlobalFixup>
    pHeap = hkMemoryRouter::getInstance().m_heap;
    globalFixups.m_size = 0;
    if (globalFixups.m_capacityAndFlags >= 0) {
        pHeap->bufFree(
            globalFixups.m_data,
            static_cast<int32_t>(
                (globalFixups.m_capacityAndFlags & hkArrayBase<hkObjectSerialize::GlobalFixup>::CAPACITY_MASK) *
                sizeof(hkObjectSerialize::GlobalFixup)
            )
        );
    }
    return result;
}

void hkInspectProcess::registerProcess() {
    m_tag = hkProcessFactory::s_instance->registerProcess(hkInspectProcess_Name, &hkInspectProcess::create);
}

hkProcess* hkInspectProcess::create(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts) {
    // EXE-INLINED none: hkInspectProcess::operator new
    void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkInspectProcess));
    static_cast<hkReferencedObject*>(pMemory)->m_memSizeAndFlags = sizeof(hkInspectProcess);
    hkInspectProcess* pProcess = ::new (pMemory) hkInspectProcess(contexts);
    return pProcess;
}
