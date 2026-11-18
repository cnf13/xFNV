#include "BSHavok/bhkWorldObject.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkMoppBvTreeShape.h"
#include "BSHavok/bhkNiTriStripsShape.h"
#include "BSHavok/bhkPackedNiTriStripsShape.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkStream.h"
#include "BSHavok/bhkWorld.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

// The class's RTTI name string.
using bhkWorldObject_Name_t = const char[15];

extern bhkWorldObject_Name_t bhkWorldObject_Name;

bhkWorldObject_Name_t bhkWorldObject_Name{"bhkWorldObject"};
NiRTTI bhkWorldObject::ms_RTTI("bhkWorldObject", &bhkSerializable::ms_RTTI);

bhkWorldObject::bhkWorldObject() : iFlags(0) {
    ++iCount;
}

bhkWorldObject::~bhkWorldObject() {
    bhkSerializable::Kill();
    --iCount;
}

// Reads the queued link ID and the base's data, then the creation info the virtual builds: swaps it to little endian
// and releases its property array, then fixes the collision filter info up for the file's version and material type.
void bhkWorldObject::LoadBinary(NiStream& arStream) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);

    arStream.ReadLinkID();
    bhkSerializable::LoadBinary(arStream);
    bool bCreated;
    auto* pkCinfo = &static_cast<hkWorldObjectCinfo&>(CreateCinfo(bCreated));
    if (!arStream.m_bSourceIsLittleEndian) {
        bhkStream::SwapHKArray(pkCinfo, static_cast<int32_t>(QCinfoSize()), &pkCinfo->m_properties);
    }
    // Clears and deallocates the creation info's property array. It reads the heap allocator before it clears the size,
    // and frees capacity * 8 bytes, the size of a property.
    hkMemoryAllocator* pkHeap = hkMemoryRouter::getInstance().m_heap;
    int32_t iCapacityAndFlags = pkCinfo->m_properties.m_capacityAndFlags;
    pkCinfo->m_properties.m_size = 0;
    if (iCapacityAndFlags >= 0) {
        pkHeap->bufFree(
            pkCinfo->m_properties.m_data,
            (iCapacityAndFlags & hkArrayBase<hkWorldObjectCinfo::Property>::CAPACITY_MASK) *
                static_cast<int32_t>(sizeof(hkWorldObjectCinfo::Property))
        );
    }
    pkCinfo->m_properties.m_data = nullptr;
    pkCinfo->m_properties.m_capacityAndFlags = hkArrayBase<hkWorldObjectCinfo::Property>::DONT_DEALLOCATE_FLAG;
    if (!arStream.m_bSourceIsLittleEndian) {
        pkCinfo->m_collisionFilterInfo = __builtin_bswap32(pkCinfo->m_collisionFilterInfo);
    }
    // Files before version 9 carry the old material type bits: bit 5 becomes the platform bit.
    if (arStream.m_BSStreamHeader.uiVersion < 9 && (pkCinfo->m_collisionFilterInfo & 0x20) != 0) {
        pkCinfo->m_collisionFilterInfo = (pkCinfo->m_collisionFilterInfo & ~0x20u) | 0x8000u;
    }
    const uint32_t uiMaterialType = pkCinfo->m_collisionFilterInfo & 0x7F;
    if (uiMaterialType == 8 || uiMaterialType == 29) {
        pkCinfo->m_collisionFilterInfo &= ~0x4000u;
    }
}

