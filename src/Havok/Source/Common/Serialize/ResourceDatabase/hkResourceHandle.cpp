#include "Havok/Source/Common/Serialize/ResourceDatabase/hkResourceHandle.h"

#include "BSCore/MemoryManager.h"
#include "Havok/Source/Animation/Animation/Rig/hkaBone.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/String/hkStringBuf.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/StringMap/hkCachedHashMap.h"
#include "Havok/Source/Common/Base/Container/StringMap/hkStringMap.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMemberAccessor.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkRefVariant.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkMemoryResourceHandleExternalLinkTypeInfo = {
    "hkMemoryResourceHandleExternalLink",
    finishLoadedObjecthkMemoryResourceHandleExternalLink,
    cleanupLoadedObjecthkMemoryResourceHandleExternalLink,
    nullptr,
    &typeid(hkMemoryResourceHandle::ExternalLink),
    sizeof(hkMemoryResourceHandle::ExternalLink)
};

hkResourceMap::~hkResourceMap() {}

void hkResourceMap::operator delete(void* apMemory) {
    ourFree(apMemory);
}

hkResourceBase::Type hkResourceHandle::getType() const {
    return TYPE_RESOURCE;
}

// The links are collected in a local hkArray (no storage, not ours) and walked from the last one. A link whose target
// the map knows is resolved when the member's class is the target's class or one of its bases: the target is stored
// through the member's accessor and the link removed, both through the vtable. A target that is the handle's own object
// ends the walk.
void hkResourceHandle::tryToResolveLinks(hkResourceMap& map) {
    XFNV_INLINED_LOCAL(hkArray<hkResourceHandle::Link>, links, std::nullopt, std::nullopt);
    // EXE-INLINED none: hkArray<hkResourceHandle::Link>::hkArray
    links.m_data = nullptr;
    links.m_size = 0;
    links.m_capacityAndFlags = hkArrayBase<hkResourceHandle::Link>::DONT_DEALLOCATE_FLAG;
    getExternalLinks(links);
    for (int32_t i = links.m_size - 1; i >= 0; --i) {
        hkResourceHandle::Link& rLink = links.m_data[i];
        const hkClass* pTargetClass;
        void* pTarget = map.findObjectByName(rLink.m_externalId, &pTargetClass);
        if (!pTarget) {
            continue;
        }
        const hkClassMember& rMember = rLink.m_memberAccessor.getClassMember();
        const hkClass* pMemberClass = rMember.getClass();
        if (pMemberClass->isSuperClass(*pTargetClass).m_bool || pTargetClass == pMemberClass) {
            if (pTarget == getObject()) {
                break;
            }
            rLink.m_memberAccessor.asPointer(0) = pTarget;
            removeExternalLink(rMember.m_name);
        }
        else {
            char buf[512];
            hkErrStream kStream(buf, 512);
            const char* pcMemberClassName = pMemberClass->getName();
            const char* pcTargetClassName = pTargetClass->getName();
            kStream << "Class mismatch, cannot resolve link: " << pcTargetClassName << " != " << pcMemberClassName;
            hkError::s_instance->message(
                hkError::MESSAGE_WARNING, static_cast<int32_t>(0xF034ED21), buf,
                ".\\ResourceDatabase\\hkResourceHandle.cpp", 178
            );
        }
    }

    // EXE-INLINED none: hkArray<hkResourceHandle::Link>::~hkArray
    hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
    const int32_t capacityAndFlags = links.m_capacityAndFlags;
    links.m_size = 0;
    if (capacityAndFlags >= 0) {
        heap.bufFree(
            links.m_data,
            (capacityAndFlags & hkArrayBase<hkResourceHandle::Link>::CAPACITY_MASK) *
                static_cast<int32_t>(sizeof(hkResourceHandle::Link))
        );
    }
}

hkResourceHandle::~hkResourceHandle() {}

hkMemoryResourceHandle::hkMemoryResourceHandle() {}

// The destructor only destroys the members: the links, the name and the variant's reference.
hkMemoryResourceHandle::~hkMemoryResourceHandle() {}

const char* hkMemoryResourceHandle::getName(hkStringBuf& buffer) const {
    const char* pcName = m_name.cString();
    if (!pcName) {
        return "null";
    }
    return pcName;
}

