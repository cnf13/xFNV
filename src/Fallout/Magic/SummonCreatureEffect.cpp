#include "Fallout/Magic/SummonCreatureEffect.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "BSHavok/bhkConvert.h"
#include "BSHavok/bhkSimpleShapePhantom.h"
#include "BSHavok/bhkSphereShape.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TempEffects/BSTempEffectParticle.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/AssociatedItemEffect.h"
#include "Fallout/Magic/CommandEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpLinearCastInput.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpAllCdPointCollector.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdlib.h"

#include <limits>

// Whether the summon-creature archetype took Instantiate; stores RegisterActiveEffect's whole return byte.
static char bRegistered = EffectArchetypes::RegisterActiveEffect(
    EffectArchetypes::SUMMON_CREATURE,
    &SummonCreatureEffect::Instantiate
);

// qsort comparator over the point cloud: sorts by the summoner's desirability, best first.
int32_t SummonPosSort(const void* p1, const void* p2) {
    const NiPoint3* pPos1 = static_cast<const NiPoint3*>(p1);
    const NiPoint3* pPos2 = static_cast<const NiPoint3*>(p2);
    float fdesirability1 = SummonCreatureEffect::computeDesirability(summoner, *pPos1);
    float fdesirability2 = SummonCreatureEffect::computeDesirability(summoner, *pPos2);
    if (fdesirability2 == fdesirability1) {
        return 0;
    }
    if (!(fdesirability2 > fdesirability1)) {
        return -1;
    }
    return 1;
}

SummonCreatureEffect::SummonCreatureEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect)
  : AssociatedItemEffect(apCaster, apSpell, apEffect), pSummonedCreature(nullptr), bTargetSet(0), fAppearTime(0.0f),
    AppearLocation(NiPoint3::ZERO), AppearRotation(NiPoint3::ZERO), bPlacement(false), bcached(false) {}

SummonCreatureEffect::~SummonCreatureEffect() {
    if (bcached && pSpell) {
        pSpell->Unload(true);
    }
}

ActiveEffect* SummonCreatureEffect::Copy() {
    SummonCreatureEffect* pCopy = new SummonCreatureEffect(pCaster, pSpell, pEffect);
    AssociatedItemEffect::CopyData(pCopy);
    pCopy->pSummonedCreature = pSummonedCreature;
    pCopy->bTargetSet = bTargetSet;
    pCopy->fAppearTime = fAppearTime;
    pCopy->AppearLocation = AppearLocation;
    pCopy->AppearRotation = AppearRotation;
    pCopy->bPlacement = bPlacement;
    if (bcached && !pCopy->bcached && GetSpell()) {
        MagicItem* pspell = GetSpell();
        pspell->Preload(false);
    }

    pCopy->bcached = bcached;
    return pCopy;
}

void SummonCreatureEffect::Update(float afSecondsElapsed) {
    PlayerCharacter* ptargetActor = nullptr;
    if (ActiveEffect::GetTarget()) {
        MagicTarget* ptarget = ActiveEffect::GetTarget();
        ptargetActor = static_cast<PlayerCharacter*>(ptarget->GetTargetAsActor());
    }
    if (ptargetActor && (!bPlacement || (pSummonedCreature && !pSummonedCreature->IsDead(false)))) {
        if (fAppearTime <= static_cast<double>(fElapsedSeconds) && !bPlacement) {
            DoSummon(ptargetActor);
            if (ptargetActor == pPlayer) {
                LimitPlayerSummonedCreatures();
            }
            bPlacement = true;
        }
        if (pSummonedCreature && !pSummonedCreature->IsDead(false)) {
            CommandEffect::UpdateCommandedActor(pSummonedCreature, ptargetActor);
        }
    }
    else {
        ActiveEffect::Dispel(false);
    }
}

uint16_t SummonCreatureEffect::GetSaveSize(Actor* apActor) {
    uint16_t usSaveSize = static_cast<uint16_t>(AssociatedItemEffect::GetSaveSize(apActor) + 5);
    if (!pSummonedCreature) {
        usSaveSize += 28;
    }
    return usSaveSize;
}

// The creature as its form ID and the target flag; without a creature, the appear time, location and rotation.
void SummonCreatureEffect::SaveGame(Actor* apActor) {
    AssociatedItemEffect::SaveGame(apActor);
    uint32_t uiCreatureID = 0;
    if (pSummonedCreature) {
        uiCreatureID = pSummonedCreature->GetFormID();
    }
    pSaveLoadGameOLD->SaveNumericID(&uiCreatureID, 4);
    pSaveLoadGameOLD->SaveGameDataOLD(&bTargetSet, sizeof(bTargetSet));
    if (!pSummonedCreature) {
        pSaveLoadGameOLD->SaveGameDataOLD(&fAppearTime, sizeof(fAppearTime));
        pSaveLoadGameOLD->SaveGameDataOLD(&AppearLocation, sizeof(AppearLocation));
        pSaveLoadGameOLD->SaveGameDataOLD(&AppearRotation, sizeof(AppearRotation));
    }
}

