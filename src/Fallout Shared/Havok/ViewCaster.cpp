#include "Fallout Shared/Havok/ViewCaster.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkEntity.h"
#include "BSHavok/bhkPickData.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkShapePhantom.h"
#include "BSHavok/bhkSimpleShapePhantom.h"
#include "BSHavok/bhkSphereShape.h"
#include "BSHavok/bhkTransform.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
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
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
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
        MEM_CONTEXT_HAVOK, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Havok\\ViewCaster.cpp", 73
    );
    bhkSphereShape* pshape;
    bhkSphereShape* pMemory = static_cast<bhkSphereShape*>(NiMemObject::operator new(sizeof(bhkSphereShape)));
    if (pMemory) {
        pshape = ::new (pMemory) bhkSphereShape(afRadius, true);
    }
    else {
        pshape = nullptr;
    }
    alignas(16) hkSimpleShapePhantomCinfo csi;
    CFilter ifilter;
    ifilter.iFilter = 0;
    ifilter.SetLayer(L_CUSTOMPICK2);
    ifilter.SetGroup(aiGroup);
    iGroup = static_cast<int32_t>(aiGroup);
    csi.m_collisionFilterInfo = ifilter.iFilter;
    csi.m_shape = **pshape;
    csi.m_transform.setIdentity();
    bhkSimpleShapePhantom* pPhantom;
    bhkSimpleShapePhantom* pPhantomMemory =
        static_cast<bhkSimpleShapePhantom*>(NiMemObject::operator new(sizeof(bhkSimpleShapePhantom)));
    if (pPhantomMemory) {
        pPhantom = ::new (pPhantomMemory) bhkSimpleShapePhantom(csi);
    }
    else {
        pPhantom = nullptr;
    }
    spShapePhantom = static_cast<bhkShapePhantom*>(pPhantom);
    bInit = true;
}

bhkWorld* ViewCaster::GetWorld() {
    bhkShapePhantom* v1;
    const hkpWorld* v2;
    bhkWorld* v5 = nullptr;
    if (spShapePhantom.m_pObject) {
        v1 = spShapePhantom.m_pObject;
        v2 = v1->bhkWorldObject::GethkWorld();
        return bhkWorld::Getbhk(v2);
    }
    return v5;
}

void ViewCaster::Add(bhkWorld* apWorld) {
    AutoMemContext apMemory(
        MEM_CONTEXT_HAVOK, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Havok\\ViewCaster.cpp", 116
    );
    if (GetObject()) {
        bhkSimpleShapePhantom* v4 = static_cast<bhkSimpleShapePhantom*>(GetObject());
        v4->Add(apWorld);
        if (apWorld) {
            uint32_t v3 = hkColor::rgbFromChars(88u, 173u, 86u, 25u);
            bhkWorldObject* v2 = static_cast<bhkWorldObject*>(GetObject());
            apWorld->SetDebugObjectColor(v2, v3);
        }
    }
}

void ViewCaster::SetShapeGroup(uint32_t aiGroup) {
    CFilter aiFilter;
    void* v2 = GetObject();
    static_cast<bhkWorldObject*>(v2)->GetCollisionFilter(&aiFilter);
    (&aiFilter)->SetGroup(aiGroup);
    iGroup = (int32_t)aiGroup;
    void* v3 = GetObject();
    static_cast<bhkWorldObject*>(v3)->SetCollisionFilter(aiFilter);
}

