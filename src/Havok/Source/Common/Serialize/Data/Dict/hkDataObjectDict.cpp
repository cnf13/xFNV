#include "Havok/Source/Common/Serialize/Data/Dict/hkDataObjectDict.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/StringMap/hkCachedHashMap.h"
#include "Havok/Source/Common/Base/Container/StringMap/hkStringMap.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMemberAccessor.h"
#include "Havok/Source/Common/Base/Reflection/hkCustomAttributes.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Serialize/Data/Util/hkDataObjectUtil.h"
#include "Havok/Source/Common/Serialize/Data/hkDataArray.h"
#include "Havok/Source/Common/Serialize/Data/hkDataObject.h"
#include "Havok/Source/Common/Serialize/Data/hkDataObjectImpl.h"
#include "Havok/Source/Common/Serialize/Data/hkDataWorld.h"
#include "Havok/Source/Common/Serialize/Util/hkSerializeMultiMap.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

#include <bit>

using BasicArrayImplementationByte = BasicArrayImplementation<uint8_t, hkDataObject::TYPE_BYTE>;
using BasicArrayImplementationReal = BasicArrayImplementation<float, hkDataObject::TYPE_REAL>;
using BasicArrayImplementationPointer = BasicArrayImplementation<hkDataObjectImpl*, hkDataObject::TYPE_OBJECT>;
using BasicArrayImplementationInt = BasicArrayImplementation<int64_t, hkDataObject::TYPE_INT>;
using BasicArrayImplementationCstring = BasicArrayImplementation<char*, hkDataObject::TYPE_CSTRING>;

InternedStringRefCounted InternedNullRefCounted;

int32_t RTC_NumErrors_2() {
    return 4;
}

// The member type of a vector of nreal reals: TYPE_VEC_4 + nreal / 4 - 1, unsigned.
hkDataObject::Type getVecTypeFromNumReals(int32_t r_eax) {
    return static_cast<hkDataObject::Type>((static_cast<uint32_t>(r_eax) >> 2) + 3);
}

// The tracker's block comes from the heap allocator, as the game does.
hkDataWorldDict::hkDataWorldDict() {
    void* pTracker = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(ObjectTracker));
    m_tracker = pTracker ? new (pTracker) ObjectTracker() : nullptr;
}

hkDataWorldDict::~hkDataWorldDict() {
    if (m_tracker) {
        m_tracker->~ObjectTracker();
        hkMemoryRouter::getInstance().m_heap->blockFree(m_tracker, sizeof(ObjectTracker));
    }
}

// A new dictionary class of cinfo's name and version, with a reference for the caller, its parent found by name among
// the tracked classes and its members added, tracked.
hkDataClassImpl* hkDataWorldDict::newClass(const hkDataClass::Cinfo& cinfo) {
    hkMemoryAllocator& kAlloc = hkMemoryRouter::getInstance().heap();
    hkDataClassDict* pMemory = static_cast<hkDataClassDict*>(kAlloc.blockAlloc(sizeof(hkDataClassDict)));
    pMemory->m_memSize = sizeof(hkDataClassDict);
    hkDataClassDict* pClass = ::new (pMemory) hkDataClassDict(this, cinfo.name, cinfo.version);
    if (cinfo.parent) {
        hkDataClassDict* pParent = m_tracker->m_classes.getWithDefault(cinfo.parent, nullptr);
        if (pParent) {
            ++pParent->m_count;
        }
        hkDataClassDict* pOld = pClass->m_parent;
        if ((pOld) && (--pOld->m_count == 0)) {
            delete pOld;
        }

        pClass->m_parent = pParent;
    }
    ++pClass->m_count;
    m_tracker->trackClass(pClass);
    for (int32_t i = 0; i < cinfo.members.m_size; ++i) {
        const hkDataClass::Cinfo::Member& rMember = cinfo.members.m_data[i];
        InternedString kName = m_tracker->intern(rMember.name);
        InternedStringHandle kTypeName;
        // addMember is handed the pointer the handle's maker returns.
        InternedStringHandle* pTypeName = m_tracker->internClassNameHandle(&kTypeName, rMember.className);
        pClass->addMember(kName, rMember.type, *pTypeName, rMember.tupleCount);
    }
    return pClass;
}

// A new dictionary object of the class, with a reference for the caller, tracked under its class; the first object made
// becomes the top-level object.
hkDataObjectImpl* hkDataWorldDict::newObject(const hkDataClass& klass) const {
    hkMemoryAllocator& kAlloc = hkMemoryRouter::getInstance().heap();
    hkDataObjectDict* pObject = static_cast<hkDataObjectDict*>(kAlloc.blockAlloc(sizeof(hkDataObjectDict)));
    pObject->m_memSize = sizeof(hkDataObjectDict);
    // The source's downcast: a dictionary world's classes are dictionary classes.
    ::new (pObject) hkDataObjectDict(static_cast<hkDataClassDict*>(klass.m_impl));
    ++pObject->m_count;
    ObjectTracker* pTracker = m_tracker;
    hkDataObjectImpl* pImpl = pObject;
    pTracker->m_objectsFromClass.insert(
        static_cast<hkDataClassDict*>(const_cast<hkDataClassImpl*>(pImpl->getClass())), pObject
    );
    if (!pTracker->m_topLevelObject) {
        pTracker->m_topLevelObject = pObject;
    }
    return pObject;
}

// The array of the member's type. An array member passes its tuple count to the array's constructor and a tuple member
// gets it as the array's size; the array flag is cleared, or else the tuple flag.
hkDataArrayImpl* hkDataWorldDict::newArray(hkDataObject& obj, const hkDataClass::MemberInfo& minfo) const {
    hkDataObject::Type type = minfo.m_type;
    int32_t iTupleCount;
    int32_t iElementCount;
    if ((type & hkDataObject::TYPE_ARRAY) != 0) {
        iElementCount = minfo.m_tupleCount;
        type = static_cast<hkDataObject::Type>(type & ~hkDataObject::TYPE_ARRAY);
        iTupleCount = 0;
    }
    else {
        iTupleCount = minfo.m_tupleCount;
        type = static_cast<hkDataObject::Type>(type & ~hkDataObject::TYPE_TUPLE);
        iElementCount = 0;
    }
    hkDataArrayImpl* pArray = createArray(type, minfo.m_class, iElementCount);
    pArray->setSize(iTupleCount);
    // A member's handle is its interned name, typed as the const char* itself.
    obj.m_impl->assign(minfo.m_name, pArray);
    return pArray;
}

hkEnum<hkDataWorld::DataWorldType, int32_t> hkDataWorldDict::getType() const {
    hkEnum<DataWorldType, int32_t> kType;
    kType.m_storage = TYPE_DICTIONARY;
    return kType;
}

// The tracked top-level object.
hkDataObject hkDataWorldDict::getContents() const {
    hkDataObject kResult;
    kResult.~hkDataObject();
    ::new (&kResult) hkDataObject(m_tracker->m_topLevelObject);
    return kResult;
}

// Every tracked class.
void hkDataWorldDict::findAllClasses(hkArray<hkDataClassImpl*>& classesOut) const {
    m_tracker->findTrackedClasses(classesOut);
}

// The tracked class called name, or null.
hkDataClassImpl* hkDataWorldDict::findClass(const char* name) const {
    if (!name) {
        return nullptr;
    }
    return m_tracker->m_classes.getWithDefault(name, nullptr);
}

// The object a handle names: a dictionary handle holds the implementation in p0.
hkDataObject hkDataWorldDict::findObject(const hkDataObject::Handle& handle) const {
    hkDataObject kResult(static_cast<hkDataObjectImpl*>(handle.p0));
    return kResult;
}

// Both names are interned first, old then new. The objects (findObjectsByExactClass: cleared, then the tracker's
// exact-class objects of klass's name) and the struct arrays of the class get the first member called oldName renamed.
// Then the class's declared member is searched from the end, with index -1 when absent, and that entry's name is set
// either way, as the game does. The arrays are freed in reverse order (the struct arrays cleared and deallocated, then
// destroyed).
void hkDataWorldDict::renameClassMember(hkDataClass& klass, const char* oldName, const char* newName) {
    hkDataClassDict* pClass = static_cast<hkDataClassDict*>(klass.m_impl);
    const char* internedOldName = m_tracker->intern(oldName).getString();
    const char* internedNewName = m_tracker->intern(newName).getString();

    hkArray<hkDataObjectImpl*> objs;
    // EXE-INLINED none: hkDataWorldDict::findObjectsByExactClass
    const char* className = klass.getName();
    objs.m_size = 0;
    m_tracker->findTrackedObjectsByBase(className, 1, 0, objs);
    for (int32_t i = 0; i < objs.m_size; ++i) {
        // EXE-INLINED none: hkDataObjectDict::renameMember
        hkDataObjectDict* pObj = static_cast<hkDataObjectDict*>(objs.m_data[i]);
        const int32_t numMembers = pObj->m_memberData.m_size;
        hkDataObjectDict::MemberData* pMembers = pObj->m_memberData.m_data;
        for (int32_t m = 0; m < numMembers; ++m) {
            if (pMembers[m].m_memberName == internedOldName) {
                pMembers[m].m_memberName = internedNewName;
                break;
            }
        }
    }

    hkArray<hkDataArrayImpl*> arrays;
    m_tracker->getTrackedStructArrays(pClass, 1, arrays);
    for (int32_t i = 0; i < arrays.m_size; ++i) {
        // EXE-INLINED none: StructArrayImplementation::renameMember
        StructArrayImplementation* pArray = static_cast<StructArrayImplementation*>(arrays.m_data[i]);
        const int32_t numMembers = pArray->m_memberData.m_size;
        StructArrayImplementation::MemberData* pMember = pArray->m_memberData.m_data;
        for (int32_t m = 0; m < numMembers; ++m, ++pMember) {
            if (pMember->m_memberName == internedOldName) {
                pMember->m_memberName = internedNewName;
                break;
            }
        }
    }

    // EXE-INLINED none: hkDataClassDict::renameMember
    int32_t index = pClass->m_memberInfo.m_size - 1;
    for (; index >= 0; --index) {
        if (pClass->m_memberInfo.m_data[index].m_name == internedOldName) {
            break;
        }
    }
    if (index < 0) {
        index = -1;
    }
    pClass->m_memberInfo.m_data[index].m_name = internedNewName;
}

// The name is interned first. Then each exact-class object and struct array of the class drops the member, then the
// class itself; the arrays are freed as in renameClassMember.
void hkDataWorldDict::removeClassMember(hkDataClass& klass, const char* name) {
    hkDataClassDict* pClass = static_cast<hkDataClassDict*>(klass.m_impl);
    const char* internedName = m_tracker->intern(name).getString();

    hkArray<hkDataObjectImpl*> objs;
    // EXE-INLINED none: hkDataWorldDict::findObjectsByExactClass
    const char* className = klass.getName();
    objs.m_size = 0;
    m_tracker->findTrackedObjectsByBase(className, 1, 0, objs);
    for (int32_t i = 0; i < objs.m_size; ++i) {
        static_cast<hkDataObjectDict*>(objs.m_data[i])->removeMember(internedName);
    }

    hkArray<hkDataArrayImpl*> arrays;
    m_tracker->getTrackedStructArrays(pClass, 1, arrays);
    for (int32_t i = 0; i < arrays.m_size; ++i) {
        static_cast<StructArrayImplementation*>(arrays.m_data[i])->removeMember(internedName);
    }

    pClass->removeMember(internedName);
}

// The class called name, copied from worldFrom when this world has none: a new class of the source class's name,
// version and parent's name (the class of the source's name when this world has one by then, the asm's
// findClass(cinfo.name) after the parent's name is set), then each of the source's declared members added, the class
// of an object or struct member copied first. hkDataClass's destructor is an empty function.
hkDataClassImpl* hkDataWorldDict::copyClassFromWorld(const char* name, const hkDataWorld& worldFrom) {
    const hkDataWorld* pThisWorld = this;
    hkDataClassImpl* pFound = pThisWorld->findClass(name);
    if (pFound) {
        return pFound;
    }
    hkDataClass srcClass(worldFrom.findClass(name));
    hkDataClass::Cinfo cinfo;
    cinfo.name = srcClass.getName();
    cinfo.version = srcClass.getVersion();
    hkDataClass parent;
    srcClass.getParent(parent);
    if (parent.isNull()) {
        cinfo.parent = nullptr;
    }
    else {
        cinfo.parent = parent.getName();
        pFound = pThisWorld->findClass(cinfo.name);
        if (pFound) {
            return pFound;
        }
    }
    hkDataWorld* pWorld = this;
    hkDataClassImpl* pNewClass = pWorld->newClass(cinfo);
    int32_t iNumMembers = srcClass.getNumDeclaredMembers();
    hkArrayBase<hkDataClass::MemberInfo> minfos;
    // EXE-INLINED none: hkArray<hkDataClass::MemberInfo, hkContainerTempAllocator>'s constructor of iNumMembers
    // elements (the temp allocator's block, its capacity the elements the returned size holds)
    minfos.m_data = nullptr;
    minfos.m_size = 0;
    minfos.m_capacityAndFlags = hkArrayBase<hkDataClass::MemberInfo>::DONT_DEALLOCATE_FLAG;
    hkMemoryAllocator& rTemp = hkMemoryRouter::getInstance().temp();
    hkDataClass::MemberInfo* pData = nullptr;
    int32_t iCapacity = 0;
    if (iNumMembers) {
        int32_t iBytes = iNumMembers * static_cast<int32_t>(sizeof(hkDataClass::MemberInfo));
        pData = static_cast<hkDataClass::MemberInfo*>(rTemp.bufAlloc(iBytes));
        iCapacity = iBytes / static_cast<int32_t>(sizeof(hkDataClass::MemberInfo));
    }
    minfos.m_data = pData;
    minfos.m_size = iCapacity;
    minfos.m_capacityAndFlags = iCapacity ? iCapacity : hkArrayBase<hkDataClass::MemberInfo>::DONT_DEALLOCATE_FLAG;
    for (int32_t i = 0; i < iCapacity; ++i) {
        hkDataClass::MemberInfo* pInfo = &pData[i];
        if (pInfo) {
            pInfo->m_name = nullptr;
            pInfo->m_owner = nullptr;
            pInfo->m_type = hkDataObject::TYPE_VOID;
            pInfo->m_class = nullptr;
            pInfo->m_tupleCount = 0;
        }
    }
    srcClass.getAllDeclaredMemberInfo(minfos);
    int32_t iSize = minfos.m_size;
    hkMemoryAllocator& rHeap = hkMemoryRouter::getInstance().heap();
    // EXE-INLINED none: hkArray<hkDataClass::Cinfo::Member>::reserve
    int32_t iMemberCapacity = cinfo.members.m_capacityAndFlags & hkArrayBase<hkDataClass::Cinfo::Member>::CAPACITY_MASK;
    if (iMemberCapacity < iSize) {
        int32_t iNewCapacity = 2 * iMemberCapacity;
        if (iSize >= iNewCapacity) {
            iNewCapacity = iSize;
        }
        hkArrayUtil::_reserve(rHeap, &cinfo.members, iNewCapacity, sizeof(hkDataClass::Cinfo::Member));
    }
    hkDataClass dstClass(pNewClass);
    for (int32_t i = 0; i < minfos.m_size; ++i) {
        const hkDataClass::MemberInfo& rInfo = minfos.m_data[i];
        const char* pTypeName = nullptr;
        if (rInfo.m_class) {
            pTypeName = rInfo.m_class->getName();
            copyClassFromWorld(pTypeName, worldFrom);
        }
        addClassMember(dstClass, rInfo.m_name, rInfo.m_type, pTypeName, rInfo.m_tupleCount);
    }
    // EXE-INLINED none: hkArray<hkDataClass::MemberInfo, hkContainerTempAllocator>::~hkArray
    hkMemoryAllocator& rTempFree = hkMemoryRouter::getInstance().temp();
    int32_t iFlags = minfos.m_capacityAndFlags;
    minfos.m_size = 0;
    if (!(iFlags & hkArrayBase<hkDataClass::MemberInfo>::DONT_DEALLOCATE_FLAG)) {
        rTempFree.bufFree(
            minfos.m_data,
            (iFlags & hkArrayBase<hkDataClass::MemberInfo>::CAPACITY_MASK) *
                static_cast<int32_t>(sizeof(hkDataClass::MemberInfo))
        );
    }
    minfos.m_data = nullptr;
    minfos.m_capacityAndFlags = hkArrayBase<hkDataClass::MemberInfo>::DONT_DEALLOCATE_FLAG;
    return pNewClass;
}

