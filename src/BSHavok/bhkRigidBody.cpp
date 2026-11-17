#include "BSHavok/bhkRigidBody.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/ahkpWorld.h"
#include "BSHavok/bhkAction.h"
#include "BSHavok/bhkBlendCollisionObject.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkConstraint.h"
#include "BSHavok/bhkConvert.h"
#include "BSHavok/bhkEntity.h"
#include "BSHavok/bhkFixedConstraint.h"
#include "BSHavok/bhkLiquidAction.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkRigidBody551Cinfo.h"
#include "BSHavok/bhkRigidBodyCinfo.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/BSCloningProcess.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSStream.h"
#include "BSMain/BSTestObjects.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSMenu/TileMenu.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSDecalManager.h"
#include "Fallout Shared/BGSDecalNode.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/CameraCaster.h"
#include "Fallout Shared/Havok/DecalCaster.h"
#include "Fallout Shared/Havok/FOCollisionListener.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/NiTreeCtrl.h"
#include "Fallout Shared/Pathfinding/NavMeshRender.h"
#include "Fallout Shared/Pathfinding/PathBuilder.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/Region System/TESRegionList.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/ScreenSplatter.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESImageSpaceModifier.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESRace.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScriptFunctions.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatCoverMap.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/Combat/CombatTargetMap.h"
#include "Fallout/AI/Combat/CombatThreatMap.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Audio/ImpactMixer.h"
#include "Fallout/Interface/FOPipboyManager.h"
#include "Fallout/Interface/FORenderedMenuRaceSex.h"
#include "Fallout/Interface/FaderManager.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/LoadingMenu.h"
#include "Fallout/Interface/Menus/VATSMenu.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadManager.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBodyCinfo.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Reconstruction.hpp"

#include <bit>
#include <cmath>
#include <cstdint>
#include <xmmintrin.h>

namespace bhkConvert {} // namespace bhkConvert

NiRTTI bhkRigidBody::ms_RTTI("bhkRigidBody", &bhkEntity::ms_RTTI);

// Frees an aligned block: the byte before apMemory holds its offset from the start of the allocation.
void AlignedFreeOffset(void* apMemory) {
    if (apMemory) {
        uint8_t* pBytes = static_cast<uint8_t*>(apMemory);
        MemoryManager_Singleton.Deallocate(pBytes - pBytes[-1]);
    }
}

// The total mass and centre of mass of a1's bodies, summed by bhkRigidBody::CalculateCOMAndMassRecursive from zero.
void GetTotalMassData(NiAVObject* a1, hkVector4* arMassCenter, float* a3) {
    float fMass = 0.0f;
    alignas(16) hkVector4 kMassCenter;
    kMassCenter.set(0.0f, 0.0f, 0.0f, 0.0f);
    bhkRigidBody::CalculateCOMAndMassRecursive(a1, &kMassCenter.m_quad, &fMass);
    *arMassCenter = kMassCenter;
    *a3 = fMass;
}

NiQuaternion* HK2NI_QUATERNION(NiQuaternion* a1, hkQuaternion* a2) {
    a1->SetX(a2->operator()(0));
    a1->SetY(a2->operator()(1));
    a1->SetZ(a2->operator()(2));
    a1->SetW(a2->operator()(3));
    return a1;
}

bhkRigidBody::bhkRigidBody() {
    ++iCount;
}

bhkRigidBody::bhkRigidBody(bhkRigidBodyCinfo& arInfo) {
    bhkRigidBody::Init(arInfo);
    ++iCount;
}

bhkRigidBody::~bhkRigidBody() {
    bhkRigidBody::Kill();
    --iCount;
}

// Charges the thread's allocations to Havok's heap context, holds the clone critical section, switches the Havok
// assertions off around the copy and builds the clone the way CreateObject does.
NiObject* bhkRigidBody::CreateClone(NiCloningProcess& arCloneData) {
    int32_t& iContext = GetThreadMemContext();
    int32_t iSavedContext = iContext;
    iContext = MEM_CONTEXT_HAVOK;
    bhkSerializable::CloneCritLock();
    SetHavokAssertState(false);
    bhkRigidBody* pClone = static_cast<bhkRigidBody*>(NiMemObject::operator new(sizeof(bhkRigidBody)));
    if (pClone) {
        ::new (pClone) bhkRigidBody;
    }
    CopyMembers(pClone, arCloneData);
    SetHavokAssertState(true);
    bhkSerializable::CloneCritUnlock();
    iContext = iSavedContext;
    return pClone;
}

// The base's data, then the body settings a file may leave out: the linear velocity is capped and, for a body with no
// mass and no dynamic motion, the motion type follows the collision filter's material. The creation info comes from the
// virtual CreateCinfo and is byte-swapped in place when the file's order is not this machine's, and the flags are read
// straight from the stream's read callback.
void bhkRigidBody::LoadBinary(NiStream& arStream) {
    bhkEntity::LoadBinary(arStream);
    bool bCreated;
    bhkRigidBodyCinfo* pCinfo = &static_cast<bhkRigidBodyCinfo&>(CreateCinfo(bCreated));
    if (!arStream.m_bSourceIsLittleEndian) {
        pCinfo->Endian();
    }
    // The store is skipped for less, equal and NaN, so only a strictly greater velocity is capped.
    if (pCinfo->hk.m_maxLinearVelocity > 1000.0f) {
        pCinfo->hk.m_maxLinearVelocity = 1000.0f;
    }
    if (pCinfo->hk.m_mass == 0.0f && GetDynamicMass() == 0.0) {
        uint32_t uiMaterial = pCinfo->m_collisionFilterInfo & 0x7F;
        if (uiMaterial != 2 && uiMaterial != 28) {
            pCinfo->hk.m_motionType.m_storage = hkpMotion::MOTION_FIXED;
        }
        if (uiMaterial == 14) {
            pCinfo->hk.m_motionType.m_storage = hkpMotion::MOTION_KEYFRAMED;
        }
    }
    arStream.ReadMultipleLinkIDs();
    uint32_t uiSize = 4;
    arStream.Istr()->BinaryRead(&iFlags, 4, &uiSize, 1);
}

void bhkRigidBody::LinkObject(NiStream& arStream) {
    uint32_t v4;
    bhkLiquidAction* objectFromLinkID;
    bhkWorldObject::LinkObject(arStream);
    uint32_t numberOfLinkIDs = (&arStream)->GetNumberOfLinkIDs();
    if (numberOfLinkIDs) {
        v4 = numberOfLinkIDs;
        do {
            objectFromLinkID = static_cast<bhkLiquidAction*>((&arStream)->GetObjectFromLinkID());
            if (objectFromLinkID) {
                AddAction(objectFromLinkID);
            }
            --v4;
        }
        while (v4);
    }
}

bool bhkRigidBody::RegisterStreamables(NiStream& arStream) {
    BSSimpleList<NiPointer<bhkSerializable>>* i;
    uint8_t v3 = (uint8_t)bhkWorldObject::RegisterStreamables(arStream);
    if (v3) {
        for (i = &kActionConstrants; i; i = i->m_pkNext) {
            if (!i->m_item) {
                break;
            }
            i->m_item->RegisterStreamables(arStream);
        }
    }
    return (bool)v3;
}

void bhkRigidBody::SaveBinary(NiStream& arStream) {
    // CreateCinfo's out flag; it is never read.
    bool bCreated;
    int32_t v14;
    uint32_t v16;
    bhkRigidBodyCinfo* cinfo = static_cast<bhkRigidBodyCinfo*>(&CreateCinfo(bCreated));
    NiStream* v4 = &arStream;
    bool m_bSaveLittleEndian = arStream.m_bSaveLittleEndian;
    if (!m_bSaveLittleEndian) {
        cinfo->Endian();
    }
    bhkEntity::SaveBinary(*v4);
    // The list head's address is tested for null before each walk. clang assumes &kActionConstrants is non-null and
    // drops those tests; the empty asm hides the pointer from it.
    BSSimpleList<NiPointer<bhkSerializable>>* pHead = &kActionConstrants;
    __asm__("" : "+r"(pHead));
    BSSimpleList<NiPointer<bhkSerializable>>* p_kActionConstrants = pHead;
    BSSimpleList<NiPointer<bhkSerializable>>* v7 = pHead;
    int32_t v8 = 0;
    if (pHead != nullptr) {
        do {
            if (!v7->m_item) {
                break;
            }
            v7 = v7->GetNext();
            ++v8;
        }
        while (v7);
    }
    NiBinaryStream* m_pkOstr = v4->Ostr();
    v14 = v8;
    NiBinaryStream::NIBINARYSTREAMWRITEFN m_pfnWrite = m_pkOstr->m_pfnWrite;
    v16 = 4;
    m_pkOstr->m_uiAbsoluteCurrentPos += m_pfnWrite(m_pkOstr, &v14, 4, &v16, 1);
    if (pHead != nullptr) {
        do {
            if (!p_kActionConstrants->m_item) {
                break;
            }
            v4->SaveLinkID(p_kActionConstrants->m_item);
            p_kActionConstrants = p_kActionConstrants->GetNext();
        }
        while (p_kActionConstrants);
    }
    NiBinaryStream* v11 = v4->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v12 = v11->m_pfnWrite;
    v16 = 4;
    v11->m_uiAbsoluteCurrentPos += v12(v11, &iFlags, 4, &v16, 1);
    if (!v4->m_bSaveLittleEndian) {
        cinfo->Endian();
    }
}

// Equal when the world objects are and their flags match.
bool bhkRigidBody::IsEqual(NiObject* apObject) {
    if (!bhkWorldObject::IsEqual(apObject)) {
        return false;
    }
    return iFlags == static_cast<bhkRigidBody*>(apObject)->iFlags;
}

