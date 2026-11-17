#include "Havok/Source/Common/Serialize/Version/hkVersionPatchManager.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/String/hkStringBuf.h"
#include "Havok/Source/Common/Base/Container/StringMap/hkCachedHashMap.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/hkMemUtil.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/Registry/hkClassNameRegistry.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Serialize/Data/Dict/hkDataObjectDict.h"
#include "Havok/Source/Common/Serialize/Data/hkDataObject.h"
#include "Havok/Source/Common/Serialize/Data/hkDataObjectImpl.h"
#include "Havok/Source/Common/Serialize/Data/hkDataWorld.h"
#include "Havok/Source/Common/Serialize/Util/hkBuiltinTypeRegistry.h"
#include "Havok/Source/Common/Serialize/Util/hkSerializeMultiMap.h"

// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
static int32_t walkDependencies(
    int32_t a1,
    hkArray<int32_t, hkContainerHeapAllocator>& a2,
    const hkSerializeMultiMap<int32_t, int32_t>& a3,
    int32_t a4,
    hkArray<int32_t, hkContainerHeapAllocator>& pending,
    const hkArray<const hkVersionPatchManager::PatchInfo*, hkContainerHeapAllocator>& a6
);
static hkResult applyPatchesToWorld(hkArrayBase<const hkVersionPatchManager::PatchInfo*>* a1, hkDataWorldDict* r_esi);
static int32_t getVersion(int32_t a1, int32_t a2);
// Whether p is a valid patch (addPatch's test).
static int32_t isValidPatch(const hkVersionPatchManager::PatchInfo* p);
// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
static void setAllDependeciesToDo(
    HeapArray<UidItem>& a1,
    const char* classname,
    int32_t version,
    const hkDataWorldDict& a4,
    const hkVersionPatchManager& a5,
    const hkPointerMap<uint64_t, int32_t, hkContainerHeapAllocator>& a6,
    const hkPointerMap<uint64_t, int32_t, hkContainerHeapAllocator>& a7
);

// m_value is untyped storage: populate and depopulate move any singleton's instance pointer through it.
static hkSingletonInitNode hkSingletonRegisterhkVersionPatchManager(
    "hkVersionPatchManager",
    &hkVersionPatchManagercreate,
    reinterpret_cast<void**>(&hkVersionPatchManager::s_instance)
);

