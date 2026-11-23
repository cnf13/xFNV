#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/StringMap/hkStringMap.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Object/hkSingleton.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Serialize/Data/hkDataObject.h"
#include "Havok/Source/Common/Serialize/Util/hkSerializeMultiMap.h"

#include <stdint.h>

class hkClassNameRegistry;
class hkDataWorldDict;

// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
int32_t findPatchIndexInDependencies(
    int32_t indexToFind,
    int32_t startIndex,
    const hkSerializeMultiMap<int32_t, int32_t>& incoming,
    hkPointerMap<int32_t, int32_t, hkContainerHeapAllocator>& done
);
// The singleton's create function: returns `new hkVersionPatchManager()`.
hkReferencedObject* hkVersionPatchManagercreate();

// One entry of a patch manager's uid list: a patch's uid and its index in the manager's patch list.
struct UidItem {
    hkUint64 uid;
    int32_t patchIndex;
};

// The patch application's queue of uids, an array kept ordered by insert.
template <typename T>
struct HeapArray {
    // Inserts t before the first element whose patch index is greater, else appends it.
    void insert(const T& e);
    // Whether an element with t's uid is in the array.
    bool hasElement(const T& e) const;

    hkArray<T> m_elems;
};
// Compares t.patchIndex (+0x8) with each element's and inserts through hkArrayBase<UidItem>::_insertAt,
// or appends the 16 bytes.
template <>
void HeapArray<UidItem>::insert(const UidItem& e);
// Compares only the uid's two words.
template <>
bool HeapArray<UidItem>::hasElement(const UidItem& e) const;

// The registry of the patches that bring old serialized classes up to date. Only the members its bodies use are
// declared.
class hkVersionPatchManager : public hkReferencedObject, public hkSingleton<hkVersionPatchManager> {
public:
    // The kind of a patch component.
    enum PatchType : int32_t {
        PATCH_INVALID,
        PATCH_MEMBER_ADDED,
        PATCH_MEMBER_REMOVED,
        PATCH_MEMBER_RENAMED,
        PATCH_FUNCTION,
        PATCH_CAST,
        PATCH_DEPENDS,
        PATCH_PARENT_SET,
        PATCH_END,
    };

    // One patch: the class it takes from oldName at oldVersion to newName at newVersion, and its steps.
    struct PatchInfo {
        // One step of a patch: its kind and the kind's own patch struct.
        struct Component {
            PatchType type;
            const void* patch;
        };

        const char* oldName;
        const char* newName;
        int32_t oldVersion;
        int32_t newVersion;
        const Component* component;
        int32_t numComponent;
    };

    // A function patch's function: rewrites one object of the patched class.
    using Function = void (*)(hkDataObject& o);

    // The patch kinds' own structs, which a PatchInfo::Component's patch points to by its type (members from the
    // SDK; layouts from Havok's release hkserialize.pdb).
    // PATCH_DEPENDS: the patch needs class name at version.
    struct DependsPatch {
        const char* name;
        int32_t version;
    };

    // PATCH_MEMBER_RENAMED.
    struct MemberRenamedPatch {
        const char* oldName;
        const char* newName;
    };

    // PATCH_MEMBER_ADDED: the member's name, type, type name (for objects and structs) and tuple count.
    struct MemberAddedPatch {
        const char* name;
        hkDataObject::Type type;
        const char* typeName;
        int32_t tuples;
    };

    // PATCH_MEMBER_REMOVED: as MemberAddedPatch, for the member removed.
    struct MemberRemovedPatch {
        const char* name;
        hkDataObject::Type type;
        const char* typeName;
        int32_t tuples;
    };

    // PATCH_PARENT_SET: the class's parent changes from oldParent to newParent.
    struct SetParentPatch {
        const char* oldParent;
        const char* newParent;
    };

    // PATCH_FUNCTION: the function named name, run on each object.
    struct FunctionPatch {
        const char* name;
        Function function;
    };

    // PATCH_CAST.
    struct CastPatch {
        const char* name;
    };

    // Numbers each class name and version pair: the low word is the name's index in m_names, the high word the
    // version.
    class UidFromClassVersion {
    public:
        ~UidFromClassVersion();

        // The class allocator: the heap allocator, by size.
        static void* operator new(size_t aSize) noexcept {
            return hkMemoryRouter::getInstance().m_heap->blockAlloc(static_cast<int32_t>(aSize));
        }
        static void operator delete(void* apMemory, size_t aSize) {
            hkMemoryRouter::getInstance().m_heap->blockFree(apMemory, static_cast<int32_t>(aSize));
        }

        // The uid of name at version ver; a name not seen before is cached and numbered.
        hkUint64 get(const char* name, int32_t ver);

    protected:
        // The map's own copy of name, made on its first use; null for null.
        const char* cache(const char* name);

    public:
        // The name of the uid's low word.
        const char* getName(hkUint64 uid) const;

        hkStringMap<int32_t> m_indexFromName;
        hkArray<const char*> m_names;
        hkStringMap<const char*> m_cachedNames;
    };

    hkVersionPatchManager();
    ~hkVersionPatchManager() override;

    // The uid of class name at version ver.
    hkUint64 getUid(const char* name, int32_t ver) const;
    const hkArray<const hkVersionPatchManager::PatchInfo*, hkContainerHeapAllocator>& getPatches();
    // Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
    int32_t findLastPatchIndexForUid(uint64_t uid, int32_t allowRenames) const;
    void addPatch(const hkVersionPatchManager::PatchInfo* p);
    // Havok SDK signature (hkSerialize.lib, tied by bytes,reloc,string).
    hkResult preparePatches(
        hkDataWorldDict& worldToUpdate,
        const hkClassNameRegistry& finalClassesReg,
        hkArray<const hkVersionPatchManager::PatchInfo*, hkContainerHeapAllocator>& patchInfosOut
    ) const;
    hkResult applyPatchesDebug(hkDataWorldDict& worldToUpdate);
    const char* getClassName(hkUint64 uid) const;
    // Checks the patches and sorts m_patchInfos so that each follows those it depends on, indexing them in
    // m_patchIndexFromUid.
    hkResult recomputePatchDependencies() const;
    hkResult applyPatches(hkDataWorldDict& worldToUpdate, const hkClassNameRegistry* finalClassesReg);
    // Havok SDK signature (hkSerialize.lib, tied by reloc).
    int32_t getClassVersion(uint64_t uid) const;

    UidFromClassVersion* m_uidFromClassVersion;
    // The patches added so far (getPatches returns its address; addPatch appends to it).
    mutable hkArray<const PatchInfo*> m_patchInfos;
    // The index in m_patchInfos of each patch, by its uid.
    mutable hkPointerMap<hkUint64, int32_t> m_patchIndexFromUid;

    // hkSingleton<hkVersionPatchManager>::s_instance.
    static inline hkVersionPatchManager* s_instance;
};
