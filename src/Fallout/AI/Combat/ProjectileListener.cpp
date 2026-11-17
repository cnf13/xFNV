#include "Fallout/AI/Combat/ProjectileListener.h"

#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionBox.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkPhantom.h"
#include "BSHavok/bhkPickData.h"
#include "Fallout Shared/Havok/TESHavokUtilities.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Havok/Source/Physics/Collide/Query/Collector/RayCollector/hkpAllRayHitCollector.h"

ProjectileListener::ProjectileListener(bhkCharacterControllerCinfo* a1, Projectile* a2)
  : bhkCharacterController(*a1), pProj(a2) {}

ProjectileListener::~ProjectileListener() {}

void ProjectileListener::contactPointAddedCallback(const hkpCharacterProxy* apProxy, const hkpRootCdPoint& point) {
    alignas(16) NiMatrix3 rot2_2;
    NiMatrix3 rot;
    CFilter v27;
    CFilter v33;
    CFilter v35;
    alignas(16) CFilter v36;
    Projectile* pprojectile = pProj;
    if (pprojectile) {
        CFilter* v3;
        CFilter* v4;
        bhkPhantom* pbp;
        NiPoint3 normal;
        NiPoint3 contact;
        TESObjectREFR* ptarget = TESHavokUtilities::FindCollidableRef_009C69E0(point.m_rootCollidableB);
        HK2NI(contact, point.m_contact.getPosition());
        HK2NINS(normal, point.m_contact.getNormal());
        const hkpCollidable* pcollidable = point.m_rootCollidableB;
        uint32_t outputShapeKey = point.m_shapeKeyB;
        hkpPhantom* pphantom = hkpGetPhantom(pcollidable);
        if (pphantom) {
            pbp = bhkPhantom::Getbhk(pphantom);
        }
        else {
            pbp = 0;
        }
        if (ptarget &&
            (pbp &&
             ((v3 = pbp->GetCollisionFilter(&v36), v3->GetLayer() == L_PROJECTILEZONE) ||
              (v4 = pbp->GetCollisionFilter(&v35), v4->GetLayer() == L_GASTRAP)))) {
            ExtraDataList* Extra = &ptarget->m_Extra;
            Script::SetActionFlag(pprojectile, Extra, SCRIPT_ACTION_ONHIT);
            if (pprojectile->pWeaponSource) {
                ExtraDataList* v9 = &ptarget->m_Extra;
                Script::SetActionFlag(pprojectile->pWeaponSource, v9, SCRIPT_ACTION_ONHITWITH);
            }
        }
        else {
            char v34 = 0;
            if (pbp) {
                CFilter* v5;
                if (NiObject::IsKindOf_0(&bhkCollisionBox::ms_RTTI, pbp) ||
                    (v5 = pbp->GetCollisionFilter(&v33), v5->GetLayer() == L_CHARCONTROLLER)) {
                    bhkRigidBody* pfirstbody;
                    NiAVObject* pav;
                    v34 = 1;
                    if (ptarget) {
                        pav = (NiAVObject*)ptarget->Get3D();
                    }
                    else {
                        pav = 0;
                    }
                    bhkCollisionObject* pcol = FindFirstCollisionObject(pav);
                    if (pcol) {
                        pfirstbody = pcol->GetRigidBody();
                    }
                    else {
                        pfirstbody = 0;
                    }
                    if (pfirstbody) {
                        CFilter* CollisionFilter = pfirstbody->GetCollisionFilter(&v27);
                        if (CollisionFilter->GetNoCollision() || !pfirstbody->GethkWorld()) {
                            NiMatrix3 rot2;
                            NiPoint3 direction;
                            bhkPickData pd;
                            NiPoint3 origin = pprojectile->GetLocationOnReference();
                            float a2 = pprojectile->GetHeading(0);
                            rot.MakeZRotation(a2);
                            float a2a = pprojectile->GetLooking();
                            rot2.MakeXRotation(a2a);
                            rot = *::new (&rot2_2) NiMatrix3(rot * rot2);
                            rot.GetCol(1u, direction);
                            direction.Unitize();
                            NiPoint3 length = direction * 1000.0f;
                            pd.NiSetFrom(origin);
                            pd.NiSetLength(length);
                            hkpAllRayHitCollector collector;
                            pd.SetAllCollector(&collector);
                            if (Projectile::PickSceneGraph(&pd, pav, 0, 1) &&
                                (&pd.pAllCollector->getHits())->m_size > 0) {
                                v34 = 0;
                                const hkpWorldRayCastOutput* v7 =
                                    &(&pd.pAllCollector->getHits())->operator[]<hkpWorldRayCastOutput>(0);
                                hkpWorldRayCastOutput castHit(*v7);
                                NiPoint3 hitVector = length * castHit.m_hitFraction;
                                contact = origin + hitVector;
                                HK2NINS(normal, castHit.m_normal);
                                pcollidable = castHit.m_rootCollidable;
                                for (int32_t i = 0; i < 8 && castHit.m_shapeKeys[i] != -1; ++i) {
                                    uint32_t outputShapeKey_2 = castHit.m_shapeKeys[i];
                                }
                            }
                        }
                    }
                }
            }
            if (!v34) {
                pprojectile->AddImpact(ptarget, contact, normal, pcollidable, outputShapeKey, BHK_MATERIAL_MAX);
            }
        }
    }
}
