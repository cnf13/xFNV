#include "Fallout/Magic/SummonCreatureEffect.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSHavok/bhkPickData.h"
#include "BSHavok/bhkSimpleShapePhantom.h"
#include "BSHavok/bhkSphereShape.h"
#include "BSHavok/bhkTransform.h"
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

// Whether the summon-creature archetype took Instantiate; stores RegisterActiveEffect's whole return byte.
static char bRegistered =
    EffectArchetypes::RegisterActiveEffect(EffectArchetypes::SUMMON_CREATURE, &SummonCreatureEffect::Instantiate);

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
    AppearLocation(NiPoint3_ZERO), AppearRotation(NiPoint3_ZERO), bPlacement(false), bcached(false) {}

SummonCreatureEffect::~SummonCreatureEffect() {
    if (bcached && pSpell) {
        pSpell->Unload(true);
    }
}

ActiveEffect* SummonCreatureEffect::Copy() {
    SummonCreatureEffect* v3;
    SummonCreatureEffect* apMemory = static_cast<SummonCreatureEffect*>(ourMalloc(sizeof(SummonCreatureEffect)));
    if (apMemory) {
        v3 = ::new (apMemory) SummonCreatureEffect(pCaster, pSpell, pEffect);
    }
    else {
        v3 = nullptr;
    }
    AssociatedItemEffect::CopyData(v3);
    v3->pSummonedCreature = pSummonedCreature;
    v3->bTargetSet = bTargetSet;
    v3->fAppearTime = fAppearTime;
    v3->AppearLocation = AppearLocation;
    v3->AppearRotation = AppearRotation;
    v3->bPlacement = bPlacement;
    if ((bcached && !v3->bcached) && (GetSpell())) {
        MagicItem* FormFlags = GetSpell();
        FormFlags->Preload(false);
    }

    v3->bcached = bcached;
    return v3;
}

void SummonCreatureEffect::Update(float afSecondsElapsed) {
    MagicTarget* unk24;
    PlayerCharacter* targetAsActor;
    if (ActiveEffect::GetTarget()) {
        unk24 = ActiveEffect::GetTarget();
        targetAsActor = static_cast<PlayerCharacter*>(unk24->GetTargetAsActor());
    }
    else {
        targetAsActor = nullptr;
    }
    if (targetAsActor && ((!bPlacement) || (pSummonedCreature && (!pSummonedCreature->IsDead(false))))) {
        if ((fAppearTime <= ((double)fElapsedSeconds)) && (!bPlacement)) {
            DoSummon(targetAsActor);
            if (targetAsActor == pPlayer) {
                LimitPlayerSummonedCreatures();
            }
            bPlacement = true;
        }
        if ((pSummonedCreature) && (!pSummonedCreature->IsDead(false))) {
            CommandEffect::UpdateCommandedActor(pSummonedCreature, targetAsActor);
        }
    }
    else {
        ActiveEffect::Dispel(false);
    }
}

uint16_t SummonCreatureEffect::GetSaveSize(Actor* apActor) {
    uint16_t v4 = (uint16_t)(((int16_t)AssociatedItemEffect::GetSaveSize(apActor)) + 5);
    if (!pSummonedCreature) {
        v4 += 28;
    }
    return v4;
}

// The creature as its form ID and the target flag; without a creature, the appear time, location and rotation.
void SummonCreatureEffect::SaveGame(Actor* apActor) {
    AssociatedItemEffect::SaveGame(apActor);
    uint32_t uiCreatureID = 0;
    if (pSummonedCreature) {
        uiCreatureID = pSummonedCreature->GetFormID();
    }
    pSaveLoadGameOLD->SaveNumericID(&uiCreatureID, 4);
    pSaveLoadGameOLD->SaveGameDataOLD(&bTargetSet, 1);
    if (!pSummonedCreature) {
        pSaveLoadGameOLD->SaveGameDataOLD(&fAppearTime, 4);
        pSaveLoadGameOLD->SaveGameDataOLD(&AppearLocation, 12);
        pSaveLoadGameOLD->SaveGameDataOLD(&AppearRotation, 12);
    }
}

