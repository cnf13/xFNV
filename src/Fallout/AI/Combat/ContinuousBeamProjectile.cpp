#include "Fallout/AI/Combat/ContinuousBeamProjectile.h"

#include "BSCore/MemoryManager.h"
#include "BSShader/BeamShaderProperty.h"
#include "Fallout Shared/BGSProjectile.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/FaceGen/NiGeometryDataIteratorNiPoint3.h"
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
    if (spBeamProperty.operator->()) {
        float fRange = GetRange();
        spBeamProperty.operator->()->SetLength(fRange);
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
            NiUpdateData kUpdateData;
            kUpdateData.~NiUpdateData();
            ::new (&kUpdateData) NiUpdateData(0.0f, false, false);
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
        Actor* pactor_2;
        CheckExplosion(afDelta);
        Actor* pactor = static_cast<Actor*>(GetShooter());
        if (pactor->IsActor()) {
            pactor_2 = static_cast<Actor*>(GetShooter());
        }
        else {
            pactor_2 = nullptr;
        }
        if (GetShooter()) {
            Actor* pactor_4 = static_cast<Actor*>(GetShooter());
            pfireNode = pactor_4->GetFireNode();
        }
        else {
            pfireNode = nullptr;
        }
        if (IsFlagSet(Projectile::FADING) && GetAlpha() < 0.01) {
            Kill();
        }
        if (pactor_2) {
            TESPackage* ppack;
            float faimLooking = pactor_2->GetLooking();
            float faimHeading = pactor_2->GetHeading(false);
            if (pactor_2 == pPlayer) {
                NiPoint3 kdirection = pPlayer->GetBulletAutoAim() - data.kPosition;
                kdirection.Unitize();
                faimHeading = GetZAngleFromVector(kdirection);
                faimLooking = -std::asinf(kdirection.z);
            }
            else {
                float funused;
                (&pfireNode->GetWorldTransform())->m_Rotate.ToEulerAnglesZXY(faimHeading, faimLooking, funused);
            }
            bool balwaysHit = false;
            if (pactor_2) {
                ppack = pactor_2->GetCurrentPackage();
            }
            else {
                ppack = nullptr;
            }
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
                float fskill;
                ItemChange* pweaponItem;
                BaseProcess* pactorAIProcess;
                if (pactor_2) {
                    pactorAIProcess = pactor_2->pCurrentProcess;
                }
                else {
                    pactorAIProcess = nullptr;
                }
                if (pactorAIProcess) {
                    pweaponItem = pactorAIProcess->GetCurrentWeapon();
                }
                else {
                    pweaponItem = nullptr;
                }
                if (pactor_2) {
                    fskill = pactor_2->GetGunSkill(GUN_SKILL_GUN);
                }
                else {
                    fskill = fGunReferenceSkill.Float();
                }
                bool bUseSpreadMod = false;
                if ((pactor_2) && (pactor_2->pCurrentProcess) && (pweaponItem) &&
                    (pweaponItem->HasModEffectActive(3))) {
                    bUseSpreadMod = true;
                }

                double MinSpreadDegrees = pWeaponSource->GetMinSpreadDegrees(bUseSpreadMod);
                double SpreadDegrees = pWeaponSource->GetSpreadDegrees(bUseSpreadMod);
                float totalSpread = (SpreadDegrees * fskill + MinSpreadDegrees) * 0.01745329238474369;
                float fwobble = pactor_2->GetGunSkill(GUN_SKILL_ACTOR);
                BaseProcess* pactorAIProcess_3 = pactor_2->pCurrentProcess;
                float fdriftRad = pactorAIProcess_3->GetAimLooking();
                float fspreadRad = totalSpread + fdriftRad;
                float fradius = RandomFloat(0.0f, fspreadRad);
                float frandomAngle = RandomFloat(0.0f, 6.2831855f);
                double v4 = std::sin(frandomAngle);
                faimLooking = v4 * fradius + faimLooking;
                double v5 = std::cos(frandomAngle);
                faimHeading = v5 * fradius + faimHeading;
            }
            SetAngleOnReferenceZ(faimHeading);
            SetAngleOnReferenceX(faimLooking);
            SetAngleOnReferenceY(0.0f);
        }
        NiNode* p3d = Get3D();
        if ((pfireNode) && (p3d)) {
            bool bRoot = false;
            if (pWeaponSource->HasScope() && pactor_2->GetIronSights() && pactor_2 == pPlayer) {
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
            float z = GetAlpha();
            spBeamProperty.m_pObject->SetAlpha(z);
            float za = GetRange();
            spBeamProperty.m_pObject->SetLength(za);
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
            location.operator*=(fscale);
            ap3D->SetTranslate(location);
        }
        if (NiObject::IsKindOf_0(&NiGeometry::ms_RTTI, ap3D)) {
            NiGeometry* pgeometry = static_cast<NiGeometry*>(ap3D);
            float fcurrentLength;
            if (!(afCurrentLength > 0.0)) {
                fcurrentLength = FindBeamGeometryLength(pgeometry);
            }
            else {
                fcurrentLength = afCurrentLength;
            }
            if (fcurrentLength != 0.0) {
                float fscale_2 = afDesiredLength / fcurrentLength;
                ScaleBeamGeometryLength(pgeometry, fscale_2);
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
    iShotsFired = (uint32_t)num;
}

void ContinuousBeamProjectile::ScaleBeamGeometryLength(NiGeometry* apGeometry, float afScaleFactor) {
    if (apGeometry) {
        float* pRadius = &apGeometry->GetModelBound().m_fRadius;
        *pRadius = *pRadius * afScaleFactor;
        apGeometry->GetModelBound().m_kCenter.operator*=(afScaleFactor);
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
                NiAdditionalGeometryData* v7;
                if (pdata->GetAdditionalGeometryData() && (v7 = pdata->GetAdditionalGeometryData(), v7->GetVBReady())) {
                    NiTConvertingStrideIterator<NiPoint3> apMemory;
                    NiAdditionalGeometryData* AdditionalGeometryData = pdata->GetAdditionalGeometryData();
                    if (pdata->LockPackedVertexData(true)) {
                        pdata->GetVerticesIterator(*static_cast<NiGeometryDataIteratorNiPoint3*>(&apMemory));
                        for (int32_t auiIndex = 0;; ++auiIndex) {
                            uint16_t LevelExact = AdditionalGeometryData->m_usVertexCount;
                            if (auiIndex >= LevelExact) {
                                break;
                            }
                            NiPoint3 a3 = NiPoint3_ZERO;
                            apMemory.Get(auiIndex, a3);
                            a3.y = a3.y * afScaleFactor;
                            static_cast<NiGeometryDataIteratorNiPoint3*>(&apMemory)
                                ->NiGeometryDataIteratorNiPoint3::Set(auiIndex, a3);
                        }
                        pdata->UnlockPackedVertexData();
                    }
                }
                else {
                    if (pdata->m_pkVertex) {
                        for (int32_t i = 0; i < pdata->m_usVertices; ++i) {
                            float* p_y = &pdata->m_pkVertex[i].y;
                            *p_y = *p_y * afScaleFactor;
                            float* v4 = &pdata->m_pkTexture[i].y;
                            *v4 = *v4 * afScaleFactor;
                        }
                        NiRenderer* Renderer_ = NiRenderer::GetRenderer();
                        apGeometry->PurgeRendererData(Renderer_);
                    }
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
                        NiPoint3 kVertex = NiPoint3_ZERO;
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