void hkMemoryResourceHandle::setName(const char* name) {
    &(&m_name)->operator=(name);
}

void* hkMemoryResourceHandle::getObject() const {
    return m_variant.m_pntr;
}

const hkClass* hkMemoryResourceHandle::getClass() const {
    return m_variant.getClass();
}

void hkMemoryResourceHandle::setObject(void* object, const hkClass* klass) {
    m_variant.set(object, klass);
}

void hkMemoryResourceHandle::addExternalLink(const char* memberName, const char* externalId) {
    hkArray<hkMemoryResourceHandle::ExternalLink, hkContainerHeapAllocator>* p_m_references = &m_references;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (p_m_references->m_size ==
        (p_m_references->m_capacityAndFlags & hkArrayBase<hkMemoryResourceHandle::ExternalLink>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_references, sizeof(hkMemoryResourceHandle::ExternalLink));
    }
    int32_t m_size = p_m_references->m_size;
    hkStringPtr* p_m_memberName = &p_m_references->m_data[m_size].m_memberName;
    if (p_m_memberName) {
        ::new (&p_m_references->m_data[m_size].m_memberName) hkStringPtr;
        ::new (p_m_memberName + 1) hkStringPtr;
    }
    int32_t v7 = p_m_references->m_size;
    hkStringPtr* v8 = &p_m_references->m_data[v7].m_memberName;
    p_m_references->m_size = v7 + 1;
    v8->operator=(memberName);
    (v8 + 1)->operator=(externalId);
}

// The first link of memberName is destroyed and the last link moved into its slot.
void hkMemoryResourceHandle::removeExternalLink(const char* memberName) {
    for (int32_t i = 0; i < m_references.m_size; ++i) {
        if (hkString::strCmp(m_references.m_data[i].m_memberName.cString(), memberName) == 0) {
            // EXE-INLINED none: hkArrayBase<hkMemoryResourceHandle::ExternalLink>::removeAt
            ExternalLink* pLink = &m_references.m_data[i];
            pLink->m_externalId.~hkStringPtr();
            pLink->m_memberName.~hkStringPtr();
            const int32_t iLast = --m_references.m_size;
            if (iLast != i) {
                // EXE-INLINED 0x6267C0
                ExternalLink* pDst = &m_references.m_data[i];
                const ExternalLink* pSrc = &m_references.m_data[iLast];
                pDst->m_memberName.m_stringAndFlag = pSrc->m_memberName.m_stringAndFlag;
                pDst->m_externalId.m_stringAndFlag = pSrc->m_externalId.m_stringAndFlag;
            }
            return;
        }
    }
}