// The creature field holds the loaded form ID until the load game resolves it. A save without a creature carries
// the placement data; otherwise the creature is already placed.
void SummonCreatureEffect::LoadGame(Actor* apActor) {
    AssociatedItemEffect::LoadGame(apActor);
    uint32_t uiCreatureID;
    bool bLoaded = pSaveLoadGameOLD->LoadNumericID(&uiCreatureID, 4);
    pSummonedCreature = reinterpret_cast<Actor*>(static_cast<uintptr_t>(uiCreatureID));
    pSaveLoadGameOLD->LoadGameDataOLD(&bTargetSet, 1);
    if (!uiCreatureID && !bLoaded) {
        pSaveLoadGameOLD->LoadGameDataOLD(&fAppearTime, 4);
        pSaveLoadGameOLD->LoadGameDataOLD(&AppearLocation, 12);
        pSaveLoadGameOLD->LoadGameDataOLD(&AppearRotation, 12);
        bPlacement = false;
    }
    else {
        bPlacement = true;
    }
}

void SummonCreatureEffect::InitLoadGame(Actor* a1) {
    AssociatedItemEffect::InitLoadGame(a1);
    uint32_t iformID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pSummonedCreature));
    if (iformID) {
        TESForm* FormByNumericID = TESForm::GetFormByNumericID(iformID);
        pSummonedCreature = dynamic_cast<Actor*>(FormByNumericID);
        if (!pSummonedCreature) {
            Dispel(true);
        }
    }
}

bool SummonCreatureEffect::CheckDeletedCaster(MagicCaster* a2) {
    int8_t v4 = (int8_t)ActiveEffect::CheckDeletedCaster(a2);
    if (v4) {
        ActiveEffect::Dispel(true);
    }
    return (bool)v4;
}

bool SummonCreatureEffect::CheckTarget(MagicTarget* a2) {
    Actor* targetAsActor;
    if (a2) {
        targetAsActor = a2->GetTargetAsActor();
    }
    else {
        targetAsActor = nullptr;
    }
    return (targetAsActor != nullptr);
}

void SummonCreatureEffect::Start() {
    Actor* pownerActor;
    NiPoint3 summonLocation_2;
    NiPoint3 summonLocation;
    if (pTarget) {
        pownerActor = pTarget->GetTargetAsActor();
    }
    else {
        pownerActor = nullptr;
    }
    if (pownerActor) {
        AppearRotation = pownerActor->GetAngleOnReference();
        this->AppearLocation = pownerActor->GetLocationOnReference();
        if (GetBestSummonLocation(pownerActor, pSummonedCreature, &summonLocation)) {
            this->AppearLocation = summonLocation;
        }
        const char* psequenceName = "SpecialIdle_SummonEffect";
        float ya = NiCos(-AppearRotation.z);
        float xa = -NiSin(-AppearRotation.z);
        NiPoint3* v1 = ::new (&summonLocation_2) NiPoint3(xa, ya, 0.0f);
        NiPoint3 move = 64.0f * *v1;
        NiPoint3 AppearLocation = this->AppearLocation;
        NiPoint3 v17 = move;
        const char* v2 = pEffect->pEffectSetting->GetModel();
        BSTempEffectParticle* peffect =
            BSTempEffectParticle::Spawn(pownerActor->GetParentCell(), 1.0f, v2, v17, AppearLocation, 1.0f, 7u, nullptr);
        peffect->PlaySequence(psequenceName);
        NiAVObject* psummonObject = peffect->GetParticleObject();
        fAppearTime = FindTimeForNoteKey(psummonObject, psequenceName, "Hit");
        if ((!bcached) && (pSpell)) {
            pSpell->Preload(false);
            bcached = true;
        }
    }
}