// Adds the base's lines, the class name, the creation info's position, rotation, centre of mass, mass, damping,
// friction, restitution, motion type, velocity limits, velocities, inertia diagonal, penetration depth and quality
// type, the number of actions and constraints and one "ACTCON" line per action or constraint with its class name and
// address, inside the Havok memory context.
void bhkRigidBody::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    NiPoint3 pnt;
    NiPoint3 kinertia;
    alignas(16) NiQuaternion rot;
    AutoMemContext tempMemContext(MEM_CONTEXT_HAVOK);
    bhkEntity::GetViewerStrings(apStrings);
    char* pcString = NiGetViewerString(bhkRigidBody::ms_RTTI.GetName());
    // EXE-INLINED none: NiTArray<char*>::Add
    uint32_t uiIndex = apStrings->m_usSize;
    if (uiIndex >= apStrings->m_usMaxSize) {
        apStrings->SetSize(uiIndex + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(uiIndex, pcString);
    FNVD_INLINED_LOCAL(bhkRigidBodyCinfo, ci, std::nullopt, std::nullopt);
    ::new (&ci) bhkRigidBodyCinfo;
    CopyCinfo(ci);
    HK2NI(pnt, ci.hk.m_position);
    rot.m_fX = ci.hk.m_rotation.m_vec.m_quad[0];
    rot.m_fY = ci.hk.m_rotation.m_vec.m_quad[1];
    rot.m_fZ = ci.hk.m_rotation.m_vec.m_quad[2];
    rot.m_fW = ci.hk.m_rotation.m_vec.m_quad[3];
    pcString = pnt.GetViewerString("Position");
    apStrings->Add(pcString);
    pcString = rot.GetViewerString("Rotation");
    apStrings->Add(pcString);
    HK2NI(pnt, ci.hk.m_centerOfMass);
    pcString = pnt.GetViewerString("COM");
    apStrings->Add(pcString);
    pcString = NiGetViewerString("MASS", ci.hk.m_mass);
    apStrings->Add(pcString);
    pcString = NiGetViewerString("LINDAMP", ci.hk.m_linearDamping);
    apStrings->Add(pcString);
    pcString = NiGetViewerString("ANGDAMP", ci.hk.m_angularDamping);
    apStrings->Add(pcString);
    pcString = NiGetViewerString("FRICTION", ci.hk.m_friction);
    apStrings->Add(pcString);
    pcString = NiGetViewerString("REST", ci.hk.m_restitution);
    apStrings->Add(pcString);
    bhkRigidBody::AddMotionType(apStrings, static_cast<hkpMotion::MotionType>(ci.hk.m_motionType.m_storage));
    pcString = NiGetViewerString("MAXLINVEL", ci.hk.m_maxLinearVelocity);
    apStrings->Add(pcString);
    pcString = NiGetViewerString("MAXANGVEL", ci.hk.m_maxAngularVelocity);
    apStrings->Add(pcString);
    HK2NI(pnt, ci.hk.m_linearVelocity);
    pcString = pnt.GetViewerString("LinVel");
    apStrings->Add(pcString);
    HK2NI(pnt, ci.hk.m_angularVelocity);
    pcString = pnt.GetViewerString("AngVel");
    apStrings->Add(pcString);
    // The inertia tensor's diagonal, read from a copy of the matrix as the game does.
    hkMatrix3 kinertiaMatrix = ci.hk.m_inertiaTensor;
    kinertia.x = kinertiaMatrix.m_col0.m_quad[0];
    kinertia.y = kinertiaMatrix.m_col1.m_quad[1];
    kinertia.z = kinertiaMatrix.m_col2.m_quad[2];
    pcString = kinertia.GetViewerString("InertiaTensor");
    apStrings->Add(pcString);
    pcString = NiGetViewerString("PENDEPTH", ci.hk.m_allowedPenetrationDepth);
    apStrings->Add(pcString);
    pcString = NiGetViewerString("QUALITYTYPE", ci.GetQualityTypeString());
    apStrings->Add(pcString);
    // EXE-INLINED 0xC8D0D0
    uint32_t uiCount = 0;
    for (BSSimpleList<NiPointer<bhkSerializable>>* pCount = &kActionConstrants; pCount; ++uiCount) {
        if (!pCount->m_item) {
            break;
        }
        pCount = pCount->GetNext();
    }
    pcString = NiGetViewerString("ACTCONCOUNT", uiCount);
    apStrings->Add(pcString);
    for (BSSimpleList<NiPointer<bhkSerializable>>* plist = &kActionConstrants; plist; plist = plist->GetNext()) {
        bhkSerializable* pItem = plist->m_item.m_pObject;
        if (!pItem) {
            break;
        }
        char pstr[128];
        BSsprintf(pstr, 128, "%s: 0x%8X", pItem->GetRTTI()->GetName(), pItem);
        pcString = NiGetViewerString("ACTCON", static_cast<const char*>(pstr));
        // EXE-INLINED none: NiTArray<char*>::Add
        uiIndex = apStrings->m_usSize;
        if (uiIndex >= apStrings->m_usMaxSize) {
            apStrings->SetSize(uiIndex + apStrings->m_usGrowBy);
        }
        // EXE-INLINED 0x96AE90
        if (uiIndex >= apStrings->m_usSize) {
            apStrings->m_usSize = static_cast<uint16_t>(uiIndex + 1);
            if (pcString) {
                ++apStrings->m_usESize;
            }
        }
        else if (pcString) {
            if (!apStrings->m_pBase[uiIndex]) {
                ++apStrings->m_usESize;
            }
        }
        else if (apStrings->m_pBase[uiIndex]) {
            --apStrings->m_usESize;
        }
        apStrings->m_pBase[uiIndex] = pcString;
    }
    // EXE-INLINED 0x56D730
    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
    ci.m_properties.m_size = 0;
    if (ci.m_properties.m_capacityAndFlags >= 0) {
        pHeap->bufFree(
            ci.m_properties.m_data,
            sizeof(hkWorldObjectCinfo::Property) *
                (ci.m_properties.m_capacityAndFlags & hkArrayBase<hkWorldObjectCinfo::Property>::CAPACITY_MASK)
        );
    }
}

// Adds the body to apWorld, with its action constraints. A keyframed blend body (both gains 1) is not added unless
// it is forced or bAddBipedWhenKeyframed is set, and a body flagged 0x40 is not added: either is removed from its
// world instead, and true is returned.
bool bhkRigidBody::Add(bhkWorld* apWorld) {
    // EXE-INLINED 0x4AE750
    hkpWorldObject* phkWorldObject = this ? static_cast<hkpWorldObject*>(phkObject) : nullptr;
    // The property is keyed by the collision object's RTTI address.
    // EXE-INLINED 0x4B5A50
    hkpPropertyValue kProperty;
    bhkNiCollisionObject* pcol = static_cast<bhkNiCollisionObject*>(
        bhkWorldObject::GetProperty(&kProperty, phkWorldObject, bhkPropertyKey(bhkNiCollisionObject::ms_RTTI))->getPtr()
    );
    bhkBlendCollisionObject* pblendCol = nullptr;
    bool bblendKeyframed = false;
    if (pcol) {
        pblendCol = pcol->IsBhkBlendCollisionObject();
        if (pblendCol && 1.0f == pblendCol->fHeirGain && 1.0f == pblendCol->fVelGain) {
            bblendKeyframed = true;
        }
    }
    if ((bblendKeyframed && pblendCol->iForceAddCount <= 0 && !bhkWorld::bAddBipedWhenKeyframed) || (iFlags & 0x40)) {
        if (bhkWorldObject::GethkWorld()) {
            Remove();
        }
        if (pblendCol) {
            pblendCol->pStoredWorld = apWorld;
        }
        return true;
    }
    bool bres = bhkWorldObject::Add(apWorld);
    bool bAddActCon = bres;
    if (!bres) {
        hkpRigidBody* pBody = static_cast<hkpRigidBody*>(phkObject);
        if (pBody) {
            MarkRead();
            bool bInWorld = pBody->m_world != nullptr;
            UnMarkRead();
            // Reloads the body and reads its motion type zero-extended.
            hkpRigidBody* pBodyAgain = static_cast<hkpRigidBody*>(phkObject);
            if (bInWorld && pBodyAgain) {
                int32_t iMotion = static_cast<uint8_t>(pBodyAgain->m_motion.m_type.m_storage);
                if (iMotion > hkpMotion::MOTION_INVALID &&
                    (iMotion <= hkpMotion::MOTION_BOX_INERTIA || iMotion == hkpMotion::MOTION_THIN_BOX_INERTIA)) {
                    bAddActCon = true;
                }
            }
        }
    }
    if (bAddActCon && (!bblendKeyframed || pblendCol->iForceAddCount > 0) && !(iFlags & 0x40)) {
        AddActConToWorld(apWorld);
    }
    if (pblendCol) {
        pblendCol->pStoredWorld = apWorld;
    }
    return bres;
}

bool bhkRigidBody::Remove() {
    iFlags &= 0xFFFFFF77u;
    RemoveActionConstraintsFromWorld();
    return bhkEntity::Remove();
}

// The scope is charged to the Havok heap context; the info is freed through hkWorldObjectCinfo's deleting destructor,
// called directly.
void bhkRigidBody::KillCinfo(bool abCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (abCreated) {
        if (pInfo) {
            delete static_cast<hkWorldObjectCinfo*>(pInfo);
        }
        pInfo = nullptr;
    }
}

// The file's creation info is the 32-bit program's structure: its serialized form is read (0xE0 bytes) and the native
// info filled from it. The game's files (stream version 34) hold the old Havok 5.5.1 form, which is read into kOldCInfo
// and converted; newer files hold bhkRigidBodyCinfo's. A short read leaves the form zeroed.
int32_t bhkRigidBody::LoadCInfo(NiStream& arStream) {
    uint32_t uiSize = QCinfoSize();
    bool bCreated;
    bhkRigidBodyCinfo* pInfo = static_cast<bhkRigidBodyCinfo*>(&CreateCinfo(bCreated));
    if (arStream.m_BSStreamHeader.uiVersion < 35) {
        FNVD_INLINED_LOCAL(bhkRigidBody551Cinfo, kOldCInfo, std::nullopt, std::nullopt);
        bhkRigidBody551Cinfo::Serialized kForm{};
        arStream.Istr()->BinaryRead(&kForm, sizeof(kForm), nullptr, 0);
        kOldCInfo.ReadSerialized(kForm);
        pInfo->ConvertFromOld(&kOldCInfo);
        // EXE-INLINED 0x57C530
        hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
        kOldCInfo.m_properties.m_size = 0;
        if (kOldCInfo.m_properties.m_capacityAndFlags >= 0) {
            pHeap->bufFree(
                kOldCInfo.m_properties.m_data,
                sizeof(hkWorldObjectCinfo::Property) *
                    (kOldCInfo.m_properties.m_capacityAndFlags &
                     hkArrayBase<hkWorldObjectCinfo::Property>::CAPACITY_MASK)
            );
        }
        alignas(16) char acMessage[256];
        BSsprintf(
            acMessage, 256,
            "File %s contains an old hkpRigidBody! Converting cinfo at runtime is slow and should be done by the build "
            "process.",
            arStream.m_acFileName
        );
        hkError::s_instance->message(
            hkError::MESSAGE_REPORT, 0x234F2250, acMessage,
            "D:\\_Fallout3\\Platforms\\Common\\Code\\BSHavok\\bhkRigidBody.cpp", 544
        );
        return uiSize;
    }
    else {
        bhkRigidBodyCinfo::Serialized kForm{};
        arStream.Istr()->BinaryRead(&kForm, sizeof(kForm), nullptr, 0);
        pInfo->ReadSerialized(kForm);
        return uiSize;
    }
}

// Builds the Havok body from the creation info, which it first clamps in place: the penetration depth into
// (FLT_EPSILON, 0.2], the maximum linear velocity to at least 1000, the restitution below 1.99. A body with mass that
// is keyframed or fixed is built dynamic and switched to its motion type afterwards; the solver deactivation falls back
// to low and the contact point properties hold 3 shape keys. A dynamic body whose smallest extent exceeds
// bhkWorld::fDebrisMinExtent, or whose scaled maximum linear velocity stays below bhkWorld::fDebrisMaxVelocity, gets
// the debris quality, else a debris one goes back to moving.
void bhkRigidBody::Init(const hkSerializableCinfo& arInfo) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    // The game's cinfo is its rigid body info, which is clamped in place.
    bhkRigidBodyCinfo& kInfo = const_cast<bhkRigidBodyCinfo&>(static_cast<const bhkRigidBodyCinfo&>(arInfo));
    int32_t iMotionType = hkpMotion::MOTION_MAX_ID;
    if (1.1920929e-07f >= kInfo.hk.m_allowedPenetrationDepth ||
        kInfo.hk.m_allowedPenetrationDepth > 0.20000000298023224) {
        kInfo.hk.m_allowedPenetrationDepth = 0.15f;
    }
    if (kInfo.hk.m_maxLinearVelocity < 1000.0) {
        kInfo.hk.m_maxLinearVelocity = 1000.0f;
    }
    if (kInfo.hk.m_restitution >= 1.9900000095367432) {
        kInfo.hk.m_restitution = 1.98f;
    }
    if (0.0f == kInfo.hk.m_mass) {
        if (kInfo.hk.m_motionType.m_storage == hkpMotion::MOTION_FIXED) {
            kInfo.hk.m_qualityType.m_storage = HK_COLLIDABLE_QUALITY_FIXED;
        }
    }
    else {
        int32_t iCinfoMotion = kInfo.hk.m_motionType.m_storage;
        if (iCinfoMotion == hkpMotion::MOTION_KEYFRAMED || iCinfoMotion == hkpMotion::MOTION_FIXED) {
            iMotionType = iCinfoMotion;
            kInfo.hk.m_motionType.m_storage = hkpMotion::MOTION_DYNAMIC;
            kInfo.hk.m_qualityType.m_storage = HK_COLLIDABLE_QUALITY_MOVING;
        }
        else if (kInfo.hk.m_qualityType.m_storage == HK_COLLIDABLE_QUALITY_INVALID) {
            kInfo.hk.m_qualityType.m_storage = HK_COLLIDABLE_QUALITY_MOVING;
        }
    }
    int8_t cSolverDeactivation = kInfo.hk.m_solverDeactivation.m_storage;
    if (cSolverDeactivation >= hkpRigidBodyCinfo::SOLVER_DEACTIVATION_MAX ||
        cSolverDeactivation <= hkpRigidBodyCinfo::SOLVER_DEACTIVATION_INVALID) {
        kInfo.hk.m_solverDeactivation.m_storage = hkpRigidBodyCinfo::SOLVER_DEACTIVATION_LOW;
    }
    kInfo.hk.m_numShapeKeysInContactPointProperties = 3;
    kInfo.hk.m_collisionFilterInfo = kInfo.m_collisionFilterInfo;
    kInfo.hk.m_shape = kInfo.m_shape;
    void* pMemory = hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpRigidBody));
    static_cast<hkReferencedObject*>(pMemory)->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkpRigidBody));
    hkpRigidBody* pBody = ::new (pMemory) hkpRigidBody(kInfo.hk);
    if (iMotionType != hkpMotion::MOTION_MAX_ID) {
        kInfo.hk.m_motionType.m_storage = static_cast<int8_t>(iMotionType);
        pBody->setMotionType(
            static_cast<hkpMotion::MotionType>(iMotionType), HK_ENTITY_ACTIVATION_DO_ACTIVATE,
            HK_UPDATE_FILTER_ON_ENTITY_FULL_CHECK
        );
    }
    SethkObject(pBody);
    pBody->removeReference();
    int8_t cMotionType = kInfo.hk.m_motionType.m_storage;
    if (cMotionType > hkpMotion::MOTION_INVALID &&
        (cMotionType <= hkpMotion::MOTION_BOX_INERTIA || cMotionType == hkpMotion::MOTION_THIN_BOX_INERTIA)) {
        hkpRigidBody* pRigidBody = static_cast<hkpRigidBody*>(phkObject);
        if (pRigidBody) {
            MarkRead();
            // EXE-INLINED none: hkpWorldObject::getCollidable
            // The exe adds 0x10 and tests the result for null (`add edi, 0x10; test edi, edi`). clang assumes
            // &pRigidBody->m_collidable is non-null and drops that test; the empty asm hides the pointer from it.
            hkpCollidable* pCollidable = &pRigidBody->m_collidable;
            __asm__("" : "+r"(pCollidable));
            UnMarkRead();
            if (pCollidable) {
                // The box is not default-constructed: it is left for GetAabbLocal to fill. It must be 16-aligned.
                hkAabb kAabb;
                GetAabbLocal(kAabb);
                hkVector4 kExtent;
                kExtent.setSub4(kAabb.m_max, kAabb.m_min);
                // The index of the smallest extent.
                int32_t iAxis;
                if (kExtent(1) < kExtent(0)) {
                    iAxis = kExtent(2) < kExtent(1) ? 2 : 1;
                }
                else {
                    iAxis = kExtent(2) < kExtent(0) ? 2 : 0;
                }
                bool bDebris = bhkWorld::fDebrisMinExtent < kExtent(iAxis);
                if (!bDebris) {
                    float fVelocity = GethkMaxLinearVelocity();
                    float fScaledVelocity = bhkConvert::fHk2BSScaleSC * fVelocity;
                    bDebris = bhkWorld::fDebrisMaxVelocity > fScaledVelocity;
                }
                if (bDebris) {
                    pCollidable->m_broadPhaseHandle.m_objectQualityType = HK_COLLIDABLE_QUALITY_DEBRIS;
                    kInfo.hk.m_qualityType.m_storage = HK_COLLIDABLE_QUALITY_DEBRIS;
                }
                else if (kInfo.hk.m_qualityType.m_storage == HK_COLLIDABLE_QUALITY_DEBRIS) {
                    pCollidable->m_broadPhaseHandle.m_objectQualityType = HK_COLLIDABLE_QUALITY_MOVING;
                    kInfo.hk.m_qualityType.m_storage = HK_COLLIDABLE_QUALITY_MOVING;
                }
            }
        }
    }
    PostInit(arInfo);
}