void hkDataWorldDict::findObjectsByExactClass(
    const char* className,
    hkArray<hkDataObjectImpl*, hkContainerHeapAllocator>& objectsOut
) {
    hkDataWorldDict::ObjectTracker* m_tracker = this->m_tracker;
    objectsOut.m_size = 0;
    m_tracker->findTrackedObjectsByBase(className, 0, 1, objectsOut);
}

void hkDataWorldDict::findObjectsByBaseClass(
    const char* className,
    hkArray<hkDataObjectImpl*, hkContainerHeapAllocator>& objectsOut
) {
    hkDataWorldDict::ObjectTracker* m_tracker = this->m_tracker;
    objectsOut.m_size = 0;
    m_tracker->findTrackedObjectsByBase(className, 1, 1, objectsOut);
}

void hkDataWorldDict::castObject(hkDataObject& obj, const hkDataClass& castClass) {
    m_tracker->retractCastedObject(
        static_cast<hkDataObjectDict*>(obj.m_impl), static_cast<hkDataClassDict*>(castClass.m_impl)
    );
}

void hkDataWorldDict::setClassVersion(hkDataClass& klass, int32_t newVersion) {
    klass.m_impl[1].m_count = newVersion;
}

void hkDataWorldDict::renameClass(hkDataClass& klass, const char* newName) {
    const char* Name = klass.getName();
    hkDataWorldDict::ObjectTracker* m_tracker = this->m_tracker;
    if (Name) {
        hkStringMap<char*, hkContainerHeapAllocator>* p_m_interns = &m_tracker->m_interns;
        char* v7 = m_tracker->m_interns.getWithDefault(Name, 0);
        if (!v7) {
            v7 = hkString::strDup(Name);
            p_m_interns->insert(v7, v7);
        }
        this->m_tracker->retrackRenamedClass(v7, newName);
    }
    else {
        this->m_tracker->retrackRenamedClass(nullptr, newName);
    }
}

// The class's parent becomes parent's class (a counted reference: the new one taken first, the old one released,
// deleted at zero); the tracker then moves the class from its old parent's derived classes to the new one's.
void hkDataWorldDict::setClassParent(hkDataClass& klass, hkDataClass& parent) {
    hkDataClassDict* pOldParent = static_cast<hkDataClassDict*>(klass.m_impl->getParent());
    hkDataClassDict* pClass = static_cast<hkDataClassDict*>(klass.m_impl);
    hkDataClassDict* pNewParent = static_cast<hkDataClassDict*>(parent.m_impl);
    if (pNewParent) {
        ++pNewParent->m_count;
    }
    hkDataClassDict* pReleased = pClass->m_parent;
    if (pReleased && --pReleased->m_count == 0) {
        delete pReleased;
    }
    pClass->m_parent = pNewParent;
    m_tracker->retrackDerivedClass(pOldParent, static_cast<hkDataClassDict*>(klass.m_impl));
}

// Adds the member name (interned) of type, typeName and tupleCount to klass, dropping a declared member of that name
// first.
void hkDataWorldDict::addClassMember(
    hkDataClass& klass,
    const char* name,
    hkDataObject::Type type,
    const char* typeName,
    int32_t tupleCount
) {
    // The source's downcast: a dictionary world's classes are dictionary classes.
    hkDataClassDict* pClass = static_cast<hkDataClassDict*>(klass.m_impl);
    InternedString kName = m_tracker->intern(name);
    if (pClass->getDeclaredMemberIndexByName(name) != -1) {
        pClass->removeMember(kName);
    }
    InternedStringHandle kTypeName;
    // addMember is handed the pointer the handle's maker returns.
    InternedStringHandle* pTypeName = m_tracker->internClassNameHandle(&kTypeName, typeName);
    pClass->addMember(kName, type, *pTypeName, tupleCount);
}

// The dictionary class of klass, tracked: its parent the tracked class of the parent's name or else the parent
// wrapped first, then each declared member added under its interned name with its data type and, for a member of a
// class, the type name from the member's or the class's hk.DataObjectType attribute, else the class's name. A
// serialize-ignored member is added as void with the empty type name.
hkDataClassImpl* hkDataWorldDict::wrapClass(const hkClass& klass) {
    hkDataClassDict* pMemory =
        static_cast<hkDataClassDict*>(hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkDataClassDict)));
    pMemory->m_memSize = sizeof(hkDataClassDict);
    int32_t iVersion = klass.getDescribedVersion();
    const char* pName = klass.getName();
    hkDataClassDict* pClass = ::new (pMemory) hkDataClassDict(this, pName, iVersion);
    if (klass.getParent()) {
        const char* pParentName = klass.getParent()->getName();
        hkDataClassDict* pParent = nullptr;
        if (pParentName) {
            pParent = m_tracker->m_classes.getWithDefault(pParentName, nullptr);
        }
        if (!pParent) {
            // The source's downcast: a dictionary world's classes are dictionary classes.
            pParent = static_cast<hkDataClassDict*>(wrapClass(*klass.getParent()));
        }
        if (pParent) {
            ++pParent->m_count;
        }
        hkDataClassDict* pReleased = pClass->m_parent;
        if (pReleased && --pReleased->m_count == 0) {
            delete pReleased;
        }
        pClass->m_parent = pParent;
    }
    ++pClass->m_count;
    m_tracker->trackClass(pClass);
    for (int32_t i = 0; i < klass.getNumDeclaredMembers(); ++i) {
        const hkClassMember& rMember = klass.getDeclaredMember(i);
        hkDataObject::Type eType = hkDataObject::TYPE_VOID;
        InternedStringHandle kTypeName;
        // The handle starts as the shared empty one.
        InternedNullRefCounted.addReference();
        kTypeName.m_cached.m_pntr = &InternedNullRefCounted;
        if (!(rMember.m_flags.get() & hkClassMember::SERIALIZE_IGNORED)) {
            hkClassMember::Type eMemberType = static_cast<hkClassMember::Type>(rMember.m_type.m_storage);
            hkClassMember::Type eSubType = static_cast<hkClassMember::Type>(rMember.m_subtype.m_storage);
            int32_t iArraySize = rMember.getCstyleArraySize();
            const hkClass* pMemberClass = rMember.getClass();
            eType = hkDataObjectUtil::getTypeFromMemberType(eMemberType, eSubType, pMemberClass, iArraySize);
            if (rMember.getClass()) {
                const hkVariant* pAttribute = rMember.getAttribute(hkDataWorldDict_s_hkDataObjectTupeAttributeID);
                if (!pAttribute) {
                    pAttribute = rMember.getClass()->getAttribute(hkDataWorldDict_s_hkDataObjectTupeAttributeID);
                }
                InternedStringHandle kHandle;
                InternedStringHandle* pHandle;
                if (pAttribute) {
                    hkClassMemberAccessor kAccessor(*pAttribute, "typeName");
                    pHandle = m_tracker->internClassNameHandle(&kHandle, kAccessor.asCstring(0));
                }
                else {
                    const char* pClassName = rMember.getClass()->getName();
                    pHandle = m_tracker->internClassNameHandle(&kHandle, pClassName);
                }
                // EXE-INLINED none: InternedStringHandle::operator= (the old handle is the shared empty one)
                if (pHandle->m_cached.m_pntr) {
                    pHandle->m_cached.m_pntr->addReference();
                }
                InternedNullRefCounted.removeReference();
                kTypeName.m_cached.m_pntr = pHandle->m_cached.m_pntr;
            }
        }
        InternedString kName = m_tracker->intern(rMember.m_name);
        int32_t iTupleCount = rMember.getCstyleArraySize();
        pClass->addMember(kName, eType, kTypeName, iTupleCount);
    }
    return pClass;
}

// The SDK's klass = hkDataClass(HK_NULL): hkDataClass's constructor only stores its impl and its
// destructor is empty, so klass is left with a null impl. The class is then untracked and released.
void hkDataWorldDict::removeClass(hkDataClass& klass) {
    hkDataClassDict* impl = static_cast<hkDataClassDict*>(klass.m_impl);
    klass.m_impl = nullptr;
    m_tracker->untrackClass(impl);
    // EXE-INLINED none: hkDataRefCounted::removeReference
    if (--impl->m_count == 0) {
        delete impl;
    }
}

// A new array of type: a tuple array over a new array of the element type, else the array of the basic type (of klass
// for objects and structs), or null.
hkDataArrayImpl*
hkDataWorldDict::createArray(hkDataObject::Type type, const hkDataClassImpl* klass, int32_t tupleCount) {
    hkMemoryAllocator* pAlloc;
    if (type & hkDataObject::TYPE_TUPLE) {
        hkDataArrayImpl* pImpl =
            createArray(static_cast<hkDataObject::Type>(type & ~hkDataObject::TYPE_TUPLE), klass, 0);
        pAlloc = &hkMemoryRouter::getInstance().heap();
        ArrayOfTuplesImplementation* pArray =
            static_cast<ArrayOfTuplesImplementation*>(pAlloc->blockAlloc(sizeof(ArrayOfTuplesImplementation)));
        pArray->m_memSize = sizeof(ArrayOfTuplesImplementation);
        return ::new (pArray) ArrayOfTuplesImplementation(pImpl, tupleCount);
    }
    switch (type & hkDataObject::TYPE_MASK_BASIC_TYPES) {
        case hkDataObject::TYPE_BYTE: {
            pAlloc = &hkMemoryRouter::getInstance().heap();
            ByteArrayImplementation* pArray =
                static_cast<ByteArrayImplementation*>(pAlloc->blockAlloc(sizeof(ByteArrayImplementation)));
            pArray->m_memSize = sizeof(ByteArrayImplementation);
            return ::new (pArray) ByteArrayImplementation;
        }
        case hkDataObject::TYPE_INT: {
            pAlloc = &hkMemoryRouter::getInstance().heap();
            IntArrayImplementation* pArray =
                static_cast<IntArrayImplementation*>(pAlloc->blockAlloc(sizeof(IntArrayImplementation)));
            pArray->m_memSize = sizeof(IntArrayImplementation);
            return ::new (pArray) IntArrayImplementation;
        }
        case hkDataObject::TYPE_REAL: {
            pAlloc = &hkMemoryRouter::getInstance().heap();
            RealArrayImplementation* pArray =
                static_cast<RealArrayImplementation*>(pAlloc->blockAlloc(sizeof(RealArrayImplementation)));
            pArray->m_memSize = sizeof(RealArrayImplementation);
            return ::new (pArray) RealArrayImplementation;
        }
        case hkDataObject::TYPE_VEC_4: {
            pAlloc = &hkMemoryRouter::getInstance().heap();
            VecArrayImplementation* pArray =
                static_cast<VecArrayImplementation*>(pAlloc->blockAlloc(sizeof(VecArrayImplementation)));
            pArray->m_memSize = sizeof(VecArrayImplementation);
            return ::new (pArray) VecArrayImplementation(4);
        }
        case hkDataObject::TYPE_VEC_12: {
            pAlloc = &hkMemoryRouter::getInstance().heap();
            VecArrayImplementation* pArray =
                static_cast<VecArrayImplementation*>(pAlloc->blockAlloc(sizeof(VecArrayImplementation)));
            pArray->m_memSize = sizeof(VecArrayImplementation);
            return ::new (pArray) VecArrayImplementation(12);
        }
        case hkDataObject::TYPE_VEC_16: {
            pAlloc = &hkMemoryRouter::getInstance().heap();
            VecArrayImplementation* pArray =
                static_cast<VecArrayImplementation*>(pAlloc->blockAlloc(sizeof(VecArrayImplementation)));
            pArray->m_memSize = sizeof(VecArrayImplementation);
            return ::new (pArray) VecArrayImplementation(16);
        }
        case hkDataObject::TYPE_OBJECT: {
            pAlloc = &hkMemoryRouter::getInstance().heap();
            PointerArrayImplementation* pArray =
                static_cast<PointerArrayImplementation*>(pAlloc->blockAlloc(sizeof(PointerArrayImplementation)));
            pArray->m_memSize = sizeof(PointerArrayImplementation);
            return ::new (pArray) PointerArrayImplementation(klass);
        }
        case hkDataObject::TYPE_STRUCT: {
            pAlloc = &hkMemoryRouter::getInstance().heap();
            StructArrayImplementation* pArray =
                static_cast<StructArrayImplementation*>(pAlloc->blockAlloc(sizeof(StructArrayImplementation)));
            pArray->m_memSize = sizeof(StructArrayImplementation);
            // The source's downcast: a dictionary world's classes are dictionary classes.
            return ::new (pArray) StructArrayImplementation(static_cast<const hkDataClassDict*>(klass));
        }
        case hkDataObject::TYPE_CSTRING: {
            pAlloc = &hkMemoryRouter::getInstance().heap();
            CstringArrayImplementation* pArray =
                static_cast<CstringArrayImplementation*>(pAlloc->blockAlloc(sizeof(CstringArrayImplementation)));
            pArray->m_memSize = sizeof(CstringArrayImplementation);
            return ::new (pArray) CstringArrayImplementation;
        }
        default: return nullptr;
    }
}

