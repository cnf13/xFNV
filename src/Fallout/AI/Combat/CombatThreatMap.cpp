#include "Fallout/AI/Combat/CombatThreatMap.h"

#include "BSCore/BSReadWriteLock.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/BGSExplosion.h"
#include "Fallout Shared/BGSProjectile.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Pathfinding/PathingAvoidNode.h"
#include "Fallout Shared/TESCombatStyle.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Combat/CombatTimeStamp.h"
#include "Fallout/AI/Combat/CombatTimer.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/rtti.h"

#include <limits>

// The game settings this file defines, with the game's IDs and values, in the game's initialization order.
SettingT<INISettingCollection> bDebugCombatThreats("bDebugCombatThreats:Combat", false);

CombatThreat::CombatThreat()
  : pThreat(nullptr), fRadius(0.0f), fStartTime(0.0f), fEndTime(0.0f), fDamage(0.0f), spDebugGeometry(nullptr) {}

CombatThreat::CombatThreat(TESObjectREFR* apThreat, float afRadius, float afStartTime, float afEndTime, float afDamage)
  : spDebugGeometry(nullptr) {
    Initialize(apThreat, afRadius, afStartTime, afEndTime, afDamage, false);
    pCachedThreat3D = nullptr;
    pCachedRigidBody = nullptr;
    CachedLinearVelocity = NiPoint3::ZERO;
    fCachedLinearVelocityTimeStamp.fTimeStamp = -std::numeric_limits<float>::max();
}

CombatThreat::~CombatThreat() {
    if (spDebugGeometry.m_pObject && spDebugGeometry->GetParent()) {
        spDebugGeometry->GetParent()->DetachChild(spDebugGeometry.m_pObject);
    }
}

float CombatThreat::ComputeThreatScore(
    const NiPoint3& arLocation,
    Actor* apActor,
    float afCompareTime,
    float afAdditionalRadius,
    float afActorHealth
) {
    float fthreatScore = 0.0;
    if (afCompareTime < 0.0 || IsActive(afCompareTime)) {
        float fmodifiedRadius = fRadius + afAdditionalRadius;
        float fsquaredDistanceFromThreat = 0.0f;
        float fextrudeRadius = fmodifiedRadius * fCombatThreatExtrudeRadiusMult.Float();
        NiPoint3 linearVelocity = GetThreatLinearVelocity();
        float fvelocityMagnitude = linearVelocity.UnitizeGetLength();
        NiPoint3 halfVelocity = linearVelocity * fvelocityMagnitude * 0.5f;
        NiPoint3 threatLocation = pThreat->GetLocationOnReference() + halfVelocity;
        if (fextrudeRadius > static_cast<double>(fvelocityMagnitude)) {
            NiPoint3 threatVector = threatLocation - arLocation;
            fsquaredDistanceFromThreat = threatVector.SqrLength();
        }
        else {
            float fnegativeExtrudeTime = BSMAX(static_cast<float>(0.0), 0.0);
            float fbaseOffset = fvelocityMagnitude * fnegativeExtrudeTime - fmodifiedRadius;
            NiPoint3 basePoint = threatLocation + linearVelocity * fbaseOffset;
            double dDiameter = fmodifiedRadius * 2.0;
            float fheight = (fCombatThreatExtrudeTime.Float() - fnegativeExtrudeTime) * fvelocityMagnitude + dDiameter;
            float fheightSquared = fheight * fheight;
            NiPoint3 deltaVector = fheight * linearVelocity;
            NiPoint3 testVector = arLocation - basePoint;
            float fdot = testVector.Dot(deltaVector);
            if (fdot < 0.0 || fheightSquared < static_cast<double>(fdot)) {
                fsquaredDistanceFromThreat = std::numeric_limits<float>::max();
            }
            else {
                double dSqrLength = testVector.SqrLength();
                fsquaredDistanceFromThreat = dSqrLength - fdot * fdot / fheightSquared;
            }
        }
        if (fsquaredDistanceFromThreat < fmodifiedRadius * fmodifiedRadius) {
            double dBaseThreatScore = GetBaseThreatScore(apActor, afActorHealth);
            return dBaseThreatScore * (1.0 - fsquaredDistanceFromThreat / (fmodifiedRadius * fmodifiedRadius));
        }
    }
    return fthreatScore;
}

CombatTimeStamp* CombatThreat::GetStartTime(CombatTimeStamp* apOut) {
    *apOut = fStartTime;
    return apOut;
}