hkSerializableCinfo& bhkRigidBody::CreateCinfo(bool& arCreated) {
    TLSData* pTLSData = GetThreadTLSData();
    MEM_CONTEXT eMemContext = pTLSData->eMemContext;
    pTLSData->eMemContext = MEM_CONTEXT_HAVOK;
    bool bCreate = pInfo == nullptr;
    arCreated = bCreate;
    if (bCreate) {
        // The cinfo's 16-aligned operator new: the padding length goes in the byte before the cinfo.
        uint8_t* pMemory = static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(sizeof(bhkRigidBodyCinfo) + 16));
        uint8_t uiPadding = static_cast<uint8_t>(16 - (reinterpret_cast<uintptr_t>(pMemory) & 0xF));
        pMemory[uiPadding - 1] = uiPadding;
        bhkRigidBodyCinfo* pNewInfo = ::new (pMemory + uiPadding) bhkRigidBodyCinfo;
        pInfo = pNewInfo;
        if (phkObject) {
            CopyCinfo(*pNewInfo);
        }
    }
    hkSerializableCinfo* pResult = pInfo;
    pTLSData->eMemContext = eMemContext;
    return *pResult;
}

// Removes the action constraints while the thread's allocations are charged to Havok's heap context, then kills
// the entity and restores the saved context.
void bhkRigidBody::Kill() {
    int32_t& iContext = GetThreadMemContext();
    int32_t iSavedContext = iContext;
    iContext = MEM_CONTEXT_HAVOK;
    kActionConstrants.RemoveAll();
    bhkEntity::Kill();
    iContext = iSavedContext;
}

// Adds the body to apWorld whatever its blend state, with its action constraints when that succeeded.
bool bhkRigidBody::ForceAdd(bhkWorld* apWorld) {
    bool bres = bhkWorldObject::Add(apWorld);
    if (bres) {
        AddActConToWorld(apWorld);
    }
    // EXE-INLINED 0x4AE750
    hkpWorldObject* phkWorldObject = this ? static_cast<hkpWorldObject*>(phkObject) : nullptr;
    // The property is keyed by the collision object's RTTI address.
    // EXE-INLINED 0x4B5A50
    hkpPropertyValue kProperty;
    bhkNiCollisionObject* pcol = static_cast<bhkNiCollisionObject*>(
        bhkWorldObject::GetProperty(&kProperty, phkWorldObject, bhkPropertyKey(bhkNiCollisionObject::ms_RTTI))->getPtr()
    );
    if (pcol) {
        bhkBlendCollisionObject* pblendCol = pcol->IsBhkBlendCollisionObject();
        if (pblendCol) {
            pblendCol->pStoredWorld = apWorld;
        }
    }
    return bres;
}

// Builds the body's debug display geometry under apNode: the base's geometry, with a coordinate jack at the
// centre of mass attached to it.
NiAVObject* bhkRigidBody::CreateDisplayGeometry(NiNode* apNode) {
    NiAVObject* pDisplay = bhkWorldObject::CreateDisplayGeometry(apNode);
    if (pDisplay) {
        hkVector4 com;
        NiPoint3 pnt;
        GetCenterOfMassLocal(com);
        HK2NI(pnt, com);
        NiAVObject* pcom = BSTestObjects::CreateCoordinateJack(6.0f);
        if (pcom) {
            pcom->m_kLocal.m_Translate = pnt;
            // The base returns its display node as an NiAVObject, so it is cast to NiNode to attach the child.
            static_cast<NiNode*>(pDisplay)->AttachChild(pcom, 1);
        }
    }
    return pDisplay;
}

// The body's world position, or the zero vector without a Havok body; also tests this for null.
hkVector4& bhkRigidBody::GetPosition(hkVector4& arPos) {
    hkpRigidBody* pBody;
    if (this && (pBody = static_cast<hkpRigidBody*>(phkObject)) != nullptr) {
        arPos = pBody->m_motion.m_motionState.m_transform.m_translation;
    }
    else {
        arPos = hkVector4Zero;
    }
    return arPos;
}

// The body's rotation, or the identity without a Havok body.
hkQuaternion& bhkRigidBody::GetRotation(hkQuaternion& arRot) {
    hkpRigidBody* pBody;
    if (this && (pBody = static_cast<hkpRigidBody*>(phkObject)) != nullptr) {
        arRot.m_vec = pBody->m_motion.m_motionState.m_sweptTransform.m_rotation1.m_vec;
    }
    else {
        arRot.m_vec = hkQuaternionIdentity.m_vec;
    }
    return arRot;
}

// Warps the Havok body to arPos inside the world's write marks.
void bhkRigidBody::SetPosition(const hkVector4& arPos) {
    if (this) {
        hkpRigidBody* pBody = static_cast<hkpRigidBody*>(phkObject);
        if (pBody) {
            MarkWrite();
            pBody->setPosition(arPos);
            UnMarkWrite();
        }
    }
}

// Warps the Havok body to arRot inside the world's write marks.
void bhkRigidBody::SetRotation(const hkQuaternion& arRot) {
    if (this) {
        hkpRigidBody* pBody = static_cast<hkpRigidBody*>(phkObject);
        if (pBody) {
            MarkWrite();
            pBody->setRotation(arRot);
            UnMarkWrite();
        }
    }
}

// Sets the body's motion type. A biped (layer BIPED or DEADBIP) also gets its layer from the new type: DEADBIP for
// a dynamic one, BIPED otherwise. Returns false for no body, an unchanged type, or a fixed or keyframed body of zero
// dynamic mass made dynamic.
bool bhkRigidBody::SetMotionType(hkpMotion::MotionType aeMotion) {
    uint32_t uiFilter;
    hkpRigidBody* pBody = static_cast<hkpRigidBody*>(phkObject);
    if (!pBody) {
        return false;
    }
    // The address &pBody->m_collidable is never null.
    uiFilter = pBody->m_collidable.m_broadPhaseHandle.m_collisionFilterInfo;
    // Reads the current motion type zero-extended.
    int32_t iCurrent = static_cast<uint8_t>(pBody->m_motion.m_type.m_storage);
    uint32_t uiLayer = uiFilter & 0x7F;
    if (uiLayer == L_BIPED || uiLayer == L_DEADBIP) {
        bool bDynamic = aeMotion > hkpMotion::MOTION_INVALID &&
            (aeMotion <= hkpMotion::MOTION_BOX_INERTIA || aeMotion == hkpMotion::MOTION_THIN_BOX_INERTIA);
        uint32_t uiNewLayer = bDynamic ? L_DEADBIP : L_BIPED;
        CFilter kFilter;
        kFilter.iFilter = ((uiNewLayer ^ uiFilter) & 0x7F) ^ uiFilter;
        if (aeMotion == iCurrent) {
            SetCollisionFilter(kFilter);
            return true;
        }
        MarkWrite();
        pBody->m_collidable.m_broadPhaseHandle.m_collisionFilterInfo = kFilter.iFilter;
        pBody->setMotionType(aeMotion, HK_ENTITY_ACTIVATION_DO_ACTIVATE, HK_UPDATE_FILTER_ON_ENTITY_FULL_CHECK);
        UnMarkWrite();
        return true;
    }
    if (iCurrent == aeMotion) {
        return false;
    }
    if (iCurrent >= hkpMotion::MOTION_KEYFRAMED && iCurrent <= hkpMotion::MOTION_FIXED &&
        bhkRigidBody::IsMotionTypeDynamic(aeMotion) && GetDynamicMass() == 0.0f) {
        return false;
    }
    MarkWrite();
    pBody->setMotionType(aeMotion, HK_ENTITY_ACTIVATION_DO_ACTIVATE, HK_UPDATE_FILTER_ON_ENTITY_FULL_CHECK);
    UnMarkWrite();
    return true;
}

