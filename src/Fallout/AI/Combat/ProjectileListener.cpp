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

ProjectileListener::ProjectileListener(bhkCharacterControllerCinfo* apCinfo, Projectile* apProjectile)
  : bhkCharacterController(*apCinfo), pProj(apProjectile) {}

ProjectileListener::~ProjectileListener() {}

void ProjectileListener::contactPointAddedCallback(const hkpCharacterProxy* apProxy, const hkpRootCdPoint& point) {
    alignas(16) NiMatrix3 rotProduct;
    NiMatrix3 rot;
    CFilter kBodyFilter;
    CFilter kCharFilter;
    CFilter kGasTrapFilter;
    alignas(16) CFilter kZoneFilter;
    Projectile* pProjectile = pProj;
    if (pProjectile) {
        NiPoint3 normal;
        NiPoint3 contact;
        TESObjectREFR* pTarget = TESHavokUtilities::FindCollidableRef(point.m_rootCollidableB);
        HK2NI(contact, point.m_contact.getPosition());
        HK2NINS(normal, point.m_contact.getNormal());
        const hkpCollidable* pCollidable = point.m_rootCollidableB;
        uint32_t outputShapeKey = point.m_shapeKeyB;
        hkpPhantom* pPhantom = hkpGetPhantom(pCollidable);
        bhkPhantom* pbp = pPhantom ? bhkPhantom::Getbhk(pPhantom) : nullptr;
        if (pTarget &&
            pbp &&
            (pbp->GetCollisionFilter(&kZoneFilter)->GetLayer() == L_PROJECTILEZONE ||
             pbp->GetCollisionFilter(&kGasTrapFilter)->GetLayer() == L_GASTRAP)) {
            Script::SetActionFlag(pProjectile, &pTarget->m_Extra, SCRIPT_ACTION_ONHIT);
            if (pProjectile->pWeaponSource) {
                Script::SetActionFlag(pProjectile->pWeaponSource, &pTarget->m_Extra, SCRIPT_ACTION_ONHITWITH);
            }
        }
        else {
            char cSkipImpact = 0;
            if (pbp) {
                if (NiObject::IsKindOf_0(&bhkCollisionBox::ms_RTTI, pbp) ||
                    pbp->GetCollisionFilter(&kCharFilter)->GetLayer() == L_CHARCONTROLLER) {
                    cSkipImpact = 1;
                    NiAVObject* pav = pTarget ? pTarget->Get3D() : nullptr;
                    bhkCollisionObject* pcol = FindFirstCollisionObject(pav);
                    bhkRigidBody* pFirstBody = pcol ? pcol->GetRigidBody() : nullptr;
                    if (pFirstBody) {
                        if (pFirstBody->GetCollisionFilter(&kBodyFilter)->GetNoCollision() ||
                            !pFirstBody->GethkWorld()) {
                            NiMatrix3 rot2;
                            NiPoint3 direction;
                            bhkPickData pd;
                            NiPoint3 origin = pProjectile->GetLocationOnReference();
                            float fHeading = pProjectile->GetHeading(false);
                            rot.MakeZRotation(fHeading);
                            float fLooking = pProjectile->GetLooking();
                            rot2.MakeXRotation(fLooking);
                            rot = *::new (&rotProduct) NiMatrix3(rot * rot2);
                            rot.GetCol(1u, direction);
                            direction.Unitize();
                            NiPoint3 length = direction * 1000.0f;
                            pd.NiSetFrom(origin);
                            pd.NiSetLength(length);
                            hkpAllRayHitCollector collector;
                            pd.SetAllCollector(&collector);
                            if (Projectile::PickSceneGraph(&pd, pav, false, true) &&
                                pd.pAllCollector->getHits().m_size > 0) {
                                cSkipImpact = 0;
                                hkpWorldRayCastOutput castHit(pd.pAllCollector->getHits()[0]);
                                NiPoint3 hitVector = length * castHit.m_hitFraction;
                                contact = origin + hitVector;
                                HK2NINS(normal, castHit.m_normal);
                                pCollidable = castHit.m_rootCollidable;
                                for (int32_t i = 0; i < 8 && castHit.m_shapeKeys[i] != -1; ++i) {}
                            }
                        }
                    }
                }
            }
            if (!cSkipImpact) {
                pProjectile->AddImpact(pTarget, contact, normal, pCollidable, outputShapeKey, BHK_MATERIAL_MAX);
            }
        }
    }
}