// The creature field holds the loaded form ID until the load game resolves it. A save without a creature carries
// the placement data; otherwise the creature is already placed.
void SummonCreatureEffect::LoadGame(Actor* apActor) {
    AssociatedItemEffect::LoadGame(apActor);
    uint32_t uiCreatureID;
    bool bLoaded = pSaveLoadGameOLD->LoadNumericID(&uiCreatureID, 4);
    pSummonedCreature = reinterpret_cast<Actor*>(static_cast<uintptr_t>(uiCreatureID));
    pSaveLoadGameOLD->LoadGameDataOLD(&bTargetSet, sizeof(bTargetSet));
    if (!uiCreatureID && !bLoaded) {
        pSaveLoadGameOLD->LoadGameDataOLD(&fAppearTime, sizeof(fAppearTime));
        pSaveLoadGameOLD->LoadGameDataOLD(&AppearLocation, sizeof(AppearLocation));
        pSaveLoadGameOLD->LoadGameDataOLD(&AppearRotation, sizeof(AppearRotation));
        bPlacement = false;
    }
    else {
        bPlacement = true;
    }
}

void SummonCreatureEffect::InitLoadGame(Actor* apActor) {
    AssociatedItemEffect::InitLoadGame(apActor);
    uint32_t iformID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pSummonedCreature));
    if (iformID) {
        TESForm* pForm = TESForm::GetFormByNumericID(iformID);
        pSummonedCreature = dynamic_cast<Actor*>(pForm);
        if (!pSummonedCreature) {
            Dispel(true);
        }
    }
}

bool SummonCreatureEffect::CheckDeletedCaster(MagicCaster* apCaster) {
    bool bDeleted = ActiveEffect::CheckDeletedCaster(apCaster);
    if (bDeleted) {
        ActiveEffect::Dispel(true);
    }
    return bDeleted;
}

bool SummonCreatureEffect::CheckTarget(MagicTarget* apTarget) {
    Actor* ptargetActor = apTarget ? apTarget->GetTargetAsActor() : nullptr;
    return ptargetActor != nullptr;
}

void SummonCreatureEffect::Start() {
    NiPoint3 summonLocation;
    Actor* pownerActor = pTarget ? pTarget->GetTargetAsActor() : nullptr;
    if (pownerActor) {
        AppearRotation = pownerActor->GetAngleOnReference();
        this->AppearLocation = pownerActor->GetLocationOnReference();
        if (GetBestSummonLocation(pownerActor, pSummonedCreature, &summonLocation)) {
            this->AppearLocation = summonLocation;
        }
        const char* psequenceName = "SpecialIdle_SummonEffect";
        float fDirY = NiCos(-AppearRotation.z);
        float fDirX = -NiSin(-AppearRotation.z);
        NiPoint3 move = 64.0f * NiPoint3(fDirX, fDirY, 0.0f);
        NiPoint3 kAppearLocation = this->AppearLocation;
        const char* pModel = pEffect->pEffectSetting->GetModel();
        BSTempEffectParticle* peffect = BSTempEffectParticle::Spawn(
            pownerActor->GetParentCell(),
            1.0f,
            pModel,
            move,
            kAppearLocation,
            1.0f,
            7u,
            nullptr
        );
        peffect->PlaySequence(psequenceName);
        NiAVObject* psummonObject = peffect->GetParticleObject();
        fAppearTime = FindTimeForNoteKey(psummonObject, psequenceName, "Hit");
        if (!bcached && pSpell) {
            pSpell->Preload(false);
            bcached = true;
        }
    }
}