// Binds the creation info's shape to the object the stream's next link ID names: that wrapper's original Havok shape,
// or a MOPP tree built over it when this object is a rigid body and the wrapper holds a triangle strip collection.
void bhkWorldObject::LinkObject(NiStream& arStream) {
    NiObject* pkObject = arStream.GetObjectFromLinkID();
    bool bCreated;
    auto* pkCinfo = &static_cast<hkWorldObjectCinfo&>(CreateCinfo(bCreated));
    if (pkObject) {
        auto* pkShape = static_cast<bhkShape*>(pkObject);
        pkCinfo->m_shape = static_cast<hkpShape*>(pkShape->phkObject);
        if ((this->DynamicCastMember(&bhkRigidBody::ms_RTTI)) &&
            (pkObject->DynamicCastMember(&bhkNiTriStripsShape::ms_RTTI) ||
             pkObject->DynamicCastMember(&bhkPackedNiTriStripsShape::ms_RTTI))) {
            auto* pkMopp = static_cast<bhkMoppBvTreeShape*>(NiMemObject::operator new(sizeof(bhkMoppBvTreeShape)));
            if (pkMopp) {
                pkMopp = ::new (pkMopp) bhkMoppBvTreeShape(pkShape);
            }
            if (pkMopp) {
                pkCinfo->m_shape = static_cast<hkpShape*>(pkMopp->phkObject);
            }
        }
    }
    bhkSerializable::LinkObject(arStream);
}

// Registers the bhkShape wrapper the wrapped object's shape carries as user data, then the base's streamables. The
// wrapper is reached through the collidable's shape and its user data. The address of m_collidable is null-tested even
// though it is never null, as the game does.
bool bhkWorldObject::RegisterStreamables(NiStream& arStream) {
    if (this) {
        auto* pkObject = static_cast<hkpWorldObject*>(phkObject);
        if (pkObject) {
            const hkpShape* pkShape = pkObject->m_collidable.m_shape;
            if (pkShape) {
                bhkShape* pkUserData = pkShape->m_userData;
                if (pkUserData) {
                    pkUserData->RegisterStreamables(arStream);
                }
            }
        }
    }
    return NiObject::RegisterStreamables(arStream);
}

// Saves the creation info: swaps it to little endian, releases its property storage, saves the shape wrapper's link ID
// and the base's data, then drops the creation info.
void bhkWorldObject::SaveBinary(NiStream& arStream) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bool bCreated;
    auto* pkCinfo = &static_cast<hkWorldObjectCinfo&>(CreateCinfo(bCreated));
    // Compares the flag's raw byte with 1: any other byte swaps, so the byte is read as untyped memory rather than a
    // bool tested.
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        bhkStream::SwapHKArray(pkCinfo, static_cast<int32_t>(QCinfoSize()), &pkCinfo->m_properties);
        pkCinfo->m_collisionFilterInfo = __builtin_bswap32(pkCinfo->m_collisionFilterInfo);
    }
    // Clears and deallocates the creation info's property array. The byte count scales the capacity by 8 bytes per
    // element, as the game does, while hkpProperty here is 0x10 wide, so the byte count is written out.
    hkMemoryRouter& kRouter = hkMemoryRouter::getInstance();
    int32_t iCapacityAndFlags = pkCinfo->m_properties.m_capacityAndFlags;
    pkCinfo->m_properties.m_size = 0;
    if (iCapacityAndFlags >= 0) {
        kRouter.m_heap->bufFree(
            pkCinfo->m_properties.m_data,
            (iCapacityAndFlags & hkArrayBase<hkpProperty>::CAPACITY_MASK) * sizeof(hkWorldObjectCinfo::Property)
        );
    }
    pkCinfo->m_properties.m_data = nullptr;
    pkCinfo->m_properties.m_capacityAndFlags = hkArrayBase<hkpProperty>::DONT_DEALLOCATE_FLAG;
    arStream.SaveLinkID(pkCinfo->m_shape ? pkCinfo->m_shape->m_userData : nullptr);
    bhkSerializable::SaveBinary(arStream);
    KillCinfo(bCreated);
}

bool bhkWorldObject::IsEqual(NiObject* apObject) {
    return bhkSerializable::IsEqual(apObject);
}

