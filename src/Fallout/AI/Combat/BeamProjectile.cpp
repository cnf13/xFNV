#include "Fallout/AI/Combat/BeamProjectile.h"

#include "BSCore/BSSimpleList.h"
#include "BSShader/BeamShaderProperty.h"
#include "Fallout Shared/ActorCause.h"
#include "Fallout Shared/BGSProjectile.h"
#include "Fallout Shared/FaceGen/NiGeometryDataIteratorNiPoint3.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/AI/PlayerCharacter.h"
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
#include "Gamebryo2.2/CoreLibs/NiMain/NiTConvertingStrideIterator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStrips.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysCylinderEmitter.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

BeamProjectile::BeamProjectile(bool abAdd) : Projectile(abAdd), spBeamProperty(nullptr) {
    cFormType = 63;
}

BeamProjectile::BeamProjectile(
    BGSProjectile* apProjectileBase,
    TESObjectREFR* apShooter,
    TESObjectWEAP* apFromWeapon,
    NiPoint3 aOrigin,
    float afZAngle,
    float afXAngle
)
  : Projectile(apProjectileBase, apShooter, apFromWeapon, aOrigin, afZAngle, afXAngle), spBeamProperty(nullptr) {
    cFormType = 63;
}

void BeamProjectile::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    Projectile::SaveGame(apSaveGameBuffer);
}

void BeamProjectile::FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    Projectile::FinishLoadGame(apLoadGameBuffer);
}

void BeamProjectile::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    Projectile::LoadGame(apLoadGameBuffer);
}

void BeamProjectile::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    Projectile::InitLoadGame(apLoadGameBuffer);
}

void BeamProjectile::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    Projectile::Revert(apLoadGameBuffer);
}

Projectile::PROJECTILE_TYPE BeamProjectile::GetProjectileType() {
    return PROJECTILE_TYPE_BEAM;
}

void BeamProjectile::Initialize() {
    Projectile::SetFlag(Projectile::TRACER, false);
    Projectile::SetFlag(Projectile::HITSCAN_MODE, true);
    Projectile::SetFlag(Projectile::NO_3D, false);
    Projectile::SetFlag(Projectile::NO_COLLISION, true);
    Projectile::SetFlag(Projectile::FADING, true);
    Projectile::SetFlag(Projectile::CHECK_BEHIND_WALL, true);
    Projectile::SetFlag(Projectile::POST_PROCESS, true);
    Projectile::SetFlag(Projectile::SPAWN_EXPLOSION_AT_IMPACT, true);
    Projectile::Initialize();
    if (spBeamProperty) {
        float fRange = GetRange();
        spBeamProperty->SetLength(fRange);
    }
}

// Keeps the beam attached to the shooter's fire node: stores the root's rotation relative to that node, then scales the
// beam to its range (at least 1).
void BeamProjectile::Process3D() {
    NiNode* pRootNode = Get3D();
    if (pRootNode) {
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
            const NiMatrix3* pInverse = pProjectileNode->GetWorldTransform().m_Rotate.Inverse(&kInverse);
            kFollowOffset.m_Rotate = Get3D()->GetWorldTransform().m_Rotate * *pInverse;
        }
        float fRange = GetRange();
        if (fRange < 1.0) {
            fRange = 1.0f;
        }
        ScaleBeam3D(pRootNode, fRange, 0.0f);
    }
}