// The name's handle comes from root's tracker.
hkDataClassDict::hkDataClassDict(hkDataWorldDict* root, const char* name, int32_t version)
  : m_world(root), m_version(version), m_parent(nullptr) {
    root->m_tracker->internClassNameHandle(&m_name, name);
}

// The members are released, then the parent's reference; the member arrays (with their type name handles), the enum
// array and the name handle free themselves.
hkDataClassDict::~hkDataClassDict() {
    selfDestruct();
    if ((m_parent) && (m_parent->m_count-- == 1)) {
        delete m_parent;
    }
}

// The interned name's text.
const char* hkDataClassDict::getName() const {
    return m_name.getString();
}

// Whether k is this class or derives from it: k's chain is walked up, comparing each interned name with ours.
hkBool hkDataClassDict::isSuperClass(const hkDataClassImpl* k) const {
    hkBool kResult;
    for (const hkDataClassImpl* pClass = k; pClass; pClass = pClass->getParent()) {
        const char* pName = m_world->m_tracker->intern(pClass->getName()).getString();
        if (pName == m_name.getString()) {
            kResult.m_bool = 1;
            return kResult;
        }
    }
    kResult.m_bool = 0;
    return kResult;
}

// Reads the type name before the tracker, and stores the class before the type and tuple count.
void hkDataClassDict::getDeclaredMemberInfo(int32_t i, hkDataClass::MemberInfo& info) const {
    const MemberInfo& kMember = m_memberInfo.m_data[i];
    info.m_name = kMember.m_name.getString();
    info.m_owner = this;
    const char* pTypeName = kMember.m_typeName.getString();
    hkDataWorldDict::ObjectTracker* pTracker = m_world->m_tracker;
    if (pTypeName) {
        info.m_class = pTracker->m_classes.getWithDefault(pTypeName, nullptr);
    }
    else {
        info.m_class = nullptr;
    }
    info.m_type = kMember.m_type;
    info.m_tupleCount = kMember.m_tupleCount;
}

// The index of the declared member called name, or -1. Member names are interned, so they compare by pointer.
int32_t hkDataClassDict::getDeclaredMemberIndexByName(const char* nameIn) const {
    InternedString pName = m_world->m_tracker->intern(nameIn);
    for (int32_t i = 0; i < m_memberInfo.m_size; ++i) {
        if (m_memberInfo.m_data[i].m_name == pName) {
            return i;
        }
    }
    return -1;
}

// The class's own members and every ancestor's.
int32_t hkDataClassDict::getNumMembers() const {
    int32_t iCount = m_memberInfo.m_size;
    for (hkDataClassDict* pParent = m_parent; pParent; pParent = pParent->m_parent) {
        iCount += pParent->m_memberInfo.m_size;
    }
    return iCount;
}

// Member i among all members, the ancestors' first. Stores this class as the owner even when an ancestor declares the
// member, as the game does, and leaves info untouched when i is out of range.
void hkDataClassDict::getMemberInfo(int32_t memberIndex, hkDataClass::MemberInfo& info) const {
    const hkDataClassDict* pClass = this;
    int32_t iIndex = memberIndex - getNumMembers();
    for (;;) {
        iIndex += pClass->m_memberInfo.m_size;
        if (iIndex >= 0) {
            break;
        }
        pClass = pClass->m_parent;
        if (!pClass) {
            return;
        }
    }
    const MemberInfo& kMember = pClass->m_memberInfo.m_data[iIndex];
    info.m_name = kMember.m_name.getString();
    info.m_owner = this;
    info.m_type = kMember.m_type;
    const char* pTypeName = kMember.m_typeName.getString();
    hkDataWorldDict::ObjectTracker* pTracker = m_world->m_tracker;
    if (pTypeName) {
        info.m_class = pTracker->m_classes.getWithDefault(pTypeName, nullptr);
    }
    else {
        info.m_class = nullptr;
    }
    info.m_tupleCount = kMember.m_tupleCount;
}

// The index of the member called name among all members, the ancestors' first, or -1. The search starts at this
// class and walks up; a hit's index is offset by the members the class's ancestors declare.
int32_t hkDataClassDict::getMemberIndexByName(const char* nameIn) const {
    InternedString pName = m_world->m_tracker->intern(nameIn);
    for (const hkDataClassDict* pClass = this; pClass; pClass = pClass->m_parent) {
        for (int32_t i = 0; i < pClass->m_memberInfo.m_size; ++i) {
            if (pClass->m_memberInfo.m_data[i].m_name == pName) {
                int32_t iIndex = i - pClass->getNumDeclaredMembers();
                return iIndex + pClass->getNumMembers();
            }
        }
    }
    return -1;
}

const hkDataClassDict::MemberInfo* hkDataClassDict::getMemberInfoByName(InternedString name) const {
    const hkDataClassDict* pThis = this;
    int32_t m_size;
    int32_t v3;
    if (!pThis) {
        return nullptr;
    }
    while (true) {
        m_size = pThis->m_memberInfo.m_size;
        v3 = 0;
        if (m_size > 0) {
            break;
        }
    LABEL_6:;
        pThis = pThis->m_parent;
        if (!pThis) {
            return nullptr;
        }
    }
    hkDataClassDict::MemberInfo* m_data = pThis->m_memberInfo.m_data;
    while (m_data->m_name != name.getString()) {
        ++v3;
        ++m_data;
        if (v3 >= m_size) {
            goto LABEL_6;
        }
    }
    return &pThis->m_memberInfo.m_data[v3];
}

// Drops the last declared member called name (interned): its type name's reference goes and the later members move
// down. HK_FAILURE when a member of that name is still found (an ancestor's or a second one). With no such member it
// works on the element before the first, deliberately, as the game does.
hkResult hkDataClassDict::removeMember(InternedString name) {
    // The search starts at m_size - 1 and leaves -1 when nothing matches, also when m_size is not positive.
    int32_t iIndex = -1;
    for (int32_t i = m_memberInfo.m_size - 1; i >= 0; --i) {
        if (m_memberInfo.m_data[i].m_name == name) {
            iIndex = i;
            break;
        }
    }
    MemberInfo* pMember = &m_memberInfo.m_data[iIndex];
    // EXE-INLINED 0xC6CCA0
    if (pMember->m_typeName.m_cached.m_pntr) {
        pMember->m_typeName.m_cached.m_pntr->removeReference();
    }
    pMember->m_typeName.m_cached.m_pntr = nullptr;
    // EXE-INLINED none: hkArray<MemberInfo>::removeAtAndCopy (a forward copy, 8 bytes a step)
    int32_t iNewSize = --m_memberInfo.m_size;
    MemberInfo* pMove = &m_memberInfo.m_data[iIndex];
    for (int32_t j = 0; j < iNewSize - iIndex; ++j) {
        pMove[j].m_name = pMove[j + 1].m_name;
        pMove[j].m_typeName.m_cached.m_pntr = pMove[j + 1].m_typeName.m_cached.m_pntr;
        pMove[j].m_default = pMove[j + 1].m_default;
        pMove[j].m_type = pMove[j + 1].m_type;
        pMove[j].m_tupleCount = pMove[j + 1].m_tupleCount;
    }
    return getMemberInfoByName(name) ? HK_FAILURE : HK_SUCCESS;
}

const char** hkDataClassDict::intern(const char** result, const char* name) const {
    hkDataWorldDict::ObjectTracker* m_tracker = m_world->m_tracker;
    if (name) {
        hkStringMap<char*, hkContainerHeapAllocator>* p_m_interns = &m_tracker->m_interns;
        char* v5 = m_tracker->m_interns.getWithDefault(name, 0);
        if (!v5) {
            v5 = hkString::strDup(name);
            p_m_interns->insert(v5, v5);
        }
        *result = v5;
        return result;
    }

    *result = nullptr;
    return result;
}

void hkDataClassDict::addMember(
    InternedString name,
    hkDataObject::Type type,
    const InternedStringHandle& typeName,
    int32_t tupleCount
) {
    InternedNullRefCounted.addReference();
    if (typeName.m_cached.val()) {
        typeName.m_cached.val()->addReference();
    }
    InternedNullRefCounted.removeReference();
    InternedStringRefCounted* m_pntr = typeName.m_cached.val();
    hkArray<hkDataClassDict::MemberInfo, hkContainerHeapAllocator>* p_m_memberInfo = &m_memberInfo;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (m_memberInfo.m_size ==
        (m_memberInfo.m_capacityAndFlags & hkArrayBase<hkDataClassDict::MemberInfo>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, &m_memberInfo, sizeof(hkDataClassDict::MemberInfo));
    }
    hkDataClassDict::MemberInfo* m_data = p_m_memberInfo->begin();
    hkDataClassDict::MemberInfo* v10 = &m_memberInfo.m_data[m_memberInfo.m_size];
    if (v10) {
        v10->m_name = name;
        if (m_pntr) {
            m_pntr->addReference();
        }
        v10->m_typeName.m_cached.m_pntr = m_pntr;
        v10->m_default.i = 0;
        v10->m_type = type;
        v10->m_tupleCount = tupleCount;
    }
    ++p_m_memberInfo->m_size;
    if (m_pntr) {
        m_pntr->removeReference();
    }
}

void hkDataClassDict::selfDestruct() {
    int32_t m_size = m_memberInfo.m_size;
    hkDataClassDict::MemberInfo* m_data = m_memberInfo.begin();
    if (m_size > 0) {
        // Indexed by element: MemberInfo holds pointers, so its size differs on x64.
        int32_t i = 0;
        do {
            InternedStringRefCounted*& rpTypeName = m_data[i].m_typeName.m_cached.m_pntr;
            if (rpTypeName) {
                rpTypeName->removeReference();
            }
            rpTypeName = nullptr;
            ++i;
        }
        while (i < m_size);
    }
    m_memberInfo.m_size = 0;
    for (int32_t i = 0; i < m_enums.m_size; ++i) {
        hkDataClassDict::Enum* v7 = m_enums.m_data[i];
        if (v7) {
            hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
            int32_t m_capacityAndFlags = v7->m_items.m_capacityAndFlags;
            v7->m_items.m_size = 0;
            if (m_capacityAndFlags >= 0) {
                m_heap->bufFree(v7->m_items.begin(), sizeof(hkDataClassDict::Enum::Item) * m_capacityAndFlags);
            }
            v7->m_items.m_data = nullptr;
            v7->m_items.m_capacityAndFlags = hkArrayBase<hkDataClassDict::Enum::Item>::DONT_DEALLOCATE_FLAG;
            hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
            Instance->m_heap->blockFree(v7, sizeof(hkDataClassDict::Enum));
        }
    }
    m_enums.m_size = 0;
    InternedNullRefCounted.addReference();
    InternedStringRefCounted* m_pntr = m_name.m_cached.val();
    if (m_pntr) {
        m_pntr->removeReference();
    }
    m_name.m_cached.m_pntr = &InternedNullRefCounted;
}

hkDataWorldDict::ObjectTracker::ObjectTracker() : m_topLevelObject(nullptr) {}