// Adds the base's lines, the class name, the collision filter's lines and, when the wrapped object's shape has a
// wrapper, a "Shape" line with that wrapper's class name and address, inside the Havok memory context.
void bhkWorldObject::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkSerializable::GetViewerStrings(apStrings);
    char* pcString = NiGetViewerString(ms_RTTI.GetName());
    apStrings->Add(pcString);
    auto* pkObject = static_cast<hkpWorldObject*>(phkObject);
    CFilter kFilter;
    kFilter.iFilter = pkObject ? pkObject->m_collidable.getCollisionFilterInfo() : 0;
    kFilter.GetViewerStrings(apStrings);
    // Re-reads the wrapped object here.
    auto* pkObjectRw = static_cast<hkpWorldObject*>(phkObject);
    if (!pkObjectRw) {
        return;
    }
    const hkpShape* pkShape = pkObjectRw->m_collidable.m_shape;
    if (!pkShape) {
        return;
    }
    bhkShape* pShape = pkShape->m_userData;
    if (!pShape) {
        return;
    }
    char acBuffer[128];
    BSsprintf(acBuffer, 128, "%s: 0x%8X", pShape->GetRTTI()->GetName(), pShape);
    pcString = NiGetViewerString("Shape", static_cast<const char*>(acBuffer));
    apStrings->Add(pcString);
}

// On attach the wrapped object's shape wrapper gains a reference; on detach the object leaves its world and drops that
// reference. The wrapped object's user data then holds this, or null. The address of m_collidable is also null-tested,
// as the game does, and the pointer is hidden from clang so that test is kept.
void bhkWorldObject::Link(bool abAttach) {
    bhkWorldObject* pLink;
    if (abAttach) {
        auto* pkObject = this ? static_cast<hkpWorldObject*>(phkObject) : nullptr;
        if (pkObject) {
            const hkpCollidable* pCollidable = &pkObject->m_collidable;
            __asm__("" : "+r"(pCollidable));
            if (pCollidable) {
                const hkpShape* pkShape = pCollidable->m_shape;
                if (pkShape) {
                    bhkShape* pShape = pkShape->m_userData;
                    if (pShape) {
                        NiIncRefCount(pShape);
                    }
                }
            }
        }
        pLink = this;
    }
    else {
        Remove();
        RembhkShapeRef();
        pLink = nullptr;
    }
    auto* pkObjectRw = this ? static_cast<hkpWorldObject*>(phkObject) : nullptr;
    if (pkObjectRw) {
        pkObjectRw->m_userData = pLink;
    }
    bhkRefObject::Link(abAttach);
}

const hkpWorld* bhkWorldObject::GethkWorld() const {
    auto* pkObject = static_cast<hkpWorldObject*>(phkObject);
    if (pkObject) {
        return pkObject->m_world;
    }
    return nullptr;
}

hkpWorld* bhkWorldObject::GethkWorldRw() {
    if (this) {
        auto* pkObject = static_cast<hkpWorldObject*>(phkObject);
        if (pkObject) {
            return pkObject->m_world;
        }
    }
    return nullptr;
}

// Adds the wrapped Havok object to apWorld's world. When the object is in another world, it is removed from that one
// and added to apWorld's, and true is returned; a null world removes it. False when there is no wrapped object or
// nothing had to be done. this is null-checked before the field is read, as the game does.
bool bhkWorldObject::Add(bhkWorld* apWorld) {
    auto* pkObject = this ? static_cast<hkpWorldObject*>(phkObject) : nullptr;
    if (!pkObject) {
        return false;
    }
    if (apWorld) {
        if (GethkWorld() != static_cast<ahkpWorld*>(apWorld->phkObject)) {
            Remove();
            apWorld->Add(pkObject);
            return true;
        }
    }
    else {
        Remove();
    }
    return false;
}

