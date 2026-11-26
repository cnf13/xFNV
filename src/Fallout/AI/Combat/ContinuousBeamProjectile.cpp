#include "Fallout/AI/Combat/ContinuousBeamProjectile.h"

#include "BSCore/MemoryManager.h"
#include "BSShader/BeamShaderProperty.h"
#include "Fallout Shared/BGSProjectile.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryDataIterators.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAdditionalGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTConvertingStrideIterator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStrips.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysCylinderEmitter.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

ContinuousBeamProjectile::ContinuousBeamProjectile(bool abAdd) : Projectile(abAdd), spBeamProperty(nullptr) {
    cFormType = PCBE_ID;
}

ContinuousBeamProjectile::ContinuousBeamProjectile(
    BGSProjectile* apProjectileBase,
    TESObjectREFR* apShooter,
    TESObjectWEAP* apFromWeapon,
    NiPoint3 aOrigin,
    float afZAngle,
    float afXAngle
)
  : Projectile(apProjectileBase, apShooter, apFromWeapon, aOrigin, afZAngle, afXAngle), spBeamProperty(nullptr) {
    cFormType = PCBE_ID;
}

void ContinuousBeamProjectile::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    Projectile::SaveGame(apSaveGameBuffer);
}

void ContinuousBeamProjectile::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    Projectile::LoadGame(apLoadGameBuffer);
}

void ContinuousBeamProjectile::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    Projectile::InitLoadGame(apLoadGameBuffer);
}

void ContinuousBeamProjectile::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    Projectile::Revert(apLoadGameBuffer);
}

void ContinuousBeamProjectile::FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    Projectile::FinishLoadGame(apLoadGameBuffer);
}

Projectile::PROJECTILE_TYPE ContinuousBeamProjectile::GetProjectileType() {
    return PROJECTILE_TYPE_CONTINUOUS_BEAM;
}

// Becomes the shooter's one persistent beam, ending the previous one; a beam without a shooter ends at once.
void ContinuousBeamProjectile::Initialize() {
    Projectile::SetFlag(Projectile::TRACER, false);
    Projectile::SetFlag(Projectile::HITSCAN_MODE, true);
    Projectile::SetFlag(Projectile::NO_3D, false);
    Projectile::SetFlag(Projectile::NO_COLLISION, true);
    Projectile::SetFlag(Projectile::FADING, false);
    Projectile::SetFlag(Projectile::CHECK_BEHIND_WALL, true);
    Projectile::SetFlag(Projectile::POST_PROCESS, true);
    Projectile::SetFlag(Projectile::SPAWN_EXPLOSION_AT_IMPACT, true);
    iShotsFired = 0;
    Projectile::Initialize();
    if (spBeamProperty) {
        float fRange = GetRange();
        spBeamProperty->SetLength(fRange);
    }
    Actor* pShooterActor = static_cast<Actor*>(GetShooter());
    if (pShooterActor) {
        if (pShooterActor->pContinuousBeamPersistant) {
            pShooterActor->pContinuousBeamPersistant->Kill();
        }
        pShooterActor->pContinuousBeamPersistant = this;
    }
    else {
        Warning("Continuous Beam created without Shooter!");
        Kill();
    }
}

// Scales the beam to its range (at least 1), then stores the root's rotation relative to the shooter's fire node.
void ContinuousBeamProjectile::Process3D() {
    NiNode* pRootNode = Get3D();
    if (pRootNode) {
        float fRange = GetRange();
        if (fRange < 1.0) {
            fRange = 1.0f;
        }
        ScaleBeam3D(pRootNode, fRange, 0.0f);
        NiAVObject* pProjectileNode;
        if (GetShooter()) {
            pProjectileNode = GetShooter()->GetFireNode();
        }
        else {
            pProjectileNode = nullptr;
        }
        if (pProjectileNode) {
            NiUpdateData kUpdateData(0.0f, false, false);
            pRootNode->UpdateWorldData(kUpdateData);
            NiMatrix3 kInverse;
            const NiMatrix3* pInverse;
            pInverse = pProjectileNode->GetWorldTransform().m_Rotate.Inverse(&kInverse);
            kFollowOffset.m_Rotate = Get3D()->GetWorldTransform().m_Rotate * *pInverse;
        }
    }
}