void CombatThreat::Initialize(
    TESObjectREFR* apThreat,
    float afRadius,
    float afStartTime,
    float afEndTime,
    float afDamage,
    bool abUpdatingThreat
) {
    float fstartStamp;
    pThreat = apThreat;
    fRadius = afRadius;
    if (!abUpdatingThreat) {
        fstartStamp = CombatTimer::GetTimer() + afStartTime;
        CombatTimeStamp kStartStamp(fstartStamp);
        fStartTime = kStartStamp;
    }
    float fendStamp = CombatTimer::GetTimer() + afEndTime;
    CombatTimeStamp kEndStamp(fendStamp);
    fEndTime = kEndStamp;
    fDamage = afDamage;
}

// Unordered compares (NaN) are inactive.
bool CombatThreat::IsActive(float afCompareTime) {
    return afCompareTime >= fStartTime && afCompareTime <= fEndTime;
}

// An unordered (NaN) comparison is not expired.
bool CombatThreat::IsExpired(float afCompareTime) {
    return afCompareTime > fEndTime;
}

void CombatThreat::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fRadius), sizeof(fRadius), 0);
    fStartTime.SaveGame(apSaveGameBuffer);
    fEndTime.SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fDamage), sizeof(fDamage), 0);
}

float CombatThreat::GetBaseThreatScore(Actor* apActor, float afActorHealth) {
    float fcurrentHealth;
    if (apActor) {
        fcurrentHealth = static_cast<float>(apActor->GetActorValue(ActorValue::HEALTH));
    }
    else {
        fcurrentHealth = afActorHealth;
    }
    return fDamage * 2.0 / fcurrentHealth;
}

NiPoint3 CombatThreat::GetThreatLinearVelocity() {
    CombatTimer kTimer;
    if (fCachedLinearVelocityTimeStamp.ElapsedTime() < 0.33000001311302185) {
        return CachedLinearVelocity;
    }
    NiAVObject* pthreat3D = pThreat->Get3D();
    if (pthreat3D) {
        if (pThreat->IsProjectile()) {
            Projectile* pprojectile = static_cast<Projectile*>(pThreat);
            bhkCharacterController* pcharacterController = pprojectile->GetCharController();
            if (pcharacterController) {
                pcharacterController->GetLinearVelocity(CachedLinearVelocity);
                float ftime = CombatTimer::GetTimer();
                CombatTimeStamp kStamp(ftime);
                fCachedLinearVelocityTimeStamp = kStamp;
                return CachedLinearVelocity;
            }
            if (pprojectile->IsFlagSet(Projectile::NO_COLLISION)) {
                CachedLinearVelocity = NiPoint3::ZERO;
                float ftime = CombatTimer::GetTimer();
                CombatTimeStamp kStamp(ftime);
                fCachedLinearVelocityTimeStamp = kStamp;
                return CachedLinearVelocity;
            }
        }
        if (pCachedThreat3D != pthreat3D) {
            pCachedThreat3D = pthreat3D;
            bhkCollisionObject* pcollisionObject = FindFirstCollisionObject(pCachedThreat3D);
            if (pcollisionObject) {
                NiObject* pworldObject = pcollisionObject->GetRigidBody();
                if (pworldObject) {
                    pCachedRigidBody = static_cast<bhkRigidBody*>(
                        NiObject::DynamicCastStatic(&bhkRigidBody::ms_RTTI, pworldObject)
                    );
                }
            }
        }
        if (pCachedRigidBody) {
            pCachedRigidBody->GetLinearVelocityNi(&CachedLinearVelocity);
        }
        else {
            CachedLinearVelocity = NiPoint3::ZERO;
        }
        float ftime = CombatTimer::GetTimer();
        ::new (&kTimer.fTargetTime) CombatTimeStamp(ftime);
        fCachedLinearVelocityTimeStamp.fTimeStamp = kTimer.fTargetTime;
        return CachedLinearVelocity;
    }

    float ftime = CombatTimer::GetTimer();
    kTimer.SetStartTime(ftime);
    fCachedLinearVelocityTimeStamp.fTimeStamp = kTimer.fStartTime;
    return NiPoint3::ZERO;
}