// Builds the debug display of the wrapped object's shape under apNode (a new unnamed-then-named node when apNode is
// null), coloured by the collision filter; without a shape wrapper it returns apNode unchanged.
NiAVObject* bhkWorldObject::CreateDisplayGeometry(NiNode* apNode) {
    if (!this) {
        return apNode;
    }
    auto* pkObject = static_cast<hkpWorldObject*>(phkObject);
    if (!pkObject) {
        return apNode;
    }
    // The collidable's address is tested for null here and at the filter read below, as the game does. clang assumes a
    // member's address is non-null and drops those tests; the empty asm hides the pointer.
    const hkpCollidable* pkCollidable = &pkObject->m_collidable;
    __asm__("" : "+r"(pkCollidable));
    if (!pkCollidable) {
        return apNode;
    }
    const hkpShape* pkShape = pkCollidable->m_shape;
    if (!pkShape) {
        return apNode;
    }
    bhkShape* pShape = pkShape->m_userData;
    if (!pShape) {
        return apNode;
    }
    NiNode* pNode = apNode;
    if (!pNode) {
        void* pMemory = NiMemObject::operator new(sizeof(NiNode));
        pNode = pMemory ? ::new (pMemory) NiNode(0) : nullptr;
    }
    if (!pNode->m_kName.GetHandle()) {
        pNode->SetName(bhkWorldObject_Name);
    }
    NiColor kColor;
    kColor.r = 0.0f;
    kColor.g = 0.0f;
    kColor.b = 0.0f;
    // Re-reads the wrapped object here.
    auto* pkObjectRw = static_cast<hkpWorldObject*>(phkObject);
    uint32_t uiFilter = 0;
    if (pkObjectRw) {
        const hkpCollidable* pkCollidableRw = &pkObjectRw->m_collidable;
        __asm__("" : "+r"(pkCollidableRw));
        if (pkCollidableRw) {
            uiFilter = pkCollidableRw->getCollisionFilterInfo();
        }
    }
    bhkCollisionFilter::GetDebugColor(CFilter{uiFilter}, kColor);
    pShape->BuildDisplayGeometry(pNode, kColor);
    return pNode;
}

// Has the wrapped object's shape wrapper destroy its debug display. The address of m_collidable is also null-tested, as
// the game does; clang assumes a member's address is non-null and drops that test, so the empty asm hides the pointer.
void bhkWorldObject::DestroyDisplayGeometry() {
    if (!this) {
        return;
    }
    auto* pkObject = static_cast<hkpWorldObject*>(phkObject);
    if (!pkObject) {
        return;
    }
    const hkpCollidable* pkCollidable = &pkObject->m_collidable;
    __asm__("" : "+r"(pkCollidable));
    if (!pkCollidable) {
        return;
    }
    const hkpShape* pkShape = pkCollidable->m_shape;
    if (!pkShape) {
        return;
    }
    bhkShape* pShape = pkShape->m_userData;
    if (pShape) {
        pShape->DestroyDisplayGeometry();
    }
}

void bhkWorldObject::ClearFlags(uint32_t aiFlags) {
    iFlags &= (~((int32_t)aiFlags));
}

// Copies this wrapper's members into apDestination for a clone: the creation info gets an empty property list and
// collision group 0, and its shape becomes the original shape of a clone of the shape's wrapper unless that wrapper can
// be shared; then the base's members, inside the Havok memory context. The creation info is not killed here.
void bhkWorldObject::CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bool bCreated;
    auto* pkCinfo = &static_cast<hkWorldObjectCinfo&>(CreateCinfo(bCreated));
    pkCinfo->m_properties.m_size = 0;
    const hkpShape* pkShape = pkCinfo->m_shape;
    if (pkShape) {
        bhkShape* pShape = pkShape->m_userData;
        if (pShape && !pShape->CanShare(arCloneData)) {
            auto* pClone = static_cast<bhkShape*>(pShape->CreateClone(arCloneData));
            pkCinfo->m_shape = pClone ? static_cast<hkpShape*>(pClone->phkObject) : nullptr;
        }
    }
    // Clears the filter info's high half, the collision group; the low half is kept unchanged.
    pkCinfo->m_collisionFilterInfo &= 0xFFFFu;
    bhkSerializable::CopyMembers(apDest, arCloneData);
}