void ContinuousBeamProjectile::UpdateProjectile(float afDelta) {
    if (!GetDelete()) {
        NiAVObject* pfireNode;
        CheckExplosion(afDelta);
        Actor* pactor = static_cast<Actor*>(GetShooter());
        if (pactor->IsActor()) {
            pactor = static_cast<Actor*>(GetShooter());
        }
        else {
            pactor = nullptr;
        }
        if (GetShooter()) {
            Actor* pShooterActor = static_cast<Actor*>(GetShooter());
            pfireNode = pShooterActor->GetFireNode();
        }
        else {
            pfireNode = nullptr;
        }
        if (IsFlagSet(Projectile::FADING) && GetAlpha() < 0.01) {
            Kill();
        }
        if (pactor) {
            float faimLooking = pactor->GetLooking();
            float faimHeading = pactor->GetHeading(false);
            if (pactor == pPlayer) {
                NiPoint3 kdirection = pPlayer->GetBulletAutoAim() - data.kPosition;
                kdirection.Unitize();
                faimHeading = GetZAngleFromVector(kdirection);
                faimLooking = -std::asinf(kdirection.z);
            }
            else {
                float funused;
                pfireNode->GetWorldTransform().m_Rotate.ToEulerAnglesZXY(faimHeading, faimLooking, funused);
            }
            bool balwaysHit = false;
            TESPackage* ppack = pactor ? pactor->GetCurrentPackage() : nullptr;
            if (ppack) {
                const TESUseWeaponPackageData* ppackData = ppack->GetUseWeaponPackageData();
                if (ppackData) {
                    balwaysHit = ppackData->bAlwaysHit;
                }
            }
            else {
                balwaysHit = IsFlagSet(Projectile::ALWAYS_HIT_TARGET);
            }
            if (!balwaysHit) {
                BaseProcess* pactorAIProcess = pactor ? pactor->pCurrentProcess : nullptr;
                ItemChange* pweaponItem = pactorAIProcess ? pactorAIProcess->GetCurrentWeapon() : nullptr;
                float fskill = pactor ? pactor->GetGunSkill(GUN_SKILL_GUN) : fGunReferenceSkill.Float();
                bool bUseSpreadMod = false;
                if (pactor && pactor->pCurrentProcess && pweaponItem && pweaponItem->HasModEffectActive(3)) {
                    bUseSpreadMod = true;
                }

                double dMinSpreadDegrees = pWeaponSource->GetMinSpreadDegrees(bUseSpreadMod);
                double dSpreadDegrees = pWeaponSource->GetSpreadDegrees(bUseSpreadMod);
                float fTotalSpread = (dSpreadDegrees * fskill + dMinSpreadDegrees) * 0.01745329238474369;
                pactor->GetGunSkill(GUN_SKILL_ACTOR);
                float fdriftRad = pactor->pCurrentProcess->GetAimLooking();
                float fspreadRad = fTotalSpread + fdriftRad;
                float fradius = RandomFloat(0.0f, fspreadRad);
                float frandomAngle = RandomFloat(0.0f, 6.2831855f);
                double dSin = std::sin(frandomAngle);
                faimLooking = dSin * fradius + faimLooking;
                double dCos = std::cos(frandomAngle);
                faimHeading = dCos * fradius + faimHeading;
            }
            SetAngleOnReferenceZ(faimHeading);
            SetAngleOnReferenceX(faimLooking);
            SetAngleOnReferenceY(0.0f);
        }
        NiNode* p3d = Get3D();
        if (pfireNode && p3d) {
            bool bRoot = false;
            if (pWeaponSource->HasScope() && pactor->GetIronSights() && pactor == pPlayer) {
                bRoot = true;
            }
            NiCamera* pcam = Main::WorldRootCamera();
            if (bRoot && pcam) {
                static NiPoint3 offset(0.0f, -15.0f, 0.0f);
                SetLocationOnReference(pcam->GetWorldTransform() * offset);
            }
            else {
                SetLocationOnReference(pfireNode->GetWorldTranslate());
            }
        }

        Sync3DWithReference(true, false);
        SetRange(10000.0f);
        RunTargetPick();
        Process3D();
        if (spBeamProperty.m_pObject) {
            float fAlpha = GetAlpha();
            spBeamProperty->SetAlpha(fAlpha);
            float fRange = GetRange();
            spBeamProperty->SetLength(fRange);
        }
    }
}