// Releases everything the tracker holds: every tracked class's objects (an object something outside still references is
// reported first, id 0x760D3795, line 1780), the class references, the interned class names and the interned strings,
// then the containers. Every tracked object loses its tracker's reference after its members are released, then every
// class; the interned class names lose their reference and the interned strings are freed. The maps free their own
// storage. The maps keep their values as untyped words (hkUlong), read back as the pointers they hold.
hkDataWorldDict::ObjectTracker::~ObjectTracker() {
    hkArray<hkDataObjectDict*> objects;
    hkArray<hkDataClassDict*> classes;
    m_derivedFromParent.clear();
    m_topLevelObject = nullptr;
    for (auto it = m_classes.m_map.getIterator(); m_classes.m_map.isValid(it).m_bool;
         it = m_classes.m_map.getNext(it)) {
        hkDataClassDict* pClass = reinterpret_cast<hkDataClassDict*>(m_classes.m_map.getValue(it));
        if (classes.m_size == (classes.m_capacityAndFlags & hkArrayBase<hkDataClassDict*>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(hkMemoryRouter::getInstance().heap(), &classes, sizeof(hkDataClassDict*));
        }
        classes.m_data[classes.m_size++] = pClass;
        int32_t iObject = m_objectsFromClass.find(pClass);
        if (iObject != -1) {
            do {
                if (objects.m_size == (objects.m_capacityAndFlags & hkArrayBase<hkDataObjectDict*>::CAPACITY_MASK)) {
                    hkArrayUtil::_reserveMore(
                        hkMemoryRouter::getInstance().heap(), &objects, sizeof(hkDataObjectDict*)
                    );
                }
                objects.m_data[objects.m_size++] = m_objectsFromClass.m_valueChain.m_data[iObject].value;
                iObject = m_objectsFromClass.m_valueChain.m_data[iObject].next;
            }
            while (iObject != -1);
            m_objectsFromClass.remove(pClass);
        }
    }
    for (int32_t i = 0; i < objects.m_size; ++i) {
        hkDataObjectDict* pObject = objects.m_data[i];
        if (pObject->m_externalCount > 0) {
            char acBuffer[0x200];
            hkErrStream kStream(acBuffer, 0x200);
            kStream << "The object of class " << static_cast<hkDataObjectImpl*>(pObject)->getClass()->getName()
                    << " is about to be removed and all references to it will be invalid.\nHowever, the object is "
                       "referenced ("
                    << static_cast<int32_t>(pObject->m_externalCount)
                    << ") from outside the world and it will lead to unexpected behavior or crash.";
            hkError::s_instance->message(
                hkError::MESSAGE_WARNING, static_cast<int32_t>(0x760D3795u), acBuffer,
                ".\\Data\\Dict\\hkDataObjectDict.cpp", 1780
            );
        }
        pObject->selfDestruct();
    }
    for (int32_t i = 0; i < objects.m_size; ++i) {
        hkDataObjectDict* pObject = objects.m_data[i];
        if (--pObject->m_count == 0) {
            delete pObject;
        }
    }
    for (int32_t i = 0; i < classes.m_size; ++i) {
        hkDataClassDict* pClass = classes.m_data[i];
        if (--pClass->m_count == 0) {
            delete pClass;
        }
    }
    for (auto it = m_internedClassNames.m_map.getIterator(); m_internedClassNames.m_map.isValid(it).m_bool;
         it = m_internedClassNames.m_map.getNext(it)) {
        reinterpret_cast<InternedStringRefCounted*>(m_internedClassNames.m_map.getValue(it))->removeReference();
    }
    for (auto it = m_interns.m_map.getIterator(); m_interns.m_map.isValid(it).m_bool;
         it = m_interns.m_map.getNext(it)) {
        hkString::strFree(reinterpret_cast<char*>(m_interns.m_map.getValue(it)));
    }
}

void hkDataWorldDict::ObjectTracker::findTrackedClasses(
    hkArray<hkDataClassImpl*, hkContainerHeapAllocator>& classesOut
) const {
    hkCachedHashMap<hkStringMapOperations, hkContainerHeapAllocator>::Iterator Iterator = m_classes.m_map.getIterator();
    hkBool arResult = m_classes.m_map.isValid(Iterator);
    while (arResult.m_bool) {
        hkDataClassImpl* Value = m_classes.getValue(Iterator);
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        if (classesOut.m_size == (classesOut.m_capacityAndFlags & hkArrayBase<hkDataClassImpl*>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*Instance->m_heap, &classesOut, sizeof(hkDataClassImpl*));
        }
        classesOut.m_data[classesOut.m_size++] = Value;
        Iterator = m_classes.m_map.getNext(Iterator);
        arResult = m_classes.m_map.isValid(Iterator);
    }
}

// Appends the tracked objects of the class called className to objectsOut; with flags, also the elements of the
// class's tracked struct arrays, and with baseClass, recursively those of the classes derived from it.
void hkDataWorldDict::ObjectTracker::findTrackedObjectsByBase(
    const char* className,
    int32_t baseClass,
    int32_t addStructs,
    hkArray<hkDataObjectImpl*, hkContainerHeapAllocator>& objectsOut
) const {
    if (!className) {
        return;
    }
    hkDataClassDict* pClass = m_classes.getWithDefault(className, nullptr);
    if (!pClass) {
        return;
    }
    for (int32_t i = m_objectsFromClass.find(pClass); i != -1; i = m_objectsFromClass.m_valueChain.m_data[i].next) {
        hkDataObjectImpl* pObject = m_objectsFromClass.m_valueChain.m_data[i].value;
        // EXE-INLINED none: hkArray<hkDataObjectImpl*>::pushBack
        hkMemoryRouter& kRouter = hkMemoryRouter::getInstance();
        if (objectsOut.m_size == (objectsOut.m_capacityAndFlags & hkArrayBase<hkDataObjectImpl*>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*kRouter.m_heap, &objectsOut, sizeof(hkDataObjectImpl*));
        }
        objectsOut.m_data[objectsOut.m_size++] = pObject;
    }
    if (addStructs) {
        for (int32_t i = m_arraysFromClass.find(pClass); i != -1; i = m_arraysFromClass.m_valueChain.m_data[i].next) {
            hkDataArrayImpl* pArray = m_arraysFromClass.m_valueChain.m_data[i].value;
            for (int32_t j = 0; j < pArray->getSize(); ++j) {
                hkDataObjectImpl* pObject = pArray->asObject(j);
                // EXE-INLINED none: hkArray<hkDataObjectImpl*>::pushBack
                hkMemoryRouter& kRouter = hkMemoryRouter::getInstance();
                if (objectsOut.m_size ==
                    (objectsOut.m_capacityAndFlags & hkArrayBase<hkDataObjectImpl*>::CAPACITY_MASK)) {
                    hkArrayUtil::_reserveMore(*kRouter.m_heap, &objectsOut, sizeof(hkDataObjectImpl*));
                }
                objectsOut.m_data[objectsOut.m_size++] = pObject;
            }
        }
    }
    if (baseClass) {
        for (int32_t i = m_derivedFromParent.find(pClass); i != -1;
             i = m_derivedFromParent.m_valueChain.m_data[i].next) {
            const char* pName = m_derivedFromParent.m_valueChain.m_data[i].value->getName();
            findTrackedObjectsByBase(pName, baseClass, addStructs, objectsOut);
        }
    }
}

void hkDataWorldDict::ObjectTracker::getTrackedStructArrays(
    hkDataClassDict* c,
    int32_t baseClass,
    hkArray<hkDataArrayImpl*, hkContainerHeapAllocator>& arraysOut
) {
    hkDataWorldDict::ObjectTracker* v4 = this;
    hkSerializeMultiMap<hkDataClassDict*, StructArrayImplementation*>* p_m_arraysFromClass = &m_arraysFromClass;
    int32_t next = m_arraysFromClass.find(c);
    if (next != -1) {
        do {
            int32_t v7 = next;
            StructArrayImplementation** p_value = &p_m_arraysFromClass->m_valueChain.m_data[next].value;
            hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
            if (arraysOut.m_size == (arraysOut.m_capacityAndFlags & hkArrayBase<hkDataArrayImpl*>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(*Instance->m_heap, &arraysOut, sizeof(hkDataArrayImpl*));
            }
            arraysOut.m_data[arraysOut.m_size++] = *p_value;
            next = p_m_arraysFromClass->m_valueChain.m_data[v7].next;
        }
        while (next != -1);
        v4 = this;
    }
    if (baseClass) {
        hkSerializeMultiMap<hkDataClassDict*, hkDataClassDict*>* p_m_derivedFromParent = &v4->m_derivedFromParent;
        int32_t v11 = v4->m_derivedFromParent.find(c);
        if (v11 != -1) {
            hkSerializeMultiMap<hkDataClassDict*, hkDataClassDict*>::Value* m_data =
                p_m_derivedFromParent->m_valueChain.begin();
            do {
                v4->getTrackedStructArrays(m_data[v11].value, baseClass, arraysOut);
                m_data = p_m_derivedFromParent->m_valueChain.begin();
                v11 = p_m_derivedFromParent->m_valueChain.m_data[v11].next;
            }
            while (v11 != -1);
        }
    }
}

// The class is looked up before newName is interned; the interned name handle moves only when the name changes.
void hkDataWorldDict::ObjectTracker::retrackRenamedClass(const char* oldName, const char* newName) {
    hkDataClassDict* pClass = oldName ? m_classes.getWithDefault(oldName, nullptr) : nullptr;
    const char* pNewName = intern(newName).getString();
    if (oldName != pNewName) {
        InternedStringRefCounted* pHandle = m_internedClassNames.getWithDefault(oldName, nullptr);
        m_internedClassNames.remove(oldName);
        pHandle->m_cachedString = pNewName;
        m_internedClassNames.insert(pNewName, pHandle);
    }
    m_classes.remove(oldName);
    m_classes.insert(pNewName, pClass);
}

// Removes every tracked object of klass and of the classes derived from it. The class is looked up by its name and the
// derived classes' contents go first; the class's objects are then collected in a stack-allocated array, the top-level
// object is forgotten, an object something outside still references is reported (id 0x760D3794, line 1977), every
// object's members are released, and the class leaves the objects map. The external count is read as a signed 16-bit
// field.
void hkDataWorldDict::ObjectTracker::removeClassContent(hkDataClassDict* klass) {
    const char* pcName = klass->getName();
    hkDataClassDict* pTrackedClass = pcName ? m_classes.getWithDefault(pcName, nullptr) : nullptr;
    for (int32_t iDerived = m_derivedFromParent.find(klass); iDerived != -1;
         iDerived = m_derivedFromParent.m_valueChain.m_data[iDerived].next) {
        removeClassContent(m_derivedFromParent.m_valueChain.m_data[iDerived].value);
    }
    int32_t iObject = m_objectsFromClass.find(pTrackedClass);
    if (iObject == -1) {
        return;
    }
    hkLocalArray<hkDataObjectDict*> objects(512);
    do {
        hkDataObjectDict* pObject = m_objectsFromClass.m_valueChain.m_data[iObject].value;
        // EXE-INLINED none: hkArray<hkDataObjectDict*>::pushBack
        hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
        if (objects.m_size == (objects.m_capacityAndFlags & hkArrayBase<hkDataObjectDict*>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*pHeap, &objects, sizeof(hkDataObjectDict*));
        }
        objects.m_data[objects.m_size] = pObject;
        objects.m_size++;
        iObject = m_objectsFromClass.m_valueChain.m_data[iObject].next;
    }
    while (iObject != -1);
    for (int32_t i = 0; i < objects.m_size; ++i) {
        hkDataObjectDict* pObject = objects.m_data[i];
        if (m_topLevelObject == pObject) {
            m_topLevelObject = nullptr;
        }
        if (pObject->m_externalCount > 0) {
            alignas(4) uint8_t kStreamStorage[sizeof(hkErrStream)];
            char acBuffer[0x200];
            hkErrStream* pStream = ::new (kStreamStorage) hkErrStream(acBuffer, 0x200);
            *pStream << "The object of class " << static_cast<hkDataObjectImpl*>(pObject)->getClass()->getName()
                     << " is about to be removed and all references to it will be invalid.\nHowever, the object is "
                        "referenced ("
                     << static_cast<int32_t>(pObject->m_externalCount)
                     << ") from outside the world and it will lead to unexpected behavior or crash.";
            hkError::s_instance->message(
                hkError::MESSAGE_WARNING, static_cast<int32_t>(0x760D3794u), acBuffer,
                ".\\Data\\Dict\\hkDataObjectDict.cpp", 1977
            );
            pStream->~hkErrStream();
        }
        pObject->selfDestruct();
    }
    for (int32_t j = 0; j < objects.m_size; ++j) {
        hkDataObjectDict* pObject = objects.m_data[j];
        if (--pObject->m_count == 0) {
            delete pObject;
        }
    }
    m_objectsFromClass.remove(pTrackedClass);
}

char** hkDataWorldDict::ObjectTracker::intern(char** a1, const char* src) {
    if (src) {
        hkStringMap<char*, hkContainerHeapAllocator>* p_m_interns = &m_interns;
        char* v4 = m_interns.getWithDefault(src, 0);
        if (!v4) {
            v4 = hkString::strDup(src);
            p_m_interns->insert(v4, v4);
        }
        *a1 = v4;
        return a1;
    }

    *a1 = nullptr;
    return a1;
}

// The handle of name's interned class name, made on first use (an InternedStringRefCounted holding the interned
// string); the shared empty handle for a null name.
InternedStringHandle*
hkDataWorldDict::ObjectTracker::internClassNameHandle(InternedStringHandle* result, const char* name) {
    InternedStringRefCounted*& rHandle = result->m_cached.m_pntr;
    rHandle = nullptr;
    InternedNullRefCounted.addReference();
    if (rHandle) {
        rHandle->removeReference();
    }
    rHandle = &InternedNullRefCounted;
    if (!name) {
        return result;
    }
    InternedStringRefCounted* pHandle = m_internedClassNames.getWithDefault(name, nullptr);
    if (!pHandle) {
        hkMemoryAllocator& kAlloc = hkMemoryRouter::getInstance().heap();
        pHandle = static_cast<InternedStringRefCounted*>(kAlloc.blockAlloc(sizeof(InternedStringRefCounted)));
        pHandle->m_memSizeAndFlags = sizeof(InternedStringRefCounted);
        char* pName;
        char* pInterned = *intern(&pName, name);
        ::new (pHandle) InternedStringRefCounted(pInterned);
        m_internedClassNames.insert(pInterned, pHandle);
    }
    pHandle->addReference();
    if (rHandle) {
        rHandle->removeReference();
    }
    rHandle = pHandle;
    return result;
}

// Moves klass from oldParent's derived classes to those of its current parent.
void hkDataWorldDict::ObjectTracker::retrackDerivedClass(hkDataClassDict* oldParent, hkDataClassDict* klass) {
    if (oldParent) {
        // EXE-INLINED 0xDC8FE0
        int32_t i = m_derivedFromParent.find(oldParent);
        if (i != -1) {
            hkSerializeMultiMap<hkDataClassDict*, hkDataClassDict*>::Value* pData =
                m_derivedFromParent.m_valueChain.m_data;
            do {
                if (pData[i].value == klass) {
                    m_derivedFromParent.removeByIndex(oldParent, i);
                    break;
                }
                i = pData[i].next;
            }
            while (i != -1);
        }
    }
    hkDataClassImpl* pClass = klass;
    hkDataClassImpl* pParent = pClass->getParent();
    if (pParent) {
        m_derivedFromParent.insert(static_cast<hkDataClassDict*>(pParent), klass);
    }
}

// The class names are the tracker's own copies (keys of m_classes), freed one by one before the map is emptied.
void hkDataWorldDict::ObjectTracker::clear() {
    hkCachedHashMap<hkStringMapOperations, hkContainerHeapAllocator>::Iterator it = m_classes.m_map.getIterator();
    hkBool bValid = m_classes.m_map.isValid(it);
    while (bValid.m_bool) {
        char* pcKey = reinterpret_cast<char*>(m_classes.m_map.getKey(it));
        hkMemoryAllocator& rHeap = *hkMemoryRouter::getInstance().m_heap;
        hkString::strFree_00CB1190(pcKey, rHeap);
        it = m_classes.m_map.getNext(it);
        bValid = m_classes.m_map.isValid(it);
    }
    m_classes.m_map.clear();
}

// Tracks klass under its name, when no class of that name is tracked, and under its parent.
void hkDataWorldDict::ObjectTracker::trackClass(hkDataClassDict* klass) {
    hkDataClassImpl* pClass = klass;
    hkDataClassImpl* pParent = pClass->getParent();
    auto it = m_classes.m_map.findKey(hkUlong(pClass->getName()));
    hkBool kValid = m_classes.m_map.isValid(it);
    if (!kValid.m_bool) {
        m_classes.insert(pClass->getName(), klass);
        if (pParent) {
            m_derivedFromParent.insert(static_cast<hkDataClassDict*>(pParent), klass);
        }
    }
}

// Forgets klass and, depth first, every class derived from it: its objects, its link to its parent, its storage, then
// the derived classes (each released, deleted with its last reference) and its name entries. The derived classes are
// gathered in a local array of 64 on the stack allocator.
void hkDataWorldDict::ObjectTracker::untrackClass(hkDataClassDict* klass) {
    const char* name = klass->getName();
    hkDataClassDict* tracked = name ? m_classes.getWithDefault(name, nullptr) : nullptr;
    removeClassContent(klass);
    hkDataClassImpl* parent = tracked->getParent();
    if (parent) {
        // The source's downcast: a dictionary class's parent is a dictionary class.
        m_derivedFromParent.removeByValue(static_cast<hkDataClassDict*>(parent), klass);
    }
    klass->selfDestruct();
    hkLocalArray<hkDataClassDict*> children(64);
    int32_t next = m_derivedFromParent.find(klass);
    bool hasChildren = next != -1;
    if (hasChildren) {
        while (next != -1) {
            hkDataClassDict* child = m_derivedFromParent.m_valueChain.m_data[next].value;
            // EXE-INLINED none: hkArray<hkDataClassDict*>::pushBack
            hkMemoryRouter& router = hkMemoryRouter::getInstance();
            if (children.m_size == (children.m_capacityAndFlags & hkArrayBase<hkDataClassDict*>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(*router.m_heap, &children, static_cast<int32_t>(sizeof(hkDataClassDict*)));
            }
            children.m_data[children.m_size++] = child;
            next = m_derivedFromParent.m_valueChain.m_data[next].next;
        }
    }
    for (int32_t i = 0; i < children.m_size; ++i) {
        untrackClass(children.m_data[i]);
        hkDataClassDict* child = children.m_data[i];
        // EXE-INLINED none: hkDataRefCounted::removeReference
        if (--child->m_count == 0) {
            delete child;
        }
    }
    if (hasChildren) {
        m_derivedFromParent.remove(klass);
    }
    InternedStringRefCounted* internedName = m_internedClassNames.getWithDefault(name, nullptr);
    m_internedClassNames.remove(name);
    internedName->removeReference();
    m_classes.remove(name);
}

// Moves obj from its class's tracked objects to newClass's.
void hkDataWorldDict::ObjectTracker::retractCastedObject(hkDataObjectDict* obj, hkDataClassDict* newClass) {
    hkDataObjectImpl* pObject = obj;
    hkDataClassDict* pOld = static_cast<hkDataClassDict*>(const_cast<hkDataClassImpl*>(pObject->getClass()));
    for (int32_t i = m_objectsFromClass.find(pOld); pOld; i = m_objectsFromClass.m_valueChain.m_data[i].next) {
        if (i == -1) {
            break;
        }
        if (m_objectsFromClass.m_valueChain.m_data[i].value == obj) {
            m_objectsFromClass.removeByIndex(pOld, i);
            m_objectsFromClass.insert(newClass, obj);
            pOld = nullptr;
        }
    }
}

// The destructor only frees the array, which frees itself.
ByteArrayImplementation::~ByteArrayImplementation() {}

int32_t ByteArrayImplementation::asInt(int32_t i) const {
    return m_data.m_data[i];
}

void ByteArrayImplementation::setInt(int32_t i, int32_t val) {
    m_data.m_data[i] = (uint8_t)val;
}

void ByteArrayImplementation::setAll(const uint8_t* vals, int32_t n) {
    hkArray<uint8_t, hkContainerHeapAllocator>* p_m_data = &m_data;
    hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v5 = p_m_data->m_capacityAndFlags & hkArrayBase<uint8_t>::CAPACITY_MASK;
    if (v5 < n) {
        int32_t v6 = 2 * v5;
        if (n >= v6) {
            v6 = n;
        }
        hkArrayUtil::_reserve(m_heap, p_m_data, v6, sizeof(uint8_t));
    }
    p_m_data->m_size = n;
    hkString::memCpy(p_m_data->begin(), vals, n);
}

// The destructor only frees the array, which frees itself.
RealArrayImplementation::~RealArrayImplementation() {}

float RealArrayImplementation::asReal(int32_t i) const {
    return m_data.m_data[i];
}

void RealArrayImplementation::setReal(int32_t i, float val) {
    m_data.m_data[i] = val;
}

void RealArrayImplementation::setAll(const float* vals, int32_t n) {
    hkArray<float, hkContainerHeapAllocator>* p_m_data = &m_data;
    hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v5 = p_m_data->m_capacityAndFlags & hkArrayBase<float>::CAPACITY_MASK;
    if (v5 < n) {
        int32_t v6 = 2 * v5;
        if (n >= v6) {
            v6 = n;
        }
        hkArrayUtil::_reserve(m_heap, p_m_data, v6, sizeof(float));
    }
    p_m_data->m_size = n;
    hkString::memCpy(p_m_data->begin(), vals, sizeof(float) * n);
}

VecArrayImplementation::VecArrayImplementation(int32_t numRealPerElem) : m_numReal(numRealPerElem) {}

// The destructor only frees the array, which frees itself.
VecArrayImplementation::~VecArrayImplementation() {}

const float* VecArrayImplementation::asVec(int32_t index) const {
    return &m_data.m_data[m_numReal * index];
}

void VecArrayImplementation::setVec(int32_t index, const float* val) {
    for (int32_t i = 0; i < m_numReal; ++i) {
        m_data.m_data[m_numReal * index + i] = val[i];
    }
}

// There is deliberately no case for 8 reals (TYPE_VEC_8): that size gives TYPE_VOID, as the game does.
hkDataObject::Type VecArrayImplementation::getType() const {
    switch (m_numReal) {
        case 4: return hkDataObject::TYPE_VEC_4;
        case 12: return hkDataObject::TYPE_VEC_12;
        case 16: return hkDataObject::TYPE_VEC_16;
        default: return hkDataObject::TYPE_VOID;
    }
}

void VecArrayImplementation::clear() {
    m_data.m_size = 0;
}

// Grows to n vectors of m_numReal reals, at least doubling the capacity.
void VecArrayImplementation::reserve(int32_t n) {
    int32_t iReals = m_numReal * n;
    hkMemoryAllocator& kAlloc = hkMemoryRouter::getInstance().heap();
    int32_t iCapacity = m_data.m_capacityAndFlags & hkArrayBase<float>::CAPACITY_MASK;
    if (iCapacity < iReals) {
        int32_t iReserve = iCapacity * 2;
        if (iReals >= iReserve) {
            iReserve = iReals;
        }
        hkArrayUtil::_reserve(&kAlloc, &m_data, iReserve, sizeof(float));
    }
}

// Grows as reserve does, then zeroes the new reals.
void VecArrayImplementation::setSize(int32_t n) {
    int32_t iReals = m_numReal * n;
    hkMemoryAllocator& kAlloc = hkMemoryRouter::getInstance().heap();
    int32_t iCapacity = m_data.m_capacityAndFlags & hkArrayBase<float>::CAPACITY_MASK;
    if (iCapacity < iReals) {
        int32_t iReserve = iCapacity * 2;
        if (iReals >= iReserve) {
            iReserve = iReals;
        }
        hkArrayUtil::_reserve(&kAlloc, &m_data, iReserve, sizeof(float));
    }
    for (int32_t i = m_data.m_size; i < iReals; ++i) {
        m_data.m_data[i] = 0.0f;
    }
    m_data.m_size = iReals;
}

// The element count: the reals over the reals per element (a signed division).
int32_t VecArrayImplementation::getSize() const {
    return (int32_t)m_data.m_size / m_numReal;
}

PointerArrayImplementation::PointerArrayImplementation(const hkDataClassImpl* a1)
  : m_class(static_cast<hkDataClassDict*>(const_cast<hkDataClassImpl*>(a1))) {
    if (m_class) {
        ++m_class->m_count;
    }
}

// Each object and the class lose the array's reference; the array frees itself.
PointerArrayImplementation::~PointerArrayImplementation() {
    for (int32_t i = 0; i < m_data.m_size; ++i) {
        hkDataObjectImpl* pObject = m_data.m_data[i];
        if ((pObject) && (pObject->m_count-- == 1)) {
            delete pObject;
        }
    }
    if ((m_class) && (m_class->m_count-- == 1)) {
        delete m_class;
    }
}

hkDataObjectImpl* PointerArrayImplementation::asObject(int32_t i) const {
    return m_data.m_data[i];
}

// Takes a reference to val first, then releases the old object (its deleting destructor at zero).
void PointerArrayImplementation::setObject(int32_t i, hkDataObjectImpl* val) {
    if (val) {
        ++val->m_count;
    }
    hkDataObjectImpl* pOld = m_data.m_data[i];
    if (pOld && --pOld->m_count == 0) {
        delete pOld;
    }
    m_data.m_data[i] = val;
}

// The destructor only frees the array, which frees itself.
IntArrayImplementation::~IntArrayImplementation() {}

int32_t IntArrayImplementation::asInt(int32_t i) const {
    return (int32_t)m_data.m_data[i];
}

void IntArrayImplementation::setInt(int32_t i, int32_t val) {
    m_data.m_data[i] = val;
}

int64_t IntArrayImplementation::asInt64(int32_t i) const {
    return m_data.m_data[i];
}

void IntArrayImplementation::setInt64(int32_t i, int64_t val) {
    m_data.m_data[i] = val;
}

void IntArrayImplementation::setAll(const hkBool* v, int32_t n) {
    hkArray<int64_t, hkContainerHeapAllocator>* p_m_data = &m_data;
    hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v5 = p_m_data->m_capacityAndFlags & hkArrayBase<int64_t>::CAPACITY_MASK;
    if (v5 < n) {
        int32_t v6 = 2 * v5;
        if (n >= v6) {
            v6 = n;
        }
        hkArrayUtil::_reserve(m_heap, p_m_data, v6, sizeof(int64_t));
    }
    int32_t v7 = 0;
    for (p_m_data->m_size = n; v7 < n; ++v7) {
        p_m_data->m_data[v7] = v[v7].m_bool != 0;
    }
}

void IntArrayImplementation::setAll(const char* v, int32_t n) {
    hkArray<int64_t, hkContainerHeapAllocator>* p_m_data = &m_data;
    hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v5 = p_m_data->m_capacityAndFlags & hkArrayBase<int64_t>::CAPACITY_MASK;
    if (v5 < n) {
        int32_t v6 = 2 * v5;
        if (n >= v6) {
            v6 = n;
        }
        hkArrayUtil::_reserve(*m_heap, p_m_data, v6, sizeof(int64_t));
    }
    int32_t v7 = 0;
    for (p_m_data->m_size = n; v7 < n; ++v7) {
        p_m_data->m_data[v7] = v[v7];
    }
}

void IntArrayImplementation::setAll(const int8_t* v, int32_t n) {
    hkArray<int64_t, hkContainerHeapAllocator>* p_m_data = &m_data;
    hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v5 = p_m_data->m_capacityAndFlags & hkArrayBase<int64_t>::CAPACITY_MASK;
    if (v5 < n) {
        int32_t v6 = 2 * v5;
        if (n >= v6) {
            v6 = n;
        }
        hkArrayUtil::_reserve(*m_heap, p_m_data, v6, sizeof(int64_t));
    }
    int32_t v7 = 0;
    for (p_m_data->m_size = n; v7 < n; ++v7) {
        p_m_data->m_data[v7] = v[v7];
    }
}

void IntArrayImplementation::setAll(const uint8_t* v, int32_t n) {
    hkArray<int64_t, hkContainerHeapAllocator>* p_m_data = &m_data;
    hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v5 = p_m_data->m_capacityAndFlags & hkArrayBase<int64_t>::CAPACITY_MASK;
    if (v5 < n) {
        int32_t v6 = 2 * v5;
        if (n >= v6) {
            v6 = n;
        }
        hkArrayUtil::_reserve(m_heap, p_m_data, v6, sizeof(int64_t));
    }
    int32_t v7 = 0;
    for (p_m_data->m_size = n; v7 < n; ++v7) {
        p_m_data->m_data[v7] = v[v7];
    }
}

void IntArrayImplementation::setAll(const int16_t* v, int32_t n) {
    hkArray<int64_t, hkContainerHeapAllocator>* p_m_data = &m_data;
    hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v5 = p_m_data->m_capacityAndFlags & hkArrayBase<int64_t>::CAPACITY_MASK;
    if (v5 < n) {
        int32_t v6 = 2 * v5;
        if (n >= v6) {
            v6 = n;
        }
        hkArrayUtil::_reserve(m_heap, p_m_data, v6, sizeof(int64_t));
    }
    int32_t v7 = 0;
    for (p_m_data->m_size = n; v7 < n; ++v7) {
        p_m_data->m_data[v7] = v[v7];
    }
}

void IntArrayImplementation::setAll(const uint16_t* v, int32_t n) {
    hkArray<int64_t, hkContainerHeapAllocator>* p_m_data = &m_data;
    hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v5 = p_m_data->m_capacityAndFlags & hkArrayBase<int64_t>::CAPACITY_MASK;
    if (v5 < n) {
        int32_t v6 = 2 * v5;
        if (n >= v6) {
            v6 = n;
        }
        hkArrayUtil::_reserve(m_heap, p_m_data, v6, sizeof(int64_t));
    }
    int32_t v7 = 0;
    for (p_m_data->m_size = n; v7 < n; ++v7) {
        p_m_data->m_data[v7] = v[v7];
    }
}

void IntArrayImplementation::setAll(const int32_t* v, int32_t n) {
    hkArray<int64_t, hkContainerHeapAllocator>* p_m_data = &m_data;
    hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v5 = p_m_data->m_capacityAndFlags & hkArrayBase<int64_t>::CAPACITY_MASK;
    if (v5 < n) {
        int32_t v6 = 2 * v5;
        if (n >= v6) {
            v6 = n;
        }
        hkArrayUtil::_reserve(*m_heap, p_m_data, v6, sizeof(int64_t));
    }
    int32_t v7 = 0;
    for (p_m_data->m_size = n; v7 < n; ++v7) {
        p_m_data->m_data[v7] = v[v7];
    }
}

// Behaves the same as the int32_t overload: each value is taken as signed.
void IntArrayImplementation::setAll(const uint32_t* v, int32_t n) {
    hkArray<int64_t, hkContainerHeapAllocator>* p_m_data = &m_data;
    hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v5 = p_m_data->m_capacityAndFlags & hkArrayBase<int64_t>::CAPACITY_MASK;
    if (v5 < n) {
        int32_t v6 = 2 * v5;
        if (n >= v6) {
            v6 = n;
        }
        hkArrayUtil::_reserve(*m_heap, p_m_data, v6, sizeof(int64_t));
    }
    int32_t v7 = 0;
    for (p_m_data->m_size = n; v7 < n; ++v7) {
        p_m_data->m_data[v7] = static_cast<int32_t>(v[v7]);
    }
}

void IntArrayImplementation::setAll(const int64_t* v, int32_t n) {
    hkArray<int64_t, hkContainerHeapAllocator>* p_m_data = &m_data;
    hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v5 = p_m_data->m_capacityAndFlags & hkArrayBase<int64_t>::CAPACITY_MASK;
    if (v5 < n) {
        int32_t v6 = 2 * v5;
        if (n >= v6) {
            v6 = n;
        }
        hkArrayUtil::_reserve(*m_heap, p_m_data, v6, sizeof(int64_t));
    }
    int32_t v7 = 0;
    for (p_m_data->m_size = n; v7 < n; ++v7) {
        p_m_data->m_data[v7] = static_cast<int32_t>(v[v7]);
    }
}

// Same as the int64_t overload: each value's low 32 bits, sign-extended.
void IntArrayImplementation::setAll(const uint64_t* v, int32_t n) {
    hkArray<int64_t, hkContainerHeapAllocator>* p_m_data = &m_data;
    hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v5 = p_m_data->m_capacityAndFlags & hkArrayBase<int64_t>::CAPACITY_MASK;
    if (v5 < n) {
        int32_t v6 = 2 * v5;
        if (n >= v6) {
            v6 = n;
        }
        hkArrayUtil::_reserve(*m_heap, p_m_data, v6, sizeof(int64_t));
    }
    int32_t v7 = 0;
    for (p_m_data->m_size = n; v7 < n; ++v7) {
        p_m_data->m_data[v7] = static_cast<int32_t>(v[v7]);
    }
}

// The strings the array owns are freed; the array frees itself.
CstringArrayImplementation::~CstringArrayImplementation() {
    for (int32_t i = 0; i < m_data.m_size; ++i) {
        hkString::strFree(m_data.m_data[i]);
    }
}

const char* CstringArrayImplementation::asString(int32_t i) const {
    return m_data.m_data[i];
}

// Frees the old string, then stores a copy of val (or null).
void CstringArrayImplementation::setString(int32_t i, const char* val) {
    hkString::strFree(m_data.m_data[i]);
    if (val) {
        m_data.m_data[i] = hkString::strDup(val);
    }
    else {
        m_data.m_data[i] = nullptr;
    }
}

// The view holds nothing of its own.
ArrayOfTuplesImplementation::View::~View() {}

hkDataObject::Type ArrayOfTuplesImplementation::View::getType() const {
    return (hkDataObject::Type)(m_owner->getType() ^ hkDataObject::TYPE_TUPLE);
}

const hkDataClassImpl* ArrayOfTuplesImplementation::View::getClass() const {
    return m_owner->getClass();
}

void ArrayOfTuplesImplementation::View::clear() {
    m_owner->m_impl->clear();
}

int32_t ArrayOfTuplesImplementation::View::getSize() const {
    return m_owner->m_tupleCount;
}

// The getters add the view's base index and call the element array's accessor.
const float* ArrayOfTuplesImplementation::View::asVec(int32_t index) const {
    return m_owner->m_impl->asVec(m_baseIndex + index);
}

void ArrayOfTuplesImplementation::View::setVec(int32_t index, const float* val) {
    m_owner->m_impl->setVec(m_baseIndex + index, val);
}

const char* ArrayOfTuplesImplementation::View::asString(int32_t index) const {
    return m_owner->m_impl->asString(m_baseIndex + index);
}

void ArrayOfTuplesImplementation::View::setString(int32_t index, const char* val) {
    m_owner->m_impl->setString(m_baseIndex + index, val);
}

float ArrayOfTuplesImplementation::View::asReal(int32_t index) const {
    return m_owner->m_impl->asReal(m_baseIndex + index);
}

void ArrayOfTuplesImplementation::View::setReal(int32_t index, float val) {
    m_owner->m_impl->setReal(m_baseIndex + index, val);
}

int32_t ArrayOfTuplesImplementation::View::asInt(int32_t index) const {
    return m_owner->m_impl->asInt(m_baseIndex + index);
}

void ArrayOfTuplesImplementation::View::setInt(int32_t index, int32_t val) {
    m_owner->m_impl->setInt(m_baseIndex + index, val);
}

int64_t ArrayOfTuplesImplementation::View::asInt64(int32_t index) const {
    return m_owner->m_impl->asInt64(m_baseIndex + index);
}

void ArrayOfTuplesImplementation::View::setInt64(int32_t index, int64_t val) {
    m_owner->m_impl->setInt64(m_baseIndex + index, val);
}

hkDataObjectImpl* ArrayOfTuplesImplementation::View::asObject(int32_t index) const {
    return m_owner->m_impl->asObject(m_baseIndex + index);
}

void ArrayOfTuplesImplementation::View::setObject(int32_t index, hkDataObjectImpl* val) {
    m_owner->m_impl->setObject(m_baseIndex + index, val);
}

hkDataArrayImpl* ArrayOfTuplesImplementation::View::asArray(int32_t index) const {
    return m_owner->m_impl->asArray(m_baseIndex + index);
}

void ArrayOfTuplesImplementation::View::setArray(int32_t index, hkDataArrayImpl* val) {
    m_owner->m_impl->setArray(m_baseIndex + index, val);
}

// Each element through the element array's setInt at the view's base index.
void ArrayOfTuplesImplementation::View::setAll(const uint8_t* v, int32_t n) {
    for (int32_t i = 0; i < n; ++i) {
        m_owner->m_impl->setInt(m_baseIndex + i, v[i]);
    }
}

// Sets each element through the element array's setReal at the view's base index.
void ArrayOfTuplesImplementation::View::setAll(const float* v, int32_t n) {
    for (int32_t i = 0; i < n; ++i) {
        m_owner->m_impl->setReal(m_baseIndex + i, v[i]);
    }
}

// The tuples' array loses its reference.
ArrayOfTuplesImplementation::~ArrayOfTuplesImplementation() {
    if (m_impl->m_count-- == 1) {
        delete m_impl;
    }
}

hkDataObject::Type ArrayOfTuplesImplementation::getType() const {
    return (hkDataObject::Type)(m_impl->getType() | hkDataObject::TYPE_TUPLE);
}

const hkDataClassImpl* ArrayOfTuplesImplementation::getClass() const {
    return m_impl->getClass();
}

void ArrayOfTuplesImplementation::clear() {
    m_impl->clear();
}

// The element array's reserve, for m_tupleCount elements per tuple.
void ArrayOfTuplesImplementation::reserve(int32_t n) {
    m_impl->reserve(m_tupleCount * n);
}

// The element array's setSize, for m_tupleCount elements per tuple.
void ArrayOfTuplesImplementation::setSize(int32_t n) {
    m_impl->setSize(m_tupleCount * n);
}

int32_t ArrayOfTuplesImplementation::getSize() const {
    return m_impl->getSize() / m_tupleCount;
}

// Points the view at tuple index.
hkDataArrayImpl* ArrayOfTuplesImplementation::asArray(int32_t idx) const {
    m_view.m_baseIndex = m_tupleCount * idx;
    return &m_view;
}

// Copies the array src into tuple idx, element by element through the view's setter for src's element type.
void ArrayOfTuplesImplementation::setArray(int32_t idx, hkDataArrayImpl* src) {
    m_view.m_baseIndex = idx * m_tupleCount;
    hkDataArrayImpl* pView = &m_view;
    switch (src->getType()) {
        case hkDataObject::TYPE_BYTE:
            for (int32_t i = 0; i < m_tupleCount; ++i) {
                pView->setInt(i, src->asInt(i));
            }
            break;
        case hkDataObject::TYPE_INT:
            for (int32_t i = 0; i < m_tupleCount; ++i) {
                pView->setInt64(i, src->asInt64(i));
            }
            break;
        case hkDataObject::TYPE_REAL:
            for (int32_t i = 0; i < m_tupleCount; ++i) {
                pView->setReal(i, src->asReal(i));
            }
            break;
        case hkDataObject::TYPE_VEC_4:
        case hkDataObject::TYPE_VEC_8:
        case hkDataObject::TYPE_VEC_12:
        case hkDataObject::TYPE_VEC_16:
            for (int32_t i = 0; i < m_tupleCount; ++i) {
                pView->setVec(i, src->asVec(i));
            }
            break;
        case hkDataObject::TYPE_OBJECT:
        case hkDataObject::TYPE_STRUCT:
            for (int32_t i = 0; i < m_tupleCount; ++i) {
                pView->setObject(i, src->asObject(i));
            }
            break;
        case hkDataObject::TYPE_CSTRING:
            for (int32_t i = 0; i < m_tupleCount; ++i) {
                pView->setString(i, src->asString(i));
            }
            break;
        default: break;
    }
}

hkDataObject::Handle StructArrayImplementation::Object::getHandle() const {
    hkDataObject::Handle kHandle;
    kHandle.p0 = const_cast<Object*>(this);
    kHandle.p1 = nullptr;
    return kHandle;
}

const hkDataClassImpl* StructArrayImplementation::Object::getClass() const {
    return m_impl->getClass();
}

hkDataObject::Value StructArrayImplementation::Object::accessByName(const char* name) {
    hkDataObject::Value result;
    result.m_impl = this;
    result.m_name = name;
    return result;
}

// Whether the element's struct has a member called name (interned through the object's own intern).
int32_t StructArrayImplementation::Object::hasMember(const char* name) const {
    const char* pInterned = name;
    intern(&pInterned, name);
    StructArrayImplementation* pImpl = m_impl;
    int32_t iSize = pImpl->m_memberData.m_size;
    for (int32_t i = 0; i < iSize; ++i) {
        if (pInterned == pImpl->m_memberData.m_data[i].m_memberName) {
            return 1;
        }
    }
    return 0;
}

int32_t StructArrayImplementation::Object::getMemberIterator() const {
    return 0;
}

int32_t StructArrayImplementation::Object::isValid(int32_t it) const {
    return it < m_impl->m_memberData.m_size;
}

int32_t StructArrayImplementation::Object::getNextMember(int32_t it) const {
    return it + 1;
}

const char* StructArrayImplementation::Object::getMemberName(int32_t it) const {
    return m_impl->m_memberData.m_data[it].m_memberName;
}

const hkDataObject::Value StructArrayImplementation::Object::getMemberValue(int32_t it) const {
    hkDataObject::Value result;
    // The source's const_cast: a Value refers to its object through a non-const hkDataObjectImpl*.
    result.m_impl = const_cast<StructArrayImplementation::Object*>(this);
    result.m_name = m_impl->m_memberData.m_data[it].m_memberName;
    return result;
}

// The member called name as an array.
hkDataArrayImpl* StructArrayImplementation::Object::asArray(const char* name) {
    return m_impl->m_memberData.m_data[m_impl->_find(name)].m_values->asArray(m_index);
}

// The member called name as a string: its value array, at the object's element index. A member's handle is its interned
// name, the const char* itself.
const char* StructArrayImplementation::Object::asString(const char* name) {
    return m_impl->m_memberData.m_data[m_impl->_find(name)].m_values->asString(m_index);
}

int64_t StructArrayImplementation::Object::asInt(const char* name) {
    hkDataArrayImpl* m_values = m_impl->m_memberData.m_data[m_impl->_find(name)].m_values;
    return m_values->asInt(m_index);
}

// The member called name as an object: the value array's element at the object's index, through the array's virtual
// asObject(int).
hkDataObjectImpl* StructArrayImplementation::Object::asObject(const char* name) {
    return m_impl->m_memberData.m_data[m_impl->_find(name)].m_values->asObject(m_index);
}

const float* StructArrayImplementation::Object::asVec(const char* nameIn, int32_t n) {
    hkDataArrayImpl* m_values = m_impl->m_memberData.m_data[m_impl->_find(nameIn)].m_values;
    return m_values->asVec(m_index);
}

// The member called name as a real.
float StructArrayImplementation::Object::asReal(const char* name) {
    return m_impl->m_memberData.m_data[m_impl->_find(name)].m_values->asReal(m_index);
}

const char** StructArrayImplementation::Object::intern(const char** result, const char* name) const {
    m_impl->m_class->intern(result, name);
    return result;
}

void StructArrayImplementation::Object::selfDestruct() {}

hkHalf StructArrayImplementation::Object::asHalf(const char* name) {
    hkHalf result;
    hkDataArrayImpl* m_values = m_impl->m_memberData.m_data[m_impl->_find(name)].m_values;
    result = m_values->asHalf(m_index);
    return result;
}

void StructArrayImplementation::Object::assign(const char* name, int8_t value) {
    hkDataArrayImpl* m_values = m_impl->m_memberData.m_data[m_impl->_findOrAdd(name)].m_values;
    m_values->setInt(m_index, value);
}

void StructArrayImplementation::Object::assign(const char* name, int16_t value) {
    hkDataArrayImpl* m_values = m_impl->m_memberData.m_data[m_impl->_findOrAdd(name)].m_values;
    m_values->setInt(m_index, value);
}

void StructArrayImplementation::Object::assign(const char* name, int32_t value) {
    hkDataArrayImpl* m_values = m_impl->m_memberData.m_data[m_impl->_findOrAdd(name)].m_values;
    m_values->setInt(m_index, value);
}

void StructArrayImplementation::Object::assign(const char* handle, const hkDataObject::Value& value) {
    int32_t v4 = m_impl->_findOrAdd(handle);
    m_impl->m_memberData.m_data[v4].m_values->set(m_index, value);
}

void StructArrayImplementation::Object::assign(const char* handle, hkDataObjectImpl* value) {
    hkDataArrayImpl* m_values = m_impl->m_memberData.m_data[m_impl->_findOrAdd(handle)].m_values;
    m_values->setObject(m_index, value);
}

void StructArrayImplementation::Object::assign(const char* handle, hkDataArrayImpl* value) {
    hkDataArrayImpl* m_values = m_impl->m_memberData.m_data[m_impl->_findOrAdd(handle)].m_values;
    m_values->setArray(m_index, value);
}

void StructArrayImplementation::Object::assign(const char* handle, const char* value) {
    hkDataArrayImpl* m_values = m_impl->m_memberData.m_data[m_impl->_findOrAdd(handle)].m_values;
    m_values->setString(m_index, value);
}

void StructArrayImplementation::Object::assign(const char* handle, float value) {
    hkDataArrayImpl* m_values = m_impl->m_memberData.m_data[m_impl->_findOrAdd(handle)].m_values;
    m_values->setReal(m_index, value);
}

void StructArrayImplementation::Object::assign(const char* handle, hkHalf value) {
    hkDataArrayImpl* m_values = m_impl->m_memberData.m_data[m_impl->_findOrAdd(handle)].m_values;
    m_values->setHalf(m_index, value);
}

void StructArrayImplementation::Object::assign(const char* handle, const float* value, int32_t nreal) {
    hkDataArrayImpl* m_values = m_impl->m_memberData.m_data[m_impl->_findOrAdd(handle)].m_values;
    m_values->setVec(m_index, value);
}

void StructArrayImplementation::Object::assign(const char* handle, int64_t value) {
    hkDataArrayImpl* m_values = m_impl->m_memberData.m_data[m_impl->_findOrAdd(handle)].m_values;
    m_values->setInt64(m_index, value);
}

// Holds a value array per member of the class, found through the parents (member i of the class is the parents' members
// first), then the array is tracked under its class.
StructArrayImplementation::StructArrayImplementation(const hkDataClassDict* a1)
  : m_class(const_cast<hkDataClassDict*>(a1)), m_size(0) {
    if (m_class) {
        ++m_class->m_count;
    }
    for (int32_t i = 0; i < m_class->getNumMembers(); ++i) {
        const hkDataClassDict* pOwner = m_class;
        int32_t iLocal = i - m_class->getNumMembers();
        for (;;) {
            iLocal += pOwner->m_memberInfo.m_size;
            if (iLocal >= 0) {
                break;
            }
            pOwner = pOwner->m_parent;
        }
        _addMember(pOwner->m_memberInfo.m_data[iLocal]);
    }
    m_class->getWorld()->m_tracker->m_arraysFromClass.insert(m_class, this);
}

// Each member's values lose the array's reference, the array leaves its class's tracked arrays and the class loses its
// reference; the member array frees itself.
StructArrayImplementation::~StructArrayImplementation() {
    for (int32_t i = 0; i < m_memberData.m_size; ++i) {
        hkDataArrayImpl* pValues = m_memberData.m_data[i].m_values;
        if (pValues->m_count-- == 1) {
            delete pValues;
        }
    }
    hkSerializeMultiMap<hkDataClassDict*, StructArrayImplementation*>& rArrays =
        m_class->getWorld()->m_tracker->m_arraysFromClass;
    int32_t iNext = rArrays.find(m_class);
    while (iNext != -1 && rArrays.m_valueChain.m_data[iNext].value != this) {
        iNext = rArrays.m_valueChain.m_data[iNext].next;
    }
    if (iNext != -1) {
        rArrays.removeByIndex(m_class, iNext);
    }
    if ((m_class) && (m_class->m_count-- == 1)) {
        delete m_class;
    }
}

// A new object view of element index, allocated from the thread's heap.
hkDataObjectImpl* StructArrayImplementation::asObject(int32_t i) const {
    hkMemoryAllocator& kAlloc = hkMemoryRouter::getInstance().heap();
    Object* pObject = static_cast<Object*>(kAlloc.blockAlloc(sizeof(Object)));
    pObject->m_memSize = sizeof(Object);
    return ::new (pObject) Object(const_cast<StructArrayImplementation*>(this), i);
}

// Each member's value of val goes into that member's array at index. The member array and the object's accessor are
// reloaded each pass, and the member count is compared signed.
void StructArrayImplementation::setObject(int32_t index, hkDataObjectImpl* val) {
    for (int32_t i = 0; i < m_memberData.m_size; ++i) {
        hkDataObject::Value kValue = val->accessByName(m_memberData.m_data[i].m_memberName);
        m_memberData.m_data[i].m_values->set(index, kValue);
    }
}

// The member count is compared signed and reloaded each pass.
void StructArrayImplementation::clear() {
    for (int32_t i = 0; i < m_memberData.m_size; ++i) {
        m_memberData.m_data[i].m_values->clear();
    }
    m_size = 0;
}

void StructArrayImplementation::reserve(int32_t n) {
    for (int32_t i = 0; i < m_memberData.m_size; ++i) {
        m_memberData.m_data[i].m_values->reserve(n);
    }
}

// Stores the size before resizing the member arrays.
void StructArrayImplementation::setSize(int32_t n) {
    m_size = n;
    for (int32_t i = 0; i < m_memberData.m_size; ++i) {
        m_memberData.m_data[i].m_values->setSize(n);
    }
}

hkDataArrayImpl* StructArrayImplementation::swizzleObjectMember(const char* name) const {
    return m_memberData.m_data[_find(name)].m_values;
}

int32_t StructArrayImplementation::_find(const char* name) const {
    char* v5;
    hkDataWorldDict::ObjectTracker* m_tracker = m_class->m_world->m_tracker;
    if (name) {
        hkStringMap<char*, hkContainerHeapAllocator>* p_m_interns = &m_tracker->m_interns;
        v5 = m_tracker->m_interns.getWithDefault(name, 0);
        if (!v5) {
            v5 = hkString::strDup(name);
            p_m_interns->insert(v5, v5);
        }
    }
    else {
        v5 = nullptr;
    }
    int32_t m_size = m_memberData.m_size;
    int32_t result = 0;
    if (m_size <= 0) {
        return -1;
    }
    for (StructArrayImplementation::MemberData* i = m_memberData.m_data; i->m_memberName != v5; ++i) {
        if (++result >= m_size) {
            return -1;
        }
    }
    return result;
}

// Drops the member called name (interned): its values lose the array's reference and the later members move down.
void StructArrayImplementation::removeMember(InternedString s) {
    int32_t iSize = m_memberData.m_size;
    for (int32_t i = 0; i < iSize; ++i) {
        MemberData* pMember = &m_memberData.m_data[i];
        if (pMember->m_memberName == s.getString()) {
            hkDataArrayImpl* pValues = pMember->m_values;
            if (--pValues->m_count == 0) {
                delete pValues;
            }
            --m_memberData.m_size;
            // EXE-INLINED none: hkArray<MemberData>::removeAtAndCopy (a forward dword copy)
            int32_t iNewSize = m_memberData.m_size;
            MemberData* pData = m_memberData.m_data;
            for (int32_t j = i; j < iNewSize; ++j) {
                pData[j] = pData[j + 1];
            }
            return;
        }
    }
}

int32_t StructArrayImplementation::_addMember(const hkDataClassDict::MemberInfo& a1) {
    hkDataClassDict* v12;
    if (a1.m_type == hkDataObject::TYPE_VOID) {
        return -1;
    }
    hkArray<StructArrayImplementation::MemberData, hkContainerHeapAllocator>* p_m_memberData = &m_memberData;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (m_memberData.m_size ==
        (m_memberData.m_capacityAndFlags & hkArrayBase<StructArrayImplementation::MemberData>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, &m_memberData, sizeof(StructArrayImplementation::MemberData));
    }
    StructArrayImplementation::MemberData* m_data = p_m_memberData->begin();
    StructArrayImplementation::MemberData* v7 = &m_memberData.m_data[m_memberData.m_size];
    if (v7) {
        v7->m_memberName = nullptr;
    }
    int32_t m_size = m_memberData.m_size;
    StructArrayImplementation::MemberData* v9 = &p_m_memberData->m_data[m_size];
    m_memberData.m_size = m_size + 1;
    v9->m_memberName = a1.m_name.getString();
    const char* m_cachedString = a1.m_typeName.m_cached.val()->getString();
    hkDataWorldDict::ObjectTracker* m_tracker = m_class->getWorld()->m_tracker;
    if (m_cachedString) {
        v12 = m_tracker->m_classes.getWithDefault(m_cachedString, 0);
    }
    else {
        v12 = nullptr;
    }
    hkDataArrayImpl* Array = hkDataWorldDict::createArray(a1.m_type, v12, a1.m_tupleCount);
    v9->m_values = Array;
    ++Array->m_count;
    return m_memberData.m_size - 1;
}

int32_t StructArrayImplementation::_findOrAdd(const char* src) {
    char* v5;
    hkDataWorldDict::ObjectTracker* m_tracker = m_class->m_world->m_tracker;
    if (src) {
        hkStringMap<char*, hkContainerHeapAllocator>* p_m_interns = &m_tracker->m_interns;
        v5 = m_tracker->m_interns.getWithDefault(src, 0);
        if (!v5) {
            v5 = hkString::strDup(src);
            p_m_interns->insert(v5, v5);
        }
    }
    else {
        v5 = nullptr;
    }
    int32_t m_size = m_memberData.m_size;
    int32_t result = 0;
    if (m_size <= 0) {
        return _addMemberByName(v5);
    }
    for (StructArrayImplementation::MemberData* i = m_memberData.begin(); i->m_memberName != v5; ++i) {
        if (++result >= m_size) {
            return _addMemberByName(v5);
        }
    }
    if (result == -1) {
        return _addMemberByName(v5);
    }
    return result;
}

// Adds the member of m_class named a1 and sizes its new value array to this array's size. Returns _addMember's index,
// or -1 when it adds none. The member's info is found by index through the class's parents; past the last parent, the
// assert aside, it is the slot before the class's own first member.
int32_t StructArrayImplementation::_addMemberByName(char* a1) {
    int32_t index = m_class->getMemberIndexByName(a1);
    hkDataClassDict* klass = m_class;
    index -= klass->getNumMembers();
    const hkDataClassDict::MemberInfo* info;
    const hkDataClassDict* current = klass;
    for (;;) {
        index += current->m_memberInfo.m_size;
        if (index >= 0) {
            info = &current->m_memberInfo.m_data[index];
            break;
        }
        current = current->m_parent;
        if (!current) {
            info = &klass->m_memberInfo.m_data[-1];
            break;
        }
    }
    int32_t added = _addMember(*info);
    if (added != -1) {
        m_memberData.m_data[added].m_values->setSize(getSize());
    }
    return added;
}

// The members' values are released, then the class's reference; the member array frees itself.
hkDataObjectDict::~hkDataObjectDict() {
    selfDestruct();
    if ((m_class) && (m_class->m_count-- == 1)) {
        delete m_class;
    }
}

// The object and a null second word.
hkDataObject::Handle hkDataObjectDict::getHandle() const {
    hkDataObject::Handle kHandle;
    kHandle.p0 = const_cast<hkDataObjectDict*>(this);
    kHandle.p1 = nullptr;
    return kHandle;
}

hkDataObject::Value hkDataObjectDict::accessByName(const char* name) {
    hkDataObject::Value result;
    hkDataWorldDict::ObjectTracker* m_tracker = m_class->m_world->m_tracker;
    if (name) {
        hkStringMap<char*, hkContainerHeapAllocator>* p_m_interns = &m_tracker->m_interns;
        char* v6 = m_tracker->m_interns.getWithDefault(name, 0);
        if (!v6) {
            v6 = hkString::strDup(name);
            p_m_interns->insert(v6, v6);
        }
        result.m_name = v6;
        result.m_impl = this;
        return result;
    }

    result.m_name = nullptr;
    result.m_impl = this;
    return result;
}

// Whether the object has a member called name. Member names are interned, so they compare by pointer.
int32_t hkDataObjectDict::hasMember(const char* name) const {
    InternedString pName = m_class->m_world->m_tracker->intern(name);
    for (int32_t i = 0; i < m_memberData.m_size; ++i) {
        if (m_memberData.m_data[i].m_memberName == pName) {
            return 1;
        }
    }
    return 0;
}

// A signed compare with the member count.
int32_t hkDataObjectDict::isValid(int32_t it) const {
    return it < (int32_t)m_memberData.m_size;
}

int32_t hkDataObjectDict::getNextMember(int32_t it) const {
    return it + 1;
}

const char* hkDataObjectDict::getMemberName(int32_t it) const {
    return m_memberData.m_data[it].m_memberName.getString();
}

const hkDataObject::Value hkDataObjectDict::getMemberValue(int32_t it) const {
    hkDataObject::Value result;
    // The source's const_cast: a Value refers to its object through a non-const hkDataObjectImpl*.
    result.m_impl = const_cast<hkDataObjectDict*>(this);
    result.m_name = m_memberData.m_data[it].m_memberName.getString();
    return result;
}

hkDataArrayImpl* hkDataObjectDict::asArray(const char* nameIn) {
    hkDataObjectDict::MemberAccess access;
    _readAccess(access, nameIn, hkDataObject::TYPE_ARRAY);
    return m_memberData.m_data[access.index].m_value.a;
}

const char* hkDataObjectDict::asString(const char* nameIn) {
    hkDataObjectDict::MemberAccess access;
    _readAccess(access, nameIn, hkDataObject::TYPE_CSTRING);
    return m_memberData.m_data[access.index].m_value.s;
}

// The member's value when its type is a byte or an integer, else 0.
int64_t hkDataObjectDict::asInt(const char* nameIn) {
    MemberAccess kAccess;
    _readAccess(kAccess, nameIn, hkDataObject::TYPE_VOID);
    if (static_cast<uint32_t>(kAccess.info->m_type) - 1u > 1u) {
        return 0;
    }
    return m_memberData.m_data[kAccess.index].m_value.i;
}

hkDataObjectImpl* hkDataObjectDict::asObject(const char* nameIn) {
    hkDataObjectDict::MemberAccess access;
    _readAccess(access, nameIn, hkDataObject::TYPE_OBJECT);
    return m_memberData.m_data[access.index].m_value.o;
}

// Returns the pointer a vector member's value holds, read with the vector type for nreal reals.
const float* hkDataObjectDict::asVec(const char* nameIn, int32_t nreal) {
    MemberAccess kAccess;
    _readAccess(kAccess, nameIn, getVecTypeFromNumReals(nreal));
    // The vector's reals: only the pointer is loaded.
    return m_memberData.m_data[kAccess.index].m_value.ra;
}

float hkDataObjectDict::asReal(const char* nameIn) {
    hkDataObjectDict::MemberAccess access;
    _readAccess(access, nameIn, hkDataObject::TYPE_REAL);
    return m_memberData.m_data[access.index].m_value.r;
}

// The source member's 8 bytes move into this object's member of the handle. An object or an array value first
// takes a reference, and the member's type comes from the source's description, read before the write lookup
// overwrites it.
void hkDataObjectDict::assign(const char* handle, const hkDataObject::Value& value) {
    // The source can be any implementation, but the access lookup is the dictionary's.
    hkDataObjectDict* pSource = static_cast<hkDataObjectDict*>(value.m_impl);
    MemberAccess kAccess;
    pSource->_readAccess(kAccess, value.m_name, hkDataObject::TYPE_VOID);
    int32_t iSource = kAccess.index;
    int64_t iSourceValue = pSource->m_memberData.m_data[iSource].m_value.i;
    if ((kAccess.info->m_type & (hkDataObject::TYPE_ARRAY | hkDataObject::TYPE_OBJECT)) != 0) {
        // A member's value is an untyped word: its low half is taken as the object or array pointer and its reference
        // count is incremented.
        hkDataRefCounted* pReferenced = reinterpret_cast<hkDataRefCounted*>(static_cast<intptr_t>(iSourceValue));
        if (pReferenced) {
            ++pReferenced->m_count;
        }
    }
    _writeAccess(kAccess, handle, kAccess.info->m_type);
    m_memberData.m_data[kAccess.index].m_value = pSource->m_memberData.m_data[iSource].m_value;
}

void hkDataObjectDict::assign(const char* handle, hkDataObjectImpl* value) {
    hkDataObjectDict::MemberAccess access;
    if (value) {
        ++value->m_count;
    }
    _writeAccess(access, handle, hkDataObject::TYPE_OBJECT);
    m_memberData.m_data[access.index].m_value.p = value;
}

void hkDataObjectDict::assign(const char* handle, hkDataArrayImpl* value) {
    hkDataObjectDict::MemberAccess access;
    ++value->m_count;
    hkDataObject::Type v5 = static_cast<hkDataObject::Type>(value->getType() | hkDataObject::TYPE_ARRAY);
    _writeAccess(access, InternedString(handle), v5);
    m_memberData.m_data[access.index].m_value.p = (void*)value;
}

void hkDataObjectDict::assign(const char* handle, const char* value) {
    char* v4;
    hkDataObjectDict::MemberAccess access;
    _writeAccess(access, handle, hkDataObject::TYPE_CSTRING);
    if (value) {
        v4 = hkString::strDup(value);
    }
    else {
        v4 = nullptr;
    }
    m_memberData.m_data[access.index].m_value.p = v4;
}

void hkDataObjectDict::assign(const char* handle, float r) {
    hkDataObjectDict::MemberAccess access;
    _writeAccess(access, handle, hkDataObject::TYPE_REAL);
    m_memberData.m_data[access.index].m_value.r = r;
}

void hkDataObjectDict::assign(const char* handle, hkHalf r) {
    hkDataObjectDict::MemberAccess access;
    _writeAccess(access, handle, hkDataObject::TYPE_REAL);
    m_memberData.m_data[access.index].m_value.r = std::bit_cast<float>((r.m_value << 16));
}

// A copy of the nreal reals becomes the member's value: the member's type from getVecTypeFromNumReals, then a block of
// 4*nreal bytes copied into it.
void hkDataObjectDict::assign(const char* handle, const float* r, int32_t nreal) {
    MemberAccess kAccess;
    _writeAccess(kAccess, handle, getVecTypeFromNumReals(nreal));
    hkMemoryAllocator& kAlloc = hkMemoryRouter::getInstance().heap();
    void* pBlock = kAlloc.blockAlloc(sizeof(float) * nreal);
    hkString::memCpy(pBlock, r, sizeof(float) * nreal);
    // The vector's reals: only the pointer is stored.
    m_memberData.m_data[kAccess.index].m_value.ra = static_cast<float*>(pBlock);
}

void hkDataObjectDict::assign(const char* handle, int64_t valueIn) {
    hkDataObjectDict::MemberAccess access;
    _writeAccess(access, handle, hkDataObject::TYPE_VOID);
    hkDataObject::Type m_type = access.info->m_type;
    hkDataObjectDict::MemberData* v5 = &m_memberData.m_data[access.index];
    if (m_type > hkDataObject::TYPE_VOID) {
        if (m_type <= hkDataObject::TYPE_INT) {
            v5->m_value.i = valueIn;
        }
        else {
            if (m_type == hkDataObject::TYPE_REAL) {
                v5->m_value.r = static_cast<float>(valueIn);
            }
        }
    }
}

// Releases every member's value by the class's member type and empties m_memberData.
void hkDataObjectDict::selfDestruct() {
    int32_t v2 = 0;
    if (m_memberData.m_size > 0) {
        int32_t v3 = 0;
        do {
            hkDataObjectDict::MemberData* v4 = &m_memberData.m_data[v3];
            const hkDataClassDict::MemberInfo* MemberInfoByName = m_class->getMemberInfoByName(v4->m_memberName);
            if (MemberInfoByName) {
                hkDataObjectDict::releaseValue(MemberInfoByName->m_type, &v4->m_value, 1);
            }
            ++v2;
            ++v3;
        }
        while (v2 < m_memberData.m_size);
    }
    m_memberData.m_size = 0;
}

// The search reads the array once; the removal is inline (the last member moves into the gap) and reloads it.
void hkDataObjectDict::removeMember(InternedString name) {
    // m_size is unsigned, but it is compared signed.
    int32_t iSize = m_memberData.m_size;
    MemberData* pData = m_memberData.m_data;
    for (int32_t i = 0; i < iSize; ++i) {
        if (pData[i].m_memberName == name.getString()) {
            const hkDataClassDict::MemberInfo* pInfo = m_class->getMemberInfoByName(pData[i].m_memberName);
            if (pInfo) {
                releaseValue(pInfo->m_type, &pData[i].m_value, 1);
            }
            --m_memberData.m_size;
            int32_t iLast = m_memberData.m_size;
            if (iLast != i) {
                m_memberData.m_data[i] = m_memberData.m_data[iLast];
            }
            return;
        }
    }
}

// The member handle names and its slot (the last one of that name), for reading it: a member the object lacks is
// appended, its value made by the class's member type (an empty array, a tuple array of the tuple count, or zero).
// type is not used.
void hkDataObjectDict::_readAccess(MemberAccess& access, InternedString handle, hkDataObject::Type type) {
    const hkDataClassDict::MemberInfo* pInfo = m_class->getMemberInfoByName(handle);
    if (!pInfo) {
        access.info = nullptr;
        access.index = -1;
        return;
    }
    int32_t iSize = m_memberData.m_size;
    int32_t iIndex = -1;
    if (iSize > 0) {
        MemberData* pData = m_memberData.m_data;
        int32_t i = 0;
        do {
            if (pData->m_memberName == handle) {
                iIndex = i;
            }
            ++i;
            ++pData;
        }
        while (i < iSize);
    }
    if (iIndex == -1) {
        iIndex = iSize;
        hkMemoryRouter& kRouter = hkMemoryRouter::getInstance();
        // EXE-INLINED none: hkArray<MemberData>::expandOne
        if (m_memberData.m_size == (m_memberData.m_capacityAndFlags & hkArrayBase<MemberData>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*kRouter.m_heap, &m_memberData, sizeof(MemberData));
        }
        MemberData* pNew = &m_memberData.m_data[m_memberData.m_size];
        if (pNew) {
            pNew->m_memberName.m_cachedString = nullptr;
        }
        MemberData* pMember = &m_memberData.m_data[m_memberData.m_size++];
        pMember->m_memberName = handle;
        int32_t iType = pInfo->m_type;
        if (iType & hkDataObject::TYPE_ARRAY) {
            hkDataClassImpl* pClass = m_class->m_world->findClass(pInfo->m_typeName.m_cached.val()->getString());
            hkDataArrayImpl* pArray = hkDataWorldDict::createArray(
                static_cast<hkDataObject::Type>(iType & ~hkDataObject::TYPE_ARRAY), pClass, 0
            );
            pMember->m_value.a = pArray;
            ++pArray->m_count;
        }
        else if (iType & hkDataObject::TYPE_TUPLE) {
            hkDataClassImpl* pClass = m_class->m_world->findClass(pInfo->m_typeName.m_cached.val()->getString());
            hkDataArrayImpl* pArray = hkDataWorldDict::createArray(
                static_cast<hkDataObject::Type>(iType & ~hkDataObject::TYPE_TUPLE), pClass, 0
            );
            pMember->m_value.a = pArray;
            pArray->setSize(pInfo->m_tupleCount);
            ++pMember->m_value.a->m_count;
        }
        else if (iType == hkDataObject::TYPE_OBJECT) {
            pMember->m_value.o = nullptr;
        }
        else {
            pMember->m_value.i = 0;
        }
    }
    access.info = pInfo;
    access.index = iIndex;
}

void hkDataObjectDict::_writeAccess(
    hkDataObjectDict::MemberAccess& access,
    InternedString handle,
    hkDataObject::Type type
) {
    const hkDataClassDict::MemberInfo* MemberInfoByName = m_class->getMemberInfoByName(handle);
    if (MemberInfoByName) {
        int32_t m_size = m_memberData.m_size;
        int32_t v7 = 0;
        if (m_size <= 0) {
        LABEL_6:;
            hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
            if (m_memberData.m_size ==
                (m_memberData.m_capacityAndFlags & hkArrayBase<hkDataObjectDict::MemberData>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(*Instance->m_heap, &m_memberData, sizeof(hkDataObjectDict::MemberData));
            }
            hkDataObjectDict::MemberData* v10 = &m_memberData.m_data[m_memberData.m_size];
            if (v10) {
                v10->m_memberName.m_cachedString = nullptr;
            }
            InternedString* p_m_memberName = &m_memberData.m_data[m_memberData.m_size++].m_memberName;
            p_m_memberName->m_cachedString = handle.getString();
            p_m_memberName[2].m_cachedString = nullptr;
            p_m_memberName[3].m_cachedString = nullptr;
            access.index = m_size;
            access.info = MemberInfoByName;
        }
        else {
            hkDataObjectDict::MemberData* m_data = m_memberData.begin();
            while (m_data->m_memberName.getString() != handle.getString()) {
                ++v7;
                ++m_data;
                if (v7 >= m_size) {
                    goto LABEL_6;
                }
            }
            hkDataObjectDict::releaseValue(MemberInfoByName->m_type, &m_memberData.m_data[v7].m_value, 1);
            access.index = v7;
            access.info = MemberInfoByName;
        }
    }
    else {
        access.info = nullptr;
        access.index = -1;
    }
}

// Releases count values of a member of type at value: an array's or tuple's reference (one value; its pointer stays),
// else each vector's block of reals, each object's reference or each string, clearing its pointer.
void hkDataObjectDict::releaseValue(hkDataObject::Type type, Union* value, int32_t count) {
    if (type & (hkDataObject::TYPE_ARRAY | hkDataObject::TYPE_TUPLE)) {
        hkDataArrayImpl* pArray = value->a;
        if ((pArray) && (--pArray->m_count == 0)) {
            delete pArray;
        }

        return;
    }
    switch (type) {
        case hkDataObject::TYPE_VEC_4:
        case hkDataObject::TYPE_VEC_12:
        case hkDataObject::TYPE_VEC_16:
            for (int32_t i = 0; i < count; ++i) {
                float* pReals = value[i].ra;
                if (pReals) {
                    hkMemoryAllocator& kAlloc = hkMemoryRouter::getInstance().heap();
                    // type - 3 vectors of four reals.
                    kAlloc.blockFree(pReals, static_cast<int32_t>(4 * sizeof(float)) * (type - 3));
                    value[i].ra = nullptr;
                }
            }
            break;
        case hkDataObject::TYPE_OBJECT:
        case hkDataObject::TYPE_STRUCT:
            for (int32_t i = 0; i < count; ++i) {
                hkDataObjectImpl* pObject = value[i].o;
                if (pObject) {
                    if (--pObject->m_count == 0) {
                        delete pObject;
                    }
                    value[i].o = nullptr;
                }
            }
            break;
        case hkDataObject::TYPE_CSTRING:
            for (int32_t i = 0; i < count; ++i) {
                hkString::strFree(value[i].s);
                value[i].s = nullptr;
            }
            break;
        default: break;
    }
}

// Element index set from the value of another object's member: a tuple array's element through setArray, else
// through the setter of the array's element type (the vectors with 4 * type - 12 reals).
void hkDataArrayImpl::set(int32_t index, hkDataObject::Value val) {
    if (getType() & hkDataObject::TYPE_TUPLE) {
        hkDataArray array;
        setArray(index, val.asArray(&array)->m_impl);
        return;
    }
    switch (getType()) {
        case hkDataObject::TYPE_BYTE:
        case hkDataObject::TYPE_INT: setInt(index, val.asInt()); break;
        case hkDataObject::TYPE_REAL: setReal(index, val.asReal()); break;
        case hkDataObject::TYPE_VEC_4:
        case hkDataObject::TYPE_VEC_8:
        case hkDataObject::TYPE_VEC_12:
        case hkDataObject::TYPE_VEC_16: setVec(index, val.asVec(4 * static_cast<int32_t>(getType()) - 12)); break;
        case hkDataObject::TYPE_OBJECT:
        case hkDataObject::TYPE_STRUCT: {
            hkDataObject object;
            setObject(index, val.asObject(&object)->m_impl);
            break;
        }
        case hkDataObject::TYPE_CSTRING: setString(index, val.asString()); break;
        default: break;
    }
}