// One link per stored reference: the member path ("a.b.c") is walked through the struct members from the object's
// class, and a reference whose member is missing (or whose path crosses a non-struct member) is dropped with a
// warning.
void hkMemoryResourceHandle::getExternalLinks(hkArray<hkResourceHandle::Link>& linksOut) {
    linksOut.m_size = 0;
    int32_t iNumReferences = m_references.m_size;
    // EXE-INLINED none: hkArray::reserve
    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
    int32_t iCapacity = static_cast<int32_t>(linksOut.m_capacityAndFlags & hkArrayBase<Link>::CAPACITY_MASK);
    if (iCapacity < iNumReferences) {
        int32_t iNewCapacity = iCapacity * 2;
        if (iNumReferences >= iNewCapacity) {
            iNewCapacity = iNumReferences;
        }
        // The array is handed to the untyped hkArrayUtil::_reserve as a raw array.
        hkArrayUtil::_reserve(
            pHeap, static_cast<hkArrayBase<void>*>(static_cast<void*>(&linksOut)), iNewCapacity, sizeof(Link)
        );
    }
    char acMemberName[512];
    char acMessage[512];
    for (int32_t i = 0; i < m_references.m_size; ++i) {
        // EXE-INLINED none: hkArray::expandOne
        Link* pLink = linksOut.expandByUnchecked(1);
        const char* pcName = m_references.m_data[i].m_memberName.cString();
        void* pObject = m_variant.m_pntr;
        const hkClass* pClass = m_variant.getClass();
        const char* pcDot = hkString::strChr(pcName, '.');
        bool bPathFound = true;
        while (pcDot) {
            int32_t iLength = static_cast<int32_t>(pcDot - pcName);
            hkString::memCpy(acMemberName, pcName, iLength);
            acMemberName[iLength] = 0;
            pcName += iLength + 1;
            const hkClassMember* pMember = pClass->getMemberByName(acMemberName);
            if (!pMember) {
                bPathFound = false;
                break;
            }
            if (pMember->m_type.m_storage != hkClassMember::TYPE_STRUCT) {
                alignas(hkErrStream) uint8_t aStream[sizeof(hkErrStream)];
                hkErrStream* pStream = ::new (aStream) hkErrStream(acMessage, 512);
                *pStream << "Member is not of type struct : " << acMemberName;
                hkError::s_instance->message(
                    hkError::MESSAGE_WARNING, static_cast<int32_t>(0xF032EDFE), acMessage,
                    ".\\ResourceDatabase\\hkResourceHandle.cpp", 0x80
                );
                pStream->~hkErrStream();
                bPathFound = false;
                break;
            }
            pClass = pMember->getClass();
            pObject = static_cast<char*>(pObject) + pMember->m_offset;
            pcDot = hkString::strChr(pcName, '.');
        }
        if (bPathFound) {
            const hkClassMember* pMember = pClass->getMemberByName(pcName);
            if (pMember) {
                pLink->m_memberName = pcName;
                hkClassMemberAccessor kAccessor;
                const hkClassMemberAccessor* pAccessor =
                    XFNV_RECONSTRUCT(hkClassMemberAccessor)(&kAccessor, pObject, pMember);
                pLink->m_memberAccessor = *pAccessor;
                pLink->m_externalId = m_references.m_data[i].m_externalId.cString();
                continue;
            }
        }
        linksOut.m_size--;
        alignas(hkErrStream) uint8_t aStream[sizeof(hkErrStream)];
        hkErrStream* pStream = ::new (aStream) hkErrStream(acMessage, 512);
        *pStream << "Cannot find member : " << pcName;
        hkError::s_instance->message(
            hkError::MESSAGE_WARNING, static_cast<int32_t>(0xF032EDF1), acMessage,
            ".\\ResourceDatabase\\hkResourceHandle.cpp", 0x92
        );
        pStream->~hkErrStream();
    }
}

void hkMemoryResourceHandle::clearExternalLinks() {
    hkMemoryResourceHandle::ExternalLink* m_data =
        static_cast<hkArrayBase<hkMemoryResourceHandle::ExternalLink>&>(m_references).begin();
    if (m_references.m_size > 0) {
        uint32_t m_size = m_references.m_size;
        do {
            m_data->m_externalId.~hkStringPtr();
            m_data->m_memberName.~hkStringPtr();
            ++m_data;
            --m_size;
        }
        while (m_size);
    }
    m_references.m_size = 0;
}

hkResourceBase::Type hkResourceContainer::getType() const {
    return TYPE_RESOURCE;
}

// The handles are collected in a local hkArray (no storage, not ours); each handle's tryToResolveLinks is called
// through the vtable with the map. The size compare is signed and the size is reloaded after every call.
void hkResourceContainer::tryToResolveLinks(hkResourceMap& map) {
    XFNV_INLINED_LOCAL(hkArray<hkResourceHandle*>, handles, std::nullopt, std::nullopt);
    // EXE-INLINED none: hkArray<hkResourceHandle*>::hkArray
    handles.m_data = nullptr;
    handles.m_size = 0;
    handles.m_capacityAndFlags = hkArrayBase<hkResourceHandle*>::DONT_DEALLOCATE_FLAG;
    findAllResourceRecursively(handles);
    for (int32_t i = 0; i < handles.m_size; ++i) {
        handles.m_data[i]->tryToResolveLinks(map);
    }

    // EXE-INLINED none: hkArray<hkResourceHandle*>::~hkArray
    hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
    const int32_t capacityAndFlags = handles.m_capacityAndFlags;
    handles.m_size = 0;
    if (capacityAndFlags >= 0) {
        heap.bufFree(
            handles.m_data,
            (capacityAndFlags & hkArrayBase<hkResourceHandle*>::CAPACITY_MASK) *
                static_cast<int32_t>(sizeof(hkResourceHandle*))
        );
    }
}