void SummonCreatureEffect::Finish() {
    Actor* TargetAsActor;
    if (GetTarget()) {
        MagicTarget* Unk24 = GetTarget();
        TargetAsActor = Unk24->GetTargetAsActor();
    }
    else {
        TargetAsActor = nullptr;
    }
    if ((pSummonedCreature) && (TargetAsActor)) {
        BaseProcess* v2;
        if (!pSummonedCreature->GetCurrentAIProcess() ||
            (v2 = pSummonedCreature->GetCurrentAIProcess(), v2->GetProcessLevel())) {
            CommandEffect::CleanupCommandedActor(pSummonedCreature, TargetAsActor, true);
            pSummonedCreature->SetDelete(true);
        }
        else {
            if (!pSummonedCreature->IsDead(false)) {
                pSummonedCreature->Kill(nullptr, 0.0);
            }
            Actor* zero = pSummonedCreature;
            HighProcess* CurrentAIProcess = static_cast<HighProcess*>(zero->GetCurrentAIProcess());
            CurrentAIProcess->FadeAndDelete(zero);
            CommandEffect::CleanupCommandedActor(pSummonedCreature, TargetAsActor, true);
        }
        pSummonedCreature = nullptr;
    }

    if ((bcached) && (GetSpell())) {
        MagicItem* FormFlags = GetSpell();
        FormFlags->Unload(false);
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
        XFNV_INLINED_LOCAL(BSStringT<char>, sName, std::nullopt, std::nullopt);
        GetEffectSetting()->GetEffectName(&sName);
        Warning("GAME EFFECTS: %s effect has no associated creature.", sName.String());
        sName.~BSStringT<char>();
        return;
    }
    // Takes the worldspace before the cell.
    TESWorldSpace* pWorld = pTarget->GetWorldSpace();
    TESObjectCELL* pCell = pTarget->GetParentCell();
    TESObjectREFR* pRef = pDataHandler->CreateReferenceAtLocation(
        pObject, AppearLocation, AppearRotation, pCell, pWorld, nullptr, nullptr, nullptr
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
    float fage = -3.4028235e+38f;
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
    if ((poldest) && (icount > iMaxPlayerSummonedCreatures.Int())) {
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
    bhkSphereShape* pMemory = static_cast<bhkSphereShape*>(NiMemObject::operator new(sizeof(bhkSphereShape)));
    bhkSphereShape* pshape;
    if (pMemory) {
        float fRadius = greatestDimension * 0.5;
        pshape = ::new (pMemory) bhkSphereShape(fRadius, true);
    }
    else {
        pshape = nullptr;
    }
    CFilter ifilter(L_PATHINGPICK, 0, P_OTHER);
    hkSimpleShapePhantomCinfo csi;
    csi.m_collisionFilterInfo = ifilter.iFilter;
    csi.m_shape = pshape->operator*();
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
    if (bStartHit.operator bool()) {
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
            bResult = !bHit.operator bool();
        }
    }
    return bResult;
}

bool SummonCreatureEffect::GetBestSummonLocation(
    Actor* apOwnerActor,
    Actor* apSummonedActor,
    NiPoint3* returnLocation
) {
    NiPoint3* pointlist;
    int32_t index;
    pointlist = nullptr;
    int32_t ipointListSize = GeneratePointCloud(&pointlist, apOwnerActor, apSummonedActor);
    if (ipointListSize <= 0 || !pointlist) {
        return false;
    }
    int32_t v7 = 0;
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
    void* v5 = ourMalloc(sizeof(SummonCreatureEffect));
    if (v5) {
        return ::new (v5) SummonCreatureEffect(apCaster, apSpell, apEffect);
    }

    return nullptr;
}

float SummonCreatureEffect::computeDesirability(Actor* summoner, const NiPoint3& aLocation) {
    NiMatrix3 apResult;
    NiMatrix3 mat;
    NiPoint3 result;
    NiPoint3 v13;
    float v8 = 250.0f;
    float v16 = 500.0f;
    const NiPoint3* v2 = &summoner->GetLocationOnReference();
    result = aLocation - *v2;
    float v11 = result.Length();
    float v14 = 1.0f;
    if (!(250.0f > static_cast<double>(v11))) {
        if (!(v16 > static_cast<double>(v11))) {
            v14 = 0.0f;
        }
        else {
            v14 = (v16 - v11) / (v16 - v8);
        }
    }
    else {
        v14 = v11 / v8;
    }
    NiPoint3 v12 = summoner->GetAngleOnReference();
    mat = NiMatrix3_IDENTITY;
    mat.MakeZRotation(v12.z);
    mat.Inverse(&apResult);
    v13 = result * mat;
    v13.Unitize();
    float v15 = 1.0 - NiAbs(v13.x);
    v15 = v15 * v15;
    float v10 = (v13.y + 1.0) * 0.5;
    return v14 * v15 * v10;
}
