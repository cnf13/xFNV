#include "Fallout Shared/Havok/ViewCaster.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkConvert.h"
#include "BSHavok/bhkEntity.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkShapePhantom.h"
#include "BSHavok/bhkSimpleShapePhantom.h"
#include "BSHavok/bhkSphereShape.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESLight.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectSTAT.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiPick.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpLinearCastInput.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpAllCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Havok/Source/Physics/Collide/Query/Collector/RayCollector/hkpAllRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastCollectorOutput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

#include <limits>

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> bUseFuzzyPicking("bUseFuzzyPicking:Interface", true);
SettingT<INISettingCollection> iMaxViewCasterPicksFuzzy("iMaxViewCasterPicksFuzzy:Interface", 5);
SettingT<INISettingCollection> iMaxViewCasterPicksHavok("iMaxViewCasterPicksHavok:Interface", 10);
SettingT<INISettingCollection> iMaxViewCasterPicksGamebryo("iMaxViewCasterPicksGamebryo:Interface", 10);

ViewCaster::ViewCaster() : spShapePhantom(nullptr), bInit(false), iGroup(0) {}

// The phantom is released by the pointer's own destructor.
ViewCaster::~ViewCaster() {}

// Creates the sphere phantom in the second custom pick layer and aiGroup.
void ViewCaster::Init(float afRadius, uint32_t aiGroup) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_HAVOK,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Havok\\ViewCaster.cpp",
        73
    );
    bhkSphereShape* pShape = new bhkSphereShape(afRadius, true);
    alignas(16) hkSimpleShapePhantomCinfo csi;
    CFilter kFilter;
    kFilter.iFilter = 0;
    kFilter.SetLayer(L_CUSTOMPICK2);
    kFilter.SetGroup(aiGroup);
    iGroup = static_cast<int32_t>(aiGroup);
    csi.m_collisionFilterInfo = kFilter.iFilter;
    csi.m_shape = **pShape;
    csi.m_transform.setIdentity();
    bhkSimpleShapePhantom* pPhantom = new bhkSimpleShapePhantom(csi);
    spShapePhantom = static_cast<bhkShapePhantom*>(pPhantom);
    bInit = true;
}

bhkWorld* ViewCaster::GetWorld() {
    if (spShapePhantom.m_pObject) {
        const hkpWorld* pWorld = spShapePhantom->bhkWorldObject::GethkWorld();
        return bhkWorld::Getbhk(pWorld);
    }
    return nullptr;
}

void ViewCaster::Add(bhkWorld* apWorld) {
    AutoMemContext kMemContext(
        MEM_CONTEXT_HAVOK,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Havok\\ViewCaster.cpp",
        116
    );
    if (GetObject()) {
        bhkSimpleShapePhantom* pPhantom = static_cast<bhkSimpleShapePhantom*>(GetObject());
        pPhantom->Add(apWorld);
        if (apWorld) {
            uint32_t uiColor = hkColor::rgbFromChars(88u, 173u, 86u, 25u);
            bhkWorldObject* pWorldObject = static_cast<bhkWorldObject*>(GetObject());
            apWorld->SetDebugObjectColor(pWorldObject, uiColor);
        }
    }
}

void ViewCaster::SetShapeGroup(uint32_t aiGroup) {
    CFilter kFilter;
    static_cast<bhkWorldObject*>(GetObject())->GetCollisionFilter(&kFilter);
    kFilter.SetGroup(aiGroup);
    iGroup = static_cast<int32_t>(aiGroup);
    static_cast<bhkWorldObject*>(GetObject())->SetCollisionFilter(kFilter);
}