PathingAvoidNode* CombatThreat::GetAvoidNode(PathingAvoidNode* apResult, Actor* apActor) {
    NiPoint3 linearVelocity = GetThreatLinearVelocity();
    float fextrudeRadius = fRadius * fCombatThreatExtrudeRadiusMult.Float();
    float fthreatScore = GetBaseThreatScore(apActor, 0.0f);
    float fvelocityMagnitude = linearVelocity.UnitizeGetLength();
    NiPoint3 halfVelocity = linearVelocity * fvelocityMagnitude * 0.5f;
    NiPoint3 threatLocation = pThreat->GetLocationOnReference() + halfVelocity;
    if (fextrudeRadius > static_cast<double>(fvelocityMagnitude)) {
        float fcost = fthreatScore * fCombatThreatAvoidCost.Float();
        ::new (apResult) PathingAvoidNode(threatLocation, fRadius, fcost);
    }
    else {
        fvelocityMagnitude = linearVelocity.UnitizeGetLength();
        float fnegativeExtrudeTime = BSMAX(0.0f, 0.0f);
        float fbaseOffset = fvelocityMagnitude * fnegativeExtrudeTime - fRadius;
        threatLocation += linearVelocity * fbaseOffset;
        double dDiameter = fRadius * 2.0;
        float fheight = (fCombatThreatExtrudeTime.Float() - fnegativeExtrudeTime) * fvelocityMagnitude + dDiameter;
        NiPoint3 threatLoc2 = threatLocation + linearVelocity * fheight;
        float fcost = fthreatScore * fCombatThreatAvoidCost.Float();
        ::new (apResult) PathingAvoidNode(threatLocation, threatLoc2, fRadius, fcost);
    }
    return apResult;
}

void CombatThreat::Load(BGSLoadGameBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fRadius), sizeof(fRadius));
    fStartTime.LoadGame(apLoadGameBuffer);
    fEndTime.LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fDamage), sizeof(fDamage));
}

void CombatThreat::InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    TESObjectREFR* pRef;
    if (pThreat) {
        // The threat holds the reference's form ID until it is resolved here (CombatThreatMap::LoadGame).
        TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pThreat)));
        pRef = dynamic_cast<TESObjectREFR*>(pForm);
    }
    else {
        pRef = nullptr;
    }
    pThreat = pRef;
    if (pThreat) {
        pThreat->SetTargeted(true);
    }
}

CombatThreatMap::CombatThreatMap() : NiTMap<uint32_t, CombatThreat*>(37u) {}

CombatThreatMap::~CombatThreatMap() {
    RemoveAll();
}

void CombatThreatMap::UpdateThreats() {
    NiTMapItem<uint32_t, CombatThreat*>* it = GetFirstPos();
    if (it) {
        Lock.LockForWrite();
        float ftime = CombatTimer::GetTimer();
        while (it) {
            uint32_t ithreatID = 0;
            CombatThreat* pthreat = nullptr;
            GetNext(it, ithreatID, pthreat);
            if (pthreat && pthreat->IsExpired(ftime)) {
                if (pthreat) {
                    delete pthreat;
                }
                RemoveAt(ithreatID);
            }
        }
        Lock.UnlockWrite();
    }
}

void CombatThreatMap::RemoveThreat(TESForm* apForm) {
    Lock.LockForWrite();
    CombatThreat* pthreat = nullptr;
    if (GetAt(apForm->iFormID, pthreat)) {
        if (pthreat) {
            delete pthreat;
        }
        RemoveAt(apForm->iFormID);
    }
    Lock.UnlockWrite();
}

void CombatThreatMap::AddThreat(
    TESObjectREFR* apRef,
    float afRadius,
    float afStartTime,
    float afEndTime,
    float afDamage
) {
    Lock.LockForWrite();
    apRef->SetTargeted(true);
    CombatThreat* pthreat = nullptr;
    if (GetAt(apRef->iFormID, pthreat)) {
        pthreat->Initialize(apRef, afRadius, afStartTime, afEndTime, afDamage, true);
    }
    else {
        pthreat = new CombatThreat(apRef, afRadius, afStartTime, afEndTime, afDamage);
        SetAt(apRef->iFormID, pthreat);
    }
    if (bDebugCombatThreats.Bool()) {}
    Lock.UnlockWrite();
}