// Warps the Havok body.
void bhkRigidBody::SetPosRot(const hkVector4& arPos, const hkQuaternion& arRot) {
    if (this) {
        hkpRigidBody* pBody = static_cast<hkpRigidBody*>(phkObject);
        if (pBody) {
            bhkWorldObject::GethkWorldRw();
            pBody->setPositionAndRotation(arPos, arRot);
        }
    }
}

// The body's center of mass in its own frame, or the zero vector without a Havok body.
hkVector4& bhkRigidBody::GetCenterOfMassLocal(hkVector4& arCOM) {
    hkpRigidBody* pBody;
    if (this && (pBody = static_cast<hkpRigidBody*>(phkObject)) != nullptr) {
        arCOM = pBody->m_motion.m_motionState.m_sweptTransform.m_centerOfMassLocal;
    }
    else {
        arCOM = hkVector4Zero;
    }
    return arCOM;
}

// The body's center of mass in the world, or the zero vector without a Havok body.
hkVector4& bhkRigidBody::GetCenterOfMassInWorld(hkVector4& arCOM) {
    hkpRigidBody* pBody;
    if (this && (pBody = static_cast<hkpRigidBody*>(phkObject)) != nullptr) {
        arCOM = pBody->m_motion.m_motionState.m_sweptTransform.m_centerOfMass1;
    }
    else {
        arCOM = hkVector4Zero;
    }
    return arCOM;
}

// The body's world transform, or the identity without a Havok body.
hkTransform& bhkRigidBody::GetTransform(hkTransform& arTrans) {
    hkpRigidBody* pBody;
    if (this && (pBody = static_cast<hkpRigidBody*>(phkObject)) != nullptr) {
        arTrans.m_rotation.m_col0 = pBody->m_motion.m_motionState.m_transform.m_rotation.m_col0;
        arTrans.m_rotation.m_col1 = pBody->m_motion.m_motionState.m_transform.m_rotation.m_col1;
        arTrans.m_rotation.m_col2 = pBody->m_motion.m_motionState.m_transform.m_rotation.m_col2;
        arTrans.m_translation = pBody->m_motion.m_motionState.m_transform.m_translation;
    }
    else {
        hkQuadReal kZero = {0.0f, 0.0f, 0.0f, 0.0f};
        arTrans.m_rotation.m_col0.m_quad = kZero;
        arTrans.m_rotation.m_col1.m_quad = kZero;
        arTrans.m_rotation.m_col2.m_quad = kZero;
        arTrans.m_rotation.m_col0.m_quad[0] = 1.0f;
        arTrans.m_rotation.m_col1.m_quad[1] = 1.0f;
        arTrans.m_rotation.m_col2.m_quad[2] = 1.0f;
        arTrans.m_translation.m_quad = kZero;
    }
    return arTrans;
}

// The body's AABB in world space: the shape's own AABB with a 0.001 tolerance, or, without a shape, a 0.001 box around
// the body's origin. The collidable is read between the world's (empty) read marks and tested for null.
void bhkRigidBody::GetAabb(hkAabb& arOut) {
    hkTransform trn;
    GetTransform(trn);
    // EXE-INLINED 0x4AE6C0
    const hkpShape* ps = nullptr;
    hkpRigidBody* pBody = static_cast<hkpRigidBody*>(phkObject);
    if (pBody) {
        MarkRead();
        // EXE-INLINED none: hkpWorldObject::getCollidable
        // The exe adds 0x10 and tests the result for null (`add edi, 0x10; test edi, edi`). clang assumes
        // &pBody->m_collidable is non-null and drops that test; the empty asm hides the pointer from it.
        const hkpCollidable* pCollidable = &pBody->m_collidable;
        __asm__("" : "+r"(pCollidable));
        UnMarkRead();
        if (pCollidable) {
            ps = pCollidable->m_shape;
        }
    }
    if (ps) {
        ps->getAabbImpl(trn, 0.001f, arOut);
    }
    else {
        hkVector4 minPoint;
        minPoint.set(-0.001f, -0.001f, -0.001f, 0.0f);
        hkVector4 maxPoint;
        maxPoint.set(0.001f, 0.001f, 0.001f, 0.0f);
        arOut.m_min.setTransformedPos(trn, minPoint);
        arOut.m_max.setTransformedPos(trn, maxPoint);
    }
}

// Switches the four Havok assertion ids on or off in the error reporter.
void bhkRigidBody::SetHavokAssertState(bool abEnabled) {
    hkError* pError = hkError::s_instance;
    hkBool lEnabled;
    lEnabled.m_bool = abEnabled ? 1 : 0;
    pError->setEnabled(-263989964, lEnabled);
    pError->setEnabled(-265407775, lEnabled);
    pError->setEnabled(-265407774, lEnabled);
    pError->setEnabled(-266259745, lEnabled);
}

void bhkRigidBody::ApplyKeyframe(
    const hkVector4& arNewPos,
    const hkQuaternion& arNewRot,
    float afInvDeltaTime,
    float afVelGain
) {
    __m128 m_quad;
    uint32_t axis_8a;
    uint32_t axis_8b;
    uint32_t axis_8c;
    hkQuaternion inverseOldRotation__nav;
    hkQuaternion v31;
    hkQuaternion dR;
    hkVector4 arVelocity;
    hkVector4 v34;
    hkVector4 c;
    hkQuaternion v36;
    hkVector4* v7 = &GetCenterOfMassLocal(c);
    __m128 v8 = _mm_mul_ps(v7->m_quad, arNewRot.m_vec.m_quad);
    __m128 v9 = _mm_shuffle_ps(v8, v8, 170);
    v9[0] = v9[0] + (_mm_shuffle_ps(v8, v8, 85)[0] + v8[0]);
    __m128 v10 = _mm_shuffle_ps(arNewRot.m_vec.m_quad, arNewRot.m_vec.m_quad, 255);
    __m128 v11 = _mm_add_ps(
        _mm_mul_ps(
            _mm_sub_ps(
                _mm_mul_ps(
                    _mm_shuffle_ps(v7->m_quad, v7->m_quad, 210),
                    _mm_shuffle_ps(arNewRot.m_vec.m_quad, arNewRot.m_vec.m_quad, 201)
                ),
                _mm_mul_ps(
                    _mm_shuffle_ps(v7->m_quad, v7->m_quad, 201),
                    _mm_shuffle_ps(arNewRot.m_vec.m_quad, arNewRot.m_vec.m_quad, 210)
                )
            ),
            v10
        ),
        _mm_add_ps(
            _mm_mul_ps(_mm_add_ps(_mm_mul_ps(v10, v10), hkQuadRealMinusHalf), v7->m_quad),
            _mm_mul_ps(_mm_shuffle_ps(v9, v9, 0), arNewRot.m_vec.m_quad)
        )
    );
    v31.m_vec.m_quad = _mm_mul_ps(
        _mm_shuffle_ps(_mm_set_ss(afInvDeltaTime), _mm_set_ss(afInvDeltaTime), 0),
        _mm_sub_ps(_mm_add_ps(_mm_add_ps(v11, v11), arNewPos.m_quad), (&GetCenterOfMassInWorld(c))->m_quad)
    );
    __m128 v12 = _mm_mul_ps(v31.m_vec.m_quad, v31.m_vec.m_quad);
    float axis_12 = _mm_cvtss_f32(
        _mm_sqrt_ss(_mm_set_ss((_mm_shuffle_ps(v12, v12, 170)[0] + (_mm_shuffle_ps(v12, v12, 85)[0] + v12[0]))))
    );
    float flinCap = GethkMaxLinearVelocity();
    if (flinCap < static_cast<double>(axis_12)) {
        *reinterpret_cast<float*>(&axis_8a) = flinCap / axis_12;
        v31.m_vec.m_quad = _mm_mul_ps(
            _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(axis_8a)), _mm_set_ss(std::bit_cast<float>(axis_8a)), 0),
            v31.m_vec.m_quad
        );
    }
    inverseOldRotation__nav.m_vec.m_quad = _mm_xor_ps((&GetRotation(v36))->m_vec.m_quad, rdata_10C4C00);
    dR.setMul(arNewRot, inverseOldRotation__nav);
    dR.normalize();
    inverseOldRotation__nav.m_vec.m_quad = hkVector4Zero.m_quad;
    float axis_12a = dR.getAngle();
    if (!(axis_12a > 0.0010000000474974513)) {
        m_quad = inverseOldRotation__nav.m_vec.m_quad;
    }
    else {
        dR.getAxis(inverseOldRotation__nav.m_vec);
        *reinterpret_cast<float*>(&axis_8b) = axis_12a * afInvDeltaTime;
        m_quad = _mm_mul_ps(
            _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(axis_8b)), _mm_set_ss(std::bit_cast<float>(axis_8b)), 0),
            inverseOldRotation__nav.m_vec.m_quad
        );
        inverseOldRotation__nav.m_vec.m_quad = m_quad;
    }
    __m128 v14 = _mm_mul_ps(m_quad, m_quad);
    float axis_8 = _mm_cvtss_f32(
        _mm_sqrt_ss(_mm_set_ss((_mm_shuffle_ps(v14, v14, 170)[0] + (_mm_shuffle_ps(v14, v14, 85)[0] + v14[0]))))
    );
    float fangCap = GetMaxAngularVelocity();
    if (fangCap < static_cast<double>(axis_8)) {
        *reinterpret_cast<float*>(&axis_8c) = fangCap / axis_8;
        inverseOldRotation__nav.m_vec.m_quad = _mm_mul_ps(
            _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(axis_8c)), _mm_set_ss(std::bit_cast<float>(axis_8c)), 0),
            inverseOldRotation__nav.m_vec.m_quad
        );
    }
    const hkpWorld* pw = bhkWorldObject::GethkWorld();
    if (pw) {
        v31.m_vec.m_quad[2] = v31.m_vec.m_quad[2] -
            _mm_shuffle_ps(pw->m_gravity.m_quad, pw->m_gravity.m_quad, 170)[0] * afVelGain / afInvDeltaTime;
    }
    hkpRigidBody* phkObject = static_cast<hkpRigidBody*>(this->phkObject);
    hkVector4* p_m_linearVelocity = &phkObject->m_motion.m_linearVelocity;
    if (!phkObject) {
        p_m_linearVelocity = &hkVector4Zero;
    }
    __m128 v18 = _mm_shuffle_ps(_mm_set_ss(afVelGain), _mm_set_ss(afVelGain), 0);
    arVelocity.m_quad = _mm_add_ps(
        _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v18), p_m_linearVelocity->m_quad), _mm_mul_ps(v18, v31.m_vec.m_quad)
    );
    hkVector4* p_m_angularVelocity = &phkObject->m_motion.m_angularVelocity;
    if (!phkObject) {
        p_m_angularVelocity = &hkVector4Zero;
    }
    __m128 v20 = _mm_shuffle_ps(_mm_set_ss(afVelGain), _mm_set_ss(afVelGain), 0);
    v34.m_quad = _mm_add_ps(
        _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v20), p_m_angularVelocity->m_quad),
        _mm_mul_ps(v20, inverseOldRotation__nav.m_vec.m_quad)
    );
    if (phkObject) {
        phkObject->setLinearVelocity(arVelocity);
    }
    hkpRigidBody* v21 = static_cast<hkpRigidBody*>(this->phkObject);
    if (v21) {
        v21->setAngularVelocity(v34);
    }
}

