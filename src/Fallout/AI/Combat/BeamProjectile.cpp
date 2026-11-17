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
    Projectile::SetFlag(Projectile::TRACER, 0);
    Projectile::SetFlag(Projectile::HITSCAN_MODE, 1);
    Projectile::SetFlag(Projectile::NO_3D, 0);
    Projectile::SetFlag(Projectile::NO_COLLISION, 1);
    Projectile::SetFlag(Projectile::FADING, 1);
    Projectile::SetFlag(Projectile::CHECK_BEHIND_WALL, 1);
    Projectile::SetFlag(Projectile::POST_PROCESS, 1);
    Projectile::SetFlag(Projectile::SPAWN_EXPLOSION_AT_IMPACT, 1);
    Projectile::Initialize();
    if (spBeamProperty.operator->()) {
        float fRange = GetRange();
        spBeamProperty.operator->()->SetLength(fRange);
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
            NiUpdateData kUpdateData;
            kUpdateData.~NiUpdateData();
            ::new (&kUpdateData) NiUpdateData(0.0f, false, false);
            pRootNode->UpdateWorldData(kUpdateData);
            NiMatrix3 kInverse;
            const NiMatrix3* pInverse;
            pInverse = pProjectileNode->GetWorldTransform().m_Rotate.Inverse(&kInverse);
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
    if (spBeamProperty.operator->()) {
        float fAlpha = GetAlpha();
        spBeamProperty.operator->()->SetAlpha(fAlpha);
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
    NiGeometryData* ModelData = apObject->GetModelData();
    float ColorRange = 0.0f;
    if (ModelData) {
        if (NiObject::IsKindOf_0(&NiPSysData::ms_RTTI, ModelData)) {
            if (NiObject::IsKindOf_0(&NiParticleSystem::ms_RTTI, apObject)) {
                for (uint32_t uiIndex = 0; ColorRange == 0.0 && uiIndex < apObject->GetModifierCount(); ++uiIndex) {
                    NiObject* ModifierAt = (NiObject*)apObject->GetModifierAt(uiIndex);
                    if (NiObject::IsKindOf_0(&NiPSysCylinderEmitter::ms_RTTI, ModifierAt)) {
                        NiObject* v15 = ModifierAt;
                        ColorRange = static_cast<NiPSysCylinderEmitter*>(ModifierAt)->m_fEmitterHeight;
                    }
                }
            }
        }
        else {
            NiAdditionalGeometryData* v6;
            if (ModelData->GetAdditionalGeometryData() &&
                (v6 = ModelData->GetAdditionalGeometryData(), v6->GetVBReady())) {
                NiTConvertingStrideIterator<NiPoint3> apMemory;
                NiAdditionalGeometryData* AdditionalGeometryData = ModelData->GetAdditionalGeometryData();
                if (ModelData->LockPackedVertexData(1)) {
                    for (int32_t a2 = 0;; ++a2) {
                        uint16_t LevelExact = AdditionalGeometryData->m_usVertexCount;
                        if (a2 >= LevelExact) {
                            break;
                        }
                        NiPoint3 a3 = NiPoint3_ZERO;
                        ModelData->GetVerticesIterator(*static_cast<NiGeometryDataIteratorNiPoint3*>(&apMemory));
                        apMemory.Get(a2, a3);
                        if (a3.y > static_cast<double>(ColorRange)) {
                            ColorRange = a3.y;
                        }
                    }
                    ModelData->UnlockPackedVertexData();
                }
            }
            else {
                if (ModelData->m_pkVertex) {
                    for (int32_t i = 0; i < ModelData->m_usVertices; ++i) {
                        NiPoint3* v3 = &ModelData->m_pkVertex[i];
                        float x = v3->x;
                        float y = v3->y;
                        float z = v3->z;
                        if (y > static_cast<double>(ColorRange)) {
                            ColorRange = y;
                        }
                    }
                }
            }
        }
    }
    return ColorRange;
}

// Scales apObject's model bound by afValue, then its geometry: a particle system's cylinder emitters' heights, else
// each vertex's and texture coordinate's y (through the packed vertex data when it is ready).
void BeamProjectile::ScaleBeamGeometryLength(NiGeometry* apObject, float afValue) {
    if (apObject) {
        float* pfRadius = &apObject->GetModelBound().m_fRadius;
        *pfRadius = *pfRadius * afValue;
        (&apObject->GetModelBound())->m_kCenter.operator*=(afValue);
        NiGeometryData* a2 = apObject->GetModelData();
        if (a2) {
            if (NiObject::IsKindOf_0(&NiPSysData::ms_RTTI, a2)) {
                if (NiObject::IsKindOf_0(&NiParticleSystem::ms_RTTI, apObject)) {
                    NiParticleSystem* v16 = static_cast<NiParticleSystem*>(apObject);
                    for (uint32_t uiIndex = 0; uiIndex < v16->GetModifierCount(); ++uiIndex) {
                        NiPSysModifier* pModifier = v16->GetModifierAt(uiIndex);
                        if (NiObject::IsKindOf_0(&NiPSysCylinderEmitter::ms_RTTI, pModifier)) {
                            NiPSysCylinderEmitter* pEmitter = static_cast<NiPSysCylinderEmitter*>(pModifier);
                            float v7 = pEmitter->GetEmitterHeight() * afValue;
                            pEmitter->SetEmitterHeight(v7);
                        }
                    }
                }
            }
            else {
                NiAdditionalGeometryData* v6;
                if (a2->GetAdditionalGeometryData() && (v6 = a2->GetAdditionalGeometryData(), v6->GetVBReady())) {
                    NiTConvertingStrideIterator<NiPoint3> apMemory;
                    NiAdditionalGeometryData* AdditionalGeometryData = a2->GetAdditionalGeometryData();
                    if (a2->LockPackedVertexData(1)) {
                        a2->GetVerticesIterator(*static_cast<NiGeometryDataIteratorNiPoint3*>(&apMemory));
                        for (int32_t auiIndex = 0;; ++auiIndex) {
                            uint16_t LevelExact = AdditionalGeometryData->m_usVertexCount;
                            if (auiIndex >= LevelExact) {
                                break;
                            }
                            NiPoint3 a3 = NiPoint3_ZERO;
                            apMemory.Get(auiIndex, a3);
                            a3.y = a3.y * afValue;
                            static_cast<NiGeometryDataIteratorNiPoint3*>(&apMemory)
                                ->NiGeometryDataIteratorNiPoint3::Set(auiIndex, a3);
                        }
                        a2->UnlockPackedVertexData();
                    }
                }
                else {
                    if (a2->m_pkVertex) {
                        for (int32_t i = 0; i < a2->m_usVertices; ++i) {
                            float* p_y = &a2->m_pkVertex[i].y;
                            *p_y = *p_y * afValue;
                            float* v4 = &a2->m_pkTexture[i].y;
                            *v4 = *v4 * afValue;
                        }
                    }
                }
            }
        }
    }
}

void BeamProjectile::ScaleBeam3D(NiAVObject* ap3D, float afDesiredLength, float afCurrentLength) {
    if (ap3D) {
        if (afCurrentLength > 0.0) {
            float fscale = afDesiredLength / afCurrentLength;
            NiPoint3 location = ap3D->GetTranslate();
            location.operator*=(fscale);
            ap3D->SetTranslate(location);
        }
        if (NiObject::IsKindOf_0(&NiGeometry::ms_RTTI, ap3D)) {
            float fcurrentLength;
            if (!(afCurrentLength > 0.0)) {
                fcurrentLength = FindBeamGeometryLength(static_cast<NiParticleSystem*>(ap3D));
            }
            else {
                fcurrentLength = afCurrentLength;
            }
            if (fcurrentLength != 0.0) {
                float fscale_2 = afDesiredLength / fcurrentLength;
                ScaleBeamGeometryLength(static_cast<NiGeometry*>(ap3D), fscale_2);
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