void CombatThreatMap::AddThreat(TESObjectREFR* apRef, BGSExplosion* apExplosion, float afTime) {
    float fDamage = apExplosion->BGSExplosion::GetDamage();
    if (fDangerousProjectileExplosionDamage.Float() <= static_cast<double>(fDamage)) {
        float afExplosionRadius = apExplosion->GetRadiusBSUnits();
        float fRadius = CombatFormulas::CalcExplosionDamageDistance(
            afExplosionRadius,
            fDamage,
            fDangerousProjectileExplosionDamage.Float()
        );
        AddThreat(apRef, fRadius, 0.0f, afTime, fDamage);
    }
}

float CombatThreatMap::ComputeThreatScore(
    NiPoint3 aLocation,
    Actor* apActor,
    float afTime,
    float afAdditionalRadius,
    CombatTimeStamp afSkipOlderThreats,
    float afActorHealth
) {
    CombatTimeStamp kStartTime;
    Lock.LockForRead();
    float fthreatScore = 0.0;
    float fcompareTime = 0.0;
    if (afTime < 0.0) {
        fcompareTime = afTime;
    }
    else {
        fcompareTime = CombatTimer::GetTimer() + afTime;
    }
    NiTMapItem<uint32_t, CombatThreat*>* it = GetFirstPos();
    while (it) {
        uint32_t ithreatID = 0;
        CombatThreat* pthreat = nullptr;
        GetNext(it, ithreatID, pthreat);
        double dStartTime = pthreat->GetStartTime(&kStartTime)->fTimeStamp;
        if (!(afSkipOlderThreats.fTimeStamp >= dStartTime)) {
            float fscore = pthreat->ComputeThreatScore(
                aLocation,
                apActor,
                fcompareTime,
                afAdditionalRadius,
                afActorHealth
            );
            fthreatScore = fthreatScore + fscore;
            if (fthreatScore > 1.0) {
                fthreatScore = 1.0f;
                break;
            }
        }
    }
    Lock.UnlockRead();
    return fthreatScore;
}

// Saves every threat as its reference's form ID and the threat, with the count in front. The position is re-tested
// after every GetNext, which advances it in place and clears the threat pointer before filling it.
void CombatThreatMap::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    uint32_t uiCount = 0;
    uint32_t uiPosition = apSaveGameBuffer->StartVariableSizedValue();
    NiTMapItem<uint32_t, CombatThreat*>* pPos = GetFirstPos();
    while (pPos) {
        CombatThreat* pThreat = nullptr;
        uint32_t uiFormID;
        GetNext(pPos, uiFormID, pThreat);
        if (pThreat) {
            apSaveGameBuffer->SaveFormID(uiFormID, 0);
            pThreat->SaveGame(apSaveGameBuffer);
            ++uiCount;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(uiCount, uiPosition);
}

void CombatThreatMap::FinishLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    NiTMapItem<uint32_t, CombatThreat*>* it;
    it = GetFirstPos();
    while (it) {
        uint32_t irefID;
        CombatThreat* pthreat = nullptr;
        GetNext(it, irefID, pthreat);
        if (pthreat) {}
    }
}

void CombatThreatMap::RemoveAll() {
    NiTMapItem<uint32_t, CombatThreat*>* it;
    it = GetFirstPos();
    while (it) {
        uint32_t ithreatID = 0;
        CombatThreat* pthreat = nullptr;
        GetNext(it, ithreatID, pthreat);
        if (pthreat) {
            delete pthreat;
        }
    }
    NiTMapBase<DFALL<NiTMapItem<uint32_t, CombatThreat*>>, uint32_t, CombatThreat*>::RemoveAll();
}