void SummonCreatureEffect::Finish() {
    Actor* ptargetActor = nullptr;
    if (GetTarget()) {
        MagicTarget* ptarget = GetTarget();
        ptargetActor = ptarget->GetTargetAsActor();
    }
    if (pSummonedCreature && ptargetActor) {
        if (!pSummonedCreature->GetCurrentAIProcess() || pSummonedCreature->GetCurrentAIProcess()->GetProcessLevel()) {
            CommandEffect::CleanupCommandedActor(pSummonedCreature, ptargetActor, true);
            pSummonedCreature->SetDelete(true);
        }
        else {
            if (!pSummonedCreature->IsDead(false)) {
                pSummonedCreature->Kill(nullptr, 0.0);
            }
            Actor* pCreature = pSummonedCreature;
            HighProcess* pProcess = static_cast<HighProcess*>(pCreature->GetCurrentAIProcess());
            pProcess->FadeAndDelete(pCreature);
            CommandEffect::CleanupCommandedActor(pSummonedCreature, ptargetActor, true);
        }
        pSummonedCreature = nullptr;
    }

    if (bcached && GetSpell()) {
        MagicItem* pspell = GetSpell();
        pspell->Unload(false);
        bcached = false;
    }
}

void SummonCreatureEffect::ClearSummonedCreature() {
    pSummonedCreature = nullptr;
}

int32_t SummonCreatureEffect::GeneratePointCloud(NiPoint3** returnArray, Actor* apOwnerActor, Actor* apSummonedActor) {
    return 0;
}

void SummonCreatureEffect::DoSummon(Actor* pOwner) {
    Actor* pTarget = GetTarget() ? GetTarget()->GetTargetAsActor() : nullptr;
    if (!pTarget) {
        return;
    }
    TESBoundObject* pObject = dynamic_cast<TESBoundObject*>(GetAssociatedItem());
    if (!pObject) {
        BSStringT<char> sName = GetEffectSetting()->GetEffectName();
        Warning("GAME EFFECTS: %s effect has no associated creature.", sName.String());
        return;
    }
    // Takes the worldspace before the cell.
    TESWorldSpace* pWorld = pTarget->GetWorldSpace();
    TESObjectCELL* pCell = pTarget->GetParentCell();
    TESObjectREFR* pRef = pDataHandler->CreateReferenceAtLocation(
        pObject,
        AppearLocation,
        AppearRotation,
        pCell,
        pWorld,
        nullptr,
        nullptr,
        nullptr
    );
    if (!pRef) {
        return;
    }
    if (pRef->IsActor()) {
        pSummonedCreature = static_cast<Actor*>(pRef);
        pSummonedCreature->SetLocationOnReference(AppearLocation);
        pSummonedCreature->MoveHavok(false);
        pSummonedCreature->ChangeProcessLevel();
        CommandEffect::InitCommandedActor(pSummonedCreature, pTarget);
        BaseProcess* pProcess = pSummonedCreature->GetCurrentAIProcess();
        pProcess->SetIsSummonedCreature(true);
    }
    else {
        Warning("GAME EFFECTS: %s summoned a non-actor.", pTarget->GetFullName());
        // Calls TESObjectREFR::SetDelete directly, not through the vtable.
        pRef->TESObjectREFR::SetDelete(true);
    }
}

void SummonCreatureEffect::LimitPlayerSummonedCreatures() {
    BSSimpleList<ActiveEffect*>* plist = pPlayer->GetActiveEffectList();
    ActiveEffect* poldest = nullptr;
    float fage = -std::numeric_limits<float>::max();
    int32_t icount = 0;
    while (plist && !plist->IsEmpty()) {
        ActiveEffect* peffect = plist->m_item;
        EffectSetting* pes = peffect->GetEffectSetting();
        if (pes->getArchType() && pes->IsFlagSet(EffectSetting::USE_ASSOCIATED_CREATURE)) {
            ++icount;
            if (static_cast<double>(fage) < peffect->fElapsedSeconds) {
                poldest = peffect;
                fage = peffect->fElapsedSeconds;
            }
        }
        plist = plist->m_pkNext;
    }
    if (poldest && icount > iMaxPlayerSummonedCreatures.Int()) {
        poldest->Dispel(false);
    }
}