// Orders a1's dependencies depth first: a2 holds -1 for unvisited patches, -2 while on the pending stack, else the
// patch's order; a4 is the next order. A dependency found on the stack is a cycle: its report is compiled out but for
// its string buffer.
static int32_t walkDependencies(
    int32_t a1,
    hkArray<int32_t, hkContainerHeapAllocator>& a2,
    const hkSerializeMultiMap<int32_t, int32_t>& a3,
    int32_t a4,
    hkArray<int32_t, hkContainerHeapAllocator>& pending,
    const hkArray<const hkVersionPatchManager::PatchInfo*, hkContainerHeapAllocator>& a6
) {
    if (a2.m_data[a1] != -1) {
        return a4;
    }
    a2.m_data[a1] = -2;
    // EXE-INLINED none: hkArray<int, hkContainerHeapAllocator>::pushBack
    hkMemoryRouter& router = hkMemoryRouter::getInstance();
    if (pending.m_size == (pending.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*router.m_heap, &pending, sizeof(int32_t));
    }
    pending.m_data[pending.m_size++] = a1;
    for (int32_t i = a3.find(a1); i != -1; i = a3.m_valueChain.m_data[i].next) {
        int32_t value = a3.m_valueChain.m_data[i].value;
        int32_t state = a2.m_data[value];
        if (state == -2) {
            // EXE-INLINED none: hkArray<int, hkContainerHeapAllocator>::pushBack
            hkMemoryRouter& cycleRouter = hkMemoryRouter::getInstance();
            if (pending.m_size == (pending.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(*cycleRouter.m_heap, &pending, sizeof(int32_t));
            }
            pending.m_data[pending.m_size++] = value;
            FNVD_INLINED_LOCAL(hkStringBuf, kMessage, std::nullopt, std::nullopt);
            ::new (&kMessage) hkStringBuf;
            --pending.m_size;
            // EXE-INLINED none: hkStringBuf::~hkStringBuf
            hkMemoryAllocator* pTemp = hkMemoryRouter::getInstance().m_temp;
            kMessage.m_string.m_size = 0;
            if (kMessage.m_string.m_capacityAndFlags >= 0) {
                pTemp->bufFree(
                    kMessage.m_string.m_data,
                    sizeof(char) * (kMessage.m_string.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK)
                );
            }
        }
        else if (state == -1) {
            a4 = walkDependencies(value, a2, a3, a4, pending, a6);
        }
    }
    --pending.m_size;
    a2.m_data[a1] = a4;
    return a4 + 1;
}

// Applies patches, in order, to world: a patch from no version first creates its class, failing when world has it
// already; then the class (of the patch's old name, else its new one) is renamed, its components applied (members
// added, removed and renamed, a function run on each object of the class and of the classes derived from it, each
// object of exactly the class cast, the parent set; a dependency does nothing), and the class removed for a patch to no
// version or else given the new version. hkDataClass's destructor is an empty function.
static hkResult
applyPatchesToWorld(hkArrayBase<const hkVersionPatchManager::PatchInfo*>* patches, hkDataWorldDict* world) {
    using ObjectArray = hkArray<hkDataObjectImpl*, hkContainerHeapAllocator>;
    const hkDataWorld* pFindWorld = world;
    hkDataWorld* pNewWorld = world;
    for (int32_t i = 0; i < patches->m_size; ++i) {
        const hkVersionPatchManager::PatchInfo* pPatch = patches->m_data[i];
        const char* pClassName = pPatch->oldName ? pPatch->oldName : pPatch->newName;
        if (pPatch->oldVersion == -1) {
            FNVD_INLINED_LOCAL(hkDataClass::Cinfo, cinfo, std::nullopt, std::nullopt);
            cinfo.version = pPatch->newVersion;
            // EXE-INLINED none: hkArray<hkDataClass::Cinfo::Member>'s constructor
            cinfo.members.m_data = nullptr;
            cinfo.members.m_size = 0;
            cinfo.members.m_capacityAndFlags = hkArrayBase<hkDataClass::Cinfo::Member>::DONT_DEALLOCATE_FLAG;
            cinfo.name = pClassName;
            cinfo.parent = nullptr;
            bool bExists = pFindWorld->findClass(pClassName) != nullptr;
            if (!bExists) {
                pNewWorld->newClass(cinfo);
            }
            // EXE-INLINED 0xDBA490
            hkMemoryAllocator& rHeap = hkMemoryRouter::getInstance().heap();
            int32_t iFlags = cinfo.members.m_capacityAndFlags;
            cinfo.members.m_size = 0;
            if (!(iFlags & hkArrayBase<hkDataClass::Cinfo::Member>::DONT_DEALLOCATE_FLAG)) {
                rHeap.bufFree(
                    cinfo.members.m_data,
                    (iFlags & hkArrayBase<hkDataClass::Cinfo::Member>::CAPACITY_MASK) *
                        static_cast<int32_t>(sizeof(hkDataClass::Cinfo::Member))
                );
            }
            if (bExists) {
                return HK_FAILURE;
            }
        }
        hkDataClass klass(pFindWorld->findClass(pClassName));
        if (pPatch->oldName && pPatch->newName && hkString::strCmp(pPatch->oldName, pPatch->newName)) {
            world->renameClass(klass, pPatch->newName);
        }
        for (int32_t c = 0; c < pPatch->numComponent; ++c) {
            const hkVersionPatchManager::PatchInfo::Component& rComponent = pPatch->component[c];
            switch (rComponent.type) {
                case hkVersionPatchManager::PATCH_MEMBER_ADDED: {
                    const hkVersionPatchManager::MemberAddedPatch* pAdded =
                        static_cast<const hkVersionPatchManager::MemberAddedPatch*>(rComponent.patch);
                    world->addClassMember(klass, pAdded->name, pAdded->type, pAdded->typeName, pAdded->tuples);
                    break;
                }
                case hkVersionPatchManager::PATCH_MEMBER_REMOVED: {
                    const hkVersionPatchManager::MemberRemovedPatch* pRemoved =
                        static_cast<const hkVersionPatchManager::MemberRemovedPatch*>(rComponent.patch);
                    world->removeClassMember(klass, pRemoved->name);
                    break;
                }
                case hkVersionPatchManager::PATCH_MEMBER_RENAMED: {
                    const hkVersionPatchManager::MemberRenamedPatch* pRenamed =
                        static_cast<const hkVersionPatchManager::MemberRenamedPatch*>(rComponent.patch);
                    world->renameClassMember(klass, pRenamed->oldName, pRenamed->newName);
                    break;
                }
                case hkVersionPatchManager::PATCH_FUNCTION: {
                    hkVersionPatchManager::Function pFunction =
                        static_cast<const hkVersionPatchManager::FunctionPatch*>(rComponent.patch)->function;
                    FNVD_INLINED_LOCAL(ObjectArray, objects, std::nullopt, std::nullopt);
                    // EXE-INLINED none: hkArray<hkDataObjectImpl*, hkContainerHeapAllocator>'s constructor
                    objects.m_data = nullptr;
                    objects.m_size = 0;
                    objects.m_capacityAndFlags = hkArrayBase<hkDataObjectImpl*>::DONT_DEALLOCATE_FLAG;
                    world->findObjectsByBaseClass(pClassName, objects);
                    for (int32_t j = 0; j < objects.m_size; ++j) {
                        hkDataObject object(objects.m_data[j]);
                        pFunction(object);
                    }
                    // EXE-INLINED none: hkArray<hkDataObjectImpl*, hkContainerHeapAllocator>::~hkArray
                    hkMemoryAllocator& rHeap = hkMemoryRouter::getInstance().heap();
                    int32_t iFlags = objects.m_capacityAndFlags;
                    objects.m_size = 0;
                    if (!(iFlags & hkArrayBase<hkDataObjectImpl*>::DONT_DEALLOCATE_FLAG)) {
                        rHeap.bufFree(
                            objects.m_data,
                            (iFlags & hkArrayBase<hkDataObjectImpl*>::CAPACITY_MASK) *
                                static_cast<int32_t>(sizeof(hkDataObjectImpl*))
                        );
                    }
                    objects.m_data = nullptr;
                    objects.m_capacityAndFlags = hkArrayBase<hkDataObjectImpl*>::DONT_DEALLOCATE_FLAG;
                    break;
                }
                case hkVersionPatchManager::PATCH_CAST: {
                    const hkVersionPatchManager::CastPatch* pCast =
                        static_cast<const hkVersionPatchManager::CastPatch*>(rComponent.patch);
                    hkDataClass castClass(pFindWorld->findClass(pCast->name));
                    FNVD_INLINED_LOCAL(ObjectArray, objects, std::nullopt, std::nullopt);
                    // EXE-INLINED none: hkArray<hkDataObjectImpl*, hkContainerHeapAllocator>'s constructor
                    objects.m_data = nullptr;
                    objects.m_size = 0;
                    objects.m_capacityAndFlags = hkArrayBase<hkDataObjectImpl*>::DONT_DEALLOCATE_FLAG;
                    world->findObjectsByExactClass(pClassName, objects);
                    for (int32_t j = 0; j < objects.m_size; ++j) {
                        hkDataObject object(objects.m_data[j]);
                        world->castObject(object, castClass);
                    }
                    // EXE-INLINED none: hkArray<hkDataObjectImpl*, hkContainerHeapAllocator>::~hkArray
                    hkMemoryAllocator& rHeap = hkMemoryRouter::getInstance().heap();
                    int32_t iFlags = objects.m_capacityAndFlags;
                    objects.m_size = 0;
                    if (!(iFlags & hkArrayBase<hkDataObjectImpl*>::DONT_DEALLOCATE_FLAG)) {
                        rHeap.bufFree(
                            objects.m_data,
                            (iFlags & hkArrayBase<hkDataObjectImpl*>::CAPACITY_MASK) *
                                static_cast<int32_t>(sizeof(hkDataObjectImpl*))
                        );
                    }
                    objects.m_data = nullptr;
                    objects.m_capacityAndFlags = hkArrayBase<hkDataObjectImpl*>::DONT_DEALLOCATE_FLAG;
                    break;
                }
                case hkVersionPatchManager::PATCH_PARENT_SET: {
                    const char* pParentName =
                        static_cast<const hkVersionPatchManager::SetParentPatch*>(rComponent.patch)->newParent;
                    hkDataClassImpl* pParentImpl = pParentName ? pFindWorld->findClass(pParentName) : nullptr;
                    hkDataClass parent(pParentImpl);
                    world->setClassParent(klass, parent);
                    break;
                }
                default: break;
            }
        }
        int32_t iNewVersion = pPatch->newVersion;
        if (iNewVersion == -1) {
            world->removeClass(klass);
        }
        else if (pPatch->oldVersion != -1 && pPatch->oldVersion != iNewVersion) {
            world->setClassVersion(klass, iNewVersion);
        }
    }
    return HK_SUCCESS;
}

static int32_t getVersion(int32_t a1, int32_t a2) {
    return a2;
}

// KEEP: preparePatches reads edx right after its call (0xDB96DC, `cmp [edx+8], esi`), which the exe's body leaves
// intact and clang does not keep.
// Whether p is a valid patch: one that adds a class (no old version) names only the new class, one that removes a
// class (no new version) names only the old class, and any other names the old class.
static int32_t isValidPatch(const hkVersionPatchManager::PatchInfo* p) {
    if (p->oldVersion == -1) {
        return !p->oldName && (p->newVersion != -1 && p->newName);
    }
    if (p->newVersion == -1) {
        if (!p->oldName) {
            return 0;
        }
        return !p->newName;
    }
    return p->oldName != nullptr;
}

// KEEP (partcopy:): the queued UidItem's last four bytes (+0xC to +0xF, the struct's padding) are never written, and
// HeapArray<UidItem>::insert copies all 16 bytes of it into the array.
// Queues the patch that brings class name at version ver up to date (unless its state in a7 is done, 2), then the
// classes its PATCH_DEPENDS components name, last component first. A class the world has at ver takes its patch from
// a6 only when its state is 1; otherwise from the manager's index.
static void setAllDependeciesToDo(
    HeapArray<UidItem>& a1,
    const char* classname,
    int32_t version,
    const hkDataWorldDict& a4,
    const hkVersionPatchManager& a5,
    const hkPointerMap<uint64_t, int32_t, hkContainerHeapAllocator>& a6,
    const hkPointerMap<uint64_t, int32_t, hkContainerHeapAllocator>& a7
) {
    uint64_t uid = a5.getUid(classname, version);
    hkDataClassImpl* pClass = a4.findClass(classname);
    int32_t patchIndex = -1;
    if (!pClass || pClass->getVersion() != version) {
        patchIndex = a6.getWithDefault(uid, -1);
    }
    int32_t state = a7.getWithDefault(uid, 0);
    if (patchIndex == -1 || state == 1) {
        patchIndex = a5.m_patchIndexFromUid.getWithDefault(uid, -1);
    }
    UidItem item;
    item.uid = uid;
    item.patchIndex = patchIndex;
    if (state != 2 && !a1.hasElement(item)) {
        a1.insert(item);
    }
    if (patchIndex >= 0) {
        const hkVersionPatchManager::PatchInfo* pPatch = a5.m_patchInfos.m_data[patchIndex];
        for (int32_t i = pPatch->numComponent - 1; i >= 0; --i) {
            if (pPatch->component[i].type == hkVersionPatchManager::PATCH_DEPENDS) {
                const hkVersionPatchManager::DependsPatch* pDepends =
                    static_cast<const hkVersionPatchManager::DependsPatch*>(pPatch->component[i].patch);
                setAllDependeciesToDo(a1, pDepends->name, pDepends->version, a4, a5, a6, a7);
            }
        }
    }
}

namespace {
    // A class's name and version, as the world to update holds it.
    struct ClassVersion {
        const char* name;
        int32_t version;
    };
} // namespace

// Inserts e before the first element with a greater patch index, else at the end.
template <>
void HeapArray<UidItem>::insert(const UidItem& e) {
    for (int32_t i = 0; i < m_elems.m_size; ++i) {
        if (e.patchIndex < m_elems.m_data[i].patchIndex) {
            m_elems._insertAt<UidItem>(*hkMemoryRouter::getInstance().m_heap, i, e);
            return;
        }
    }
    hkMemoryRouter& router = hkMemoryRouter::getInstance();
    // EXE-INLINED none: hkArrayBase<UidItem>::_pushBack
    if (m_elems.m_size == (m_elems.m_capacityAndFlags & hkArrayBase<UidItem>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(router.heap(), &m_elems, sizeof(UidItem));
    }
    UidItem* pDst = m_elems.m_data + m_elems.m_size;
    if (pDst) {
        *pDst = e;
    }
    ++m_elems.m_size;
}

template <>
bool HeapArray<UidItem>::hasElement(const UidItem& e) const {
    int32_t m_size = m_elems.m_size;
    int32_t v3 = 0;
    if (m_size <= 0) {
        return false;
    }
    for (UidItem* i = m_elems.m_data; static_cast<uint32_t>(e.uid) != static_cast<uint32_t>(i->uid) ||
         static_cast<uint32_t>(e.uid >> 32) != static_cast<uint32_t>(i->uid >> 32);
         ++i) {
        if (++v3 >= m_size) {
            return false;
        }
    }
    return true;
}

// Whether patchIndex is among the patches current depends on, directly or through their own dependencies; visited
// holds the patches walked so far, each walked once.
int32_t findPatchIndexInDependencies(
    int32_t indexToFind,
    int32_t startIndex,
    const hkSerializeMultiMap<int32_t, int32_t>& incoming,
    hkPointerMap<int32_t, int32_t, hkContainerHeapAllocator>& done
) {
    if (done.m_map.findKey(done.ToMapInt(startIndex)) <= done.m_map.m_hashMod) {
        return 0;
    }
    done.m_map.insert(*hkMemoryRouter::getInstance().m_heap, done.ToMapInt(startIndex), 0);
    for (int32_t i = incoming.find(startIndex); i != -1; i = incoming.m_valueChain.m_data[i].next) {
        int32_t value = incoming.m_valueChain.m_data[i].value;
        if (indexToFind == value || findPatchIndexInDependencies(indexToFind, value, incoming, done)) {
            return 1;
        }
    }
    return 0;
}

// The singleton's create function (`new hkVersionPatchManager()`): allocates through the class allocator, then runs the
// constructor.
hkReferencedObject* hkVersionPatchManagercreate() {
    // EXE-INLINED 0x56D280
    void* pMemory = hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkVersionPatchManager));
    static_cast<hkReferencedObject*>(pMemory)->m_memSizeAndFlags = sizeof(hkVersionPatchManager);
    return ::new (pMemory) hkVersionPatchManager;
}