PathingAvoidNodeArray* CombatThreatMap::CreateAvoidNodeArray(const NiPoint3& arLocation, Actor* apActor, float afTime) {
    PathingAvoidNode kBestAvoidNode;
    PathingAvoidNode kAvoidNode;
    NiTMapItem<uint32_t, CombatThreat*>* it;
    TESCombatStyle* pcombatStyle = apActor->GetCombatStyle();
    if (pcombatStyle->GetIgnoreThreats()) {
        return nullptr;
    }
    float fcompareTime = 0.0;
    if (afTime < 0.0) {
        fcompareTime = afTime;
    }
    else {
        fcompareTime = CombatTimer::GetTimer() + afTime;
    }
    PathingAvoidNodeArray* pavoidNodeArray = nullptr;
    CombatThreat* pbestThreat = nullptr;
    float fhighestThreatScore = 0.0f;
    Lock.LockForRead();
    it = GetFirstPos();
    while (it) {
        uint32_t ithreatID = 0;
        CombatThreat* pthreat = nullptr;
        GetNext(it, ithreatID, pthreat);
        float fthreatScore = pthreat->ComputeThreatScore(arLocation, apActor, fcompareTime, 1024.0f, 0.0f);
        if (fhighestThreatScore < static_cast<double>(fthreatScore)) {
            fhighestThreatScore = fthreatScore;
            pbestThreat = pthreat;
        }
        if (fCombatThreatSignificantScore.Float() < static_cast<double>(fthreatScore)) {
            if (!pavoidNodeArray) {
                pavoidNodeArray = new PathingAvoidNodeArray;
            }
            PathingAvoidNode* pavoidNode = pthreat->GetAvoidNode(&kAvoidNode, apActor);
            pavoidNodeArray->Add(*pavoidNode);
        }
    }
    Lock.UnlockRead();
    if (!pavoidNodeArray && pbestThreat) {
        pavoidNodeArray = new PathingAvoidNodeArray;
        PathingAvoidNode* pavoidNode = pbestThreat->GetAvoidNode(&kBestAvoidNode, apActor);
        pavoidNodeArray->Add(*pavoidNode);
    }

    return pavoidNodeArray;
}

void CombatThreatMap::AddProjectile(Projectile* apProjectile) {
    if (!apProjectile->IsFlagSet(Projectile::HITSCAN_MODE)) {
        BGSProjectile* pBaseForm = apProjectile->GetProjectileBase();
        BGSExplosion* pExplosion = pBaseForm->GetExplosion();
        if (pBaseForm->GetHasExplosion() && pExplosion) {
            float afExplosionRadius = pExplosion->GetRadiusBSUnits();
            float afBaseExplosionDamage = pExplosion->GetDamage();
            if (fDangerousProjectileExplosionDamage.Float() <= static_cast<double>(afBaseExplosionDamage)) {
                float fRadius = CombatFormulas::CalcExplosionDamageDistance(
                    afExplosionRadius,
                    afBaseExplosionDamage,
                    fDangerousProjectileExplosionDamage.Float()
                );
                if (fDangerousProjectileExplosionRadius.Float() <= static_cast<double>(fRadius)) {
                    float afStartTime = 0.0;
                    float afEndTime = std::numeric_limits<float>::max();
                    if (pBaseForm->GetHasExplosionAltTrigger()) {
                        if (apProjectile->GetExplosionTimer() == 3.4028234663852886e+38) {
                            afEndTime = fCombatThreatProximityExplosionAvoidTime.Float();
                        }
                        else {
                            afEndTime = apProjectile->GetExplosionTimer() + 1.0;
                            if (fCombatThreatTimedExplosionLength.Float() < static_cast<double>(afEndTime)) {
                                afStartTime = afEndTime - fCombatThreatTimedExplosionLength.Float();
                            }
                        }
                    }
                    if (afEndTime > 0.0) {
                        AddThreat(apProjectile, fRadius, afStartTime, afEndTime, afBaseExplosionDamage);
                    }
                }
            }
        }
    }
}

void CombatThreatMap::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
        uint32_t irefID;
        bool bremove = false;
        CombatThreat* pthreat = new CombatThreat;
        apLoadGameBuffer->LoadFormID(irefID);
        // The threat keeps the reference's form ID until InitLoadGame resolves it.
        // Stores a form ID (uint32_t) in pThreat, cast to TESObjectREFR*.
        pthreat->pThreat = reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(irefID));
        pthreat->Load(apLoadGameBuffer);
        if (!irefID) {
            bremove = true;
        }
        if (bremove) {
            if (pthreat) {
                delete pthreat;
            }
        }
        else {
            SetAt(irefID, pthreat);
        }
    }
}

void CombatThreatMap::InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    NiTMapItem<uint32_t, CombatThreat*>* it = GetFirstPos();
    while (it) {
        uint32_t irefID;
        CombatThreat* pthreat = nullptr;
        GetNext(it, irefID, pthreat);
        if (pthreat) {
            pthreat->InitLoadGame(apLoadGameBuffer);
            if (!pthreat->pThreat) {
                if (pthreat) {
                    delete pthreat;
                }
                RemoveAt(irefID);
            }
        }
    }
}

// The projectile's alternate explosion trigger flag.
bool BGSProjectile::GetHasExplosionAltTrigger() {
    return IsFlagSet(EXPLOSION_ALT_TRIGGER);
}

float Projectile::GetExplosionTimer() {
    return fExplosionTimer;
}