// Whether the summoned creature fits at location: a sphere of its size cast from above the point down to it hits
// nothing, the owner stands above the water, and nothing blocks the line from above the owner to above the point.
bool SummonCreatureEffect::CheckSummonLocation(Actor* apSummoned, NiPoint3& location) {
    Actor* pownerActor = GetTarget() ? GetTarget()->GetTargetAsActor() : nullptr;
    TESBoundObject* pitemObject = dynamic_cast<TESBoundObject*>(GetAssociatedItem());
    float greatestDimension = pitemObject->GetBoundSize();
    if (greatestDimension < 64.0) {
        greatestDimension = 64.0f;
    }
    bhkSphereShape* pshape = new bhkSphereShape(greatestDimension * 0.5, true);
    CFilter ifilter(L_PATHINGPICK, 0, P_OTHER);
    hkSimpleShapePhantomCinfo csi;
    csi.m_collisionFilterInfo = ifilter.iFilter;
    csi.m_shape = **pshape;
    csi.m_transform.setIdentity();
    bhkSimpleShapePhantom spShapePhantom(csi);
    if (pownerActor->GetParentCell()) {
        bhkWorld* pWorld = pownerActor->GetParentCell()->GetbhkWorld();
        spShapePhantom.bhkWorldObject::Add(pWorld);
    }
    hkVector4 hkFrom;
    hkpAllCdPointCollector startCol;
    hkpAllCdPointCollector col;
    hkpLinearCastInput input;
    float fFromHeight = greatestDimension * 0.5 + 10.0;
    alignas(16) NiPoint3 kFromOffset;
    ::new (&kFromOffset) NiPoint3(0.0f, 0.0f, fFromHeight);
    NI2HK(hkFrom, location + kFromOffset);
    float fToHeight = greatestDimension * 0.5 + 10.0;
    NiPoint3 kToOffset;
    ::new (&kToOffset) NiPoint3(0.0f, 0.0f, fToHeight);
    NI2HK(input.m_to, location + kToOffset);
    spShapePhantom.SetPositionAndLinearCast(hkFrom, input, col, &startCol);
    hkBool bStartHit = startCol.hasHit();
    bool bResult;
    if (bStartHit) {
        spShapePhantom.bhkPhantom::Remove();
        delete pshape;
        bResult = false;
    }
    else {
        spShapePhantom.bhkPhantom::Remove();
        delete pshape;
        double fWaterHeight = pownerActor->GetRelevantWaterHeight();
        if (pownerActor->GetLocationOnReference().z < fWaterHeight) {
            bResult = false;
        }
        else {
            bhkPickData pd;
            CFilter fltr;
            fltr.iFilter = 0;
            fltr.SetLayer(L_PATHINGPICK);
            pd.SetFilter(fltr);
            alignas(16) NiPoint3 from = pownerActor->GetLocationOnReference();
            from.z = from.z + 64.0;
            NiPoint3 to = location;
            to.z = to.z + 64.0;
            pd.NiSetFrom(from);
            pd.NiSetTo(to);
            TES::pInstance->Pick(pd);
            hkBool bHit = pd.hasHit();
            bResult = !bHit;
        }
    }
    return bResult;
}

bool SummonCreatureEffect::GetBestSummonLocation(
    Actor* apOwnerActor,
    Actor* apSummonedActor,
    NiPoint3* returnLocation
) {
    NiPoint3* pointlist = nullptr;
    int32_t index;
    int32_t ipointListSize = GeneratePointCloud(&pointlist, apOwnerActor, apSummonedActor);
    if (ipointListSize <= 0 || !pointlist) {
        return false;
    }
    summoner = apOwnerActor;
    std::qsort(pointlist, ipointListSize, 12u, &SummonPosSort);
    for (index = 0; index < ipointListSize && !CheckSummonLocation(apSummonedActor, pointlist[index]); ++index) {
        ;
    }
    if (index >= ipointListSize) {
        ourFree(pointlist);
        return false;
    }

    *returnLocation = pointlist[index];
    ourFree(pointlist);
    return true;
}

ActiveEffect* SummonCreatureEffect::Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    return new SummonCreatureEffect(apCaster, apSpell, apEffect);
}

float SummonCreatureEffect::computeDesirability(Actor* summoner, const NiPoint3& aLocation) {
    NiMatrix3 kInverse;
    NiMatrix3 mat;
    float fNearDistance = 250.0f;
    float fFarDistance = 500.0f;
    NiPoint3 kOffset = aLocation - summoner->GetLocationOnReference();
    float fDistance = kOffset.Length();
    float fDistanceScore = 1.0f;
    if (!(250.0f > static_cast<double>(fDistance))) {
        if (!(fFarDistance > static_cast<double>(fDistance))) {
            fDistanceScore = 0.0f;
        }
        else {
            fDistanceScore = (fFarDistance - fDistance) / (fFarDistance - fNearDistance);
        }
    }
    else {
        fDistanceScore = fDistance / fNearDistance;
    }
    NiPoint3 kAngle = summoner->GetAngleOnReference();
    mat = NiMatrix3::IDENTITY;
    mat.MakeZRotation(kAngle.z);
    mat.Inverse(&kInverse);
    NiPoint3 kLocalOffset = kOffset * mat;
    kLocalOffset.Unitize();
    float fSideScore = 1.0 - NiAbs(kLocalOffset.x);
    fSideScore = fSideScore * fSideScore;
    float fFrontScore = (kLocalOffset.y + 1.0) * 0.5;
    return fDistanceScore * fSideScore * fFrontScore;
}