ContinuousBeamProjectile::~ContinuousBeamProjectile() {}

// A fading beam takes no impacts. A new impact replaces the processed ones and sets the beam's range to the hit's
// distance.
bool ContinuousBeamProjectile::AddImpact(
    TESObjectREFR* apCollidee,
    const NiPoint3& aContactPoint,
    const NiPoint3& aContactNormal,
    const hkpCollidable* apTargetCollidable,
    uint32_t aiKey,
    HavokMaterialType aeForceMaterial
) {
    if (!Projectile::IsValidImpact(apCollidee, apTargetCollidable, this, GetProjectileBase()) ||
        Projectile::IsFlagSet(Projectile::FADING) ||
        !Projectile::AddImpact(apCollidee, aContactPoint, aContactNormal, apTargetCollidable, aiKey, aeForceMaterial)) {
        return false;
    }
    if (bImpactProcessed) {
        ImpactList.RemoveAll();
    }
    bImpactProcessed = false;
    SetRange((aContactPoint - GetLocationOnReference()).Length());
    return true;
}

void ContinuousBeamProjectile::SetKill(Actor* apActor) {
    Projectile::SetFlag(Projectile::FADING, true);
}

void ContinuousBeamProjectile::ScaleBeam3D(NiAVObject* ap3D, float afDesiredLength, float afCurrentLength) {
    if (ap3D) {
        if (afCurrentLength > 0.0) {
            float fscale = afDesiredLength / afCurrentLength;
            NiPoint3 location = ap3D->GetTranslate();
            location *= fscale;
            ap3D->SetTranslate(location);
        }
        if (NiObject::IsKindOf_0(&NiGeometry::ms_RTTI, ap3D)) {
            NiGeometry* pgeometry = static_cast<NiGeometry*>(ap3D);
            float fcurrentLength = afCurrentLength > 0.0 ? afCurrentLength : FindBeamGeometryLength(pgeometry);
            if (fcurrentLength != 0.0) {
                float fscale = afDesiredLength / fcurrentLength;
                ScaleBeamGeometryLength(pgeometry, fscale);
            }
        }
        else {
            NiNode* pnode = ap3D->IsNode();
            if (pnode) {
                for (uint32_t n = 0; n < pnode->GetArrayCount(); ++n) {
                    NiAVObject* pobj = pnode->GetAt(n);
                    ScaleBeam3D(pobj, afDesiredLength, afCurrentLength);
                }
            }
        }
    }
}

void ContinuousBeamProjectile::SetShotsFired(int32_t num) {
    iShotsFired = static_cast<uint32_t>(num);
}