hkResourceContainer::~hkResourceContainer() {}

void hkResourceContainer::findAllResourceRecursively(
    hkArray<hkResourceHandle*, hkContainerHeapAllocator>& resourcesOut
) {
    for (hkResourceContainer* i = findContainerByName(nullptr, nullptr); i; i = findContainerByName(nullptr, i)) {
        i->findAllResourceRecursively(resourcesOut);
    }
    for (hkResourceHandle* j = findResourceByName(nullptr, nullptr, nullptr); j;
         j = findResourceByName(nullptr, nullptr, j)) {
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        if (resourcesOut.m_size == (resourcesOut.m_capacityAndFlags & hkArrayBase<hkResourceHandle*>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*Instance->m_heap, &resourcesOut, sizeof(hkResourceHandle*));
        }
        resourcesOut.m_data[resourcesOut.m_size++] = j;
    }
}

void hkResourceContainer::findAllContainersRecursively(
    hkArray<hkResourceContainer*, hkContainerHeapAllocator>& resourcesOut
) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (resourcesOut.m_size == (resourcesOut.m_capacityAndFlags & hkArrayBase<hkResourceContainer*>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, &resourcesOut, sizeof(hkResourceContainer*));
    }
    resourcesOut.m_data[resourcesOut.m_size++] = this;
    for (hkResourceContainer* i = findContainerByName(nullptr, nullptr); i; i = findContainerByName(nullptr, i)) {
        i->findAllContainersRecursively(resourcesOut);
    }
}

void hkResourceContainer::getPath(hkStringBuf& pathOut) {
    hkResourceContainer* v3 = getParent();
    if (v3) {
        v3->getPath(pathOut);
    }
    XFNV_INLINED_LOCAL(hkStringBuf, apMemory, std::nullopt, std::nullopt);
    ::new (&apMemory) hkStringBuf;
    const char* v4 = getName(apMemory);
    pathOut.operator+=("/");
    pathOut.operator+=(v4);
    hkMemoryAllocator* m_temp = (&hkMemoryRouter::getInstance())->m_temp;
    apMemory.m_string.m_size = 0;
    if (apMemory.m_string.m_capacityAndFlags >= 0) {
        m_temp->bufFree(
            apMemory.m_string.begin(), apMemory.m_string.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK
        );
    }
}

hkMemoryResourceContainer::hkMemoryResourceContainer(const char* a1) : m_name(a1), m_parent(nullptr) {}

// The children and handles lose the references the container holds; the arrays and the name free themselves.
hkMemoryResourceContainer::~hkMemoryResourceContainer() {
    for (int32_t i = 0; i < m_children.m_size; ++i) {
        if (m_children.m_data[i]) {
            m_children.m_data[i]->removeReference();
        }
        m_children.m_data[i] = nullptr;
    }
    for (int32_t i = 0; i < m_resourceHandles.m_size; ++i) {
        if (m_resourceHandles.m_data[i]) {
            m_resourceHandles.m_data[i]->removeReference();
        }
        m_resourceHandles.m_data[i] = nullptr;
    }
}

const char* hkMemoryResourceContainer::getName(hkStringBuf& buffer) const {
    return m_name.cString();
}

hkResourceHandle* hkMemoryResourceContainer::createResource(const char* name, void* object, const hkClass* klass) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkMemoryResourceHandle* v6 =
        static_cast<hkMemoryResourceHandle*>(Instance->m_heap->blockAlloc(sizeof(hkMemoryResourceHandle)));
    v6->m_memSizeAndFlags = sizeof(hkMemoryResourceHandle);
    hkMemoryResourceHandle* v7 = ::new (v6) hkMemoryResourceHandle;
    v7->setName(name);
    v7->setObject(object, klass);
    v7->addReference();
    hkArray<hkMemoryResourceHandle*, hkContainerHeapAllocator>* p_m_resourceHandles = &m_resourceHandles;
    hkMemoryRouter* v9 = &hkMemoryRouter::getInstance();
    if (p_m_resourceHandles->m_size ==
        (p_m_resourceHandles->m_capacityAndFlags & hkArrayBase<hkMemoryResourceHandle*>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*v9->m_heap, p_m_resourceHandles, sizeof(hkMemoryResourceHandle*));
    }
    hkMemoryResourceHandle** v10 = &p_m_resourceHandles->m_data[p_m_resourceHandles->m_size];
    if (v10) {
        v7->addReference();
        *v10 = v7;
    }
    ++p_m_resourceHandles->m_size;
    v7->removeReference();
    v7->removeReference();
    return v7;
}