// The dynamic motion's mass, rounded to float.
float bhkRigidBody::GetDynamicMass() {
    hkpMotion* StoredDynamicMotion = GetStoredDynamicMotion();
    float fMass;
    if (StoredDynamicMotion) {
        fMass = StoredDynamicMotion->getMass();
    }
    else {
        fMass = GetMass();
    }
    return fMass;
}

hkpMotion* bhkRigidBody::GetStoredDynamicMotion() {
    hkpRigidBody* v3 = operator*();
    if (v3) {
        return v3->getStoredDynamicMotion();
    }
    else {
        return nullptr;
    }
}

// Returns the wrapped Havok object as the body.
hkpRigidBody* bhkRigidBody::Gethk() const {
    return static_cast<hkpRigidBody*>(phkObject);
}

hkpRigidBody* bhkRigidBody::operator*() const {
    if (this) {
        return Gethk();
    }
    else {
        return nullptr;
    }
}

// Puts a constraint or an action at the head of kActionConstrants, through a NiPointer temporary; anything else is
// ignored.
void bhkRigidBody::AddAction(bhkSerializable* apAction) {
    TLSData* pTLSData = GetThreadTLSData();
    MEM_CONTEXT eMemContext = pTLSData->eMemContext;
    pTLSData->eMemContext = MEM_CONTEXT_HAVOK;
    // EXE-INLINED 0x6532C0
    if (apAction && (apAction->IsKindOf(&bhkConstraint::ms_RTTI) || apAction->IsKindOf(&bhkAction::ms_RTTI))) {
        NiPointer<bhkSerializable> spAction = apAction;
        kActionConstrants.AddHead(spAction);
    }
    pTLSData->eMemContext = eMemContext;
}

void bhkRigidBody::SetActive(bool abTF) {
    hkpRigidBody* v3 = operator*();
    if (v3) {
        bhkWorldObject::MarkWrite();
        if (abTF) {
            v3->activate();
        }
        else {
            v3->deactivate();
        }
        bhkWorldObject::MarkWrite();
    }
}

void bhkRigidBody::CopyCinfo(bhkRigidBodyCinfo& arInfo) {
    bhkWorldObject::CopyCinfo(arInfo);
    hkpRigidBody* phkObject = static_cast<hkpRigidBody*>(this->phkObject);
    if (phkObject) {
        phkObject->getCinfo(arInfo.hk);
        hkpRigidBody* v4 = static_cast<hkpRigidBody*>(this->phkObject);
        if (v4) {
            hkpMotion* pmotion = v4->getStoredDynamicMotion();
            if (pmotion) {
                arInfo.hk.m_mass = pmotion->getMass();
                arInfo.hk.m_centerOfMass = pmotion->m_motionState.m_sweptTransform.m_centerOfMassLocal;
                arInfo.hk.m_linearDamping = pmotion->m_motionState.m_linearDamping;
                arInfo.hk.m_angularDamping = pmotion->m_motionState.m_angularDamping;
                pmotion->getInertiaLocal(arInfo.hk.m_inertiaTensor);
                arInfo.hk.m_centerOfMass = pmotion->m_motionState.m_sweptTransform.m_centerOfMassLocal;
            }
        }
        hkpRigidBody* v7 = static_cast<hkpRigidBody*>(this->phkObject);
        if (v7) {
            // Tests the body's collidable for null, as the game does.
            const hkpCollidable* pCollidable = &v7->m_collidable;
            if (pCollidable) {
                arInfo.hk.m_maxLinearVelocity = GethkMaxLinearVelocity();
                arInfo.hk.m_maxAngularVelocity = GetMaxAngularVelocity();
            }
        }
    }
}

// The body's shape bounds around the identity transform with a 0.001 tolerance, or a 0.001 box around the origin
// without a shape. The collidable is read between the world's (empty) read marks, and both this and the Havok body are
// tested for null.
void bhkRigidBody::GetAabbLocal(hkAabb& arOut) {
    const hkpShape* pShape = nullptr;
    if (this) {
        hkpRigidBody* pBody = static_cast<hkpRigidBody*>(phkObject);
        if (pBody) {
            MarkRead();
            // EXE-INLINED none: hkpWorldObject::getCollidable
            // The exe adds 0x10 and tests the result for null (`add edi, 0x10; test edi, edi`). clang assumes
            // &pBody->m_collidable is non-null and drops that test; the empty asm hides the pointer from it.
            const hkpCollidable* pCollidable = &pBody->m_collidable;
            __asm__("" : "+r"(pCollidable));
            UnMarkRead();
            if (pCollidable) {
                pShape = pCollidable->m_shape;
            }
        }
    }
    if (pShape) {
        pShape->getAabbImpl(hkTransformIdentity, 0.001f, arOut);
    }
    else {
        // EXE-INLINED 0x4B4D50
        arOut.m_min.m_quad = hkQuadReal{-0.001f, -0.001f, -0.001f, 0.0f};
        // EXE-INLINED 0x4B4D50
        arOut.m_max.m_quad = hkQuadReal{0.001f, 0.001f, 0.001f, 0.0f};
    }
}

// The game-unit position arPos, converted to Havok units and handed to the virtual Havok overload. NI2HK's returned
// reference is passed on, not the local.
void bhkRigidBody::SetPosition(const NiPoint3& arPos) {
    hkVector4 kPos;
    SetPosition(NI2HK(kPos, arPos));
}

// The hkVector4 constructor is trivial (it only returns this).
void bhkRigidBody::GetPosition(NiPoint3& arPos) {
    hkVector4 hkPos;
    HK2NI(arPos, GetPosition(hkPos));
}

void bhkRigidBody::GetRotation(NiQuaternion& arRot) {
    hkQuaternion hkRot;
    hkRot.~hkQuaternion();
    ::new (&hkRot) hkQuaternion;
    HK2NI_QUATERNION(&arRot, &GetRotation(hkRot));
}

void bhkRigidBody::SetRotation(const NiQuaternion& arRot) {
    hkQuaternion hkrot;
    hkrot.~hkQuaternion();
    ::new (&hkrot) hkQuaternion;
    NI2HKMIGRATION_QUATERNION(hkrot, arRot);
    hkrot.normalize();
    SetRotation(hkrot);
}

void bhkRigidBody::SethkMaxLinearVelocity(float afVel) {
    hkpRigidBody* result = operator*();
    if (result) {
        result->hkpEntity::setMaxLinearVelocity(afVel);
        return;
    }
}

void bhkRigidBody::CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) {
    bool bCreated;
    bhkRigidBodyCinfo* pInfo = static_cast<bhkRigidBodyCinfo*>(&CreateCinfo(bCreated));
    if ((iFlags & KEYFRAMED_BY_LOADEDAREABOUND) != 0) {
        // EXE-INLINED none: hkEnum<E, S>::operator=
        pInfo->hk.m_motionType.m_storage = hkpMotion::MOTION_DYNAMIC;
    }
    // The cloning process is a BSCloningProcess: its scale's x (+0x10; NiCloningProcess is 0x10 bytes).
    float fScale = static_cast<BSCloningProcess&>(arCloneData).Scale.x;
    if (fScale != 1.0f) {
        pInfo->Scale(fScale);
    }
    pInfo->hk.m_localFrame = static_cast<hkpRigidBody*>(phkObject)->m_localFrame;
    bhkWorldObject::CopyMembers(apDest, arCloneData);
    bhkRigidBody* pDestination = static_cast<bhkRigidBody*>(apDest);
    pDestination->iFlags = (pDestination->iFlags | iFlags) &
        ~static_cast<uint32_t>(UNDERWATER | BATCH_ADD | BATCH_REMOVE | DISABLED | UNDER_PLACEABLE_WATER |
                               STICKING_TARGET | KEYFRAMED_BY_LOADEDAREABOUND);
    hkpLinkedCollidable* pCollidable;
    uint32_t uiFilterInfo;
    hkpRigidBody* pBody = static_cast<hkpRigidBody*>(phkObject);
    if (pBody && (pCollidable = &pBody->m_collidable) != nullptr) {
        uiFilterInfo = pCollidable->m_broadPhaseHandle.m_collisionFilterInfo;
    }
    else {
        uiFilterInfo = 0;
    }
    // Bit 13 of the filter info (0x2000), which the clone gets when either body has it or the clone is scaled.
    if ((uiFilterInfo & 0x2000) != 0 || 1.0f != static_cast<BSCloningProcess&>(arCloneData).Scale.x) {
        hkpRigidBody* pDestBody = static_cast<hkpRigidBody*>(pDestination->phkObject);
        uint32_t uiDestFilterInfo;
        if (pDestBody && (pCollidable = &pDestBody->m_collidable) != nullptr) {
            uiDestFilterInfo = pCollidable->m_broadPhaseHandle.m_collisionFilterInfo;
        }
        else {
            uiDestFilterInfo = 0;
        }
        uiDestFilterInfo |= 0x2000;
        if (pDestBody) {
            pDestination->MarkWrite();
            pDestination->UnMarkWrite();
            pCollidable = &pDestBody->m_collidable;
            if (pCollidable) {
                pCollidable->m_broadPhaseHandle.m_collisionFilterInfo = uiDestFilterInfo;
            }
        }
        static_cast<bhkWorldObject*>(pDestination)->UpdateCFilter();
    }
    for (BSSimpleList<NiPointer<bhkSerializable>>* pNode = &kActionConstrants; pNode; pNode = pNode->GetNext()) {
        if (!pNode->m_item) {
            break;
        }
        bhkSerializable* pClone = static_cast<bhkSerializable*>(pNode->m_item->CreateClone(arCloneData));
        pDestination->AddAction(pClone);
    }
}

// The number of entries of kActionConstrants up to the first empty one.
int32_t bhkRigidBody::CountActions() {
    int32_t result = 0;
    // The list head's address is tested for null, as the game does. clang assumes &kActionConstrants is non-null and
    // drops that test; the empty asm hides the pointer from it.
    BSSimpleList<NiPointer<bhkSerializable>>* i = &kActionConstrants;
    __asm__("" : "+r"(i));
    for (; i; ++result) {
        if (!i->m_item) {
            break;
        }
        i = i->GetNext();
    }
    return result;
}

BSSimpleList<NiPointer<bhkSerializable>>* bhkRigidBody::GetActionConstraints() {
    return &kActionConstrants;
}

// Empties kActionConstrants from the head, first taking each constraint or action that is in a world out of it.
void bhkRigidBody::RemoveActionConstraints() {
    BSSimpleList<NiPointer<bhkSerializable>>* pList = &kActionConstrants;
    if (pList) {
        while (true) {
            bhkSerializable* pItem = pList->m_item;
            if (!pItem) {
                return;
            }
            const hkpWorld* pWorld;
            // EXE-INLINED 0x6532C0
            if (pItem->IsKindOf(&bhkConstraint::ms_RTTI)) {
                pWorld = static_cast<bhkConstraint*>(pItem)->bhkConstraint::GethkWorld();
                // EXE-INLINED 0x6532C0
            }
            else if (pItem->IsKindOf(&bhkAction::ms_RTTI)) {
                pWorld = static_cast<bhkAction*>(pItem)->bhkAction::GethkWorld();
            }
            else {
                pWorld = nullptr;
            }
            if (pWorld) {
                pItem->Remove();
            }
            pList->RemoveHead();
        }
    }
}