// Follows the shooter's fire node (the player's beam also takes its rotation), fades the beam with the projectile,
// and ends it after 5 seconds, or once its fade has run out after an impact.
void BeamProjectile::UpdateProjectile(float afDelta) {
    if (GetDelete()) {
        return;
    }
    if (GetAge() == 0.0) {
        CheckExplosion(afDelta);
    }
    NiAVObject* pProjectileNode;
    if (GetShooter()) {
        pProjectileNode = GetShooter()->GetFireNode();
    }
    else {
        pProjectileNode = nullptr;
    }
    NiNode* pRootNode = Get3D();
    if (pProjectileNode && pRootNode) {
        if (GetShooter() == pPlayer) {
            pRootNode->SetRotate(kFollowOffset.m_Rotate * pProjectileNode->GetWorldTransform().m_Rotate);
            NiMatrix3 kRotation;
            kRotation = kFollowOffset.m_Rotate * pProjectileNode->GetWorldTransform().m_Rotate;
            SetRotation(&kRotation);
        }
        pRootNode->SetTranslate(pProjectileNode->GetWorldTranslate());
        SetLocationOnReference(pProjectileNode->GetWorldTranslate());
    }
    if (spBeamProperty) {
        float fAlpha = GetAlpha();
        spBeamProperty->SetAlpha(fAlpha);
    }
    if (GetAge() > 5.0) {
        Kill();
    }
    if (bImpactProcessed) {
        double dAge = GetAge();
        if (GetProjectileBase()->GetFadeDuration() <= dAge) {
            if (Get3D()) {
                Get3D()->SetAppCulled(true);
            }
            Kill();
        }
    }
}

// A beam takes one impact: the first valid hit, which also sets the beam's range to the hit's distance.
bool BeamProjectile::AddImpact(
    TESObjectREFR* apCollidee,
    const NiPoint3& aContactPoint,
    const NiPoint3& aContactNormal,
    const hkpCollidable* apTargetCollidable,
    uint32_t aiKey,
    HavokMaterialType aeForceMaterial
) {
    if (!ImpactList.IsEmpty()) {
        return false;
    }
    if (!Projectile::IsValidImpact(apCollidee, apTargetCollidable, this, GetProjectileBase()) ||
        !Projectile::AddImpact(apCollidee, aContactPoint, aContactNormal, apTargetCollidable, aiKey, aeForceMaterial)) {
        return false;
    }
    SetRange((aContactPoint - GetLocationOnReference()).Length());
    return true;
}