// Fills arInfo's world-object part from the wrapped object: its shape and broadphase type, an empty property list and
// its collision filter info (zero without an object), inside the Havok memory context.
void bhkWorldObject::CopyCinfo(hkWorldObjectCinfo& arInfo) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    auto* pkObject = static_cast<hkpWorldObject*>(phkObject);
    if (pkObject) {
        arInfo.m_shape = pkObject->m_collidable.m_shape;
        // EXE-INLINED none: hkEnum<E, S>::operator=
        arInfo.m_broadPhaseType.m_storage = pkObject->m_collidable.m_broadPhaseHandle.m_type;
    }
    arInfo.m_properties.m_size = 0;
    // Re-reads the wrapped object here.
    auto* pkObjectRw = static_cast<hkpWorldObject*>(phkObject);
    arInfo.m_collisionFilterInfo = pkObjectRw ? pkObjectRw->m_collidable.getCollisionFilterInfo() : 0;
}

const hkpCollidable* bhkWorldObject::GetCollidable() const {
    hkpWorldObject* pkObject = const_cast<bhkWorldObject*>(this)->Gethk();
    const hkpCollidable* pkCollidable = nullptr;
    if (pkObject) {
        pkObject->markForRead();
        pkCollidable = pkObject->getCollidable();
        pkObject->unmarkForRead();
    }
    return pkCollidable;
}

// Clears the no-collision bit (0x4000) of the wrapped object's collision filter info when abEnable is true, else sets
// it, and tells the world the filter info changed.
void bhkWorldObject::EnableCollision(bool abTF) {
    auto* pkObject = static_cast<hkpWorldObject*>(phkObject);
    uint32_t uiFilter = pkObject ? pkObject->m_collidable.getCollisionFilterInfo() : 0;
    if (abTF) {
        uiFilter &= ~0x4000u;
    }
    else {
        uiFilter |= 0x4000u;
    }
    if (pkObject) {
        pkObject->m_collidable.m_broadPhaseHandle.m_collisionFilterInfo = uiFilter;
    }
    UpdateCFilter();
}

// The scene graph object bound to the wrapped object under the property aiKey, null when the object has no such
// property; a zero key stands for the scene graph base's RTTI.
NiAVObject* bhkWorldObject::GetAVObject(uint32_t aiKey) {
    // A zero key stands for the address of NiAVObject::ms_RTTI, used as a raw untyped word.
    if (!aiKey) {
        aiKey = bhkPropertyKey(NiAVObject::ms_RTTI);
    }
    auto* pkObject = static_cast<hkpWorldObject*>(phkObject);
    if (!pkObject) {
        return nullptr;
    }
    // The size is compared signed.
    int32_t iSize = pkObject->m_properties.m_size;
    for (int32_t i = 0; i < iSize; ++i) {
        if (pkObject->m_properties.m_data[i].m_key == aiKey) {
            // The value's low word is the object SetAVObject stored.
            return static_cast<NiAVObject*>(
                pkObject->getProperty(aiKey, hkpWorldObject::MULTI_THREADING_CHECKS_ENABLE).getPtr()
            );
        }
    }
    return nullptr;
}

CFilter* bhkWorldObject::GetCollisionFilter(CFilter* apResult) const {
    const hkpCollidable* pkCollidable = GetCollidable();
    apResult->iFilter = pkCollidable ? pkCollidable->getCollisionFilterInfo() : 0;
    return apResult;
}

void bhkWorldObject::SetCollisionGroup(uint32_t aiGroup) {
    auto* pkObject = static_cast<hkpWorldObject*>(phkObject);
    uint32_t uiFilter = pkObject ? pkObject->m_collidable.getCollisionFilterInfo() : 0;
    uiFilter = static_cast<uint16_t>(uiFilter) | (aiGroup << 16);
    if (pkObject) {
        pkObject->m_collidable.m_broadPhaseHandle.m_collisionFilterInfo = uiFilter;
    }
    UpdateCFilter();
}