const float bhkRigidBody::GetMaxAngularVelocity() {
    hkReferencedObject* phkObject;
    if (this && (phkObject = this->phkObject) != nullptr) {
        hkpMotion* pdm = static_cast<hkpRigidBody*>(this->phkObject)->hkpRigidBody::getStoredDynamicMotion();
        if (pdm) {
            return hkUFloat8_intToReal[pdm->m_motionState.m_maxAngularVelocity.m_value];
        }
        else {
            // Reads the maximum angular velocity from the body's own motion (a fixed or keyframed body has no stored
            // dynamic motion).
            hkUFloat8 v5;
            float afValue = hkUFloat8_intToReal[static_cast<hkpRigidBody*>(phkObject)
                                                    ->m_motion.m_motionState.m_maxAngularVelocity.m_value];
            v5.operator=(afValue);
            return hkUFloat8_intToReal[v5.m_value];
        }
    }
    else {
        return 0.0f;
    }
}

// Adds the body's action constraints to apWorld, up to the first empty entry; with flag 4 set, only the fixed
// constraints.
void bhkRigidBody::AddActConToWorld(bhkWorld* apWorld) {
    bool bFixedOnly = (iFlags & 4) != 0;
    for (BSSimpleList<NiPointer<bhkSerializable>>* pNode = &kActionConstrants; pNode; pNode = pNode->m_pkNext) {
        bhkSerializable* pConstraint = pNode->m_item;
        if (!pConstraint) {
            break;
        }
        bool bFixed = false;
        if (bFixedOnly) {
            bFixed = NiObject::IsKindOf(
                &bhkFixedConstraint::ms_RTTI, NiObject::DynamicCast(&bhkConstraint::ms_RTTI, pConstraint)
            );
        }
        if (!bFixedOnly || bFixed) {
            pConstraint->Add(apWorld);
        }
    }
}

// Drives a keyframed body to arPos/arRot over the world's step: where the target moved, the linear and angular
// velocities that reach it in one step (the body is teleported with SetPosRot when the speed would pass its maximum,
// and set to arPos under apSetPosLock with no velocity when it would pass 1e6); where it did not, both velocities are
// cleared.
void bhkRigidBody::ApplyHardKeyframe(const NiPoint3& arPos, const NiQuaternion& arRot, BSSpinLock* apSetPosLock) {
    // EXE-INLINED 0x4AE750
    hkpRigidBody* pwo = this ? static_cast<hkpRigidBody*>(phkObject) : nullptr;
    // EXE-INLINED none: bhkWorld::Getbhk
    ahkpWorld* pWorld = static_cast<ahkpWorld*>(pwo->m_world);
    if (!pWorld || !pWorld->m_userData) {
        return;
    }
    // EXE-INLINED 0x4A3E00
    hkVector4 hkpos;
    float fX = arPos.x * bhkConvert::fBS2HkScaleSC;
    float fY = arPos.y * bhkConvert::fBS2HkScaleSC;
    float fZ = bhkConvert::fBS2HkScaleSC * arPos.z;
    hkpos.m_quad = hkQuadReal{fX, fY, fZ, 0.0f};
    // EXE-INLINED none: NI2HK(hkQuaternion&, const NiQuaternion&), each lane copied with fld/fstp.
    hkQuaternion hkrot;
    hkrot.m_vec.m_quad = hkQuadReal{arRot.m_fX, arRot.m_fY, arRot.m_fZ, arRot.m_fW};
    hkVector4 bodyTrans;
    GetPosition(bodyTrans);
    hkQuaternion bodyRot;
    GetRotation(bodyRot);
    hkVector4 deltaPos;
    deltaPos.setSub4(hkpos, bodyTrans);
    float fLenSq = deltaPos.lengthSquared3();
    bool bMoved = fLenSq > 1.0000001e-06f;
    if (!bMoved) {
        float fDot = hkrot.m_vec.dot4(bodyRot.m_vec);
        float fDiff = fDot - 1.0f;
        float fAbs = std::fabs(fDiff);
        bMoved = fAbs > 1e-06f;
    }
    if (!bMoved) {
        // EXE-INLINED 0x560D80
        hkpRigidBody* pBody = static_cast<hkpRigidBody*>(phkObject);
        const hkVector4& rLinVel = pBody ? pBody->getLinearVelocity() : hkVector4::getZero();
        if (!rLinVel.equals4(hkVector4::getZero(), 0.001f)) {
            // EXE-INLINED 0x561690
            if (pBody) {
                MarkWrite();
                pBody->setLinearVelocity(hkVector4::getZero());
                UnMarkWrite();
            }
        }
        // EXE-INLINED 0x560E10
        pBody = static_cast<hkpRigidBody*>(phkObject);
        const hkVector4& rAngVel = pBody ? pBody->getAngularVelocity() : hkVector4::getZero();
        if (!rAngVel.equals4(hkVector4::getZero(), 0.001f)) {
            // EXE-INLINED 0x5617C0
            if (pBody) {
                MarkWrite();
                pBody->setAngularVelocity(hkVector4::getZero());
                UnMarkWrite();
            }
        }
        return;
    }
    float fDeltaTime = bhkWorld::fDeltaTime;
    if (fDeltaTime == 0.0f) {
        return;
    }
    float finvDelta = 1.0f / fDeltaTime;
    hkVector4 hkcom;
    hkVector4 newCenterOfMassPosition;
    newCenterOfMassPosition.setRotatedDir(hkrot, GetCenterOfMassLocal(hkcom));
    newCenterOfMassPosition.add4(hkpos);
    hkVector4 linearVelocity;
    linearVelocity.setSub4(newCenterOfMassPosition, GetCenterOfMassInWorld(hkcom));
    // EXE-INLINED 0x630AE0
    linearVelocity.m_quad = _mm_mul_ps(linearVelocity.m_quad, _mm_set1_ps(finvDelta));
    float linVelSq = linearVelocity.lengthSquared3();
    float fmaxLinVel = GethkMaxLinearVelocity();
    if (linVelSq > fmaxLinVel * fmaxLinVel) {
        hkVector4 hkpos2;
        NI2HK(hkpos2, arPos);
        // EXE-INLINED none: NI2HK(hkQuaternion&, const NiQuaternion&), each lane copied with fld/fstp.
        hkQuaternion hkrot2;
        hkrot2.m_vec.m_quad = hkQuadReal{arRot.m_fX, arRot.m_fY, arRot.m_fZ, arRot.m_fW};
        SetPosRot(hkpos2, hkrot2);
        return;
    }
    float fLinVel = linearVelocity.length3();
    if (fLinVel > 1000000.0) {
        if (apSetPosLock) {
            apSetPosLock->Lock(nullptr);
        }
        SetPosition(arPos);
        if (apSetPosLock) {
            // EXE-INLINED none: BSSpinLock::Unlock
            if (!--apSetPosLock->uiLockCount) {
                apSetPosLock->uiOwningThread = 0;
            }
        }
        SetLinearVelocity(hkVector4::getZero());
    }
    else {
        SetLinearVelocity(linearVelocity);
    }
    hkQuaternion quatDif;
    quatDif.setMulInverse(hkrot, bodyRot);
    quatDif.normalize();
    float angle = quatDif.getAngle();
    hkVector4 angularVelocity;
    if (angle < 0.001f) {
        angularVelocity.setZero4();
        SetAngularVelocity(angularVelocity);
    }
    else {
        quatDif.getAxis(angularVelocity);
        float fScale = angle * finvDelta;
        // EXE-INLINED 0x630AE0
        angularVelocity.m_quad = _mm_mul_ps(_mm_set1_ps(fScale), angularVelocity.m_quad);
        SetAngularVelocity(angularVelocity);
    }
}

void bhkRigidBody::Fix() {
    TLSData* pTLSData = GetThreadTLSData();
    MEM_CONTEXT eMemContext = pTLSData->eMemContext;
    pTLSData->eMemContext = MEM_CONTEXT_HAVOK;
    if ((iFlags & FIXEDCONSTRAINTS) == 0) {
        const ahkpWorld* pHkWorld = static_cast<const ahkpWorld*>(bhkWorldObject::GethkWorld());
        bhkWorld* pw = pHkWorld ? static_cast<bhkWorld*>(pHkWorld->m_userData) : nullptr;
        if (pw) {
            BSSimpleList<NiPointer<bhkSerializable>>* pNode = &kActionConstrants;
            if (pNode) {
                do {
                    bool bIsConstraint;
                    bhkSerializable* pItem = pNode->m_item;
                    if (!pItem) {
                        break;
                    }
                    const NiRTTI* pRTTI = pItem->GetRTTI();
                    if (pRTTI) {
                        while (pRTTI != &bhkConstraint::ms_RTTI) {
                            pRTTI = pRTTI->GetBaseRTTI();
                            if (!pRTTI) {
                                goto LABEL_11;
                            }
                        }
                        bIsConstraint = true;
                    }
                    else {
                    LABEL_11:;
                        bIsConstraint = false;
                    }
                    pNode = pNode->GetNext();
                    bhkConstraint* pcon = bIsConstraint ? static_cast<bhkConstraint*>(pItem) : nullptr;
                    if (pcon) {
                        hkpLinkedCollidable* pCollidable;
                        uint32_t uiFilterInfo;
                        hkpRigidBody* pBody = static_cast<hkpRigidBody*>(phkObject);
                        float fStrength = 1.0f;
                        if (pBody && (pCollidable = &pBody->m_collidable) != nullptr) {
                            uiFilterInfo = pCollidable->m_broadPhaseHandle.m_collisionFilterInfo;
                        }
                        else {
                            uiFilterInfo = 0;
                        }
                        // The layer (low seven bits) and the biped part (bits 8 to 12).
                        uint32_t uiLayer = uiFilterInfo & 0x7F;
                        if (uiLayer == L_BIPED || uiLayer == L_DEADBIP) {
                            uint32_t uiPart = uiFilterInfo >> 8 & 0x1F;
                            if (uiPart == P_TAIL || uiPart - P_PONYTAIL <= 1u) {
                                fStrength = 0.3f;
                            }
                        }
                        bhkConstraint* pFixed = bhkFixedConstraint::FixConstraint(pcon, fStrength);
                        if (pFixed) {
                            AddAction(pFixed);
                            pcon->Remove();
                            pFixed->Add(pw);
                        }
                    }
                }
                while (pNode);
            }
            iFlags |= FIXEDCONSTRAINTS;
        }
    }
    pTLSData->eMemContext = eMemContext;
}

// The hkVector4 constructor is trivial (it only returns this).
void bhkRigidBody::ApplyLinearImpulseNi(const NiPoint3& arImpulse) {
    hkVector4 hkImpulse;
    ApplyLinearImpulse(NI2HK(hkImpulse, arImpulse));
}