TESObjectREFR* ViewCaster::FindActivateREFR(
    const NiPoint3& aOrigin,
    const NiPoint3& aDirection,
    float afPickDistance,
    float& afObjectDistance,
    bool& abIsFuzzy
) {
    CFilter v38;
    CFilter v57;
    hkpShapeRayCastInput rci;
    alignas(16) CFilter fltr;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_HAVOK, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Havok\\ViewCaster.cpp", 155
    );
    ViewCaster::pFuzzyBody = nullptr;
    TESObjectREFR* presult = nullptr;
    if ((bInit) && (GetObject()) && (GetWorld())) {
        hkVector4 hkto;
        hkVector4 hkfrom;
        hkpAllCdPointCollector PointCollector;
        bhkWorld* pw = GetWorld();
        NiPoint3 nito = aOrigin + aDirection * afPickDistance;
        abIsFuzzy = false;
        afObjectDistance = 3.4028235e+38f;
        alignas(16) NiPick activatePick(0, 8u);
        activatePick.SetFrontOnly(false);
        activatePick.SetObserveAppCullFlag(true);
        uiInterfaceCheckpoint = 3501;
        NI2HK(hkfrom, aOrigin);
        NI2HK(hkto, nito);
        bool bhits = false;
        hkpLinearCastInput input;
        input.m_startPointTolerance = 0.01f;
        input.m_maxExtraPenetration = 0.01f;
        input.m_to = hkto;
        SetCustomPickCollisions();
        static_cast<bhkShapePhantom*>(GetObject())->SetPositionAndLinearCast(hkfrom, input, PointCollector, nullptr);
        if (PointCollector.hasHit().operator bool()) {
            PointCollector.sortHits();
            bhits = true;
        }
        TESObjectREFR* phavokPickRef = nullptr;
        float fhavokPickDistance = 3.4028235e+38f;
        TESObjectREFR* pshapeCastRef = nullptr;
        float fshapeCastDistance = 3.4028235e+38f;
        float fshapeCastCenterPointDistance = 3.4028235e+38f;
        bhkRigidBody* pfuzzyBody = nullptr;
        bool buseFuzzy = bUseFuzzyPicking.Bool();
        int32_t ifuzzyPicks = 0;
        int32_t ihavokPicks = 0;
        int32_t igamebryoPicks = 0;
        if (bhits) {
            uiInterfaceCheckpoint = 3502;
            TESObjectREFR* plast = nullptr;
            TESObjectREFR* pcurrent = nullptr;
            hkpRigidBody* plastBody = nullptr;
            for (int32_t i = 0;; ++i) {
                int32_t iHitCount = (&PointCollector.getHits())->m_size;
                if (i >= iHitCount) {
                    break;
                }
                uiInterfaceCheckpoint = 3503;
                const hkpRootCdPoint* pHit = &(&PointCollector.getHits())->operator[]<hkpRootCdPoint>(i);
                hkpRootCdPoint point(*pHit);
                if (point.m_rootCollidableB) {
                    uiInterfaceCheckpoint = 3504;
                    NiAVObject* pav = GetAVObjectForCollidable(point.m_rootCollidableB);
                    uiInterfaceCheckpoint = 3505;
                    if (pav) {
                        pcurrent = TESObjectREFR::FindReferenceFor3D(pav);
                    }
                    if ((pcurrent) && (pcurrent != pPlayer)) {
                        bool bculled = false;
                        NiAVObject* pcullTest = pav;
                        NiNode* p3D = pcurrent->Get3D();
                        while (!bculled && (pcullTest && pcullTest != p3D)) {
                            int32_t iCulled = bculled;
                            bool AppCulled = pcullTest->GetAppCulled();
                            bculled = AppCulled || iCulled;
                            pcullTest = pcullTest->m_pkParent;
                        }
                        if (!bculled) {
                            hkpRigidBody* prb = hkpGetRigidBody(point.m_rootCollidableB);
                            bool v62 = false;
                            fltr.iFilter = point.m_rootCollidableB->getCollisionFilterInfo();
                            COL_LAYER Layer = fltr.GetLayer();
                            if (Layer == L_STATIC) {
                                const hkpShape* Shape = point.m_rootCollidableB->m_shape;
                                if ((Shape) && (Shape->getType() == HK_SHAPE_MOPP)) {
                                    v62 = true;
                                }
                            }
                            else {
                                if (Layer > L_TRANSPARENT && Layer <= L_PROJECTILE) {
                                    v62 = true;
                                }
                            }
                            TESObjectSTAT* ObjectReference =
                                static_cast<TESObjectSTAT*>(pcurrent->GetObjectReference());
                            if (ObjectReference == pCollisionMarker) {
                                v62 = true;
                            }
                            if (prb && (prb != plastBody && v62)) {
                                plastBody = prb;
                                int32_t* piMaxPicks = &iMaxViewCasterPicksHavok.Int();
                                if (ihavokPicks < *piMaxPicks) {
                                    ++ihavokPicks;
                                    rci.~hkpShapeRayCastInput();
                                    ::new (&rci) hkpShapeRayCastInput;
                                    rci.m_filterInfo =
                                        static_cast<bhkWorldObject*>(GetObject())->GetCollisionFilter(&v57)->iFilter;
                                    const hkTransform* Transform = &prb->getTransform();
                                    hkTransform hkt(*Transform);
                                    rci.m_from.setTransformedInversePos(hkt, hkfrom);
                                    rci.m_to.setTransformedInversePos(hkt, hkto);
                                    hkpShapeRayCastOutput rco;
                                    rco.reset_0();
                                    const hkpShape* pcurShape = point.m_rootCollidableB->m_shape;
                                    hkBool bCastHit = pcurShape->castRay(rci, rco);
                                    if (rco.hasHit().operator bool() &&
                                        fhavokPickDistance > static_cast<double>(rco.m_hitFraction)) {
                                        phavokPickRef = pcurrent;
                                        fhavokPickDistance = rco.m_hitFraction;
                                    }
                                }
                            }
                            else {
                                uiInterfaceCheckpoint = 3506;
                                if ((plast != pcurrent) && (pcurrent->Get3D())) {
                                    int32_t* piMaxPicks = &iMaxViewCasterPicksGamebryo.Int();
                                    if (igamebryoPicks < *piMaxPicks) {
                                        ++igamebryoPicks;
                                        NiNode* pcurrent3D = pcurrent->Get3D();
                                        PickDistance(
                                            &activatePick, aOrigin, aDirection, afPickDistance, pcurrent3D, false
                                        );
                                    }
                                }
                            }
                            bool v51 = true;
                            switch (fltr.GetLayer()) {
                                case L_STATIC:
                                case L_TERRAIN:
                                case L_GROUND:
                                case L_PORTAL: {
                                    v51 = false;
                                    break;
                                }
                                case L_PROJECTILE: {
                                    if (!prb) {
                                        v51 = false;
                                    }
                                    break;
                                }
                                case L_CHARCONTROLLER: {
                                    if (pcurrent->IsActor()) {
                                        MobileObject* pmobile = static_cast<MobileObject*>(pcurrent);
                                        BaseProcess* pprocess = pmobile->GetCurrentAIProcess();
                                        bhkCharacterController* pcon = pmobile->MobileObject::GetCharController();
                                        if ((pprocess) && (pcon) &&
                                            (pcon->GetSize() == bhkCharacterController::CS_SIZED)) {
                                            SIT_SLEEP_STATE estate = pprocess->GetSitSleepState();
                                            if (estate == SIT_SLEEP_IS_SLEEPING || estate == SIT_SLEEP_IS_SITTING) {
                                                v51 = false;
                                            }
                                        }
                                    }
                                    break;
                                }
                                default: {
                                    break;
                                }
                            }
                            if ((v51 && buseFuzzy) && (pw)) {
                                hkVector4 distanceFromPick;
                                hkVector4 pickDirection;
                                hkVector4 distanceToContact;
                                distanceToContact.setSub4(point.m_contact.getPosition(), hkfrom);
                                pickDirection.setSub4(hkto, hkfrom);
                                pickDirection.normalize3();
                                pickDirection.mul4(pickDirection.dot3(distanceToContact));
                                distanceFromPick.setSub4(distanceToContact, pickDirection);
                                float fScalar = distanceFromPick.length3().operator float();
                                float fdistancePick = HK2NI(fScalar);
                                if (fshapeCastCenterPointDistance > static_cast<double>(fdistancePick)) {
                                    int32_t* piMaxPicks = &iMaxViewCasterPicksFuzzy.Int();
                                    if (ifuzzyPicks < *piMaxPicks) {
                                        if (pshapeCastRef && pshapeCastRef == pcurrent) {
                                            float fScalara = distanceToContact.length3().operator float();
                                            fshapeCastDistance = HK2NI(fScalara);
                                            fshapeCastCenterPointDistance = fdistancePick;
                                        }
                                        else {
                                            ++ifuzzyPicks;
                                            bhkPickData pd;
                                            CFilter* CollisionFilter =
                                                static_cast<bhkWorldObject*>(GetObject())->GetCollisionFilter(&v38);
                                            pd.SetFilter(*CollisionFilter);
                                            pd.SetFrom(hkfrom);
                                            pd.SetTo(point.m_contact.getPosition());
                                            hkpAllRayHitCollector pcollector;
                                            pd.SetAllCollector(&pcollector);
                                            pw->PickObject(pd);
                                            if (!pd.bPickFailed) {
                                                if ((&pcollector.getHits())->m_size <= 0) {
                                                    pshapeCastRef = pcurrent;
                                                    float fScalarc = distanceToContact.length3().operator float();
                                                    fshapeCastDistance = HK2NI(fScalarc);
                                                    fshapeCastCenterPointDistance = fdistancePick;
                                                    pfuzzyBody = static_cast<bhkRigidBody*>(bhkEntity::Getbhk(prb));
                                                }
                                                else {
                                                    const hkpWorldRayCastOutput* pRayHit =
                                                        &(&pcollector.getHits())->operator[]<hkpWorldRayCastOutput>(0);
                                                    NiAVObject* pav_2 =
                                                        GetAVObjectForCollidable(pRayHit->m_rootCollidable);
                                                    if (pav_2) {
                                                        TESObjectREFR* pref = TESObjectREFR::FindReferenceFor3D(pav_2);
                                                        if ((pref) && (pref == pcurrent)) {
                                                            pshapeCastRef = pcurrent;
                                                            float fScalarb =
                                                                distanceToContact.length3().operator float();
                                                            fshapeCastDistance = HK2NI(fScalarb);
                                                            fshapeCastCenterPointDistance = fdistancePick;
                                                            pfuzzyBody =
                                                                static_cast<bhkRigidBody*>(bhkEntity::Getbhk(prb));
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            uiInterfaceCheckpoint = 3507;
                            plast = pcurrent;
                        }
                    }
                }
            }
        }
        fhavokPickDistance = fhavokPickDistance * afPickDistance;
        uiInterfaceCheckpoint = 3509;
        if ((&activatePick.m_pickResults)->m_uiValidResults) {
            uiInterfaceCheckpoint = 3510;
            NiAVObject* pav_3 = (&activatePick.m_pickResults)->GetAt(0)->GetAVObject();
            uiInterfaceCheckpoint = 3511;
            presult = TESObjectREFR::FindReferenceFor3D(pav_3);
            uiInterfaceCheckpoint = 3512;
            afObjectDistance = (&activatePick.m_pickResults)->GetAt(0)->m_fDistance;
            uiInterfaceCheckpoint = 3513;
            if (fhavokPickDistance < static_cast<double>(afObjectDistance)) {
                afObjectDistance = fhavokPickDistance;
                presult = phavokPickRef;
            }
        }
        else {
            if (phavokPickRef) {
                afObjectDistance = fhavokPickDistance;
                presult = phavokPickRef;
            }
        }
        uiInterfaceCheckpoint = 3514;
        if (buseFuzzy) {
            bool bfuzzyOverride = true;
            if ((presult) && (presult->GetObjectReference())) {
                bfuzzyOverride = false;
                switch (presult->GetObjectReference()->GetFormType()) {
                    case LIGH_ID: {
                        if (!static_cast<TESObjectLIGH*>(presult->GetObjectReference())->GetCanCarry()) {
                            bfuzzyOverride = true;
                        }
                        break;
                    }
                    case STAT_ID:
                    case SCOL_ID:
                    case TREE_ID: {
                        bfuzzyOverride = true;
                        break;
                    }
                    default: {
                        break;
                    }
                }
            }

            if ((bfuzzyOverride) && (pshapeCastRef) && (presult != pshapeCastRef)) {
                afObjectDistance = fshapeCastDistance;
                presult = pshapeCastRef;
                abIsFuzzy = true;
                ViewCaster::pFuzzyBody = pfuzzyBody;
            }
        }
    }

    return presult;
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

hkBool hkpShapeRayCastCollectorOutput::hasHit() const {
    hkBool result;
    result.~hkBool();
    ::new (&result) hkBool(m_hitFraction < 1.0);
    return result;
}

// The ray's ends are left as they are.
hkpShapeRayCastInput::hkpShapeRayCastInput() : m_filterInfo(0), m_rayShapeCollectionFilter(nullptr) {}