TESObjectREFR* ViewCaster::FindActivateREFR(
    const NiPoint3& aOrigin,
    const NiPoint3& aDirection,
    float afPickDistance,
    float& afObjectDistance,
    bool& abIsFuzzy
) {
    CFilter kPickFilter;
    CFilter kRayFilter;
    alignas(16) CFilter fltr;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_HAVOK,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Havok\\ViewCaster.cpp",
        155
    );
    ViewCaster::pFuzzyBody = nullptr;
    TESObjectREFR* pResult = nullptr;
    if (bInit && GetObject() && GetWorld()) {
        hkVector4 hkto;
        hkVector4 hkfrom;
        hkpAllCdPointCollector kPointCollector;
        bhkWorld* pWorld = GetWorld();
        NiPoint3 nito = aOrigin + aDirection * afPickDistance;
        abIsFuzzy = false;
        afObjectDistance = std::numeric_limits<float>::max();
        alignas(16) NiPick kActivatePick(0, 8u);
        kActivatePick.SetFrontOnly(false);
        kActivatePick.SetObserveAppCullFlag(true);
        uiInterfaceCheckpoint = 3501;
        NI2HK(hkfrom, aOrigin);
        NI2HK(hkto, nito);
        bool bHits = false;
        hkpLinearCastInput input;
        input.m_startPointTolerance = 0.01f;
        input.m_maxExtraPenetration = 0.01f;
        input.m_to = hkto;
        SetCustomPickCollisions();
        static_cast<bhkShapePhantom*>(GetObject())->SetPositionAndLinearCast(hkfrom, input, kPointCollector, nullptr);
        if (kPointCollector.hasHit()) {
            kPointCollector.sortHits();
            bHits = true;
        }
        TESObjectREFR* pHavokPickRef = nullptr;
        float fHavokPickDistance = std::numeric_limits<float>::max();
        TESObjectREFR* pShapeCastRef = nullptr;
        float fShapeCastDistance = std::numeric_limits<float>::max();
        float fShapeCastCenterPointDistance = std::numeric_limits<float>::max();
        bhkRigidBody* pfuzzyBody = nullptr;
        bool bUseFuzzy = bUseFuzzyPicking.Bool();
        int32_t iFuzzyPicks = 0;
        int32_t iHavokPicks = 0;
        int32_t iGamebryoPicks = 0;
        if (bHits) {
            uiInterfaceCheckpoint = 3502;
            TESObjectREFR* pLast = nullptr;
            TESObjectREFR* pCurrent = nullptr;
            hkpRigidBody* pLastBody = nullptr;
            for (int32_t i = 0; i < kPointCollector.getHits().m_size; ++i) {
                uiInterfaceCheckpoint = 3503;
                hkpRootCdPoint kPoint(kPointCollector.getHits()[i]);
                if (kPoint.m_rootCollidableB) {
                    uiInterfaceCheckpoint = 3504;
                    NiAVObject* pAV = GetAVObjectForCollidable(kPoint.m_rootCollidableB);
                    uiInterfaceCheckpoint = 3505;
                    if (pAV) {
                        pCurrent = TESObjectREFR::FindReferenceFor3D(pAV);
                    }
                    if (pCurrent && pCurrent != pPlayer) {
                        bool bCulled = false;
                        NiAVObject* pCullTest = pAV;
                        NiNode* p3D = pCurrent->Get3D();
                        while (!bCulled && pCullTest && pCullTest != p3D) {
                            int32_t iCulled = bCulled;
                            bool bAppCulled = pCullTest->GetAppCulled();
                            bCulled = bAppCulled || iCulled;
                            pCullTest = pCullTest->m_pkParent;
                        }
                        if (!bCulled) {
                            hkpRigidBody* pRB = hkpGetRigidBody(kPoint.m_rootCollidableB);
                            bool bHavokPick = false;
                            fltr.iFilter = kPoint.m_rootCollidableB->getCollisionFilterInfo();
                            COL_LAYER eLayer = fltr.GetLayer();
                            if (eLayer == L_STATIC) {
                                const hkpShape* pShape = kPoint.m_rootCollidableB->m_shape;
                                if (pShape && pShape->getType() == HK_SHAPE_MOPP) {
                                    bHavokPick = true;
                                }
                            }
                            else if (eLayer > L_TRANSPARENT && eLayer <= L_PROJECTILE) {
                                bHavokPick = true;
                            }
                            TESObjectSTAT* pBaseObject = static_cast<TESObjectSTAT*>(pCurrent->GetObjectReference());
                            if (pBaseObject == pCollisionMarker) {
                                bHavokPick = true;
                            }
                            if (pRB && pRB != pLastBody && bHavokPick) {
                                pLastBody = pRB;
                                if (iHavokPicks < iMaxViewCasterPicksHavok.Int()) {
                                    ++iHavokPicks;
                                    hkpShapeRayCastInput rci;
                                    rci.m_filterInfo = static_cast<bhkWorldObject*>(GetObject())
                                                           ->GetCollisionFilter(&kRayFilter)
                                                           ->iFilter;
                                    hkTransform hkt(pRB->getTransform());
                                    rci.m_from.setTransformedInversePos(hkt, hkfrom);
                                    rci.m_to.setTransformedInversePos(hkt, hkto);
                                    hkpShapeRayCastOutput rco;
                                    rco.reset_0();
                                    const hkpShape* pCurShape = kPoint.m_rootCollidableB->m_shape;
                                    pCurShape->castRay(rci, rco);
                                    if (rco.hasHit() && fHavokPickDistance > static_cast<double>(rco.m_hitFraction)) {
                                        pHavokPickRef = pCurrent;
                                        fHavokPickDistance = rco.m_hitFraction;
                                    }
                                }
                            }
                            else {
                                uiInterfaceCheckpoint = 3506;
                                if (pLast != pCurrent && pCurrent->Get3D()) {
                                    if (iGamebryoPicks < iMaxViewCasterPicksGamebryo.Int()) {
                                        ++iGamebryoPicks;
                                        NiNode* pCurrent3D = pCurrent->Get3D();
                                        PickDistance(
                                            &kActivatePick,
                                            aOrigin,
                                            aDirection,
                                            afPickDistance,
                                            pCurrent3D,
                                            false
                                        );
                                    }
                                }
                            }
                            bool bFuzzyPick = true;
                            switch (fltr.GetLayer()) {
                                case L_STATIC:
                                case L_TERRAIN:
                                case L_GROUND:
                                case L_PORTAL: {
                                    bFuzzyPick = false;
                                    break;
                                }
                                case L_PROJECTILE: {
                                    if (!pRB) {
                                        bFuzzyPick = false;
                                    }
                                    break;
                                }
                                case L_CHARCONTROLLER: {
                                    if (pCurrent->IsActor()) {
                                        MobileObject* pMobile = static_cast<MobileObject*>(pCurrent);
                                        BaseProcess* pProcess = pMobile->GetCurrentAIProcess();
                                        bhkCharacterController* pCon = pMobile->MobileObject::GetCharController();
                                        if (pProcess && pCon && pCon->GetSize() == bhkCharacterController::CS_SIZED) {
                                            SIT_SLEEP_STATE eState = pProcess->GetSitSleepState();
                                            if (eState == SIT_SLEEP_IS_SLEEPING || eState == SIT_SLEEP_IS_SITTING) {
                                                bFuzzyPick = false;
                                            }
                                        }
                                    }
                                    break;
                                }
                                default: {
                                    break;
                                }
                            }
                            if (bFuzzyPick && bUseFuzzy && pWorld) {
                                hkVector4 distanceFromPick;
                                hkVector4 pickDirection;
                                hkVector4 distanceToContact;
                                distanceToContact.setSub4(kPoint.m_contact.getPosition(), hkfrom);
                                pickDirection.setSub4(hkto, hkfrom);
                                pickDirection.normalize3();
                                pickDirection.mul4(pickDirection.dot3(distanceToContact));
                                distanceFromPick.setSub4(distanceToContact, pickDirection);
                                float fScalar = distanceFromPick.length3();
                                float fDistancePick = HK2NI(fScalar);
                                if (fShapeCastCenterPointDistance > static_cast<double>(fDistancePick)) {
                                    if (iFuzzyPicks < iMaxViewCasterPicksFuzzy.Int()) {
                                        if (pShapeCastRef && pShapeCastRef == pCurrent) {
                                            fScalar = distanceToContact.length3();
                                            fShapeCastDistance = HK2NI(fScalar);
                                            fShapeCastCenterPointDistance = fDistancePick;
                                        }
                                        else {
                                            ++iFuzzyPicks;
                                            bhkPickData pd;
                                            CFilter* pFilter = static_cast<bhkWorldObject*>(GetObject())
                                                                   ->GetCollisionFilter(&kPickFilter);
                                            pd.SetFilter(*pFilter);
                                            pd.SetFrom(hkfrom);
                                            pd.SetTo(kPoint.m_contact.getPosition());
                                            hkpAllRayHitCollector kCollector;
                                            pd.SetAllCollector(&kCollector);
                                            pWorld->PickObject(pd);
                                            if (!pd.bPickFailed) {
                                                if (kCollector.getHits().m_size <= 0) {
                                                    pShapeCastRef = pCurrent;
                                                    fScalar = distanceToContact.length3();
                                                    fShapeCastDistance = HK2NI(fScalar);
                                                    fShapeCastCenterPointDistance = fDistancePick;
                                                    pfuzzyBody = static_cast<bhkRigidBody*>(bhkEntity::Getbhk(pRB));
                                                }
                                                else {
                                                    const hkpWorldRayCastOutput* pRayHit = &kCollector.getHits()[0];
                                                    pAV = GetAVObjectForCollidable(pRayHit->m_rootCollidable);
                                                    if (pAV) {
                                                        TESObjectREFR* pRef = TESObjectREFR::FindReferenceFor3D(pAV);
                                                        if (pRef && pRef == pCurrent) {
                                                            pShapeCastRef = pCurrent;
                                                            fScalar = distanceToContact.length3();
                                                            fShapeCastDistance = HK2NI(fScalar);
                                                            fShapeCastCenterPointDistance = fDistancePick;
                                                            pfuzzyBody = static_cast<bhkRigidBody*>(
                                                                bhkEntity::Getbhk(pRB)
                                                            );
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            uiInterfaceCheckpoint = 3507;
                            pLast = pCurrent;
                        }
                    }
                }
            }
        }
        fHavokPickDistance = fHavokPickDistance * afPickDistance;
        uiInterfaceCheckpoint = 3509;
        if (kActivatePick.m_pickResults.m_uiValidResults) {
            uiInterfaceCheckpoint = 3510;
            NiAVObject* pAV = kActivatePick.m_pickResults.GetAt(0)->GetAVObject();
            uiInterfaceCheckpoint = 3511;
            pResult = TESObjectREFR::FindReferenceFor3D(pAV);
            uiInterfaceCheckpoint = 3512;
            afObjectDistance = kActivatePick.m_pickResults.GetAt(0)->m_fDistance;
            uiInterfaceCheckpoint = 3513;
            if (fHavokPickDistance < static_cast<double>(afObjectDistance)) {
                afObjectDistance = fHavokPickDistance;
                pResult = pHavokPickRef;
            }
        }
        else if (pHavokPickRef) {
            afObjectDistance = fHavokPickDistance;
            pResult = pHavokPickRef;
        }
        uiInterfaceCheckpoint = 3514;
        if (bUseFuzzy) {
            bool bFuzzyOverride = true;
            if (pResult && pResult->GetObjectReference()) {
                bFuzzyOverride = false;
                switch (pResult->GetObjectReference()->GetFormType()) {
                    case LIGH_ID: {
                        if (!static_cast<TESObjectLIGH*>(pResult->GetObjectReference())->GetCanCarry()) {
                            bFuzzyOverride = true;
                        }
                        break;
                    }
                    case STAT_ID:
                    case SCOL_ID:
                    case TREE_ID: {
                        bFuzzyOverride = true;
                        break;
                    }
                    default: {
                        break;
                    }
                }
            }

            if (bFuzzyOverride && pShapeCastRef && pResult != pShapeCastRef) {
                afObjectDistance = fShapeCastDistance;
                pResult = pShapeCastRef;
                abIsFuzzy = true;
                ViewCaster::pFuzzyBody = pfuzzyBody;
            }
        }
    }

    return pResult;
}

// The layer bits are COL_LAYER_BIT values (static through charcontroller): 0x700EEF7E.
void ViewCaster::SetCustomPickCollisions() {
    bhkCollisionFilter::SetMultiLayerCollision(L_CUSTOMPICK2, 0x700EEF7Eull);
}

void* ViewCaster::GetObject() {
    return spShapePhantom.m_pObject;
}

bhkRigidBody* ViewCaster::GetFuzzyBody() {
    return ViewCaster::pFuzzyBody;
}

void ViewCaster::operator delete(void* apMemory) {
    ourFree(apMemory);
}