// Frees the cached copies of the names; the maps and the array free their storage themselves.
hkVersionPatchManager::UidFromClassVersion::~UidFromClassVersion() {
    hkBool valid;
    auto it = m_cachedNames.m_map.getIterator();
    for (m_cachedNames.m_map.isValid(&valid, it); valid.m_bool; m_cachedNames.m_map.isValid(&valid, it)) {
        hkMemoryRouter::easyFree(*hkMemoryRouter::getInstance().m_heap, const_cast<char*>(m_cachedNames.getValue(it)));
        it = m_cachedNames.m_map.getNext(it);
    }
}

// A new name gets the next index and is appended to m_names.
hkUint64 hkVersionPatchManager::UidFromClassVersion::get(const char* name, int32_t ver) {
    const char* cachedName = cache(name);
    int32_t index =
        static_cast<int32_t>(m_indexFromName.m_map.getOrInsert(reinterpret_cast<hkUlong>(cachedName), m_names.m_size));
    if (index == m_names.m_size) {
        hkMemoryRouter& kRouter = hkMemoryRouter::getInstance();
        if (m_names.m_size == (m_names.m_capacityAndFlags & hkArrayBase<const char*>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(
                kRouter.heap(), reinterpret_cast<hkArrayBase<void>*>(&m_names), sizeof(const char*)
            );
        }
        m_names.m_data[m_names.m_size] = cachedName;
        ++m_names.m_size;
    }
    return (static_cast<hkUint64>(ver) << 32) | static_cast<uint32_t>(index);
}

// The SDK's cache: the map's own copy of name, made on its first use; null for null.
const char* hkVersionPatchManager::UidFromClassVersion::cache(const char* name) {
    if (!name) {
        return nullptr;
    }
    const char* cached = m_cachedNames.getWithDefault(name, nullptr);
    if (!cached) {
        cached = hkString::strDup(name);
        m_cachedNames.insert(cached, cached);
    }
    return cached;
}

// The SDK's getName: the name of the uid's low word.
const char* hkVersionPatchManager::UidFromClassVersion::getName(hkUint64 uid) const {
    return m_names.m_data[static_cast<int32_t>(uid)];
}

// The members start empty, then the uid table is allocated on the heap (null when out of memory).
hkVersionPatchManager::hkVersionPatchManager() {
    void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(UidFromClassVersion));
    m_uidFromClassVersion = pMemory ? new (pMemory) UidFromClassVersion() : nullptr;
}

// The uid table's operator delete (the heap's blockFree) is written out.
hkVersionPatchManager::~hkVersionPatchManager() {
    if (m_uidFromClassVersion) {
        m_uidFromClassVersion->~UidFromClassVersion();
        hkMemoryRouter::getInstance().m_heap->blockFree(m_uidFromClassVersion, sizeof(UidFromClassVersion));
    }
}

hkUint64 hkVersionPatchManager::getUid(const char* name, int32_t ver) const {
    return m_uidFromClassVersion->get(name, ver);
}

const hkArray<const hkVersionPatchManager::PatchInfo*, hkContainerHeapAllocator>& hkVersionPatchManager::getPatches() {
    return m_patchInfos;
}

// The SDK's findLastPatchIndexForUid: follows the patches from uid's along each patch's new name and version to the
// last one; a patch that removes the class (new version -1) or, unless allowRenames, renames it ends the walk there.
int32_t hkVersionPatchManager::findLastPatchIndexForUid(uint64_t uid, int32_t allowRenames) const {
    int32_t index = m_patchIndexFromUid.getWithDefault(uid, -1);
    while (index != -1) {
        const PatchInfo* p = m_patchInfos.m_data[index];
        if (p->newVersion == -1) {
            return index;
        }
        if (!allowRenames && p->newName && hkString::strCmp(p->oldName, p->newName)) {
            return index;
        }
        uint64_t newUid = m_uidFromClassVersion->get(p->newName, p->newVersion);
        index = m_patchIndexFromUid.getWithDefault(newUid, -1);
    }
    return -1;
}