float BeamProjectile::FindBeamGeometryLength(NiParticleSystem* apObject) {
    NiGeometryData* pModelData = apObject->GetModelData();
    float fLength = 0.0f;
    if (pModelData) {
        if (NiObject::IsKindOf_0(&NiPSysData::ms_RTTI, pModelData)) {
            if (NiObject::IsKindOf_0(&NiParticleSystem::ms_RTTI, apObject)) {
                for (uint32_t uiIndex = 0; fLength == 0.0 && uiIndex < apObject->GetModifierCount(); ++uiIndex) {
                    NiObject* pModifier = apObject->GetModifierAt(uiIndex);
                    if (NiObject::IsKindOf_0(&NiPSysCylinderEmitter::ms_RTTI, pModifier)) {
                        fLength = static_cast<NiPSysCylinderEmitter*>(pModifier)->m_fEmitterHeight;
                    }
                }
            }
        }
        else if (pModelData->GetAdditionalGeometryData() && pModelData->GetAdditionalGeometryData()->GetVBReady()) {
            NiTConvertingStrideIterator<NiPoint3> kIter;
            NiAdditionalGeometryData* pAddData = pModelData->GetAdditionalGeometryData();
            if (pModelData->LockPackedVertexData(true)) {
                for (int32_t i = 0; i < pAddData->m_usVertexCount; ++i) {
                    NiPoint3 kVertex = NiPoint3::ZERO;
                    pModelData->GetVerticesIterator(*static_cast<NiGeometryDataIteratorNiPoint3*>(&kIter));
                    kIter.Get(i, kVertex);
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
    return fLength;
}

// Scales apObject's model bound by afValue, then its geometry: a particle system's cylinder emitters' heights, else
// each vertex's and texture coordinate's y (through the packed vertex data when it is ready).
void BeamProjectile::ScaleBeamGeometryLength(NiGeometry* apObject, float afValue) {
    if (apObject) {
        float* pfRadius = &apObject->GetModelBound().m_fRadius;
        *pfRadius = *pfRadius * afValue;
        apObject->GetModelBound().m_kCenter *= afValue;
        NiGeometryData* pModelData = apObject->GetModelData();
        if (pModelData) {
            if (NiObject::IsKindOf_0(&NiPSysData::ms_RTTI, pModelData)) {
                if (NiObject::IsKindOf_0(&NiParticleSystem::ms_RTTI, apObject)) {
                    NiParticleSystem* pParticleSystem = static_cast<NiParticleSystem*>(apObject);
                    for (uint32_t uiIndex = 0; uiIndex < pParticleSystem->GetModifierCount(); ++uiIndex) {
                        NiPSysModifier* pModifier = pParticleSystem->GetModifierAt(uiIndex);
                        if (NiObject::IsKindOf_0(&NiPSysCylinderEmitter::ms_RTTI, pModifier)) {
                            NiPSysCylinderEmitter* pEmitter = static_cast<NiPSysCylinderEmitter*>(pModifier);
                            float fHeight = pEmitter->GetEmitterHeight() * afValue;
                            pEmitter->SetEmitterHeight(fHeight);
                        }
                    }
                }
            }
            else if (pModelData->GetAdditionalGeometryData() && pModelData->GetAdditionalGeometryData()->GetVBReady()) {
                NiTConvertingStrideIterator<NiPoint3> kIter;
                NiAdditionalGeometryData* pAddData = pModelData->GetAdditionalGeometryData();
                if (pModelData->LockPackedVertexData(true)) {
                    pModelData->GetVerticesIterator(*static_cast<NiGeometryDataIteratorNiPoint3*>(&kIter));
                    for (int32_t i = 0; i < pAddData->m_usVertexCount; ++i) {
                        NiPoint3 kVertex = NiPoint3::ZERO;
                        kIter.Get(i, kVertex);
                        kVertex.y = kVertex.y * afValue;
                        static_cast<NiGeometryDataIteratorNiPoint3*>(&kIter)->NiGeometryDataIteratorNiPoint3::Set(
                            i,
                            kVertex
                        );
                    }
                    pModelData->UnlockPackedVertexData();
                }
            }
            else if (pModelData->m_pkVertex) {
                for (int32_t i = 0; i < pModelData->m_usVertices; ++i) {
                    pModelData->m_pkVertex[i].y = pModelData->m_pkVertex[i].y * afValue;
                    pModelData->m_pkTexture[i].y = pModelData->m_pkTexture[i].y * afValue;
                }
            }
        }
    }
}

void BeamProjectile::ScaleBeam3D(NiAVObject* ap3D, float afDesiredLength, float afCurrentLength) {
    if (ap3D) {
        if (afCurrentLength > 0.0) {
            float fScale = afDesiredLength / afCurrentLength;
            NiPoint3 kLocation = ap3D->GetTranslate();
            kLocation *= fScale;
            ap3D->SetTranslate(kLocation);
        }
        if (NiObject::IsKindOf_0(&NiGeometry::ms_RTTI, ap3D)) {
            float fCurrentLength;
            if (!(afCurrentLength > 0.0)) {
                fCurrentLength = FindBeamGeometryLength(static_cast<NiParticleSystem*>(ap3D));
            }
            else {
                fCurrentLength = afCurrentLength;
            }
            if (fCurrentLength != 0.0) {
                float fScale = afDesiredLength / fCurrentLength;
                ScaleBeamGeometryLength(static_cast<NiGeometry*>(ap3D), fScale);
            }
        }
        else {
            NiNode* pNode = ap3D->IsNode();
            if (pNode) {
                for (uint32_t n = 0; n < pNode->GetArrayCount(); ++n) {
                    NiAVObject* pObj = pNode->GetAt(n);
                    ScaleBeam3D(pObj, afDesiredLength, afCurrentLength);
                }
            }
        }
    }
}

void BeamShaderProperty::SetLength(float afValue) {
    fLength = afValue;
}

void BeamShaderProperty::SetAlpha(float afValue) {
    fAlpha = afValue;
}

void Projectile::SetActorCause(ActorCause* apActorCause) {
    spActorCause = apActorCause;
}

float Projectile::GetAge() {
    return fAge;
}

void Projectile::SetRange(float afRange) {
    fRange = afRange;
}