// The handle's index is looked up under a reference held for the search, then its reference is released and the handles
// after it moved down one slot.
void hkMemoryResourceContainer::destroyResource(hkResourceHandle* resourceHandle) {
    if (resourceHandle) {
        resourceHandle->addReference();
    }
    // EXE-INLINED none: hkArrayBase<hkRefPtr<hkMemoryResourceHandle>>::indexOf
    const int32_t iSize = m_resourceHandles.m_size;
    int32_t index = -1;
    for (int32_t i = 0; i < iSize; ++i) {
        if (m_resourceHandles.m_data[i] == resourceHandle) {
            index = i;
            break;
        }
    }
    if (resourceHandle) {
        resourceHandle->removeReference();
    }
    if (index > -1) {
        // EXE-INLINED none: hkArrayBase<hkRefPtr<hkMemoryResourceHandle>>::removeAtAndCopy
        hkMemoryResourceHandle** ppSlot = &m_resourceHandles.m_data[index];
        if (*ppSlot) {
            (*ppSlot)->removeReference();
        }
        *ppSlot = nullptr;
        m_resourceHandles.removeAtAndCopy(index);
    }
}

hkResourceHandle* hkMemoryResourceContainer::findResourceByName(
    const char* objectName,
    const hkClass* klass,
    const hkResourceHandle* prevObject
) const {
    hkMemoryResourceHandle** v6;
    XFNV_INLINED_LOCAL(hkStringBuf, a2, std::nullopt, std::nullopt);
    int32_t v5 = 0;
    do {
        if (!prevObject) {
            break;
        }
        if (v5 >= m_resourceHandles.m_size) {
            break;
        }
        v6 = &m_resourceHandles.m_data[v5++];
    }
    while (*v6 != prevObject);
    bool v7 = v5 < m_resourceHandles.m_size;
    int32_t v22 = v5;
    if (!v7) {
        return nullptr;
    }
    while (true) {
        const hkClass* v13;
        const hkClass* v14;
        hkMemoryResourceHandle* v8 = m_resourceHandles.m_data[v22];
        if (objectName) {
            a2.m_string.m_data = a2.m_string.m_storage;
            a2.m_string.m_size = 0;
            a2.m_string.m_capacityAndFlags = -0x7FFFFF80;
            hkMemoryAllocator* m_temp = (&hkMemoryRouter::getInstance())->m_temp;
            if ((a2.m_string.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK) == 0) {
                hkArrayUtil::_reserve(*m_temp, &a2, 1, sizeof(char));
            }
            a2.m_string.m_size = 1;
            *a2.m_string.begin() = 0;
            *a2.m_string.begin() = 0;
            const char* v10 = v8->getName(a2);
            if (hkString::strCmp(objectName, v10)) {
                hkMemoryAllocator* v11 = (&hkMemoryRouter::getInstance())->m_temp;
                a2.m_string.m_size = 0;
                if (a2.m_string.m_capacityAndFlags >= 0) {
                    v11->bufFree(
                        a2.m_string.begin(), a2.m_string.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK
                    );
                }
                goto LABEL_18;
            }
            hkMemoryAllocator* v12 = (&hkMemoryRouter::getInstance())->m_temp;
            a2.m_string.m_size = 0;
            if (a2.m_string.m_capacityAndFlags >= 0) {
                v12->bufFree(a2.m_string.begin(), a2.m_string.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK);
            }
        }
        if (!klass) {
            return v8;
        }
        v13 = v8->getClass();
        v14 = v13;
        if (klass == v13 || klass->isSuperClass(*v13).m_bool) {
            return v8;
        }
        if (objectName) {
            char buf[512];
            hkErrStream apThis(buf, 512);
            const char* Name = v14->getName();
            const char* v19 = klass->getName();
            hkOstream* v16 = &apThis.operator<<("Class mismatch, cannot resolve link: ");
            hkOstream* v17 = &v16->operator<<(v19);
            hkOstream* v18 = &v17->operator<<(" != ");
            v18->operator<<(Name);
            hkError::s_instance->message(
                hkError::MESSAGE_WARNING, -0xFCB12DE, buf, ".\\ResourceDatabase\\hkResourceHandle.cpp", 312
            );
            return nullptr;
        }
    LABEL_18:;
        v7 = ++v22 < m_resourceHandles.m_size;
        if (!v7) {
            return nullptr;
        }
    }
}