// Takes every constraint and action on kActionConstrants that is in a world out of it; the list keeps them.
void bhkRigidBody::RemoveActionConstraintsFromWorld() {
    BSSimpleList<NiPointer<bhkSerializable>>* pList = &kActionConstrants;
    if (pList) {
        do {
            if (!pList->m_pkNext && !pList->m_item) {
                return;
            }
            bhkSerializable* pItem = pList->m_item;
            if (pItem) {
                const hkpWorld* pWorld;
                // EXE-INLINED 0x6532C0
                if (pItem->IsKindOf(&bhkConstraint::ms_RTTI)) {
                    pWorld = static_cast<bhkConstraint*>(pItem)->bhkConstraint::GethkWorld();
                    // EXE-INLINED 0x6532C0
                }
                else if (pItem->IsKindOf(&bhkAction::ms_RTTI)) {
                    pWorld = static_cast<bhkAction*>(pItem)->bhkAction::GethkWorld();
                }
                else {
                    pWorld = nullptr;
                }
                if (pWorld) {
                    pItem->Remove();
                }
            }
            pList = pList->m_pkNext;
        }
        while (pList);
    }
}

// Takes the body out of the fixed-constraints state: each constraint on kActionConstrants that is in a world is removed
// from it and from the list, the others are added to the body's world when abAddRemainingConstraints is set. An item
// that is not a constraint leaves the walk on its node, as the game does. Only abAddRemainingConstraints' low byte is
// tested.
void bhkRigidBody::UnFix(int32_t abAddRemainingConstraints) {
    TLSData* pTLSData = GetThreadTLSData();
    MEM_CONTEXT eMemContext = pTLSData->eMemContext;
    pTLSData->eMemContext = MEM_CONTEXT_HAVOK;
    if ((iFlags & FIXEDCONSTRAINTS) != 0) {
        const hkpWorld* pWorld = bhkWorldObject::GethkWorld();
        // EXE-INLINED 0x621480
        bhkWorld* pw = pWorld ? static_cast<bhkWorld*>(static_cast<const ahkpWorld*>(pWorld)->m_userData) : nullptr;
        if (pw) {
            BSSimpleList<NiPointer<bhkSerializable>>* pList = &kActionConstrants;
            if (pList) {
                do {
                    bhkSerializable* pItem = pList->m_item;
                    if (!pItem) {
                        break;
                    }
                    // EXE-INLINED 0x6532C0
                    bhkConstraint* pcon =
                        pItem->IsKindOf(&bhkConstraint::ms_RTTI) ? static_cast<bhkConstraint*>(pItem) : nullptr;
                    if (pcon) {
                        if (pcon->bhkConstraint::GethkWorld()) {
                            pcon->Remove();
                            kActionConstrants.RemoveHead();
                        }
                        else {
                            pList = pList->m_pkNext;
                            if (static_cast<uint8_t>(abAddRemainingConstraints)) {
                                pcon->Add(pw);
                            }
                        }
                    }
                }
                while (pList);
            }
            iFlags &= ~static_cast<uint32_t>(FIXEDCONSTRAINTS);
        }
    }
    pTLSData->eMemContext = eMemContext;
}

// abFix and abAddRemainingConstraints are int32_t rather than bool because they are passed on whole, as the caller's
// full values; bool would be zero-extended. Only their low bytes are tested.
void bhkRigidBody::SetFixed(NiAVObject* apObject, int32_t abFix, int32_t abAddRemainingConstraints) {
    if (apObject) {
        apObject->UpdateWorldLoc();
        bhkNiCollisionObject* pCollisionObject = apObject->m_spCollisionObject.m_pObject;
        if (pCollisionObject) {
            NiObject* pCast = pCollisionObject->DynamicCastMember(&bhkCollisionObject::ms_RTTI);
            if (pCast) {
                bhkRigidBody* pBody = static_cast<bhkCollisionObject*>(pCast)->spWorldObject;
                if (pBody) {
                    if (static_cast<uint8_t>(abFix)) {
                        pBody->Fix();
                    }
                    else {
                        pBody->UnFix(abAddRemainingConstraints);
                    }
                }
            }
        }
        NiNode* pNode = apObject->IsNode();
        if (pNode) {
            for (uint32_t i = 0; i < pNode->m_kChildren.m_usSize; ++i) {
                bhkRigidBody::SetFixed(pNode->m_kChildren.m_pBase[i].m_pObject, abFix, abAddRemainingConstraints);
            }
        }
    }
}

bool bhkRigidBody::IsMotionTypeDynamic(const hkpMotion::MotionType& areMotion) {
    int32_t v1 = areMotion;
    if ((areMotion > 0) && ((v1 <= 3) || (v1 == 6))) {
        return true;
    }
    return false;
}

// The class loader's factory.
NiObject* bhkRigidBody::CreateObject() {
    bhkRigidBody* pObject = static_cast<bhkRigidBody*>(NiMemObject::operator new(sizeof(bhkRigidBody)));
    if (!pObject) {
        return nullptr;
    }
    return ::new (pObject) bhkRigidBody;
}

// Adds the MOTION line for aeType to the viewer strings.
void bhkRigidBody::AddMotionType(NiTPrimitiveArray<char*>* apStrings, hkpMotion::MotionType aeType) {
    char* pcMotion;
    switch (aeType) {
        case hkpMotion::MOTION_DYNAMIC:
            pcMotion = NiGetViewerString("MOTION", "DYNAMIC");
            apStrings->Add(pcMotion);
            return;
        case hkpMotion::MOTION_SPHERE_INERTIA:
            pcMotion = NiGetViewerString("MOTION", "SPHERE");
            apStrings->Add(pcMotion);
            return;
        case hkpMotion::MOTION_BOX_INERTIA:
            pcMotion = NiGetViewerString("MOTION", "BOX");
            apStrings->Add(pcMotion);
            return;
        case hkpMotion::MOTION_KEYFRAMED:
            pcMotion = NiGetViewerString("MOTION", "KEYFRAMED");
            apStrings->Add(pcMotion);
            return;
        case hkpMotion::MOTION_FIXED:
            pcMotion = NiGetViewerString("MOTION", "FIXED");
            apStrings->Add(pcMotion);
            return;
        case hkpMotion::MOTION_THIN_BOX_INERTIA:
            pcMotion = NiGetViewerString("MOTION", "THIN BOX");
            apStrings->Add(pcMotion);
            return;
        default: AddMotionTypeUnknown(apStrings); return;
    }
}

// The MOTION line for a motion type outside the known set: the INVALID viewer string, appended at m_usSize after
// growing the array when it is full.
void bhkRigidBody::AddMotionTypeUnknown(NiTPrimitiveArray<char*>* apStrings) {
    char* pcMotion = NiGetViewerString("MOTION", "INVALID");
    apStrings->Add(pcMotion);
}

bhkRigidBody* bhkRigidBody::FindHeaviestRigidBody(NiAVObject* ap3D) {
    if (!ap3D) {
        return nullptr;
    }
    bhkRigidBody* pHeaviestChild = nullptr;
    NiNode* pNode = ap3D->IsNode();
    if (pNode) {
        uint32_t uiCount = pNode->GetArrayCount();
        for (uint32_t i = 0; i < uiCount; ++i) {
            bhkRigidBody* pChildBody = FindHeaviestRigidBody(pNode->GetAt(i));
            if (!pChildBody) {
                continue;
            }
            if (pHeaviestChild) {
                // The first mass is kept as a double.
                double dHeaviestMass = pHeaviestChild->GetMass();
                if (!(pChildBody->GetMass() > dHeaviestMass)) {
                    continue;
                }
            }
            pHeaviestChild = pChildBody;
        }
    }
    bhkCollisionObject* pCollisionObject = bhkCollisionObject::GetbhkCollisionObject(ap3D);
    bhkRigidBody* pBody = pCollisionObject ? static_cast<bhkRigidBody*>(pCollisionObject->GetWorldObject()) : nullptr;
    if (pHeaviestChild) {
        if (!pBody) {
            return pHeaviestChild;
        }
        double dBodyMass = pBody->GetMass();
        if (pHeaviestChild->GetMass() > dBodyMass) {
            return pHeaviestChild;
        }
    }
    return pBody;
}

void bhkRigidBody::RemoveAllActionConstraints(NiAVObject* apObject) {
    bhkNiCollisionObject* m_pObject;
    bhkCollisionObject* v2;
    bhkRigidBody* spWorldObject;
    NiNode* v4;
    uint32_t i;
    if (apObject) {
        m_pObject = apObject->m_spCollisionObject.m_pObject;
        if (m_pObject) {
            v2 = static_cast<bhkCollisionObject*>(m_pObject->DynamicCastMember(&bhkCollisionObject::ms_RTTI));
            if (v2) {
                spWorldObject = v2->spWorldObject;
                if (spWorldObject) {
                    spWorldObject->RemoveActionConstraints();
                }
            }
        }
        v4 = apObject->IsNode();
        if (v4) {
            for (i = 0; i < ((uint32_t)((int32_t)v4->m_kChildren.m_usSize));
                 RemoveAllActionConstraints(v4->m_kChildren.m_pBase[i++].m_pObject)) {
                ;
            }
        }
    }
}

void bhkRigidBody::CalculateCOMAndMassRecursive(NiAVObject* a1, __m128* arMassCenter, float* a3) {
    uint32_t v15;
    if (a1) {
        NiNode* pnode = a1->IsNode();
        if (pnode) {
            uint32_t v5 = 0;
            if (pnode->m_kChildren.m_usSize) {
                do {
                    bhkRigidBody::CalculateCOMAndMassRecursive(
                        pnode->m_kChildren.m_pBase[v5++].m_pObject, arMassCenter, a3
                    );
                }
                while (v5 < pnode->m_kChildren.m_usSize);
            }
        }
        bhkNiCollisionObject* m_pObject = a1->m_spCollisionObject.m_pObject;
        if (m_pObject) {
            char v8;
            const NiRTTI* v7 = m_pObject->GetRTTI();
            if (v7) {
                while (v7 != &bhkCollisionObject::ms_RTTI) {
                    v7 = v7->GetBaseRTTI();
                    if (!v7) {
                        goto LABEL_9;
                    }
                }
                v8 = 1;
            }
            else {
            LABEL_9:;
                v8 = 0;
            }
            bhkNiCollisionObject* v9 = v8 != 0 ? m_pObject : nullptr;
            if (v9) {
                bhkRigidBody* pbody = v9->spWorldObject;
                if (pbody) {
                    hkVector4 v16;
                    float fmass = v9->spWorldObject->GetDynamicMass();
                    pbody->GetCenterOfMassInWorld(v16);
                    double fmass_2 = fmass;
                    float fratio = *a3 + fmass;
                    *a3 = fratio;
                    *reinterpret_cast<float*>(&v15) = fmass_2 / fratio;
                    __m128 v12 =
                        _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v15)), _mm_set_ss(std::bit_cast<float>(v15)), 0);
                    *arMassCenter = _mm_add_ps(
                        _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v12), *arMassCenter), _mm_mul_ps(v16.m_quad, v12)
                    );
                }
            }
        }
    }
}

// hkEntityCinfo's constructor sets the entity's fields; the Havok body's defaults are overridden here.
bhkRigidBodyCinfo::bhkRigidBodyCinfo() {
    hk.m_maxLinearVelocity = 10000.0f;
    hk.m_qualityType.m_storage = HK_COLLIDABLE_QUALITY_INVALID;
    hk.m_maxAngularVelocity = 31.415901f;
}

// The property list is freed by the world object info's destructor.
bhkRigidBodyCinfo::~bhkRigidBodyCinfo() {}

void bhkRigidBodyCinfo::SetPosition(const hkVector4& arPos) {
    hk.m_position = arPos;
}

void bhkRigidBodyCinfo::SetFriction(float afFriction) {
    hk.m_friction = afFriction;
}

void bhkRigidBodyCinfo::SetMass(float afMass) {
    hk.m_mass = afMass;
}