void ContinuousBeamProjectile::ScaleBeamGeometryLength(NiGeometry* apGeometry, float afScaleFactor) {
    if (apGeometry) {
        float* pRadius = &apGeometry->GetModelBound().m_fRadius;
        *pRadius = *pRadius * afScaleFactor;
        apGeometry->GetModelBound().m_kCenter *= afScaleFactor;
        NiGeometryData* pdata = apGeometry->GetModelData();
        if (pdata) {
            if (NiObject::IsKindOf_0(&NiPSysData::ms_RTTI, pdata)) {
                if (NiObject::IsKindOf_0(&NiParticleSystem::ms_RTTI, apGeometry)) {
                    NiParticleSystem* pparticles = static_cast<NiParticleSystem*>(apGeometry);
                    for (uint32_t i = 0; i < pparticles->GetModifierCount(); ++i) {
                        NiPSysModifier* pmod = pparticles->GetModifierAt(i);
                        if (NiObject::IsKindOf_0(&NiPSysCylinderEmitter::ms_RTTI, pmod)) {
                            NiPSysCylinderEmitter* pemitter = static_cast<NiPSysCylinderEmitter*>(pmod);
                            float fheight = pemitter->GetEmitterHeight() * afScaleFactor;
                            pemitter->SetEmitterHeight(fheight);
                        }
                    }
                }
            }
            else {
                if (pdata->GetAdditionalGeometryData() && pdata->GetAdditionalGeometryData()->GetVBReady()) {
                    NiTConvertingStrideIterator<NiPoint3> kIterator;
                    NiAdditionalGeometryData* pAdditionalGeometryData = pdata->GetAdditionalGeometryData();
                    if (pdata->LockPackedVertexData(true)) {
                        pdata->GetVerticesIterator(*static_cast<NiGeometryDataIteratorNiPoint3*>(&kIterator));
                        for (int32_t iIndex = 0; iIndex < pAdditionalGeometryData->m_usVertexCount; ++iIndex) {
                            NiPoint3 kVertex = NiPoint3::ZERO;
                            kIterator.Get(iIndex, kVertex);
                            kVertex.y = kVertex.y * afScaleFactor;
                            static_cast<NiGeometryDataIteratorNiPoint3*>(&kIterator)
                                ->NiGeometryDataIteratorNiPoint3::Set(iIndex, kVertex);
                        }
                        pdata->UnlockPackedVertexData();
                    }
                }
                else if (pdata->m_pkVertex) {
                    for (int32_t i = 0; i < pdata->m_usVertices; ++i) {
                        pdata->m_pkVertex[i].y = pdata->m_pkVertex[i].y * afScaleFactor;
                        pdata->m_pkTexture[i].y = pdata->m_pkTexture[i].y * afScaleFactor;
                    }
                    apGeometry->PurgeRendererData(NiRenderer::GetRenderer());
                }
            }
        }
    }
}

float ContinuousBeamProjectile::FindBeamGeometryLength(NiGeometry* apGeometry) {
    NiGeometryData* pModelData = apGeometry->GetModelData();
    float fLength = 0.0f;
    if (pModelData) {
        if (NiObject::IsKindOf_0(&NiPSysData::ms_RTTI, pModelData)) {
            if (NiObject::IsKindOf_0(&NiParticleSystem::ms_RTTI, apGeometry)) {
                NiParticleSystem* pSystem = static_cast<NiParticleSystem*>(apGeometry);
                // The emitter height of the system's cylinder emitter.
                for (uint32_t uiIndex = 0; fLength == 0.0 && uiIndex < pSystem->GetModifierCount(); ++uiIndex) {
                    NiPSysModifier* pModifier = pSystem->GetModifierAt(uiIndex);
                    if (NiObject::IsKindOf_0(&NiPSysCylinderEmitter::ms_RTTI, pModifier)) {
                        fLength = static_cast<NiPSysCylinderEmitter*>(pModifier)->m_fEmitterHeight;
                    }
                }
            }
        }
        else {
            // The largest y of the geometry's vertices, from the packed vertex buffer when it is ready.
            NiAdditionalGeometryData* pAdditionalData;
            if (pModelData->GetAdditionalGeometryData() &&
                (pAdditionalData = pModelData->GetAdditionalGeometryData(), pAdditionalData->GetVBReady())) {
                NiTConvertingStrideIterator<NiPoint3> kIterator;
                NiAdditionalGeometryData* pVertexData = pModelData->GetAdditionalGeometryData();
                if (pModelData->LockPackedVertexData(true)) {
                    for (int32_t i = 0; i < pVertexData->m_usVertexCount; ++i) {
                        NiPoint3 kVertex = NiPoint3::ZERO;
                        pModelData->GetVerticesIterator(*static_cast<NiGeometryDataIteratorNiPoint3*>(&kIterator));
                        kIterator.Get(i, kVertex);
                        if (kVertex.y > static_cast<double>(fLength)) {
                            fLength = kVertex.y;
                        }
                    }
                    pModelData->UnlockPackedVertexData();
                }
            }
            else if (pModelData->m_pkVertex) {
                for (int32_t i = 0; i < pModelData->m_usVertices; ++i) {
                    float fY = pModelData->m_pkVertex[i].y;
                    if (fY > static_cast<double>(fLength)) {
                        fLength = fY;
                    }
                }
            }
        }
    }
    return fLength;
}