bool bhkWorldObject::HasProperty(uint32_t aiKey) {
    auto* pkObject = static_cast<hkpWorldObject*>(phkObject);
    if (!pkObject) {
        return false;
    }
    for (int32_t i = 0; i < pkObject->m_properties.m_size; ++i) {
        if (pkObject->m_properties.m_data[i].m_key == aiKey) {
            return true;
        }
    }
    return false;
}

// Binds the scene graph object apObject to the wrapped object under the property aiKey. Walks the property list itself
// and unbinds the key it finds before it binds the new value; a zero key stands for the scene graph base's RTTI.
void bhkWorldObject::SetAVObject(NiAVObject* apObject, uint32_t aiKey) {
    // A zero key stands for the RTTI object's address.
    if (!aiKey) {
        aiKey = bhkPropertyKey(NiAVObject::ms_RTTI);
    }
    auto* pkObject = static_cast<hkpWorldObject*>(phkObject);
    if (pkObject) {
        // The size is compared signed.
        int32_t iSize = pkObject->m_properties.m_size;
        for (int32_t i = 0; i < iSize; ++i) {
            if (pkObject->m_properties.m_data[i].m_key == aiKey) {
                pkObject->removeProperty(aiKey);
                break;
            }
        }
    }
    if (apObject) {
        // Re-reads the wrapped object here.
        auto* pkObjectRw = static_cast<hkpWorldObject*>(phkObject);
        if (pkObjectRw) {
            // The object pointer is handed over as the property value, zero-extended to 64 bits.
            hkpPropertyValue kValue;
            kValue.m_data = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(apObject));
            pkObjectRw->addProperty(aiKey, kValue);
        }
    }
}

// Drops the reference the wrapped object's shape holds on its bhkShape wrapper, inside the Havok memory context: the
// shape's user data is that wrapper, and it is deleted when its count reaches zero.
void bhkWorldObject::RembhkShapeRef() {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (this) {
        auto* pkObject = static_cast<hkpWorldObject*>(phkObject);
        if (pkObject) {
            const hkpShape* pkShape = pkObject->m_collidable.m_shape;
            if (pkShape) {
                bhkShape* pkUserData = pkShape->m_userData;
                if (pkUserData) {
                    NiDecRefCount(pkUserData);
                }
            }
        }
    }
}

bhkShape* bhkWorldObject::GetShape() {
    hkpShape* v1 = GethkShape();
    return bhkShape::Getbhk(v1);
}

hkpShape* bhkWorldObject::GethkShape() {
    hkpLinkedCollidable* collidableRw = static_cast<hkpLinkedCollidable*>(GetCollidableRw());
    if (collidableRw) {
        return const_cast<hkpShape*>(collidableRw->m_shape);
    }

    return nullptr;
}

hkpCollidable* bhkWorldObject::GetCollidableRw() {
    hkpLinkedCollidable* pCollideRw = nullptr;
    hkpWorldObject* pwo = operator*();
    if (pwo) {
        pCollideRw = &pwo->m_collidable;
    }
    return pCollideRw;
}

uint32_t bhkWorldObject::GetFlag(int32_t a2) {
    return a2 & iFlags;
}

int32_t bhkWorldObject::IsFlagSet(int32_t a2) {
    return (int32_t)((uint8_t)(GetFlag(a2) != 0));
}

hkpPropertyValue*
bhkWorldObject::GetProperty(hkpPropertyValue* apResult, const hkpWorldObject* apObject, uint32_t auiKey) {
    // The value is pointer-sized: intptr_t is the low word, sign-extended, on 32 bits and the whole value, which can
    // hold an address, on 64.
    intptr_t iData = 0;
    if (apObject) {
        iData =
            static_cast<intptr_t>(apObject->getProperty(auiKey, hkpWorldObject::MULTI_THREADING_CHECKS_ENABLE).m_data);
    }
    apResult->m_data = static_cast<uint64_t>(static_cast<int64_t>(iData));
    return apResult;
}