void bhkRigidBodyCinfo::SetCollisionFilter(uint32_t aiFilter) {
    m_collisionFilterInfo = (uint32_t)((int32_t)aiFilter);
    hk.m_collisionFilterInfo = (uint32_t)((int32_t)aiFilter);
}

void bhkRigidBodyCinfo::SetRestitution(float afRest) {
    bhkRigidBodyCinfo* result = this;
    hk.m_restitution = afRest;
}

// Byte-swaps every element of the creation info in place, in this order: the four vectors, the inertia tensor through
// its helper, the center of mass, the callback delay and then the body settings.
void bhkRigidBodyCinfo::Endian() {
    hkpRigidBodyCinfo& rkHk = hk;
    rkHk.m_position.m_quad[0] = SwapFloat(rkHk.m_position.m_quad[0]);
    rkHk.m_position.m_quad[1] = SwapFloat(rkHk.m_position.m_quad[1]);
    rkHk.m_position.m_quad[2] = SwapFloat(rkHk.m_position.m_quad[2]);
    rkHk.m_position.m_quad[3] = SwapFloat(rkHk.m_position.m_quad[3]);
    rkHk.m_rotation.m_vec.m_quad[0] = SwapFloat(rkHk.m_rotation.m_vec.m_quad[0]);
    rkHk.m_rotation.m_vec.m_quad[1] = SwapFloat(rkHk.m_rotation.m_vec.m_quad[1]);
    rkHk.m_rotation.m_vec.m_quad[2] = SwapFloat(rkHk.m_rotation.m_vec.m_quad[2]);
    rkHk.m_rotation.m_vec.m_quad[3] = SwapFloat(rkHk.m_rotation.m_vec.m_quad[3]);
    rkHk.m_linearVelocity.m_quad[0] = SwapFloat(rkHk.m_linearVelocity.m_quad[0]);
    rkHk.m_linearVelocity.m_quad[1] = SwapFloat(rkHk.m_linearVelocity.m_quad[1]);
    rkHk.m_linearVelocity.m_quad[2] = SwapFloat(rkHk.m_linearVelocity.m_quad[2]);
    rkHk.m_linearVelocity.m_quad[3] = SwapFloat(rkHk.m_linearVelocity.m_quad[3]);
    rkHk.m_angularVelocity.m_quad[0] = SwapFloat(rkHk.m_angularVelocity.m_quad[0]);
    rkHk.m_angularVelocity.m_quad[1] = SwapFloat(rkHk.m_angularVelocity.m_quad[1]);
    rkHk.m_angularVelocity.m_quad[2] = SwapFloat(rkHk.m_angularVelocity.m_quad[2]);
    rkHk.m_angularVelocity.m_quad[3] = SwapFloat(rkHk.m_angularVelocity.m_quad[3]);
    EndianMatrix(rkHk.m_inertiaTensor);
    rkHk.m_centerOfMass.m_quad[0] = SwapFloat(rkHk.m_centerOfMass.m_quad[0]);
    rkHk.m_centerOfMass.m_quad[1] = SwapFloat(rkHk.m_centerOfMass.m_quad[1]);
    rkHk.m_centerOfMass.m_quad[2] = SwapFloat(rkHk.m_centerOfMass.m_quad[2]);
    rkHk.m_centerOfMass.m_quad[3] = SwapFloat(rkHk.m_centerOfMass.m_quad[3]);
    uint16_t uiDelay = rkHk.m_contactPointCallbackDelay;
    rkHk.m_contactPointCallbackDelay = (uint16_t)((uiDelay >> 8) | (uiDelay << 8));
    rkHk.m_mass = SwapFloat(rkHk.m_mass);
    rkHk.m_linearDamping = SwapFloat(rkHk.m_linearDamping);
    rkHk.m_angularDamping = SwapFloat(rkHk.m_angularDamping);
    rkHk.m_gravityFactor = SwapFloat(rkHk.m_gravityFactor);
    rkHk.m_friction = SwapFloat(rkHk.m_friction);
    rkHk.m_restitution = SwapFloat(rkHk.m_restitution);
    rkHk.m_maxLinearVelocity = SwapFloat(rkHk.m_maxLinearVelocity);
    rkHk.m_maxAngularVelocity = SwapFloat(rkHk.m_maxAngularVelocity);
    rkHk.m_allowedPenetrationDepth = SwapFloat(rkHk.m_allowedPenetrationDepth);
}

void bhkRigidBodyCinfo::DestroyPropertiesArray() {}

// Scales the centre of mass by afScale, the inertia tensor's three columns by its cube and the mass by afScale.
void bhkRigidBodyCinfo::Scale(float afScale) {
    hk.m_centerOfMass.m_quad =
        _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(afScale), _mm_set_ss(afScale), 0), hk.m_centerOfMass.m_quad);
    // The cube is rounded to a float before it is used.
    float fScaleCubed = afScale * afScale * afScale;
    for (int32_t i = 0; i < 3; ++i) {
        hk.m_inertiaTensor.m_col0.m_quad[i] = hk.m_inertiaTensor.m_col0.m_quad[i] * fScaleCubed;
        hk.m_inertiaTensor.m_col1.m_quad[i] = hk.m_inertiaTensor.m_col1.m_quad[i] * fScaleCubed;
        hk.m_inertiaTensor.m_col2.m_quad[i] = fScaleCubed * hk.m_inertiaTensor.m_col2.m_quad[i];
    }
    hk.m_mass = afScale * hk.m_mass;
}

// The quality type's name; null for the values without one.
const char* bhkRigidBodyCinfo::GetQualityTypeString() {
    switch (hk.m_qualityType.m_storage) {
        case -1: return "Invalid";
        case 0: return "Fixed";
        case 1: return "Keyframed";
        case 2: return "Debris";
        case 4: return "Moving";
        case 5: return "Critical";
        case 6: return "Bullet";
        case 7: return "User";
        default: return nullptr;
    }
}

// Converts a 5.5.1 rigid body cinfo; the motion and quality types are renumbered.
void bhkRigidBodyCinfo::ConvertFromOld(bhkRigidBody551Cinfo* apOldCInfo) {
    hk.m_gravityFactor = 1.0f;
    hk.m_localFrame = nullptr;
    uint32_t m_collisionFilterInfo = apOldCInfo->hkWorldObjectCinfo::m_collisionFilterInfo;
    this->m_collisionFilterInfo = apOldCInfo->hkWorldObjectCinfo::m_collisionFilterInfo;
    hk.m_collisionFilterInfo = m_collisionFilterInfo;
    hk.m_shape = apOldCInfo->bhkRigidBody551Cinfo::m_shape;
    hk.m_collisionResponse.m_storage = apOldCInfo->m_hkCollisionResponse;
    hk.m_contactPointCallbackDelay = apOldCInfo->m_contactPointCallbackDelay;
    hk.m_position = apOldCInfo->m_position;
    hk.m_rotation = apOldCInfo->m_rotation;
    hk.m_linearVelocity = apOldCInfo->m_linearVelocity;
    hk.m_angularVelocity = apOldCInfo->m_angularVelocity;
    hk.m_inertiaTensor = apOldCInfo->m_inertiaTensor;
    hk.m_centerOfMass = apOldCInfo->m_centerOfMass;
    hk.m_mass = apOldCInfo->m_mass;
    hk.m_linearDamping = apOldCInfo->m_linearDamping;
    hk.m_angularDamping = apOldCInfo->m_angularDamping;
    hk.m_friction = apOldCInfo->m_friction;
    hk.m_restitution = apOldCInfo->m_restitution;
    hk.m_maxLinearVelocity = apOldCInfo->m_maxLinearVelocity;
    hk.m_maxAngularVelocity = apOldCInfo->m_maxAngularVelocity;
    hk.m_allowedPenetrationDepth = apOldCInfo->m_allowedPenetrationDepth;
    // Sign-extends the old byte and compares it unsigned.
    uint32_t uiMotionType = static_cast<uint32_t>(static_cast<int32_t>(static_cast<int8_t>(apOldCInfo->ucMotionType)));
    if (uiMotionType >= 5) {
        uiMotionType -= 2;
    }
    else if (uiMotionType >= 3) {
        --uiMotionType;
    }
    hk.m_motionType.m_storage = static_cast<int8_t>(uiMotionType);
    hk.m_enableDeactivation.m_bool = apOldCInfo->ucDeactivatorType == 2;
    hk.m_solverDeactivation.m_storage = apOldCInfo->ucSolverDeactivation;
    // Signed compare of the sign-extended old byte.
    int32_t iQualityType = static_cast<int8_t>(apOldCInfo->ucQualityType);
    if (iQualityType < 3) {
        --iQualityType;
    }
    hk.m_qualityType.m_storage = static_cast<int8_t>(iQualityType);
}

void bhkRigidBodyCinfo::ReadSerialized(const Serialized& arForm) {
    hkEntityCinfo::ReadSerialized(arForm);
    hk.m_collisionFilterInfo = arForm.m_collisionFilterInfo;
    hk.m_shape = nullptr;
    hk.m_localFrame = nullptr;
    hk.m_collisionResponse.m_storage = arForm.m_collisionResponse;
    hk.m_contactPointCallbackDelay = arForm.m_contactPointCallbackDelay;
    hk.m_position = arForm.m_position;
    hk.m_rotation = arForm.m_rotation;
    hk.m_linearVelocity = arForm.m_linearVelocity;
    hk.m_angularVelocity = arForm.m_angularVelocity;
    hk.m_inertiaTensor = arForm.m_inertiaTensor;
    hk.m_centerOfMass = arForm.m_centerOfMass;
    hk.m_mass = arForm.m_mass;
    hk.m_linearDamping = arForm.m_linearDamping;
    hk.m_angularDamping = arForm.m_angularDamping;
    hk.m_gravityFactor = arForm.m_gravityFactor;
    hk.m_friction = arForm.m_friction;
    hk.m_restitution = arForm.m_restitution;
    hk.m_maxLinearVelocity = arForm.m_maxLinearVelocity;
    hk.m_maxAngularVelocity = arForm.m_maxAngularVelocity;
    hk.m_allowedPenetrationDepth = arForm.m_allowedPenetrationDepth;
    hk.m_motionType.m_storage = arForm.m_motionType;
    hk.m_enableDeactivation.m_bool = arForm.m_enableDeactivation;
    hk.m_solverDeactivation.m_storage = arForm.m_solverDeactivation;
    hk.m_qualityType.m_storage = arForm.m_qualityType;
    hk.m_autoRemoveLevel = arForm.m_autoRemoveLevel;
    hk.m_responseModifierFlags = arForm.m_responseModifierFlags;
    hk.m_numShapeKeysInContactPointProperties = arForm.m_numShapeKeysInContactPointProperties;
    hk.m_forceCollideOntoPpu.m_bool = arForm.m_forceCollideOntoPpu;
}

// Byte-swaps every element of arMatrix in place.
void bhkRigidBodyCinfo::EndianMatrix(hkMatrix3& arMatrix) {
    uint32_t* p_z = reinterpret_cast<uint32_t*>(&reinterpret_cast<float*>(&arMatrix.m_col0.m_quad)[2]);
    int32_t v2 = 3;
    do {
        *(p_z - 2) = __builtin_bswap32(*(p_z - 2));
        *(p_z - 1) = __builtin_bswap32(*(p_z - 1));
        *p_z = __builtin_bswap32(*p_z);
        p_z[1] = __builtin_bswap32(p_z[1]);
        p_z += 4;
        --v2;
    }
    while (v2);
}

hkEntityCinfo::~hkEntityCinfo() {}