hkResourceContainer* hkMemoryResourceContainer::createContainer(const char* name) {
    hkResourceContainer* result = findContainerByName(name, nullptr);
    if (!result) {
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        hkMemoryResourceContainer* v5 =
            static_cast<hkMemoryResourceContainer*>(Instance->m_heap->blockAlloc(sizeof(hkMemoryResourceContainer)));
        v5->m_memSizeAndFlags = sizeof(hkMemoryResourceContainer);
        hkMemoryResourceContainer* v6 = ::new (v5) hkMemoryResourceContainer(name);
        if (v6) {
            v6->addReference();
        }
        hkMemoryRouter* v8 = &hkMemoryRouter::getInstance();
        if (m_children.m_size ==
            (m_children.m_capacityAndFlags & hkArrayBase<hkMemoryResourceContainer*>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*v8->m_heap, &m_children, sizeof(hkMemoryResourceContainer*));
        }
        hkMemoryResourceContainer** v9 = &m_children.m_data[m_children.m_size];
        if (v9) {
            if (v6) {
                v6->addReference();
            }
            *v9 = v6;
        }
        ++m_children.m_size;
        if (v6) {
            v6->removeReference();
        }
        v6->m_parent = this;
        v6->removeReference();
        return v6;
    }
    return result;
}

void hkMemoryResourceContainer::destroyContainer(hkResourceContainer* container2) {
    int32_t v6;
    if (container2) {
        container2->addReference();
    }
    int32_t m_size = m_children.m_size;
    int32_t v4 = 0;
    if (m_size <= 0) {
    LABEL_7:;
        v6 = -1;
    }
    else {
        hkMemoryResourceContainer** m_data = m_children.begin();
        while (*m_data != container2) {
            ++v4;
            ++m_data;
            if (v4 >= m_size) {
                goto LABEL_7;
            }
        }
        v6 = v4;
    }
    if (container2) {
        container2->removeReference();
    }
    if (v6 > -1) {
        hkMemoryResourceContainer** v7 = m_children.begin();
        hkMemoryResourceContainer* v8 = v7[v6];
        hkMemoryResourceContainer** v9 = &v7[v6];
        if (v8) {
            v8->removeReference();
        }
        *v9 = nullptr;
        uint32_t v10 = --m_children.m_size;
        if (v10 != v6) {
            m_children.m_data[v6] = m_children.m_data[v10];
        }
    }
}

int32_t hkMemoryResourceContainer::getNumContainers() {
    return m_children.m_size;
}

hkResourceContainer* hkMemoryResourceContainer::findContainerByName(
    const char* containerName,
    const hkResourceContainer* prevContainer
) const {
    hkMemoryResourceContainer* const* v5;
    hkMemoryResourceContainer* v7;
    int32_t v4 = 0;
    do {
        if (!prevContainer) {
            break;
        }
        if (v4 >= m_children.m_size) {
            break;
        }
        v5 = &m_children.m_data[v4++];
    }
    while (*v5 != prevContainer);
    int32_t v6 = v4;
    if (v4 >= m_children.m_size) {
        return nullptr;
    }
    while (true) {
        v7 = m_children.m_data[v6];
        if (!containerName ||
            !hkString::strCmp(
                containerName,
                reinterpret_cast<const char*>(reinterpret_cast<uintptr_t>(v7->m_name.m_stringAndFlag) & ~uintptr_t(1))
            )) {
            break;
        }
        if (++v6 >= m_children.m_size) {
            return nullptr;
        }
    }
    return v7;
}