// A valid patch is appended and the uid index, rebuilt by the next preparation, is emptied.
void hkVersionPatchManager::addPatch(const hkVersionPatchManager::PatchInfo* p) {
    if (isValidPatch(p)) {
        hkMemoryRouter& router = hkMemoryRouter::getInstance();
        // EXE-INLINED none: hkArrayBase<const hkVersionPatchManager::PatchInfo*>::_pushBack
        if (m_patchInfos.m_size == (m_patchInfos.m_capacityAndFlags & hkArrayBase<const PatchInfo*>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(router.heap(), &m_patchInfos, sizeof(const PatchInfo*));
        }
        m_patchInfos.m_data[m_patchInfos.m_size] = p;
        ++m_patchInfos.m_size;
        m_patchIndexFromUid.m_map.clear();
    }
}

// Queues the patches that bring each class of worldToUpdate up to date (patchInfosOut, in the order they apply), then
// builds the classes they start from in a temporary world, walking those patches backwards from the final classes of
// finalClassesReg, and copies into worldToUpdate the classes it lacks. Fails on an invalid patch, on two patches adding
// one class, or on a class neither patched nor registered at its version.
hkResult hkVersionPatchManager::preparePatches(
    hkDataWorldDict& worldToUpdate,
    const hkClassNameRegistry& finalClassesReg,
    hkArray<const hkVersionPatchManager::PatchInfo*, hkContainerHeapAllocator>& patchInfosOut
) const {
    using ClassArray = hkArray<hkDataClassImpl*>;
    using VersionArray = hkArray<ClassVersion>;
    using UidMap = hkPointerMap<hkUint64, int32_t>;
    using NameMap = hkCachedHashMap<hkStringMapOperations, hkContainerHeapAllocator>;
    FNVD_INLINED_LOCAL(ClassArray, origClasses, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(VersionArray, origVersions, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(hkDataWorldDict, tempWorld, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(UidMap, uidState, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(UidMap, addedClassPatch, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(HeapArray<UidItem>, toDo, std::nullopt, std::nullopt);

    // EXE-INLINED none: hkArray<hkDataClassImpl*, hkContainerHeapAllocator>::hkArray
    origClasses.m_data = nullptr;
    origClasses.m_size = 0;
    origClasses.m_capacityAndFlags = hkArrayBase<hkDataClassImpl*>::DONT_DEALLOCATE_FLAG;
    worldToUpdate.findAllClasses(origClasses);

    // EXE-INLINED none: hkArray<ClassVersion, hkContainerHeapAllocator>::hkArray
    origVersions.m_capacityAndFlags = hkArrayBase<ClassVersion>::DONT_DEALLOCATE_FLAG;
    origVersions.m_data = nullptr;
    origVersions.m_size = 0;
    {
        int32_t numClasses = origClasses.m_size;
        hkMemoryAllocator& heap = *hkMemoryRouter::getInstance().m_heap;
        // EXE-INLINED none: hkArray<ClassVersion, hkContainerHeapAllocator>::reserve
        if ((origVersions.m_capacityAndFlags & hkArrayBase<ClassVersion>::CAPACITY_MASK) < numClasses) {
            hkArrayUtil::_reserve(heap, &origVersions, numClasses, sizeof(ClassVersion));
        }
    }
    for (int32_t i = 0; i < origClasses.m_size; ++i) {
        const char* name = origClasses.m_data[i]->getName();
        int32_t version = origClasses.m_data[i]->getVersion();
        // EXE-INLINED none: hkArray<ClassVersion, hkContainerHeapAllocator>::pushBackUnchecked
        ClassVersion* pEntry = origVersions.m_data + origVersions.m_size;
        if (pEntry) {
            pEntry->name = name;
            pEntry->version = version;
        }
        ++origVersions.m_size;
    }

    ::new (&tempWorld) hkDataWorldDict;
    // EXE-INLINED none: hkPointerMap<hkUint64, int32_t>::hkPointerMap
    uidState.m_map.m_elem = nullptr;
    uidState.m_map.m_numElems = 0;
    uidState.m_map.m_hashMod = -1;
    uidState.m_map.hkMapBase<uint64_t, uint64_t, hkMapOperations<uint64_t>>::reserve(
        *hkMemoryRouter::getInstance().m_heap, 0
    );
    // EXE-INLINED none: hkPointerMap<hkUint64, int32_t>::hkPointerMap
    addedClassPatch.m_map.m_elem = nullptr;
    addedClassPatch.m_map.m_numElems = 0;
    addedClassPatch.m_map.m_hashMod = -1;
    addedClassPatch.m_map.hkMapBase<uint64_t, uint64_t, hkMapOperations<uint64_t>>::reserve(
        *hkMemoryRouter::getInstance().m_heap, 0
    );

    // Each patch must be valid; the patches that add a class are indexed by the uid of the class they add.
    bool failed = false;
    for (int32_t i = 0; i < m_patchInfos.m_size; ++i) {
        const PatchInfo* p = m_patchInfos.m_data[i];
        if (!isValidPatch(p)) {
            failed = true;
            break;
        }
        if (p->oldVersion == -1) {
            hkUint64 uid = m_uidFromClassVersion->get(p->newName, p->newVersion);
            if (addedClassPatch.isValid(addedClassPatch.findKey(uid))) {
                failed = true;
                break;
            }
            addedClassPatch.insert(uid, i);
        }
    }
    if (failed) {
        addedClassPatch.m_map.clearAndDeallocate(*hkMemoryRouter::getInstance().m_heap);
        uidState.m_map.clearAndDeallocate(*hkMemoryRouter::getInstance().m_heap);
        tempWorld.~hkDataWorldDict();
        // EXE-INLINED none: hkArray<ClassVersion, hkContainerHeapAllocator>::~hkArray
        hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
        origVersions.m_size = 0;
        if (origVersions.m_capacityAndFlags >= 0) {
            pHeap->bufFree(
                origVersions.m_data,
                (origVersions.m_capacityAndFlags & hkArrayBase<ClassVersion>::CAPACITY_MASK) * sizeof(ClassVersion)
            );
        }
        origVersions.m_data = nullptr;
        origVersions.m_capacityAndFlags = hkArrayBase<ClassVersion>::DONT_DEALLOCATE_FLAG;
        // EXE-INLINED none: hkArray<hkDataClassImpl*, hkContainerHeapAllocator>::~hkArray
        pHeap = hkMemoryRouter::getInstance().m_heap;
        origClasses.m_size = 0;
        if (origClasses.m_capacityAndFlags >= 0) {
            pHeap->bufFree(
                origClasses.m_data,
                (origClasses.m_capacityAndFlags & hkArrayBase<hkDataClassImpl*>::CAPACITY_MASK) *
                    sizeof(hkDataClassImpl*)
            );
        }
        return HK_FAILURE;
    }

    // Queue the patches each class of the world needs, then work through the queue in patch order.
    patchInfosOut.m_size = 0;
    // EXE-INLINED none: hkArray<UidItem, hkContainerHeapAllocator>::hkArray
    toDo.m_elems.m_data = nullptr;
    toDo.m_elems.m_size = 0;
    toDo.m_elems.m_capacityAndFlags = hkArrayBase<UidItem>::DONT_DEALLOCATE_FLAG;
    for (int32_t i = 0; i < origVersions.m_size; ++i) {
        setAllDependeciesToDo(
            toDo, origVersions.m_data[i].name, origVersions.m_data[i].version, worldToUpdate, *this, addedClassPatch,
            uidState
        );
    }
    while (toDo.m_elems.m_size != 0) {
        UidItem item = toDo.m_elems.m_data[0];
        // EXE-INLINED none: hkArrayBase<UidItem>::removeAtAndCopy
        --toDo.m_elems.m_size;
        for (int32_t k = 0; k < toDo.m_elems.m_size; ++k) {
            toDo.m_elems.m_data[k] = toDo.m_elems.m_data[k + 1];
        }
        int32_t patchIndex = item.patchIndex;
        if (patchIndex != -1 && m_patchInfos.m_data[patchIndex]->oldVersion == -1) {
            uidState.insert(item.uid, 1);
        }
        else {
            uidState.insert(item.uid, 2);
        }
        if (patchIndex >= 0) {
            const PatchInfo* p = m_patchInfos.m_data[patchIndex];
            hkMemoryAllocator& heap = *hkMemoryRouter::getInstance().m_heap;
            // EXE-INLINED none: hkArray<const hkVersionPatchManager::PatchInfo*, hkContainerHeapAllocator>::pushBack
            if (patchInfosOut.m_size ==
                (patchInfosOut.m_capacityAndFlags & hkArrayBase<const PatchInfo*>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(heap, &patchInfosOut, sizeof(const PatchInfo*));
            }
            patchInfosOut.m_data[patchInfosOut.m_size] = p;
            ++patchInfosOut.m_size;
            const PatchInfo* patch = m_patchInfos.m_data[patchIndex];
            const char* name = patch->newName;
            if (!name) {
                name = patch->oldName;
            }
            setAllDependeciesToDo(toDo, name, patch->newVersion, worldToUpdate, *this, addedClassPatch, uidState);
            continue;
        }
        // No patch: the class must be registered at the version the world has.
        const char* className = getClassName(item.uid);
        int32_t classVersion = getClassVersion(item.uid);
        if (classVersion == -1) {
            continue;
        }
        const hkClass* klass = finalClassesReg.getClassByName(className);
        if (klass) {
            if (klass->getDescribedVersion() == classVersion) {
                if (!tempWorld.hkDataWorldDict::findClass(klass->getName())) {
                    tempWorld.wrapClass(*klass);
                }
                continue;
            }
            // HK_WARN: the described version for the message is read twice, before any of it is written.
            char buf[512];
            hkErrStream kStream(buf, 512);
            int32_t describedVersion = klass->getDescribedVersion();
            int32_t foundVersion = klass->getDescribedVersion();
            kStream << "Found class " << className << " of version " << foundVersion << ", but expected version "
                    << classVersion << ".\nMake sure required patches are registered to update the class " << className
                    << " from version " << classVersion << " to " << describedVersion
                    << ",\nor update the class definition with the //+version(" << classVersion << ") annotation.";
            hkError::s_instance->message(
                hkError::MESSAGE_WARNING, 0x3f79ddb1, buf, ".\\Version\\hkVersionPatchManager.cpp", 766
            );
        }
        // A class neither patched nor registered at its version.
        // EXE-INLINED none: hkArray<UidItem, hkContainerHeapAllocator>::~hkArray
        hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
        toDo.m_elems.m_size = 0;
        if (toDo.m_elems.m_capacityAndFlags >= 0) {
            pHeap->bufFree(
                toDo.m_elems.m_data,
                (toDo.m_elems.m_capacityAndFlags & hkArrayBase<UidItem>::CAPACITY_MASK) * sizeof(UidItem)
            );
        }
        toDo.m_elems.m_data = nullptr;
        toDo.m_elems.m_capacityAndFlags = hkArrayBase<UidItem>::DONT_DEALLOCATE_FLAG;
        addedClassPatch.m_map.clearAndDeallocate(*hkMemoryRouter::getInstance().m_heap);
        uidState.m_map.clearAndDeallocate(*hkMemoryRouter::getInstance().m_heap);
        tempWorld.~hkDataWorldDict();
        // EXE-INLINED none: hkArray<ClassVersion, hkContainerHeapAllocator>::~hkArray
        pHeap = hkMemoryRouter::getInstance().m_heap;
        origVersions.m_size = 0;
        if (origVersions.m_capacityAndFlags >= 0) {
            pHeap->bufFree(
                origVersions.m_data,
                (origVersions.m_capacityAndFlags & hkArrayBase<ClassVersion>::CAPACITY_MASK) * sizeof(ClassVersion)
            );
        }
        origVersions.m_data = nullptr;
        origVersions.m_capacityAndFlags = hkArrayBase<ClassVersion>::DONT_DEALLOCATE_FLAG;
        // EXE-INLINED none: hkArray<hkDataClassImpl*, hkContainerHeapAllocator>::~hkArray
        pHeap = hkMemoryRouter::getInstance().m_heap;
        origClasses.m_size = 0;
        if (origClasses.m_capacityAndFlags >= 0) {
            pHeap->bufFree(
                origClasses.m_data,
                (origClasses.m_capacityAndFlags & hkArrayBase<hkDataClassImpl*>::CAPACITY_MASK) *
                    sizeof(hkDataClassImpl*)
            );
        }
        return HK_FAILURE;
    }
    {
        // EXE-INLINED none: hkArray<UidItem, hkContainerHeapAllocator>::~hkArray
        hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
        toDo.m_elems.m_size = 0;
        if (toDo.m_elems.m_capacityAndFlags >= 0) {
            pHeap->bufFree(
                toDo.m_elems.m_data,
                (toDo.m_elems.m_capacityAndFlags & hkArrayBase<UidItem>::CAPACITY_MASK) * sizeof(UidItem)
            );
        }
    }

    // Undo the queued patches on the temporary world, last first, from the final classes down to the ones the patches
    // start from.
    for (int32_t patchNum = patchInfosOut.m_size - 1; patchNum >= 0; --patchNum) {
        const PatchInfo* p = patchInfosOut.m_data[patchNum];
        const char* oldName = p->oldName;
        const char* newName = p->newName ? p->newName : oldName;
        hkDataClass klass(tempWorld.hkDataWorldDict::findClass(newName));
        if (klass.isNull()) {
            // The class as the patch leaves it, still at the version the patch starts from (oldVersion is passed).
            FNVD_INLINED_LOCAL(hkDataClass::Cinfo, cinfo, std::nullopt, std::nullopt);
            // EXE-INLINED none: hkArray<hkDataClass::Cinfo::Member, hkContainerHeapAllocator>::hkArray
            cinfo.members.m_data = nullptr;
            cinfo.members.m_size = 0;
            cinfo.members.m_capacityAndFlags = hkArrayBase<hkDataClass::Cinfo::Member>::DONT_DEALLOCATE_FLAG;
            cinfo.name = newName;
            cinfo.version = p->oldVersion;
            cinfo.parent = nullptr;
            hkDataClass created(tempWorld.hkDataWorldDict::newClass(cinfo));
            // EXE-INLINED none: hkDataClass::operator=
            klass.m_impl = created.m_impl;
            // EXE-INLINED 0xDBA490: hkDataClass::Cinfo::~Cinfo
            hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
            cinfo.members.m_size = 0;
            if (cinfo.members.m_capacityAndFlags >= 0) {
                pHeap->bufFree(
                    cinfo.members.m_data,
                    (cinfo.members.m_capacityAndFlags & hkArrayBase<hkDataClass::Cinfo::Member>::CAPACITY_MASK) *
                        sizeof(hkDataClass::Cinfo::Member)
                );
            }
        }
        for (int32_t j = p->numComponent - 1; j >= 0; --j) {
            const PatchInfo::Component& component = p->component[j];
            switch (component.type) {
                case PATCH_MEMBER_ADDED: {
                    const MemberAddedPatch* added = static_cast<const MemberAddedPatch*>(component.patch);
                    tempWorld.removeClassMember(klass, added->name);
                    break;
                }
                case PATCH_MEMBER_REMOVED: {
                    const MemberRemovedPatch* removed = static_cast<const MemberRemovedPatch*>(component.patch);
                    tempWorld.addClassMember(klass, removed->name, removed->type, removed->typeName, removed->tuples);
                    break;
                }
                case PATCH_MEMBER_RENAMED: {
                    const MemberRenamedPatch* renamed = static_cast<const MemberRenamedPatch*>(component.patch);
                    tempWorld.renameClassMember(klass, renamed->newName, renamed->oldName);
                    break;
                }
                case PATCH_DEPENDS: {
                    // The SDK's check of the dependency's class (its test compiled out): only the class is made.
                    const DependsPatch* depends = static_cast<const DependsPatch*>(component.patch);
                    hkDataClass dependency(tempWorld.hkDataWorldDict::findClass(depends->name));
                    (void)dependency;
                    break;
                }
                case PATCH_PARENT_SET: {
                    const SetParentPatch* parentSet = static_cast<const SetParentPatch*>(component.patch);
                    if (parentSet->oldParent) {
                        hkDataClass parent(tempWorld.hkDataWorldDict::findClass(parentSet->oldParent));
                        tempWorld.setClassParent(klass, parent);
                    }
                    else {
                        hkDataClass noParent(nullptr);
                        tempWorld.setClassParent(klass, noParent);
                    }
                    break;
                }
                default: break;
            }
        }
        if (p->oldVersion == -1) {
            tempWorld.removeClass(klass);
        }
        else {
            if (newName != oldName) {
                tempWorld.renameClass(klass, oldName);
            }
            tempWorld.setClassVersion(klass, p->oldVersion);
        }
    }

    // Copy into worldToUpdate the classes of the temporary world it does not have. The name map holds the names and
    // classes as hkUlong integers (the SDK's casts).
    FNVD_INLINED_LOCAL(ClassArray, tempClasses, std::nullopt, std::nullopt);
    // EXE-INLINED none: hkArray<hkDataClassImpl*, hkContainerHeapAllocator>::hkArray
    tempClasses.m_data = nullptr;
    tempClasses.m_size = 0;
    tempClasses.m_capacityAndFlags = hkArrayBase<hkDataClassImpl*>::DONT_DEALLOCATE_FLAG;
    tempWorld.hkDataWorldDict::findAllClasses(tempClasses);
    FNVD_INLINED_LOCAL(NameMap, classFromName, std::nullopt, std::nullopt);
    ::new (&classFromName) NameMap(0);
    FNVD_INLINED_LOCAL(NameMap, unusedMap, std::nullopt, std::nullopt);
    ::new (&unusedMap) NameMap(0);
    for (int32_t i = 0; i < tempClasses.m_size; ++i) {
        hkDataClassImpl* pClass = tempClasses.m_data[i];
        classFromName.insert(reinterpret_cast<hkUlong>(pClass->getName()), reinterpret_cast<hkUlong>(pClass));
    }
    for (int32_t i = 0; i < origClasses.m_size; ++i) {
        if (classFromName.getWithDefault(reinterpret_cast<hkUlong>(origClasses.m_data[i]->getName()), 0)) {
            classFromName.remove(reinterpret_cast<hkUlong>(origClasses.m_data[i]->getName()));
        }
    }
    {
        hkBool valid;
        NameMap::Iterator it = classFromName.getIterator();
        for (classFromName.isValid(&valid, it); valid.m_bool; classFromName.isValid(&valid, it)) {
            hkDataClassImpl* pClass = reinterpret_cast<hkDataClassImpl*>(classFromName.getValue(it));
            worldToUpdate.copyClassFromWorld(pClass->getName(), tempWorld);
            it = classFromName.getNext(it);
        }
    }
    unusedMap.clearAndDeallocate();
    classFromName.clearAndDeallocate();

    // EXE-INLINED none: hkArray<hkDataClassImpl*, hkContainerHeapAllocator>::~hkArray
    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
    tempClasses.m_size = 0;
    if (tempClasses.m_capacityAndFlags >= 0) {
        pHeap->bufFree(
            tempClasses.m_data,
            (tempClasses.m_capacityAndFlags & hkArrayBase<hkDataClassImpl*>::CAPACITY_MASK) * sizeof(hkDataClassImpl*)
        );
    }
    addedClassPatch.m_map.clearAndDeallocate(*hkMemoryRouter::getInstance().m_heap);
    uidState.m_map.clearAndDeallocate(*hkMemoryRouter::getInstance().m_heap);
    tempWorld.~hkDataWorldDict();
    // EXE-INLINED none: hkArray<ClassVersion, hkContainerHeapAllocator>::~hkArray
    pHeap = hkMemoryRouter::getInstance().m_heap;
    origVersions.m_size = 0;
    if (origVersions.m_capacityAndFlags >= 0) {
        pHeap->bufFree(
            origVersions.m_data,
            (origVersions.m_capacityAndFlags & hkArrayBase<ClassVersion>::CAPACITY_MASK) * sizeof(ClassVersion)
        );
    }
    origVersions.m_data = nullptr;
    origVersions.m_capacityAndFlags = hkArrayBase<ClassVersion>::DONT_DEALLOCATE_FLAG;
    // EXE-INLINED none: hkArray<hkDataClassImpl*, hkContainerHeapAllocator>::~hkArray
    pHeap = hkMemoryRouter::getInstance().m_heap;
    origClasses.m_size = 0;
    if (origClasses.m_capacityAndFlags >= 0) {
        pHeap->bufFree(
            origClasses.m_data,
            (origClasses.m_capacityAndFlags & hkArrayBase<hkDataClassImpl*>::CAPACITY_MASK) * sizeof(hkDataClassImpl*)
        );
    }
    return HK_SUCCESS;
}

hkResult hkVersionPatchManager::applyPatchesDebug(hkDataWorldDict& worldToUpdate) {
    if ((m_patchIndexFromUid.m_map.m_numElems &
         hkMapBase<uint64_t, uint64_t, hkMapOperations<uint64_t>>::NUM_ELEMS_MASK) != 0) {
        return applyPatchesToWorld(&m_patchInfos, &worldToUpdate);
    }
    if (!m_patchInfos.m_size) {
        return applyPatchesToWorld(&m_patchInfos, &worldToUpdate);
    }
    hkResult result = recomputePatchDependencies();
    if (result != HK_FAILURE) {
        return applyPatchesToWorld(&m_patchInfos, &worldToUpdate);
    }
    return result;
}

const char* hkVersionPatchManager::getClassName(hkUint64 uid) const {
    return m_uidFromClassVersion->getName(uid);
}

// Checks the patches and indexes them by the uid of the class they patch (m_patchIndexFromUid), works out which patch
// must run before which (a patch before the patches of its class's later versions, its PATCH_DEPENDS classes' patches
// before it, a removed or renamed class's patches before the patch adding a class of that name), drops the dependencies
// already implied by others, and sorts m_patchInfos so that each patch follows those it depends on. Fails on an invalid
// patch, a class patched twice from one version, a patch adding a class twice, or a dependency cycle (m_patchInfos is
// then left as it was).
hkResult hkVersionPatchManager::recomputePatchDependencies() const {
    using UidIndexMap = hkPointerMap<hkUint64, int32_t>;
    using UidMultiMap = hkSerializeMultiMap<uint64_t, int32_t>;
    using NameMultiMap = hkSerializeMultiMap<const void*, int32_t>;
    using IndexMultiMap = hkSerializeMultiMap<int32_t, int32_t>;
    using IndexMap = hkPointerMap<int32_t, int32_t>;
    using NameMap = hkCachedHashMap<hkStringMapOperations, hkContainerHeapAllocator>;
    using PatchArray = hkArray<const PatchInfo*>;
    using IndexArray = hkArray<int32_t>;
    FNVD_INLINED_LOCAL(UidIndexMap, addedPatchIndex, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(UidMultiMap, newUidPatches, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(NameMultiMap, removedNames, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(NameMap, addedNames, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(IndexMultiMap, dependencies, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(IndexMultiMap, laterDependencies, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(PatchArray, patchInfos, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(IndexArray, order, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(IndexArray, done, std::nullopt, std::nullopt);
    hkResult result = HK_FAILURE;
    hkMemoryAllocator* pHeap;
    int32_t numPatches;
    int32_t index;
    UidFromClassVersion* pUids;

    // EXE-INLINED none: hkPointerMap<hkUint64, int32_t>::hkPointerMap
    addedPatchIndex.m_map.m_elem = nullptr;
    addedPatchIndex.m_map.m_numElems = 0;
    addedPatchIndex.m_map.m_hashMod = -1;
    addedPatchIndex.m_map.hkMapBase<uint64_t, uint64_t, hkMapOperations<uint64_t>>::reserve(
        *hkMemoryRouter::getInstance().m_heap, 0
    );
    // EXE-INLINED none: hkSerializeMultiMap<uint64_t, int>::hkSerializeMultiMap
    newUidPatches.m_valueChain.m_data = nullptr;
    newUidPatches.m_valueChain.m_size = 0;
    newUidPatches.m_valueChain.m_capacityAndFlags = hkArrayBase<UidMultiMap::Value>::DONT_DEALLOCATE_FLAG;
    newUidPatches.m_indexMap.m_map.m_elem = nullptr;
    newUidPatches.m_indexMap.m_map.m_numElems = 0;
    newUidPatches.m_indexMap.m_map.m_hashMod = -1;
    newUidPatches.m_indexMap.m_map.hkMapBase<uint64_t, uint64_t, hkMapOperations<uint64_t>>::reserve(
        *hkMemoryRouter::getInstance().m_heap, 0
    );
    newUidPatches.m_freeChainStart = -1;
    pUids = m_uidFromClassVersion;
    // EXE-INLINED 0xDC9E70: hkSerializeMultiMap<const void*, int32_t>::hkSerializeMultiMap
    removedNames.m_valueChain.m_data = nullptr;
    removedNames.m_valueChain.m_size = 0;
    removedNames.m_valueChain.m_capacityAndFlags = hkArrayBase<NameMultiMap::Value>::DONT_DEALLOCATE_FLAG;
    removedNames.m_indexMap.m_map.m_elem = nullptr;
    removedNames.m_indexMap.m_map.m_numElems = 0;
    removedNames.m_indexMap.m_map.m_hashMod = -1;
    removedNames.m_indexMap.m_map.hkUlongMapBase::reserve(*hkMemoryRouter::getInstance().m_heap, 0);
    removedNames.m_freeChainStart = -1;
    m_patchIndexFromUid.m_map.clear();

    // Index each patch by the uid of the class it starts from (or, for one adding a class, the class it adds) and by
    // the uid of the class it makes; remember the names of the classes patches remove or rename.
    for (int32_t i = 0; i < m_patchInfos.m_size; ++i) {
        const PatchInfo* p = m_patchInfos.m_data[i];
        if (!isValidPatch(p)) {
            goto destroyRemovedNames;
        }
        if (p->oldVersion != -1) {
            hkUint64 uid = m_uidFromClassVersion->get(p->oldName, p->oldVersion);
            if (m_patchIndexFromUid.isValid(m_patchIndexFromUid.findKey(uid))) {
                goto destroyCalled;
            }
            m_patchIndexFromUid.insert(uid, i);
            if (p->newVersion == -1 || (p->newName && hkString::strCmp(p->oldName, p->newName) != 0)) {
                removedNames.insert(p->oldName, i);
            }
        }
        else {
            hkUint64 uid = m_uidFromClassVersion->get(p->newName, p->newVersion);
            if (addedPatchIndex.isValid(addedPatchIndex.findKey(uid))) {
                goto destroyCalled;
            }
            addedPatchIndex.insert(uid, i);
        }
        if (p->newVersion != -1) {
            const char* newName = p->newName ? p->newName : p->oldName;
            hkUint64 uid = pUids->get(newName, p->newVersion);
            newUidPatches.insert(uid, i);
        }
    }

    ::new (&addedNames) NameMap(0);
    // EXE-INLINED none: hkSerializeMultiMap<int32_t, int32_t>::hkSerializeMultiMap
    dependencies.m_valueChain.m_data = nullptr;
    dependencies.m_valueChain.m_size = 0;
    dependencies.m_valueChain.m_capacityAndFlags = hkArrayBase<IndexMultiMap::Value>::DONT_DEALLOCATE_FLAG;
    dependencies.m_indexMap.m_map.m_elem = nullptr;
    dependencies.m_indexMap.m_map.m_numElems = 0;
    dependencies.m_indexMap.m_map.m_hashMod = -1;
    dependencies.m_indexMap.m_map.hkUlongMapBase::reserve(*hkMemoryRouter::getInstance().m_heap, 0);
    dependencies.m_freeChainStart = -1;
    // EXE-INLINED none: hkSerializeMultiMap<int32_t, int32_t>::hkSerializeMultiMap
    laterDependencies.m_valueChain.m_data = nullptr;
    laterDependencies.m_valueChain.m_size = 0;
    laterDependencies.m_valueChain.m_capacityAndFlags = hkArrayBase<IndexMultiMap::Value>::DONT_DEALLOCATE_FLAG;
    laterDependencies.m_indexMap.m_map.m_elem = nullptr;
    laterDependencies.m_indexMap.m_map.m_numElems = 0;
    laterDependencies.m_indexMap.m_map.m_hashMod = -1;
    laterDependencies.m_indexMap.m_map.hkUlongMapBase::reserve(*hkMemoryRouter::getInstance().m_heap, 0);
    laterDependencies.m_freeChainStart = -1;

    // Each patch depends on the patches that make the class it starts from and on its PATCH_DEPENDS classes'
    // patches; a patch adding a class depends on the patches removing or renaming a class of that name.
    for (int32_t i = 0; i < m_patchInfos.m_size; ++i) {
        const PatchInfo* p = m_patchInfos.m_data[i];
        const char* name = p->oldName;
        int32_t version = p->oldVersion;
        if (version == -1) {
            name = p->newName;
            version = p->newVersion;
            if (!name || version == -1) {
                goto destroyDependencies;
            }
        }
        hkUint64 uid = pUids->get(name, version);
        int32_t patchIndex;
        if (p->oldVersion == -1) {
            patchIndex = addedPatchIndex.getWithDefault(uid, -1);
        }
        else {
            patchIndex = m_patchIndexFromUid.getWithDefault(uid, -1);
            for (int32_t j = newUidPatches.find(uid); j != -1; j = newUidPatches.m_valueChain.m_data[j].next) {
                int32_t previous = newUidPatches.m_valueChain.m_data[j].value;
                dependencies.insert(patchIndex, previous);
            }
        }
        for (int32_t k = 0; k < p->numComponent; ++k) {
            if (p->component[k].type != PATCH_DEPENDS) {
                continue;
            }
            const DependsPatch* depends = static_cast<const DependsPatch*>(p->component[k].patch);
            hkUint64 dependsUid = pUids->get(depends->name, depends->version);
            int32_t dependsIndex = m_patchIndexFromUid.getWithDefault(dependsUid, -1);
            if (dependsIndex != -1) {
                dependencies.insert(dependsIndex, patchIndex);
            }
            for (int32_t j = newUidPatches.find(dependsUid); j != -1; j = newUidPatches.m_valueChain.m_data[j].next) {
                int32_t previous = newUidPatches.m_valueChain.m_data[j].value;
                dependencies.insert(patchIndex, previous);
            }
        }
        if (p->oldVersion == -1 && removedNames.find(name) != -1) {
            int32_t j = removedNames.find(name);
            if (j != -1) {
                int32_t last = findLastPatchIndexForUid(uid, 0);
                IndexMultiMap* pTarget;
                if (last == -1) {
                    // The SDK's map holds the names and patch indices as hkUlong integers.
                    if (addedNames.getWithDefault(reinterpret_cast<hkUlong>(name), static_cast<hkUlong>(last)) !=
                        static_cast<hkUlong>(last)) {
                        goto destroyDependencies;
                    }
                    addedNames.insert(reinterpret_cast<hkUlong>(name), static_cast<hkUlong>(i));
                    pTarget = &dependencies;
                }
                else {
                    pTarget = &laterDependencies;
                }
                for (; j != -1; j = removedNames.m_valueChain.m_data[j].next) {
                    int32_t removing = removedNames.m_valueChain.m_data[j].value;
                    if (last != removing) {
                        pTarget->insert(patchIndex, removing);
                    }
                }
            }
        }
    }

    // Keep each later dependency only where the dependencies do not imply it already.
    for (int32_t it = laterDependencies.m_indexMap.getIterator(); laterDependencies.m_indexMap.isValid(it);
         it = laterDependencies.m_indexMap.getNext(it)) {
        int32_t key = laterDependencies.m_indexMap.getKey(it);
        for (int32_t j = laterDependencies.find(key); j != -1; j = laterDependencies.m_valueChain.m_data[j].next) {
            FNVD_INLINED_LOCAL(IndexMap, visited, std::nullopt, std::nullopt);
            // EXE-INLINED none: hkPointerMap<int32_t, int32_t>::hkPointerMap
            visited.m_map.m_elem = nullptr;
            visited.m_map.m_numElems = 0;
            visited.m_map.m_hashMod = -1;
            visited.m_map.hkUlongMapBase::reserve(*hkMemoryRouter::getInstance().m_heap, 0);
            int32_t value = laterDependencies.m_valueChain.m_data[j].value;
            if (!findPatchIndexInDependencies(key, value, dependencies, visited)) {
                dependencies.insert(key, value);
            }
            visited.m_map.clearAndDeallocate(*hkMemoryRouter::getInstance().m_heap);
        }
    }

    // Sort the patches: walkDependencies numbers each patch after those it depends on.
    // EXE-INLINED none: hkArray<const hkVersionPatchManager::PatchInfo*, hkContainerHeapAllocator>::hkArray
    patchInfos.m_data = nullptr;
    patchInfos.m_size = 0;
    patchInfos.m_capacityAndFlags = hkArrayBase<const PatchInfo*>::DONT_DEALLOCATE_FLAG;
    patchInfos.swap(m_patchInfos);
    numPatches = patchInfos.m_size;
    // EXE-INLINED none: hkArray<int32_t, hkContainerHeapAllocator>::hkArray
    order.m_data = nullptr;
    order.m_size = 0;
    order.m_capacityAndFlags = hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG;
    {
        hkMemoryAllocator& heap = *hkMemoryRouter::getInstance().m_heap;
        // EXE-INLINED none: hkArray<int32_t, hkContainerHeapAllocator>::setSize
        int32_t capacity = order.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK;
        if (capacity < numPatches) {
            int32_t newCapacity = 2 * capacity;
            if (numPatches >= newCapacity) {
                newCapacity = numPatches;
            }
            hkArrayUtil::_reserve(heap, &order, newCapacity, sizeof(int32_t));
        }
        for (int32_t k = order.m_size; k < numPatches; ++k) {
            order.m_data[k] = -1;
        }
        order.m_size = numPatches;
    }
    // EXE-INLINED none: hkArray<int32_t, hkContainerHeapAllocator>::hkArray
    done.m_data = nullptr;
    done.m_size = 0;
    done.m_capacityAndFlags = hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG;
    index = 0;
    for (int32_t i = 0; i < patchInfos.m_size; ++i) {
        index = walkDependencies(i, order, dependencies, index, done, patchInfos);
    }
    numPatches = patchInfos.m_size;
    {
        hkMemoryAllocator& heap = *hkMemoryRouter::getInstance().m_heap;
        // EXE-INLINED none: hkArray<const hkVersionPatchManager::PatchInfo*, hkContainerHeapAllocator>::setSize
        int32_t capacity = m_patchInfos.m_capacityAndFlags & hkArrayBase<const PatchInfo*>::CAPACITY_MASK;
        if (capacity < numPatches) {
            int32_t newCapacity = 2 * capacity;
            if (numPatches >= newCapacity) {
                newCapacity = numPatches;
            }
            hkArrayUtil::_reserve(heap, &m_patchInfos, newCapacity, sizeof(const PatchInfo*));
        }
        m_patchInfos.m_size = numPatches;
    }
    for (int32_t i = 0; i < patchInfos.m_size; ++i) {
        int32_t position = order.m_data[i];
        if (position < 0) {
            // A dependency cycle: the patches go back as they were.
            m_patchInfos.swap(patchInfos);
            goto destroyArrays;
        }
        m_patchInfos.m_data[position] = patchInfos.m_data[i];
    }
    m_patchIndexFromUid.m_map.clear();
    for (int32_t i = 0; i < m_patchInfos.m_size; ++i) {
        const PatchInfo* p = m_patchInfos.m_data[i];
        if (p->oldName) {
            hkUint64 uid = m_uidFromClassVersion->get(p->oldName, p->oldVersion);
            m_patchIndexFromUid.insert(uid, i);
        }
    }
    result = HK_SUCCESS;

destroyArrays:
    // EXE-INLINED none: hkArray<int32_t, hkContainerHeapAllocator>::~hkArray
    pHeap = hkMemoryRouter::getInstance().m_heap;
    done.m_size = 0;
    if (done.m_capacityAndFlags >= 0) {
        pHeap->bufFree(done.m_data, (done.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK) * sizeof(int32_t));
    }
    done.m_data = nullptr;
    done.m_capacityAndFlags = hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG;
    // EXE-INLINED none: hkArray<int32_t, hkContainerHeapAllocator>::~hkArray
    pHeap = hkMemoryRouter::getInstance().m_heap;
    order.m_size = 0;
    if (order.m_capacityAndFlags >= 0) {
        pHeap->bufFree(
            order.m_data, (order.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK) * sizeof(int32_t)
        );
    }
    order.m_data = nullptr;
    order.m_capacityAndFlags = hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG;
    // EXE-INLINED none: hkArray<const hkVersionPatchManager::PatchInfo*, hkContainerHeapAllocator>::~hkArray
    pHeap = hkMemoryRouter::getInstance().m_heap;
    patchInfos.m_size = 0;
    if (patchInfos.m_capacityAndFlags >= 0) {
        pHeap->bufFree(
            patchInfos.m_data,
            (patchInfos.m_capacityAndFlags & hkArrayBase<const PatchInfo*>::CAPACITY_MASK) * sizeof(const PatchInfo*)
        );
    }
    patchInfos.m_data = nullptr;
    patchInfos.m_capacityAndFlags = hkArrayBase<const PatchInfo*>::DONT_DEALLOCATE_FLAG;

destroyDependencies:
    // EXE-INLINED none: hkSerializeMultiMap<int32_t, int32_t>::~hkSerializeMultiMap
    laterDependencies.m_indexMap.m_map.clearAndDeallocate(*hkMemoryRouter::getInstance().m_heap);
    pHeap = hkMemoryRouter::getInstance().m_heap;
    laterDependencies.m_valueChain.m_size = 0;
    if (laterDependencies.m_valueChain.m_capacityAndFlags >= 0) {
        pHeap->bufFree(
            laterDependencies.m_valueChain.m_data,
            (laterDependencies.m_valueChain.m_capacityAndFlags & hkArrayBase<IndexMultiMap::Value>::CAPACITY_MASK) *
                sizeof(IndexMultiMap::Value)
        );
    }
    laterDependencies.m_valueChain.m_data = nullptr;
    laterDependencies.m_valueChain.m_capacityAndFlags = hkArrayBase<IndexMultiMap::Value>::DONT_DEALLOCATE_FLAG;
    // EXE-INLINED none: hkSerializeMultiMap<int32_t, int32_t>::~hkSerializeMultiMap
    dependencies.m_indexMap.m_map.clearAndDeallocate(*hkMemoryRouter::getInstance().m_heap);
    pHeap = hkMemoryRouter::getInstance().m_heap;
    dependencies.m_valueChain.m_size = 0;
    if (dependencies.m_valueChain.m_capacityAndFlags >= 0) {
        pHeap->bufFree(
            dependencies.m_valueChain.m_data,
            (dependencies.m_valueChain.m_capacityAndFlags & hkArrayBase<IndexMultiMap::Value>::CAPACITY_MASK) *
                sizeof(IndexMultiMap::Value)
        );
    }
    dependencies.m_valueChain.m_data = nullptr;
    dependencies.m_valueChain.m_capacityAndFlags = hkArrayBase<IndexMultiMap::Value>::DONT_DEALLOCATE_FLAG;
    addedNames.clearAndDeallocate();

destroyRemovedNames:
    // EXE-INLINED 0xDBA430: hkSerializeMultiMap<const void*, int32_t>::~hkSerializeMultiMap
    removedNames.m_indexMap.m_map.clearAndDeallocate(*hkMemoryRouter::getInstance().m_heap);
    pHeap = hkMemoryRouter::getInstance().m_heap;
    removedNames.m_valueChain.m_size = 0;
    if (removedNames.m_valueChain.m_capacityAndFlags >= 0) {
        pHeap->bufFree(
            removedNames.m_valueChain.m_data,
            (removedNames.m_valueChain.m_capacityAndFlags & hkArrayBase<NameMultiMap::Value>::CAPACITY_MASK) *
                sizeof(NameMultiMap::Value)
        );
    }
    removedNames.m_valueChain.m_data = nullptr;
    removedNames.m_valueChain.m_capacityAndFlags = hkArrayBase<NameMultiMap::Value>::DONT_DEALLOCATE_FLAG;
    // EXE-INLINED 0xDBA3D0: hkSerializeMultiMap<uint64_t, int>::~hkSerializeMultiMap
    newUidPatches.m_indexMap.m_map.clearAndDeallocate(*hkMemoryRouter::getInstance().m_heap);
    pHeap = hkMemoryRouter::getInstance().m_heap;
    newUidPatches.m_valueChain.m_size = 0;
    if (newUidPatches.m_valueChain.m_capacityAndFlags >= 0) {
        pHeap->bufFree(
            newUidPatches.m_valueChain.m_data,
            (newUidPatches.m_valueChain.m_capacityAndFlags & hkArrayBase<UidMultiMap::Value>::CAPACITY_MASK) *
                sizeof(UidMultiMap::Value)
        );
    }
    newUidPatches.m_valueChain.m_data = nullptr;
    newUidPatches.m_valueChain.m_capacityAndFlags = hkArrayBase<UidMultiMap::Value>::DONT_DEALLOCATE_FLAG;
    addedPatchIndex.m_map.clearAndDeallocate(*hkMemoryRouter::getInstance().m_heap);
    return result;

destroyCalled:
    // A class patched twice from one version, or added twice: the multimaps are destroyed here before failing.
    removedNames.~NameMultiMap();
    newUidPatches.~UidMultiMap();
    addedPatchIndex.m_map.clearAndDeallocate(*hkMemoryRouter::getInstance().m_heap);
    return HK_FAILURE;
}

hkResult
hkVersionPatchManager::applyPatches(hkDataWorldDict& worldToUpdate, const hkClassNameRegistry* finalClassesReg) {
    hkResult result;
    using apArray_t = hkArray<const hkVersionPatchManager::PatchInfo*, hkContainerHeapAllocator>;
    FNVD_INLINED_LOCAL(apArray_t, apArray, std::nullopt, std::nullopt);
    const hkClassNameRegistry* v3 = finalClassesReg;
    if (!finalClassesReg) {
        v3 = hkBuiltinTypeRegistry::s_instance->getClassNameRegistry();
    }
    v3->addReference();
    if ((m_patchIndexFromUid.m_map.m_numElems &
         hkMapBase<uint64_t, uint64_t, hkMapOperations<uint64_t>>::NUM_ELEMS_MASK) != 0 ||
        (!m_patchInfos.m_size || (result = recomputePatchDependencies(), result != HK_FAILURE))) {
        apArray.m_data = nullptr;
        apArray.m_size = 0;
        apArray.m_capacityAndFlags = hkArrayBase<const hkVersionPatchManager::PatchInfo*>::DONT_DEALLOCATE_FLAG;
        hkResult v6 = preparePatches(worldToUpdate, *v3, apArray);
        if (v6 == HK_SUCCESS) {
            v6 = applyPatchesToWorld(&apArray, &worldToUpdate);
        }
        v3->removeReference();
        hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
        apArray.m_size = 0;
        if (apArray.m_capacityAndFlags >= 0) {
            m_heap->bufFree(apArray.m_data, sizeof(hkVersionPatchManager::PatchInfo*) * apArray.m_capacityAndFlags);
        }
        return v6;
    }
    return result;
}

int32_t hkVersionPatchManager::getClassVersion(uint64_t uid) const {
    hkVersionPatchManager::UidFromClassVersion* m_uidFromClassVersion = this->m_uidFromClassVersion;
    return getVersion(uid, static_cast<int32_t>(uid >> 32));
}