// A new parent that has this container among its ancestors (or is it) would make a cycle: a warning, and nothing moves.
// Otherwise the container leaves its parent's children (its index is used unchecked, as the game does) and is appended
// to the new parent's. The new parent is treated as a memory container.
hkResult hkMemoryResourceContainer::parentTo(hkResourceContainer* newParent) {
    for (hkResourceContainer* pAncestor = newParent; pAncestor; pAncestor = pAncestor->getParent()) {
        if (pAncestor == this) {
            char buf[512];
            hkErrStream kStream(buf, 512);
            kStream << "Cannot parent '" << m_name << "' to '"
                    << static_cast<hkMemoryResourceContainer*>(newParent)->m_name
                    << "' as this would create a circular dependency ";
            hkError::s_instance->message(
                hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA4554), buf,
                ".\\ResourceDatabase\\hkResourceHandle.cpp", 408
            );
            return HK_FAILURE;
        }
    }

    // EXE-INLINED none: hkRefPtr<hkMemoryResourceContainer>::hkRefPtr
    addReference();
    // EXE-INLINED none: hkArrayBase<hkRefPtr<hkMemoryResourceContainer>>::indexOf
    if (this) {
        addReference();
    }
    hkMemoryResourceContainer* pOldParent = m_parent;
    const int32_t iSize = pOldParent->m_children.m_size;
    int32_t index = -1;
    for (int32_t i = 0; i < iSize; ++i) {
        if (pOldParent->m_children.m_data[i] == this) {
            index = i;
            break;
        }
    }
    removeReference();

    // EXE-INLINED none: hkArrayBase<hkRefPtr<hkMemoryResourceContainer>>::removeAtAndCopy
    hkArray<hkMemoryResourceContainer*>& rOldSiblings = m_parent->m_children;
    hkMemoryResourceContainer** ppSlot = &rOldSiblings.m_data[index];
    if (*ppSlot) {
        (*ppSlot)->removeReference();
    }
    *ppSlot = nullptr;
    rOldSiblings.removeAtAndCopy(index);

    // EXE-INLINED none: hkArray<hkRefPtr<hkMemoryResourceContainer>>::pushBack
    addReference();
    hkMemoryResourceContainer* pNewParent = static_cast<hkMemoryResourceContainer*>(newParent);
    hkArray<hkMemoryResourceContainer*>& rSiblings = pNewParent->m_children;
    hkMemoryRouter& router = hkMemoryRouter::getInstance();
    if (rSiblings.m_size == (rSiblings.m_capacityAndFlags & hkArrayBase<hkMemoryResourceContainer*>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(router.heap(), &rSiblings, static_cast<int32_t>(sizeof(hkMemoryResourceContainer*)));
    }
    hkMemoryResourceContainer** ppNew = &rSiblings.m_data[rSiblings.m_size];
    if (ppNew) {
        addReference();
        *ppNew = this;
    }
    rSiblings.m_size++;
    removeReference();

    m_parent = pNewParent;
    removeReference();
    return HK_SUCCESS;
}

// Maps every handle of container and its children by its name (getName).
hkContainerResourceMap::hkContainerResourceMap(hkResourceContainer* container) {
    hkArray<hkResourceHandle*> handles;
    container->findAllResourceRecursively(handles);
    for (int32_t i = 0; i < handles.m_size; ++i) {
        hkResourceHandle* pHandle = handles.m_data[i];
        hkStringBuf buffer;
        m_resources.insert(pHandle->getName(buffer), pHandle);
    }
}

// Returns the object of the handle with that name and writes its class into klassOut when given; returns null and a
// null class when no handle has the name.
void* hkContainerResourceMap::findObjectByName(const char* objectName, const hkClass** klassOut) const {
    if (klassOut) {
        *klassOut = nullptr;
    }
    hkResourceHandle* pHandle = m_resources.getWithDefault(objectName, nullptr);
    if (!pHandle) {
        return nullptr;
    }
    if (klassOut) {
        *klassOut = pHandle->getClass();
    }
    return pHandle->getObject();
}
