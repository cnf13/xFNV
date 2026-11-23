#include "Fallout Shared/ExtraDataObjects.h"

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSMultiBound.h"
#include "BSMain/BSMultiBoundRoom.h"
#include "BSMain/BSOcclusionPlane.h"
#include "BSMain/BSPortal.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSShader/BSShaderNoLightingProperty.h"
#include "Fallout Shared/ActorCause.h"
#include "Fallout Shared/AiFormulas.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/AudioMarkerData.h"
#include "Fallout Shared/BGSAcousticSpace.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSEntryPoint.h"
#include "Fallout Shared/BGSMusicType.h"
#include "Fallout Shared/BGSPrimitive.h"
#include "Fallout Shared/BGSTalkingActivator.h"
#include "Fallout Shared/BSAnimNoteReceiver.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraGuardedRefData.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/CellMopp.h"
#include "Fallout Shared/Havok/TESWaterListener.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/LockLevel.h"
#include "Fallout Shared/Magic/AlchemyItem.h"
#include "Fallout Shared/MediaLocationController.h"
#include "Fallout Shared/MultiBoundMarkerData.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageEventAction.h"
#include "Fallout Shared/Pathfinding/PathingAvoidNode.h"
#include "Fallout Shared/Region System/TESRegion.h"
#include "Fallout Shared/Region System/TESRegionList.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAmmo.h"
#include "Fallout Shared/TESClimate.h"
#include "Fallout Shared/TESCombatStyle.h"
#include "Fallout Shared/TESFaction.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESGlobal.h"
#include "Fallout Shared/TESImageSpace.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESTopicInfo.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout Shared/TeleportData.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/ActorPackageData.h"
#include "Fallout/AI/AlarmPackage.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatTimeStamp.h"
#include "Fallout/AI/Combat/CombatTimer.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/AI/TrespassPackage.h"
#include "Fallout/Dialogue/MenuTopic.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameSubBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

#include <cmath>
#include <cstring>
#include <limits>

pLockLevelNames_t pLockLevelNames{
    &sLockLevelNameVeryEasy,
    &sLockLevelNameEasy,
    &sLockLevelNameAverage,
    &sLockLevelNameHard,
    &sLockLevelNameVeryHard,
    &sLockLevelNameImpossible
};
LeveledCreatureMultiplierSettings
    pLevCreaModMult{&fLeveledActorMultEasy, &fLeveledActorMultMedium, &fLeveledActorMultHard, &fLeveledActorMultBoss};

// The game settings this file defines, with their IDs and values, in the game's initialization order.
SettingT<GameSettingCollection> fLeveledLockMult("fLeveledLockMult", 1.5f);
SettingT<GameSettingCollection> sLockLevelNameVeryEasy("sLockLevelNameVeryEasy", "Very Easy");
SettingT<GameSettingCollection> sLockLevelNameEasy("sLockLevelNameEasy", "Easy");
SettingT<GameSettingCollection> sLockLevelNameAverage("sLockLevelNameAverage", "Average");
SettingT<GameSettingCollection> sLockLevelNameHard("sLockLevelNameHard", "Hard");
SettingT<GameSettingCollection> sLockLevelNameVeryHard("sLockLevelNameVeryHard", "Very Hard");
SettingT<GameSettingCollection> sLockLevelNameImpossible("sLockLevelNameImpossible", "Requires Key");
SettingT<GameSettingCollection> iLockLevelMaxVeryEasy("iLockLevelMaxVeryEasy", 0);
SettingT<GameSettingCollection> iLockLevelMaxEasy("iLockLevelMaxEasy", 25);
SettingT<GameSettingCollection> iLockLevelMaxAverage("iLockLevelMaxAverage", 50);
SettingT<GameSettingCollection> iLockLevelMaxHard("iLockLevelMaxHard", 75);
SettingT<GameSettingCollection> iLockLevelMaxVeryHard("iLockLevelMaxVeryHard", 100);
SettingT<GameSettingCollection> iLockLevelMaxImpossible("iLockLevelMaxImpossible", 255);
SettingT<GameSettingCollection> fLeveledActorMultEasy("fLeveledActorMultEasy", 0.25f);
SettingT<GameSettingCollection> fLeveledActorMultMedium("fLeveledActorMultMedium", 0.5f);
SettingT<GameSettingCollection> fLeveledActorMultHard("fLeveledActorMultHard", 0.75f);
SettingT<GameSettingCollection> fLeveledActorMultBoss("fLeveledActorMultBoss", 1.0f);

void RADIO_DATA::Copy(RADIO_DATA* apCopy) {
    if (apCopy) {
        *this = *apCopy;
    }
}

void RADIO_DATA::Endian() {
    ENDIAN(fRadius, false);
    ENDIAN(eRangeType, false);
    ENDIAN(fStaticPct, false);
    ENDIAN(pPositionRef, false);
}

WORLD_LOCATION::WORLD_LOCATION() {}

void WORLD_LOCATION::Endian() {
    ENDIAN(pLocationForm, false);
    ENDIAN(locPt.x, false);
    ENDIAN(locPt.y, false);
    ENDIAN(locPt.z, false);
}

ExtraAction::ExtraAction() : BSExtraData(EXTRA_ACTION), eAction(1), pActionRef(nullptr) {}

bool ExtraAction::Compare(BSExtraData* apCompare) {
    ExtraAction* pCompare = dynamic_cast<ExtraAction*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return eAction != pCompare->eAction;
}

ExtraActivateRef::ExtraActivateRef() : BSExtraData(83), cActivateFlags(0) {
    ActivateTextOverride.Set("", 0u);
}

bool ExtraActivateRef::Compare(BSExtraData* apCompare) {
    ExtraActivateRef* pComp = dynamic_cast<ExtraActivateRef*>(apCompare);
    if (!pComp) {
        return true;
    }
    int32_t iCount = ParentList.ItemsInList();
    if (iCount != pComp->ParentList.ItemsInList()) {
        return true;
    }
    for (BSSimpleList<REF_ACTIVATE_DATA*>* pList = &ParentList; pList && !pList->IsEmpty(); pList = pList->m_pkNext) {
        REF_ACTIVATE_DATA* pData = pList->m_item;
        REF_ACTIVATE_DATA* pCompData = pComp->GetData(pData->pActivateRef);
        if (!pCompData || std::memcmp(pCompData, pData, sizeof(REF_ACTIVATE_DATA))) {
            return true;
        }
    }
    if (cActivateFlags != pComp->cActivateFlags) {
        return true;
    }
    if (!ActivateTextOverride.GetLength() && !pComp->ActivateTextOverride.GetLength()) {
        return false;
    }
    if (!ActivateTextOverride.GetLength() || !pComp->ActivateTextOverride.GetLength()) {
        return true;
    }
    return BSstrcmp(ActivateTextOverride.pString, pComp->ActivateTextOverride.pString) != 0;
}

ExtraActivateRef::~ExtraActivateRef() {
    FreeActivateList(&ParentList);
}

REF_ACTIVATE_DATA* ExtraActivateRef::GetData(TESObjectREFR* apRef) {
    return GetActivateDataFromList(apRef, &ParentList);
}

void ExtraActivateRef::Copy(BSExtraData* apCopy) {
    ExtraActivateRef* pCopy = dynamic_cast<ExtraActivateRef*>(apCopy);
    if (pCopy) {
        FreeActivateList(&ParentList);
        BSSimpleList<REF_ACTIVATE_DATA*>* pList = &pCopy->ParentList;
        while (pList && !pList->IsEmpty()) {
            REF_ACTIVATE_DATA* pData = pList->m_item;
            pList = pList->m_pkNext;
            REF_ACTIVATE_DATA* pCopyData = new REF_ACTIVATE_DATA;
            ourMemcpy(pCopyData, pData, sizeof(REF_ACTIVATE_DATA));
            ParentList.AddHead(pCopyData);
        }
        cActivateFlags = pCopy->cActivateFlags;
        ActivateTextOverride.Set(pCopy->ActivateTextOverride);
    }
}

REF_ACTIVATE_DATA::REF_ACTIVATE_DATA() : pActivateRef(nullptr), fActivateDelay(0.0) {}

void REF_ACTIVATE_DATA::Endian() {
    ENDIAN(pActivateRef, false);
    ENDIAN(fActivateDelay, false);
}

REF_ACTIVATE_DATA* GetActivateDataFromList(TESObjectREFR* apRef, BSSimpleList<REF_ACTIVATE_DATA*>* apList) {
    while (apList && !apList->IsEmpty()) {
        REF_ACTIVATE_DATA* pData = apList->m_item;
        if (pData->pActivateRef == apRef) {
            return pData;
        }
        apList = apList->m_pkNext;
    }
    return nullptr;
}

int32_t GetLockPickLevel(int32_t aiLockLevel) {
    if (aiLockLevel <= static_cast<int32_t>(iLockLevelMaxVeryEasy.UInt())) {
        return 0;
    }
    if (aiLockLevel <= static_cast<int32_t>(iLockLevelMaxEasy.UInt())) {
        return 1;
    }
    if (aiLockLevel <= static_cast<int32_t>(iLockLevelMaxAverage.UInt())) {
        return 2;
    }
    if (aiLockLevel <= static_cast<int32_t>(iLockLevelMaxHard.UInt())) {
        return 3;
    }
    if (aiLockLevel > static_cast<int32_t>(iLockLevelMaxVeryHard.UInt())) {
        return 5;
    }
    return 4;
}

uint32_t getLockLevelName(int32_t aiLevel) {
    uint32_t name = 0;
    switch (aiLevel) {
        case 0: {
            name = iLockLevelMaxVeryEasy.UInt();
            break;
        }
        case 1: {
            name = iLockLevelMaxEasy.UInt();
            break;
        }
        case 2: {
            name = iLockLevelMaxAverage.UInt();
            break;
        }
        case 3: {
            name = iLockLevelMaxHard.UInt();
            break;
        }
        case 4: {
            name = iLockLevelMaxVeryHard.UInt();
            break;
        }
        case 5: {
            name = iLockLevelMaxImpossible.UInt();
            break;
        }
        default: {
            break;
        }
    }
    return name;
}

// Defined here, after the iLockLevelMax settings it reads, so that their constructors have run whatever order the
// files are linked in.
LockLevelMaxVeryEasy_t LockLevelMaxVeryEasy = {
    static_cast<int32_t>(getLockLevelName(LL_VERYEASY)),
    static_cast<int32_t>(getLockLevelName(LL_EASY)),
    static_cast<int32_t>(getLockLevelName(LL_AVERAGE)),
    static_cast<int32_t>(getLockLevelName(LL_HARD)),
    static_cast<int32_t>(getLockLevelName(LL_VERYHARD)),
    101
};

void FreeActivateList(BSSimpleList<REF_ACTIVATE_DATA*>* apList) {
    if (apList) {
        while (!apList->IsEmpty()) {
            ourFree(apList->m_item);
            apList->RemoveHead();
        }
    }
}

ExtraActivateRefChildren::ExtraActivateRefChildren() : BSExtraData(84u), fActivateChildrenTimer(0.0f) {}

ExtraActivateRefChildren::~ExtraActivateRefChildren() {
    FreeActivateList(&ChildList);
}

ExtraAmmo::ExtraAmmo(TESAmmo* apAmmo, int32_t aiCount) : BSExtraData(EXTRA_AMMO), pAmmo(apAmmo), iCount(aiCount) {}

bool ExtraAmmo::Compare(BSExtraData* apCompare) {
    ExtraAmmo* pCompare = dynamic_cast<ExtraAmmo*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    if (iCount == pCompare->iCount) {
        return pAmmo != pCompare->pAmmo;
    }
    return true;
}

ExtraAnim::ExtraAnim(Animation* apAnim) : BSExtraData(EXTRA_ANIM), pAnimation(apAnim) {}

ExtraAnim::~ExtraAnim() {
    delete pAnimation;
}

ExtraAnimNoteReceiver::ExtraAnimNoteReceiver(BSAnimNoteReceiver* apReceiver)
  : BSExtraData(EXTRA_ANIMNOTE_RECEIVER), pReceiver(apReceiver) {}

ExtraAudioMarker::ExtraAudioMarker(AudioMarker* apAudioData)
  : BSExtraData(EXTRA_AUDIOMARKER), pAudioData(apAudioData) {}

bool ExtraAudioMarker::Compare(BSExtraData* apCompare) {
    ExtraAudioMarker* pComp = dynamic_cast<ExtraAudioMarker*>(apCompare);
    if (!pComp) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return pAudioData->Compare(pComp->pAudioData);
}

ExtraAudioMarker::~ExtraAudioMarker() {
    delete pAudioData;
}

ExtraCellImageSpace::ExtraCellImageSpace(TESImageSpace* apImageSpace)
  : BSExtraData(EXTRA_IMAGESPACE), pImageSpace(apImageSpace) {}

ExtraCellMusicType::ExtraCellMusicType(BGSMusicType* apType) : BSExtraData(EXTRA_CELLMUSICTYPE), pType(apType) {}

ExtraCellWaterType::ExtraCellWaterType() : BSExtraData(EXTRA_WATERTYPE), pWater(nullptr) {}

void CollisionData::SetLayer(COL_LAYER aeLayer) {
    eLayer = aeLayer;
}

void CollisionData::Endian() {
    ENDIAN(eLayer, false);
}

ExtraCollisionData::ExtraCollisionData() : BSExtraData(EXTRA_COLLISION_DATA) {}

ExtraCombatStyle::ExtraCombatStyle(TESCombatStyle* apCombatStyle)
  : BSExtraData(EXTRA_COMBAT_STYLE), pCombatStyle(apCombatStyle) {}

bool ExtraCombatStyle::Compare(BSExtraData* apCompare) {
    ExtraCombatStyle* pCompare = dynamic_cast<ExtraCombatStyle*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return pCombatStyle != pCompare->pCombatStyle;
}

ExtraContainerChanges::ExtraContainerChanges(InventoryChanges* apChanges)
  : BSExtraData(EXTRA_CONTAINER_CHANGES), pChanges(apChanges) {}

ExtraContainerChanges::~ExtraContainerChanges() {
    delete pChanges;
}

ExtraCount::ExtraCount() : BSExtraData(36), iCount(0) {}

ExtraCount::ExtraCount(int16_t aiCount) : BSExtraData(EXTRA_COUNT), iCount(aiCount) {}

bool ExtraCount::Compare(BSExtraData* apCompare) {
    ExtraCount* pCompare = dynamic_cast<ExtraCount*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return iCount != pCompare->iCount;
}

ExtraDecalRefs::ExtraDecalRefs() : BSExtraData(87u) {}

bool ExtraDecalRefs::Compare(BSExtraData* apCompare) {
    ExtraDecalRefs* pComp = dynamic_cast<ExtraDecalRefs*>(apCompare);
    if (!pComp) {
        return true;
    }
    int32_t iCount = DecalRefList.ItemsInList();
    if (iCount != pComp->DecalRefList.ItemsInList()) {
        return true;
    }
    for (BSSimpleList<REF_DECAL_DATA*>* pList = &pComp->DecalRefList; pList && !pList->IsEmpty();
         pList = pList->m_pkNext) {
        REF_DECAL_DATA* pCompData = pList->m_item;
        REF_DECAL_DATA* pData = GetRefDecalData(pCompData->pDecalRef);
        if (!pData) {
            return true;
        }
        if (std::memcmp(pData, pCompData, sizeof(REF_DECAL_DATA))) {
            return true;
        }
    }
    return false;
}

void ExtraDecalRefs::InitItem(TESForm* apOwner) {
    BSSimpleList<REF_DECAL_DATA*>* pRefList = &DecalRefList;
    BSSimpleList<REF_DECAL_DATA*>* pLastList = nullptr;
    while (pRefList && !pRefList->IsEmpty()) {
        bool bAdvanced = false;
        REF_DECAL_DATA* pData = pRefList->m_item;
        uint32_t tempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pData->pDecalRef));
        TESFile* pFile = apOwner->GetFile(-1);
        TESForm::AddCompileIndex(tempint, pFile);
        TESForm* pForm = TESForm::GetFormByNumericID(tempint);
        TESObjectREFR* pDecal = dynamic_cast<TESObjectREFR*>(pForm);
        if (pDecal) {
            pData->pDecalRef = pDecal;
        }
        else {
            Warning("MASTERFILE: Unable to find decal reference %08X.", tempint);
            if (pLastList) {
                pLastList->Remove(pData);
                pRefList = pLastList->m_pkNext;
            }
            else {
                pRefList->RemoveHead();
            }
            ourFree(pData);
            pData = nullptr;
            bAdvanced = true;
        }
        if (!bAdvanced) {
            pLastList = pRefList;
            pRefList = pRefList->m_pkNext;
        }
    }
}

REF_DECAL_DATA* ExtraDecalRefs::GetRefDecalData(TESObjectREFR* apRef) {
    BSSimpleList<REF_DECAL_DATA*>* pList = &DecalRefList;
    while (pList && !pList->IsEmpty()) {
        REF_DECAL_DATA* pData = static_cast<REF_DECAL_DATA*>(pList->m_item);
        pList = pList->m_pkNext;
        if (pData->pDecalRef == apRef) {
            return pData;
        }
    }
    return nullptr;
}

// Each point is copied 4 bytes at a time, each load followed by its store, so a source overlapping the record reads the
// bytes already written; the 4-byte copies deliberately keep that order and the bits.
void ExtraDecalRefs::SetRefDecalData(TESObjectREFR* apRef, const NiPoint3& aIntersect, const NiPoint3& aNormal) {
    REF_DECAL_DATA* pData = GetRefDecalData(apRef);
    if (!pData) {
        pData = new REF_DECAL_DATA;
        pData->pDecalRef = apRef;
        DecalRefList.AddHead(pData);
    }
    std::memcpy(&pData->Intersect.x, &aIntersect.x, sizeof(pData->Intersect.x));
    std::memcpy(&pData->Intersect.y, &aIntersect.y, sizeof(pData->Intersect.y));
    std::memcpy(&pData->Intersect.z, &aIntersect.z, sizeof(pData->Intersect.z));
    std::memcpy(&pData->Normal.x, &aNormal.x, sizeof(pData->Normal.x));
    std::memcpy(&pData->Normal.y, &aNormal.y, sizeof(pData->Normal.y));
    std::memcpy(&pData->Normal.z, &aNormal.z, sizeof(pData->Normal.z));
}

int32_t ExtraDecalRefs::ClearDecalData() {
    int32_t iEmpty;
    while (true) {
        iEmpty = DecalRefList.IsEmpty();
        if (iEmpty) {
            break;
        }
        void* pData = DecalRefList.m_item;
        DecalRefList.RemoveHead();
        ourFree(pData);
    }
    return iEmpty;
}

void ExtraDecalRefs::Copy(BSExtraData* apCopy) {
    ExtraDecalRefs* pCopy = dynamic_cast<ExtraDecalRefs*>(apCopy);
    if (pCopy) {
        ClearDecalData();
        for (BSSimpleList<REF_DECAL_DATA*>* pList = &pCopy->DecalRefList; pList && !pList->IsEmpty();
             pList = pList->m_pkNext) {
            REF_DECAL_DATA* pCopyData = pList->m_item;
            REF_DECAL_DATA* pData = new REF_DECAL_DATA;
            ourMemcpy(pData, pCopyData, sizeof(REF_DECAL_DATA));
            DecalRefList.AddHead(pData);
        }
    }
}

ExtraDetachTime::ExtraDetachTime() : BSExtraData(EXTRA_CELLDETACHTIME), iTime(0) {}

ExtraDroppedItemList::ExtraDroppedItemList() : BSExtraData(58u) {}

ExtraDroppedItemList::~ExtraDroppedItemList() {
    DroppedItemList.RemoveAll();
}

ExtraEmittanceSource::ExtraEmittanceSource() : BSExtraData(EXTRA_EMITTANCE_SOURCE), pSource(nullptr) {}

bool ExtraEmittanceSource::Compare(BSExtraData* apCompare) {
    ExtraEmittanceSource* pCompare = dynamic_cast<ExtraEmittanceSource*>(apCompare);
    return !pCompare || pSource != pCompare->pSource;
}

ExtraEnableStateChildren::ExtraEnableStateChildren() : BSExtraData(0x38u) {}

ExtraEnableStateChildren::~ExtraEnableStateChildren() {
    ChildList.RemoveAll();
}

ExtraEnableStateParent::ExtraEnableStateParent() : BSExtraData(EXTRA_ENABLESTATEPARENT), pParent(nullptr), cFlags(0) {}

bool ExtraEnableStateParent::Compare(BSExtraData* apCompare) {
    ExtraEnableStateParent* pComp = dynamic_cast<ExtraEnableStateParent*>(apCompare);
    if (!pComp) {
        return true;
    }
    if (pComp->pParent == pParent) {
        return pComp->cFlags != cFlags;
    }
    return true;
}

bool ExtraEnableStateParent::ShouldPopIn() {
    return (cFlags & FLAG_POP_IN) != 0;
}

bool ExtraEnableStateParent::ShouldPopOut() {
    return (cFlags & FLAG_POP_OUT) != 0;
}

void REF_ENABLE_PARENT_DATA::Endian() {
    ENDIAN(pParent, false);
}

ExtraEncounterZone::ExtraEncounterZone() : BSExtraData(EXTRA_ENCOUNTERZONE), pZone(nullptr) {}

bool ExtraEncounterZone::Compare(BSExtraData* apCompare) {
    ExtraEncounterZone* pCompare = dynamic_cast<ExtraEncounterZone*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return pZone != pCompare->pZone;
}

ExtraFollower::ExtraFollower() : BSExtraData(29), pActorlist(new BSSimpleList<Actor*>) {}

ExtraFollower::~ExtraFollower() {
    pActorlist->RemoveAll();
    delete pActorlist;
}

ExtraGlobal::ExtraGlobal(TESGlobal* apGlobal) : BSExtraData(EXTRA_GLOBAL), pGlobal(apGlobal) {}

bool ExtraGlobal::Compare(BSExtraData* apCompare) {
    ExtraGlobal* pCompare = dynamic_cast<ExtraGlobal*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return pGlobal != pCompare->pGlobal;
}

ExtraHealth::ExtraHealth() : BSExtraData(37u), fHealth(0.0) {}

ExtraHealth::ExtraHealth(float afHealth) : BSExtraData(EXTRA_HEALTH), fHealth(afHealth) {}

bool ExtraHealth::Compare(BSExtraData* apCompare) {
    ExtraHealth* pCompare = dynamic_cast<ExtraHealth*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return pCompare->fHealth != fHealth;
}

ExtraHealthPerc::ExtraHealthPerc() : BSExtraData(EXTRA_HEALTH_PERC), fHealthPerc(0.0f) {}

bool ExtraHealthPerc::Compare(BSExtraData* apCompare) {
    ExtraHealthPerc* pCompare = dynamic_cast<ExtraHealthPerc*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return pCompare->fHealthPerc != fHealthPerc;
}

ExtraIgnoredBySandbox::ExtraIgnoredBySandbox() : BSExtraData(EXTRA_IGNORED_BY_SANDBOX) {}

ExtraLastFinishedSequence::ExtraLastFinishedSequence(const char* apSequenceName) : BSExtraData(65) {
    int32_t iSeqNameLen = BSstrlen(apSequenceName) + 1;
    pLastSequenceName = static_cast<char*>(ourMalloc(iSeqNameLen));
    BSstrcpy(pLastSequenceName, iSeqNameLen, apSequenceName);
}

bool ExtraLastFinishedSequence::Compare(BSExtraData* apCompare) {
    ExtraLastFinishedSequence* pComp = dynamic_cast<ExtraLastFinishedSequence*>(apCompare);
    return !pComp || BSstrcmp(pLastSequenceName, pComp->pLastSequenceName) != 0;
}

ExtraLastFinishedSequence::~ExtraLastFinishedSequence() {
    ourFree(pLastSequenceName);
}

ExtraLeveledCreature::ExtraLeveledCreature()
  : BSExtraData(EXTRA_LEVELEDCREATURE), pOriginalBase(nullptr), pTemplate(nullptr) {}

ExtraLeveledItem::ExtraLeveledItem() : BSExtraData(47u), iIndex(0), bdefault(0) {}

ExtraLeveledItem::ExtraLeveledItem(int32_t aiIndex) : BSExtraData(EXTRA_LEVELITEM), iIndex(aiIndex), bdefault(1) {}

bool ExtraLeveledItem::Compare(BSExtraData* apCompare) {
    return !dynamic_cast<ExtraLeveledItem*>(apCompare) || BSExtraData::Compare(apCompare);
}

ExtraLight::ExtraLight(LightRefData* apLight) : BSExtraData(EXTRA_LIGHT), pLight(apLight) {}

ExtraLight::~ExtraLight() {
    delete pLight;
}

ExtraLinkedRef::ExtraLinkedRef() : BSExtraData(EXTRA_LINKED_REF), pLinkedRef(nullptr) {}

bool ExtraLinkedRef::Compare(BSExtraData* apCompare) {
    ExtraLinkedRef* pCompare = dynamic_cast<ExtraLinkedRef*>(apCompare);
    return !pCompare || pCompare->pLinkedRef != pLinkedRef;
}

ExtraLinkedRefChildren::ExtraLinkedRefChildren() : BSExtraData(0x52u) {}

ExtraLinkedRefChildren::~ExtraLinkedRefChildren() {
    ChildList.RemoveAll();
}

ExtraLitWaterRefs::ExtraLitWaterRefs() : BSExtraData(133u) {}

bool ExtraLitWaterRefs::Compare(BSExtraData* apCompare) {
    ExtraLitWaterRefs* pComp = dynamic_cast<ExtraLitWaterRefs*>(apCompare);
    if (!pComp) {
        return true;
    }
    int32_t iCount = RefList.ItemsInList();
    if (iCount != pComp->RefList.ItemsInList()) {
        return true;
    }
    for (BSSimpleList<TESObjectREFR*>* pList = &pComp->RefList; pList && !pList->IsEmpty(); pList = pList->m_pkNext) {
        if (!RefList.IsInList(pList->m_item)) {
            return true;
        }
    }
    return false;
}

void ExtraLitWaterRefs::InitItem(TESForm* apOwner) {
    BSSimpleList<TESObjectREFR*>* pRefList = &RefList;
    BSSimpleList<TESObjectREFR*> newlist;
    while (pRefList && !pRefList->IsEmpty()) {
        uint32_t tempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pRefList->m_item));
        TESFile* pFile = apOwner->GetFile(-1);
        TESForm::AddCompileIndex(tempint, pFile);
        TESForm* pForm = TESForm::GetFormByNumericID(tempint);
        TESObjectREFR* pRef = dynamic_cast<TESObjectREFR*>(pForm);
        if (pRef && pRef->GetRefPersists()) {
            pRef->m_Extra.SetWaterLightRef(static_cast<TESObjectREFR*>(apOwner), true);
            newlist.AddHead(pRef);
        }
        else if (pRef) {
            Warning(
                "MASTERFILE: Reference %08X is not persistent so cannot reflect or refract other references.",
                tempint
            );
        }
        else {
            Warning("MASTERFILE: Unable to find lit water reference %08X.", tempint);
        }
        pRefList = pRefList->m_pkNext;
    }
    RefList.RemoveAll();
    for (TESObjectREFR*& rpItem : newlist.ItemsUntilNull()) {
        RefList.AddHead(rpItem);
    }
    newlist.RemoveAll();
}

void ExtraLitWaterRefs::Copy(BSExtraData* apCopy) {
    ExtraLitWaterRefs* pCopy = dynamic_cast<ExtraLitWaterRefs*>(apCopy);
    if (pCopy) {
        ClearReferenceData();
        for (BSSimpleList<TESObjectREFR*>* pList = &pCopy->RefList; pList && !pList->IsEmpty();
             pList = pList->m_pkNext) {
            AddReference(pList->m_item);
        }
    }
}

void ExtraLitWaterRefs::AddReference(TESObjectREFR* apRef) {
    if (!RefList.IsInList(apRef)) {
        RefList.AddHead(apRef);
    }
}

void ExtraLitWaterRefs::ClearReferenceData() {
    RefList.RemoveAll();
}

LockData::LockData() : cLockLevel(0), pKey(nullptr), cFlags(0), iNumTries(0), iTimesUnlocked(0) {}

void LockData::Endian() {
    ENDIAN(pKey, false);
}

bool LockData::IsBroken() {
    float args = 0.0f;
    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::IGNORE_BROKEN_LOCK, pPlayer, &args);
    uint32_t uiMaxTries = args == 0.0 ? 1 : 2;
    return iNumTries >= uiMaxTries;
}

bool LockData::IsLocked() {
    return (cFlags & FLAG_LOCKED) != 0;
}

int32_t LockData::GetLockLevel(TESObjectREFR* apOwner) {
    return GetLockPickLevel(GetLevel(apOwner));
}

int32_t LockData::GetLevel(TESObjectREFR* apOwner) {
    int32_t iLevel = cLockLevel;
    if (cFlags & FLAG_LEVELED_LOCK) {
        int32_t iOwnerLevel;
        if (apOwner) {
            iOwnerLevel = apOwner->GetCalcLevel(false);
        }
        else {
            iOwnerLevel = pPlayer->GetLevel();
        }
        iLevel += iOwnerLevel * static_cast<int32_t>(fLeveledLockMult.Float());
        if (iLevel > 99) {
            iLevel = 99;
        }
    }
    return iLevel;
}

void LockData::SetLocked(bool abLocked) {
    if (abLocked) {
        cFlags |= FLAG_LOCKED;
    }
    else {
        cFlags &= ~FLAG_LOCKED;
        iNumTries = 0;
    }
}

ExtraLock::ExtraLock() : BSExtraData(EXTRA_LOCK), pLock(new LockData()) {}

ExtraLock::ExtraLock(LockData* apLock) : BSExtraData(EXTRA_LOCK), pLock(apLock) {}

bool ExtraLock::Compare(BSExtraData* apCompare) {
    ExtraLock* pComp = dynamic_cast<ExtraLock*>(apCompare);
    if (!pComp) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    if (pLock->cFlags != pComp->pLock->cFlags) {
        return true;
    }
    if (pLock->pKey != pComp->pLock->pKey) {
        return true;
    }
    int32_t iLockPickLevel = GetLockPickLevel(pLock->cLockLevel);
    if (iLockPickLevel != GetLockPickLevel(pComp->pLock->cLockLevel)) {
        return true;
    }
    if (pLock->iNumTries == pComp->pLock->iNumTries) {
        return pLock->iTimesUnlocked != pComp->pLock->iTimesUnlocked;
    }
    return true;
}

ExtraLock::~ExtraLock() {
    ourFree(pLock);
}

void ExtraLock::UnLockDoor() {
    pLock->SetLocked(false);
}

void ExtraLock::LockDoor() {
    pLock->SetLocked(true);
}

ExtraMapMarker::ExtraMapMarker(MapMarkerData* apMapData) : BSExtraData(EXTRA_MAPMARKER), pMapData(apMapData) {}

bool ExtraMapMarker::Compare(BSExtraData* apCompare) {
    ExtraMapMarker* pComp = dynamic_cast<ExtraMapMarker*>(apCompare);
    if (!pComp) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return pMapData->Compare(pComp->pMapData);
}

ExtraMapMarker::~ExtraMapMarker() {
    delete pMapData;
}

ExtraMerchantContainer::ExtraMerchantContainer() : BSExtraData(EXTRA_MERCHANTCONTAINER), pContainer(nullptr) {}

bool ExtraMerchantContainer::Compare(BSExtraData* apCompare) {
    ExtraMerchantContainer* pCompare = dynamic_cast<ExtraMerchantContainer*>(apCompare);
    return !pCompare || pCompare->pContainer != pContainer;
}

ExtraModelSwap::ExtraModelSwap() : BSExtraData(91), pModelSwap(nullptr), pModelSwapForm(nullptr) {}

ExtraModelSwap::ExtraModelSwap(TESModel* apModelSwap, TESForm* apModelSwapForm)
  : BSExtraData(EXTRA_MODEL_SWAP), pModelSwap(apModelSwap), pModelSwapForm(apModelSwapForm) {}

ExtraMultiBoundData::ExtraMultiBoundData() : BSExtraData(EXTRA_MULTIBOUND_DATA), pBound(nullptr) {}

bool ExtraMultiBoundData::Compare(BSExtraData* apCompare) {
    ExtraMultiBoundData* pComp = dynamic_cast<ExtraMultiBoundData*>(apCompare);
    return !pComp || pBound->Compare(pComp->pBound);
}

ExtraMultiBoundData::~ExtraMultiBoundData() {
    ourFree(pBound);
}

ExtraMultiBoundRef::ExtraMultiBoundRef() : BSExtraData(EXTRA_MULTIBOUND_REF), pBoundRef(nullptr) {}

bool ExtraMultiBoundRef::Compare(BSExtraData* apCompare) {
    ExtraMultiBoundRef* pCompare = dynamic_cast<ExtraMultiBoundRef*>(apCompare);
    return !pCompare || pBoundRef != pCompare->pBoundRef;
}

ExtraNorthRotation::ExtraNorthRotation() : BSExtraData(EXTRA_NORTHROTATION), fNorthRot(0.0f) {}

ExtraOwnership::ExtraOwnership(TESForm* apOwner) : BSExtraData(EXTRA_OWNERSHIP), pOwner(apOwner) {}

bool ExtraOwnership::Compare(BSExtraData* apCompare) {
    ExtraOwnership* pCompare = dynamic_cast<ExtraOwnership*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return pOwner != pCompare->pOwner;
}

ExtraPackage::ExtraPackage()
  : BSExtraData(EXTRA_PACKAGE), pPack(nullptr), iindex((-1)), pTarg(nullptr), bActionComplete(false), bActivated(false),
    bDoneOnce(false) {}

ExtraPackage::ExtraPackage(
    TESPackage* apPack,
    int32_t aiIndex,
    TESObjectREFR* apRef,
    bool abcomplete,
    bool abactivated,
    bool abDoneOnce
)
  : BSExtraData(EXTRA_PACKAGE), pPack(apPack), iindex(aiIndex), pTarg(apRef), bActionComplete(abcomplete),
    bActivated(abactivated), bDoneOnce(abDoneOnce) {}

ExtraRagDollData::ExtraRagDollData() : BSExtraData(EXTRA_RAGDOLLDATA), pRagDollData(nullptr) {}

bool ExtraRagDollData::Compare(BSExtraData* apCompare) {
    ExtraRagDollData* pComp = dynamic_cast<ExtraRagDollData*>(apCompare);
    if (!pComp) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    if (pRagDollData) {
        if (pRagDollData->Compare(pComp->pRagDollData)) {
            return true;
        }
    }
    else if (pComp->pRagDollData) {
        return true;
    }
    return false;
}

ExtraRagDollData::~ExtraRagDollData() {
    delete pRagDollData;
}

ExtraPackageStartLocation::ExtraPackageStartLocation() : BSExtraData(24u) {}

ExtraPackageStartLocation::ExtraPackageStartLocation(
    TESWorldSpace* apWorld,
    TESObjectCELL* apIntCell,
    const NiPoint3& aPt,
    float afzrotation
)
  : BSExtraData(EXTRA_PACKAGESTARTLOC) {
    if (apWorld) {
        worldLoc.pLocationForm = apWorld;
    }
    else {
        worldLoc.pLocationForm = apIntCell;
    }
    worldLoc.locPt = aPt;
    worldLoc.fZRot = afzrotation;
}

bool ExtraPackageStartLocation::Compare(BSExtraData* apCompare) {
    ExtraPackageStartLocation* pComp = dynamic_cast<ExtraPackageStartLocation*>(apCompare);
    if (!pComp) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return std::memcmp(&worldLoc, &pComp->worldLoc, sizeof(worldLoc)) != 0;
}

ExtraPatrolRefData::ExtraPatrolRefData() : BSExtraData(EXTRA_PATROL_REF_DATA) {}

void ExtraPatrolRefData::SetPatrolRefData(PatrolRefData* apPatrolRefData) {
    pPatrolData = apPatrolRefData;
}

ExtraPersistentCell::ExtraPersistentCell(TESObjectCELL* apCell)
  : BSExtraData(EXTRA_PERSISTENT_CELL), pPersistentCell(apCell) {}

ExtraPlayerCrimeList::ExtraPlayerCrimeList() : BSExtraData(53u), pCrime(new BSSimpleList<Crime*>) {}

ExtraPlayerCrimeList::ExtraPlayerCrimeList(Crime* apCrime) : BSExtraData(53u), pCrime(new BSSimpleList<Crime*>) {
    pCrime->AddHead(apCrime);
}

ExtraPlayerCrimeList::~ExtraPlayerCrimeList() {
    delete pCrime;
}

ExtraPrimitive::ExtraPrimitive(BGSPrimitive* apPrimitive) : BSExtraData(EXTRA_PRIMITIVE), pPrimitive(apPrimitive) {}

bool ExtraPrimitive::Compare(BSExtraData* apCompare) {
    ExtraPrimitive* pCompare = dynamic_cast<ExtraPrimitive*>(apCompare);
    return !pCompare || pCompare->pPrimitive != pPrimitive;
}

ExtraPrimitive::~ExtraPrimitive() {
    delete pPrimitive;
}

ExtraRadiation::ExtraRadiation() : BSExtraData(93), fRadiation(0.0f) {}

ExtraRadiation::ExtraRadiation(float afRadiation) : BSExtraData(EXTRA_RADIATION), fRadiation(afRadiation) {}

bool ExtraRadiation::Compare(BSExtraData* apCompare) {
    ExtraRadiation* pCompare = dynamic_cast<ExtraRadiation*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return pCompare->fRadiation != fRadiation;
}

ExtraRadioData::ExtraRadioData() : BSExtraData(0x68u) {
    Data.fRadius = 0.0f;
    Data.eRangeType = RADIO_RANGE_RADIUS;
    Data.fStaticPct = 0.0f;
    Data.pPositionRef = nullptr;
}

bool ExtraRadioData::Compare(BSExtraData* apCompare) {
    ExtraRadioData* pComp = dynamic_cast<ExtraRadioData*>(apCompare);
    if (!pComp) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return std::memcmp(&Data.fRadius, &pComp->Data.fRadius, sizeof(Data.fRadius)) != 0 ||
        Data.eRangeType != pComp->Data.eRangeType ||
        std::memcmp(&Data.fStaticPct, &pComp->Data.fStaticPct, sizeof(Data.fStaticPct)) != 0 ||
        Data.pPositionRef != pComp->Data.pPositionRef;
}

ExtraRadius::ExtraRadius() : BSExtraData(92), fRadius(0.0f) {}

ExtraRadius::ExtraRadius(float afRadius) : BSExtraData(EXTRA_RADIUS), fRadius(afRadius) {}

bool ExtraRadius::Compare(BSExtraData* apCompare) {
    ExtraRadius* pCompare = dynamic_cast<ExtraRadius*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return pCompare->fRadius != fRadius;
}

ExtraRank::ExtraRank(int32_t aiRank) : BSExtraData(EXTRA_RANK), iRank(aiRank) {}

bool ExtraRank::Compare(BSExtraData* apCompare) {
    ExtraRank* pCompare = dynamic_cast<ExtraRank*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return iRank != pCompare->iRank;
}

REF_REFLECTOR_DATA::REF_REFLECTOR_DATA() : pRef(nullptr), iEffectFlags(0) {}

void REF_REFLECTOR_DATA::Endian() {
    ENDIAN(pRef, false);
    ENDIAN(iEffectFlags, false);
}

REF_REFLECTED_DATA::REF_REFLECTED_DATA() : pRef(nullptr), iEffectFlags(0) {}

ExtraReflectedRefs::ExtraReflectedRefs() : BSExtraData(0x65u) {}

bool ExtraReflectedRefs::Compare(BSExtraData* apCompare) {
    ExtraReflectedRefs* pComp = dynamic_cast<ExtraReflectedRefs*>(apCompare);
    if (!pComp) {
        return true;
    }
    int32_t iCount = RefList.ItemsInList();
    if (iCount != pComp->RefList.ItemsInList()) {
        return true;
    }
    for (BSSimpleList<REF_REFLECTED_DATA*>* pList = &pComp->RefList; pList && !pList->IsEmpty();
         pList = pList->m_pkNext) {
        REF_REFLECTED_DATA* pCompData = pList->m_item;
        REF_REFLECTED_DATA* pData = GetRefData(pCompData->pRef);
        if (!pData) {
            return true;
        }
        if (std::memcmp(pData, pCompData, sizeof(REF_REFLECTED_DATA))) {
            return true;
        }
    }
    return false;
}

ExtraReflectedRefs::~ExtraReflectedRefs() {
    for (BSSimpleList<REF_REFLECTED_DATA*>* pList = &RefList; pList && !pList->IsEmpty(); pList = pList->m_pkNext) {
        ourFree(pList->m_item);
    }
    RefList.RemoveAll();
}

void ExtraReflectedRefs::Copy(BSExtraData* apCopy) {
    ExtraReflectedRefs* pCopy = dynamic_cast<ExtraReflectedRefs*>(apCopy);
    if (pCopy) {
        ClearReferenceData();
        for (BSSimpleList<REF_REFLECTED_DATA*>* pList = &pCopy->RefList; pList && !pList->IsEmpty();
             pList = pList->m_pkNext) {
            REF_REFLECTED_DATA* pCopyData = pList->m_item;
            REF_REFLECTED_DATA* pData = new REF_REFLECTED_DATA;
            ourMemcpy(pData, pCopyData, sizeof(REF_REFLECTED_DATA));
            RefList.AddHead(pData);
        }
    }
}

void ExtraReflectedRefs::RemoveReference(TESObjectREFR* apRef) {
    BSSimpleList<REF_REFLECTED_DATA*>* pList = &RefList;
    BSSimpleList<REF_REFLECTED_DATA*>* pLastList = nullptr;
    while (pList && !pList->IsEmpty()) {
        REF_REFLECTED_DATA* pData = pList->m_item;
        if (pData->pRef == apRef) {
            if (pLastList) {
                pLastList->Remove(pData);
            }
            else {
                pList->RemoveHead();
            }
            ourFree(pData);
            pData = nullptr;
            return;
        }
        pLastList = pList;
        pList = pList->m_pkNext;
    }
}

void ExtraReflectedRefs::SetReference(TESObjectREFR* apRef, uint32_t aiEffectFlags) {
    REF_REFLECTED_DATA* pData = GetRefData(apRef);
    if (!pData) {
        pData = new REF_REFLECTED_DATA;
        pData->pRef = apRef;
        RefList.AddHead(pData);
    }
    pData->iEffectFlags = aiEffectFlags;
}

void ExtraReflectedRefs::ClearReferenceData() {
    while (!RefList.IsEmpty()) {
        RefList.RemoveHead();
    }
}

REF_REFLECTED_DATA* ExtraReflectedRefs::GetRefData(TESObjectREFR* apRef) {
    BSSimpleList<REF_REFLECTED_DATA*>* pList = &RefList;
    while (pList && !pList->IsEmpty()) {
        REF_REFLECTED_DATA* pData = pList->m_item;
        pList = pList->m_pkNext;
        if (pData->pRef == apRef) {
            return pData;
        }
    }
    return nullptr;
}

ExtraReflectorRefs::ExtraReflectorRefs() : BSExtraData(102u) {}

bool ExtraReflectorRefs::Compare(BSExtraData* apCompare) {
    ExtraReflectorRefs* pComp = dynamic_cast<ExtraReflectorRefs*>(apCompare);
    if (!pComp) {
        return true;
    }
    int32_t iCount = RefList.ItemsInList();
    if (iCount != pComp->RefList.ItemsInList()) {
        return true;
    }
    for (BSSimpleList<REF_REFLECTOR_DATA*>* pList = &pComp->RefList; pList && !pList->IsEmpty();
         pList = pList->m_pkNext) {
        REF_REFLECTOR_DATA* pCompData = pList->m_item;
        REF_REFLECTOR_DATA* pData = GetRefData(pCompData->pRef);
        if (!pData) {
            return true;
        }
        if (std::memcmp(pData, pCompData, sizeof(REF_REFLECTOR_DATA))) {
            return true;
        }
    }
    return false;
}

ExtraReflectorRefs::~ExtraReflectorRefs() {
    for (BSSimpleList<REF_REFLECTOR_DATA*>* pList = &RefList; pList && !pList->IsEmpty(); pList = pList->m_pkNext) {
        ourFree(pList->m_item);
    }
    RefList.RemoveAll();
}

void ExtraReflectorRefs::Copy(BSExtraData* apCopy) {
    ExtraReflectorRefs* pCopy = dynamic_cast<ExtraReflectorRefs*>(apCopy);
    if (pCopy) {
        ClearReferenceData();
        for (BSSimpleList<REF_REFLECTOR_DATA*>* pList = &pCopy->RefList; pList && !pList->IsEmpty();
             pList = pList->m_pkNext) {
            REF_REFLECTOR_DATA* pCopyData = pList->m_item;
            REF_REFLECTOR_DATA* pData = new REF_REFLECTOR_DATA;
            ourMemcpy(pData, pCopyData, sizeof(REF_REFLECTOR_DATA));
            RefList.AddHead(pData);
        }
    }
}

void ExtraReflectorRefs::RemoveReference(TESObjectREFR* apRef) {
    BSSimpleList<REF_REFLECTOR_DATA*>* pList = &RefList;
    BSSimpleList<REF_REFLECTOR_DATA*>* pLastList = nullptr;
    while (pList && !pList->IsEmpty()) {
        REF_REFLECTOR_DATA* pData = pList->m_item;
        if (pData->pRef == apRef) {
            if (pLastList) {
                pLastList->Remove(pData);
            }
            else {
                pList->RemoveHead();
            }
            ourFree(pData);
            return;
        }
        pLastList = pList;
        pList = pList->m_pkNext;
    }
}

void ExtraReflectorRefs::SetReference(TESObjectREFR* apRef, uint32_t aiEffectFlags) {
    REF_REFLECTOR_DATA* pData = GetRefData(apRef);
    if (!pData) {
        pData = new REF_REFLECTOR_DATA;
        pData->pRef = apRef;
        RefList.AddHead(pData);
    }
    pData->iEffectFlags = aiEffectFlags;
}

void ExtraReflectorRefs::InitItem(TESForm* apOwner) {
    BSSimpleList<REF_REFLECTOR_DATA*>* pRefList = &RefList;
    BSSimpleList<REF_REFLECTOR_DATA*>* pLastList = nullptr;
    while (pRefList && !pRefList->IsEmpty()) {
        bool bAdvanced = false;
        REF_REFLECTOR_DATA* pData = pRefList->m_item;
        uint32_t tempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pData->pRef));
        TESFile* pFile = apOwner->GetFile(-1);
        TESForm::AddCompileIndex(tempint, pFile);
        TESForm* pForm = TESForm::GetFormByNumericID(tempint);
        TESObjectREFR* pRef = dynamic_cast<TESObjectREFR*>(pForm);
        if (pRef && pRef->GetRefPersists()) {
            if ((pData->iEffectFlags & REF_REFLECTOR_DATA::FLAG_REFLECTION) != 0) {
                pRef->m_Extra.SetReflectsRef(static_cast<TESObjectREFR*>(apOwner), true);
            }
            if ((pData->iEffectFlags & REF_REFLECTOR_DATA::FLAG_REFRACTION) != 0) {
                pRef->m_Extra.SetRefractsRef(static_cast<TESObjectREFR*>(apOwner), true);
            }
            pData->pRef = pRef;
        }
        else {
            if (pRef) {
                Warning(
                    "MASTERFILE: Reference %08X is not persistent so cannot reflect or refract other references.",
                    tempint
                );
            }
            else {
                Warning("MASTERFILE: Unable to find water reflector reference %08X.", tempint);
            }
            if (pLastList) {
                pLastList->Remove(pData);
                pRefList = pLastList->m_pkNext;
            }
            else {
                pRefList->RemoveHead();
            }
            ourFree(pData);
            pData = nullptr;
            bAdvanced = true;
        }
        if (!bAdvanced) {
            pLastList = pRefList;
            pRefList = pRefList->m_pkNext;
        }
    }
}

REF_REFLECTOR_DATA* ExtraReflectorRefs::GetRefData(TESObjectREFR* apRef) {
    BSSimpleList<REF_REFLECTOR_DATA*>* pList = &RefList;
    while (pList && !pList->IsEmpty()) {
        REF_REFLECTOR_DATA* pData = pList->m_item;
        pList = pList->m_pkNext;
        if (pData->pRef == apRef) {
            return pData;
        }
    }
    return nullptr;
}

void ExtraReflectorRefs::ClearReferenceData() {
    while (!RefList.IsEmpty()) {
        RefList.RemoveHead();
    }
}

ExtraRunOncePacks::ExtraRunOncePacks() : BSExtraData(27), pPackageList(new BSSimpleList<RunOncePackage*>) {}

ExtraRunOncePacks::~ExtraRunOncePacks() {
    for (RunOncePackage*& rpItem : pPackageList->ItemsUntilNull()) {
        ourFree(rpItem);
    }
    pPackageList->RemoveAll();
    delete pPackageList;
}

void ExtraRunOncePacks::AddRunOncePack(TESPackage* apPack, char acDay) {
    RunOncePackage* pMatch = nullptr;
    RunOncePackage* pItem = nullptr;
    BSSimpleList<RunOncePackage*>* pIter = pPackageList;
    while (pIter) {
        pItem = pIter->m_item;
        if (pItem) {
            if (pItem->pPackage == apPack) {
                pMatch = pItem;
                break;
            }
            pIter = pIter->m_pkNext;
        }
        else {
            pIter = nullptr;
        }
    }
    if (!pMatch) {
        pMatch = static_cast<RunOncePackage*>(ourMalloc(sizeof(RunOncePackage)));
        pPackageList->AddHead(pMatch);
    }
    pMatch->pPackage = apPack;
    pMatch->cday = acDay;
}

ExtraScript::ExtraScript() : BSExtraData(13u), pScript(nullptr), pScriptVars(nullptr) {}

ExtraScript::ExtraScript(Script* apScript) : BSExtraData(EXTRA_SCRIPT), pScript(apScript), pScriptVars(nullptr) {}

bool ExtraScript::Compare(BSExtraData* apCompare) {
    ExtraScript* pCompare = dynamic_cast<ExtraScript*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return pScript != pCompare->pScript;
}

ExtraScript::~ExtraScript() {
    if (pScriptVars) {
        delete pScriptVars;
        pScriptVars = nullptr;
    }
}

ExtraTeleport::ExtraTeleport() : BSExtraData(EXTRA_TELEPORT), pTeleportData(nullptr) {}

ExtraTeleport::ExtraTeleport(DoorTeleportData* apTeleport) : BSExtraData(EXTRA_TELEPORT), pTeleportData(apTeleport) {}

bool ExtraTeleport::Compare(BSExtraData* apCompare) {
    ExtraTeleport* pComp = dynamic_cast<ExtraTeleport*>(apCompare);
    if (!pComp) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return pTeleportData->Compare(pComp->pTeleportData);
}

ExtraTeleport::~ExtraTeleport() {
    delete pTeleportData;
}

ExtraEditorID::ExtraEditorID(const char* apEditorID) : BSExtraData(6) {
    cEditorID.Set(apEditorID, 0u);
}

ExtraOcclusionPlane::ExtraOcclusionPlane() : BSExtraData(113), spPlane(nullptr) {}

ExtraWeaponModFlags::ExtraWeaponModFlags() : BSExtraData(EXTRA_WEAPON_MOD_SLOTS), cWeaponModsActive(0) {}

ExtraWeaponModFlags::ExtraWeaponModFlags(uint8_t acSlot)
  : BSExtraData(EXTRA_WEAPON_MOD_SLOTS), cWeaponModsActive(acSlot) {}

bool ExtraWeaponModFlags::Compare(BSExtraData* apCompare) {
    ExtraWeaponModFlags* pCompare = dynamic_cast<ExtraWeaponModFlags*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return cWeaponModsActive != pCompare->cWeaponModsActive;
}

bool ExtraWeaponModFlags::GetWeaponModActive(uint8_t slot) {
    return (cWeaponModsActive & slot) != 0;
}

bool ExtraWeaponModFlags::HasWeaponMods() {
    return cWeaponModsActive != 0;
}

void ExtraWeaponModFlags::SetWeaponModFlags(uint8_t flags) {
    cWeaponModsActive = flags;
}

void ExtraWeaponModFlags::SetWeaponModActive(const uint8_t slot) {
    cWeaponModsActive |= slot;
}

ExtraXTarget::ExtraXTarget() : BSExtraData(EXTRA_XTARGET), pTarget(nullptr) {}

bool ExtraXTarget::Compare(BSExtraData* apCompare) {
    ExtraXTarget* pCompare = dynamic_cast<ExtraXTarget*>(apCompare);
    return !pCompare || pTarget != pCompare->pTarget;
}

ExtraUsedMarkers::ExtraUsedMarkers() : BSExtraData(EXTRA_USEDMARKERS), iUsedMarkers(0) {}

void ExtraUsedMarkers::SetMarkerUsed(uint32_t aiMarkerIndex, bool abVal) {
    if (aiMarkerIndex >= 30) {
        return;
    }
    uint32_t uiMask = 1u << aiMarkerIndex;
    if (abVal) {
        iUsedMarkers |= uiMask;
    }
    else {
        iUsedMarkers &= ~uiMask;
    }
}

bool ExtraUsedMarkers::GetMarkerUsed(uint32_t aiMarkerIndex) {
    if (aiMarkerIndex < 30) {
        return ((1 << aiMarkerIndex) & iUsedMarkers) != 0;
    }
    return false;
}

ExtraSpecialRenderFlags::ExtraSpecialRenderFlags() : BSExtraData(EXTRA_SPECIAL_RENDER_FLAGS), m_flags(0) {}

bool ExtraSpecialRenderFlags::Compare(BSExtraData* apCompare) {
    ExtraSpecialRenderFlags* pCompare = dynamic_cast<ExtraSpecialRenderFlags*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    uint32_t uiFlags = GetFlags();
    return uiFlags != pCompare->GetFlags() || pCompare->m_fNodeMaxFadeDistance != m_fNodeMaxFadeDistance;
}

void ExtraSpecialRenderFlags::Copy(BSExtraData* apCopy) {
    ExtraSpecialRenderFlags* pCopy = dynamic_cast<ExtraSpecialRenderFlags*>(apCopy);
    if (pCopy) {
        m_flags = pCopy->m_flags;
        m_fNodeMaxFadeDistance = pCopy->m_fNodeMaxFadeDistance;
    }
}

ExtraStartingPosition::ExtraStartingPosition(TESObjectREFR* apObject) : BSExtraData(EXTRA_STARTINGPOSITION) {
    startPosition = apObject->GetLocationOnReference();
    startRotation = apObject->GetAngleOnReference();
}

bool ExtraStartingPosition::Compare(BSExtraData* apCompare) {
    ExtraStartingPosition* pComp = dynamic_cast<ExtraStartingPosition*>(apCompare);
    if (!pComp) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return std::memcmp(&startPosition, &pComp->startPosition, sizeof(startPosition)) != 0 ||
        std::memcmp(&startRotation, &pComp->startRotation, sizeof(startRotation)) != 0;
}

ExtraLevCreaModifier::ExtraLevCreaModifier() : BSExtraData(EXTRA_LEVCREA_MOD), eModifier(LCM_NONE) {}

bool ExtraLevCreaModifier::Compare(BSExtraData* apCompare) {
    ExtraLevCreaModifier* pCompare = dynamic_cast<ExtraLevCreaModifier*>(apCompare);
    return !pCompare || pCompare->eModifier != eModifier;
}

// The multiplier comes from one of a table of game settings.
float ExtraLevCreaModifier::GetMultiplier() {
    if (eModifier != 4) {
        return pLevCreaModMult[eModifier]->Float();
    }
    return 1.0f;
}

int32_t ExtraLevCreaModifier::GetAllBelowForce() {
    switch (eModifier) {
        case 0: return 1;
        case 4: return 0;
        default: return -1;
    }
}

RoomRefData::RoomRefData() {}

ExtraRoomRefData::ExtraRoomRefData() : BSExtraData(EXTRA_ROOM_REF_DATA) {}

ExtraRoomRefData::~ExtraRoomRefData() {
    Clear();
}

void ExtraRoomRefData::Clear() {
    if (pData) {
        delete pData;
    }
    pData = nullptr;
}

ExtraSayToTopicInfo::ExtraSayToTopicInfo()
  : BSExtraData(EXTRA_SAY_TO_TOPIC_INFO), pInfo(nullptr), pTopic(nullptr), nResponseID(0), bVoiceDone(false) {}

ExtraSayToTopicInfo::ExtraSayToTopicInfo(TESTopicInfo* apTopicInfo)
  : BSExtraData(EXTRA_SAY_TO_TOPIC_INFO), pInfo(apTopicInfo), pTopic(nullptr), nResponseID(0), bVoiceDone(false) {}

void ExtraSayToTopicInfo::SetTopic(TESTopic* apTopic) {
    pTopic = apTopic;
}

ExtraSayTopicInfoOnceADay::ExtraSayTopicInfoOnceADay()
  : BSExtraData(115), pListofSaidOnceTopicInfos(new BSSimpleList<SayOnceTopicInfos*>) {}

ExtraSayTopicInfoOnceADay::ExtraSayTopicInfoOnceADay(SayOnceTopicInfos* apInfo)
  : BSExtraData(115), pListofSaidOnceTopicInfos(new BSSimpleList<SayOnceTopicInfos*>) {
    pListofSaidOnceTopicInfos->AddHead(apInfo);
}

bool ExtraSayTopicInfoOnceADay::Compare(BSExtraData* apCompare) {
    return false;
}

ExtraSayTopicInfoOnceADay::~ExtraSayTopicInfoOnceADay() {
    BSSimpleList<SayOnceTopicInfos*>* pList = pListofSaidOnceTopicInfos;
    while (pList && pList->m_item) {
        SayOnceTopicInfos* pInfo = pList->m_item;
        pList->RemoveHead();
        ourFree(pInfo);
    }
    delete pListofSaidOnceTopicInfos;
}

ExtraWorn::ExtraWorn() : BSExtraData(EXTRA_WORN) {}

ExtraWornLeft::ExtraWornLeft() : BSExtraData(EXTRA_WORN_LEFT) {}

ExtraCannotWear::ExtraCannotWear() : BSExtraData(EXTRA_CANNOTWEAR) {}

ExtraHavok::ExtraHavok(bhkWorld* apWorld) : BSExtraData(1), spWorld(apWorld), spCellMopp(nullptr) {}

ExtraCellAcousticSpace::ExtraCellAcousticSpace(BGSAcousticSpace* apSpace)
  : BSExtraData(EXTRA_CELL_ACOUSTIC_SPACE), pSpace(apSpace) {}

ExtraCellClimate::ExtraCellClimate(TESClimate* apClimate) : BSExtraData(EXTRA_CLIMATE), pClimate(apClimate) {}

ExtraPoison::ExtraPoison() : BSExtraData(63u), pPoison(nullptr) {}

ExtraPoison::ExtraPoison(AlchemyItem* apPoison) : BSExtraData(EXTRA_POISON), pPoison(apPoison) {}

bool ExtraPoison::Compare(BSExtraData* apCompare) {
    ExtraPoison* pCompare = dynamic_cast<ExtraPoison*>(apCompare);
    return !pCompare || pCompare->pPoison != pPoison;
}

ExtraRandomTeleportMarker::ExtraRandomTeleportMarker() : BSExtraData(59u), pMarker(nullptr) {}

bool ExtraRandomTeleportMarker::Compare(BSExtraData* apCompare) {
    ExtraRandomTeleportMarker* pCompare = dynamic_cast<ExtraRandomTeleportMarker*>(apCompare);
    return !pCompare || pCompare->pMarker != pMarker;
}

OcclusionPlaneLinkedRefData::OcclusionPlaneLinkedRefData() : pLinkedRefs{} {}

ExtraOcclusionPlaneRefData::ExtraOcclusionPlaneRefData() : BSExtraData(EXTRA_OCCLUSION_PLANE_REF_DATA) {}

void ExtraOcclusionPlaneRefData::Clear() {
    if (pData) {
        ourFree(pData);
    }
    pData = nullptr;
}

PortalLinkedRefData::PortalLinkedRefData() : pLinkedRefs{nullptr, nullptr} {}

void PortalLinkedRefData::Endian() {
    ENDIAN(pLinkedRefs[0], false);
    ENDIAN(pLinkedRefs[1], false);
}

ExtraPortalRefData::ExtraPortalRefData() : BSExtraData(EXTRA_PORTAL_REF_DATA) {}

void ExtraPortalRefData::Clear() {
    if (pData) {
        ourFree(pData);
    }
    pData = nullptr;
}

REF_PATROL_DATA::REF_PATROL_DATA() {}

void REF_PATROL_DATA::Endian() {
    ENDIAN(fTimeAtRef, false);
}

float* REF_PATROL_DATA::SetTimeAtRef(float* apTimeAtRef) {
    fTimeAtRef = *apTimeAtRef;
    return &fTimeAtRef;
}

PatrolRefData::PatrolRefData() : fTimeAtRef(0.0f) {}

PatrolRefData::PatrolRefData(const REF_PATROL_DATA& aData) : fTimeAtRef(aData.fTimeAtRef) {}

void PatrolRefData::InitItem(TESForm* apOwner) {
    OnPatrol.SetType(3);
    OnPatrol.InitItem(apOwner);
}

Script* PatrolRefData::GetOnPatrolScript() {
    return OnPatrol.pResultScript;
}

PRIMITIVE_DATA::PRIMITIVE_DATA() : aColor(0.0f, 0.0f, 0.0f, 0.0f) {}

void PRIMITIVE_DATA::Endian() {
    ENDIAN(aRadii.x, false);
    ENDIAN(aRadii.y, false);
    ENDIAN(aRadii.z, false);
    ENDIAN(aColor.r, false);
    ENDIAN(aColor.g, false);
    ENDIAN(aColor.b, false);
    ENDIAN(aColor.a, false);
    ENDIAN(eType, false);
}

ExtraSound::ExtraSound(BSSoundHandle aSound) : BSExtraData(79) {
    pHandle = aSound;
}

ExtraCreatureAwakeSound::ExtraCreatureAwakeSound(BSSoundHandle aSound) : BSExtraData(125) {
    pHandle = aSound;
}

ExtraSavedAnimation::ExtraSavedAnimation() : BSExtraData(EXTRA_SAVED_ANIMATION), pAnimationBuffer(nullptr) {}

ExtraSavedAnimation::~ExtraSavedAnimation() {
    DeleteBuffer();
}

void ExtraSavedAnimation::DeleteBuffer() {
    BGSLoadGameSubBuffer kSubBuffer(pAnimationBuffer);
    kSubBuffer.DeleteBuffer();
}

ExtraSavedHavokData::ExtraSavedHavokData() : BSExtraData(EXTRA_SAVED_HAVOK_DATA), pHavokBuffer(nullptr) {}

ExtraSavedHavokData::~ExtraSavedHavokData() {
    DeleteBuffer();
}

void ExtraSavedHavokData::DeleteBuffer() {
    BGSLoadGameSubBuffer kSubBuffer(pHavokBuffer);
    kSubBuffer.DeleteBuffer();
}

ExtraWaterZoneMap::ExtraWaterZoneMap() : BSExtraData(0x7Eu), WaterZoneMap(0x25u), pHighestWaterZone(nullptr) {}

void ExtraWaterZoneMap::SetInWaterZone(WaterZone* apZone, bool abValue) {
    float fHighestWaterHeight = -std::numeric_limits<float>::max();
    if (abValue) {
        int32_t iCount = 0;
        WaterZoneMap.GetAt(apZone, iCount);
        WaterZoneMap.SetAt(apZone, 1);
        if (fHighestWaterHeight < static_cast<double>(apZone->fWaterHeight)) {
            fHighestWaterHeight = apZone->fWaterHeight;
            pHighestWaterZone = apZone;
        }
    }
    else {
        int32_t iCount = 0;
        WaterZoneMap.GetAt(apZone, iCount);
        if (iCount > 1) {
            WaterZoneMap.SetAt(apZone, iCount - 1);
        }
        else {
            WaterZoneMap.RemoveAt(apZone);
            NiTMapItem<WaterZone*, int32_t>* pPos = WaterZoneMap.GetFirstPos();
            fHighestWaterHeight = -std::numeric_limits<float>::max();
            while (pPos) {
                int32_t iZoneCount;
                WaterZone* pZone;
                WaterZoneMap.GetNext(pPos, pZone, iZoneCount);
                if (fHighestWaterHeight < static_cast<double>(pZone->fWaterHeight)) {
                    fHighestWaterHeight = pZone->fWaterHeight;
                    pHighestWaterZone = pZone;
                }
            }
        }
    }
}

LightRefData::LightRefData() : spLight(nullptr) {}

bool LightRefData::IsEqual(NiPointLight** apLight) {
    return static_cast<const void*>(spLight.m_pObject) == static_cast<const void*>(*apLight);
}

// The name is copied through a BaseFormComponent pointer so that CopyComponent is a virtual call; a call on the member
// itself would be devirtualised.
void MapMarkerData::Copy(MapMarkerData* apCopy) {
    BaseFormComponent* pName = &LocationName;
    pName->CopyComponent(&apCopy->LocationName);
    cFlags = apCopy->cFlags;
    sType = apCopy->sType;
    pReputation = apCopy->pReputation;
}

bool MapMarkerData::GetVisible() {
    return (cFlags & FLAG_VISIBLE) != 0;
}

bool MapMarkerData::GetTravelLoc() {
    return (cFlags & FLAG_CAN_TRAVEL_TO) != 0;
}

void MapMarkerData::SetTravelLoc(bool abVal) {
    if (abVal) {
        cFlags |= FLAG_CAN_TRAVEL_TO;
    }
    else {
        cFlags &= ~FLAG_CAN_TRAVEL_TO;
    }
}

TESReputation* MapMarkerData::GetReputation() {
    return pReputation;
}

void MapMarkerData::SetReputation(TESReputation* pRep) {
    pReputation = pRep;
}

int32_t MapMarkerData::GetType() {
    return sType;
}

void MapMarkerData::ResetFlags() {
    cFlags = cOriginalFlags;
}

void MapMarkerData::SetIsVisible(uint8_t acVisible) {
    if (acVisible) {
        cFlags |= FLAG_VISIBLE;
    }
    else {
        cFlags &= ~FLAG_VISIBLE;
    }
}

AudioMarker::AudioMarker()
  : fLayer2TriggerPerc(0.33000001311302185f), fLayer3TriggerPerc(0.6700000166893005f), Location(NiPoint3::ZERO),
    pMediaLocationController(nullptr) {}

MediaLocationController* AudioMarker::GetLocationControllerID() {
    return pMediaLocationController;
}

void AudioMarker::Copy(AudioMarker* apCopy) {
    kName.CopyComponent(&apCopy->kName);
    pMediaLocationController = apCopy->pMediaLocationController;
    bUseControllerValues = apCopy->bUseControllerValues;
    fLayer2TriggerPerc = apCopy->fLayer2TriggerPerc;
    fLayer3TriggerPerc = apCopy->fLayer3TriggerPerc;
}

bool AudioMarker::Load(TESFile* apFile) {
    if (!apFile) {
        return false;
    }
    if (apFile->GetTESChunk() != MMRK_ID) {
        return false;
    }
    uint32_t arData = 0;
    if (!apFile->NextChunk()) {
        return false;
    }
    if (apFile->GetTESChunk() != FULL_ID) {
        return false;
    }
    TESFullName::LoadFullNameChunk(&kName, apFile);
    if (!apFile->NextChunk()) {
        return false;
    }
    if (apFile->GetTESChunk() != CNAM_ID) {
        return false;
    }
    apFile->GetChunkData<uint32_t>(arData);
    pMediaLocationController = reinterpret_cast<MediaLocationController*>(static_cast<uintptr_t>(arData));
    if (!apFile->NextChunk()) {
        return false;
    }
    if (apFile->GetTESChunk() != BNAM_ID) {
        return false;
    }
    apFile->GetChunkData<uint32_t>(arData);
    bUseControllerValues = arData == 1;
    if (!apFile->NextChunk()) {
        return false;
    }
    if (apFile->GetTESChunk() != MNAM_ID) {
        return false;
    }
    apFile->GetChunkData<uint32_t>(*reinterpret_cast<uint32_t*>(&fLayer2TriggerPerc));
    if (!apFile->NextChunk()) {
        return false;
    }
    if (apFile->GetTESChunk() != NNAM_ID) {
        return false;
    }
    apFile->GetChunkData<uint32_t>(*reinterpret_cast<uint32_t*>(&fLayer3TriggerPerc));
    return true;
}

bool AudioMarker::Compare(AudioMarker* apCompare) {
    if (kName.CompareComponent(&apCompare->kName)) {
        return true;
    }
    if (pMediaLocationController != apCompare->pMediaLocationController) {
        return true;
    }
    if (bUseControllerValues != apCompare->bUseControllerValues) {
        return true;
    }
    if (apCompare->fLayer2TriggerPerc == fLayer2TriggerPerc) {
        return apCompare->fLayer3TriggerPerc != fLayer3TriggerPerc;
    }
    return true;
}

RagDollData::RagDollData() : iBoneCount(0), pBoneArray(nullptr), Bip01Rot(NiPoint3::ZERO) {}

void RagDollData::CreateRotation(TESObjectREFR* apRef) {
    NiNode* p3D = apRef && apRef->IsActor() ? apRef->Get3D() : nullptr;
    NiNode* pNode = p3D ? p3D->IsNode() : nullptr;
    if (pNode) {
        NiAVObject* pBip01 = pNode->GetAtChecked(0);
        if (pBip01) {
            pBip01->GetLocalTransform().m_Rotate.ToEulerAnglesXYZ(Bip01Rot.x, Bip01Rot.y, Bip01Rot.z);
        }
    }
}

ExtraDismemberedLimbs::ExtraDismemberedLimbs()
  : BSExtraData(95), sLimbs(0), eCauseofDeath(CAUSE_OF_DEATH_NONE), pDeathObject(nullptr),
    eLastHitLimb(BGSBodyPart::LIMB_NONE), bEaten(false) {}

ExtraDismemberedLimbs::~ExtraDismemberedLimbs() {
    for (DismemberedLimb* pDismemberedLimb : DismemberedLimbs) {
        delete pDismemberedLimb;
    }
}

void ExtraDismemberedLimbs::RemoveLimb(BGSBodyPart::LIMB_ENUM aeLimb, bool abRemoved) {
    for (DismemberedLimb* pDismemberedLimb : DismemberedLimbs) {
        if (pDismemberedLimb && pDismemberedLimb->cLimb == aeLimb) {
            pDismemberedLimb->bLimbRemoved = abRemoved;
        }
    }
}

DismemberedLimb* ExtraDismemberedLimbs::GetDismemberedLimb(uint32_t aiIndex) {
    return *static_cast<DismemberedLimb**>(&DismemberedLimbs[aiIndex]);
}

uint32_t ExtraDismemberedLimbs::GetDismemberedLimbCount() {
    return DismemberedLimbs.BSSimpleArray<DismemberedLimb*>::GetSize();
}

bool ExtraDismemberedLimbs::Dismembered(BGSBodyPart::LIMB_ENUM aeLimb) {
    return (sLimbs & (1 << aeLimb)) != 0;
}

void ExtraDismemberedLimbs::Dismember(TESObjectREFR* apRef, BGSBodyPart::LIMB_ENUM aeLimb, bool abExploded) {
    if (!Dismembered(aeLimb)) {
        sLimbs |= 1 << aeLimb;
        DismemberedLimb* pDismemberedLimb = new DismemberedLimb;
        uint32_t uiIndex = DismemberedLimbs.Add(pDismemberedLimb);
        pDismemberedLimb->cLimb = static_cast<uint8_t>(aeLimb);
        pDismemberedLimb->bLimbExploded = abExploded;
        pDismemberedLimb->bObjectArrayIdentical = false;
        if (abExploded) {
            pDismemberedLimb->bObjectArrayIdentical = true;
        }
        else if (apRef->IsActor() && apRef != pPlayer && apRef->IsNpc()) {
            TESNPC* pNPC = static_cast<TESNPC*>(static_cast<Actor*>(apRef)->GetActorBase());
            pNPC->BuildObjectArray(apRef, apRef->GetBiped(), &pDismemberedLimb->ObjectArray);
            if (uiIndex) {
                for (int32_t i = uiIndex - 1; i >= 0; --i) {
                    DismemberedLimb* pPrevDismemberedLimb = DismemberedLimbs[i];
                    if (!pPrevDismemberedLimb->bObjectArrayIdentical) {
                        if (pPrevDismemberedLimb->ObjectArray.CompareArrays(pDismemberedLimb->ObjectArray)) {
                            pDismemberedLimb->bObjectArrayIdentical = true;
                            pDismemberedLimb->ObjectArray.Clear(true);
                        }
                        return;
                    }
                }
            }
        }
    }
}

bool ExtraDismemberedLimbs::CheckDismemberedLimbsIdentical(ExtraDismemberedLimbs* apDismemberedLimbs) {
    if (apDismemberedLimbs->sLimbs != sLimbs) {
        return false;
    }
    uint32_t uiSize = apDismemberedLimbs->DismemberedLimbs.iSize;
    if (uiSize != DismemberedLimbs.iSize) {
        return false;
    }
    for (uint32_t i = 0; i < DismemberedLimbs.iSize; ++i) {
        DismemberedLimb* pDismemberedLimb1 = DismemberedLimbs[i];
        DismemberedLimb* pDismemberedLimb2 = apDismemberedLimbs->DismemberedLimbs[i];
        if (pDismemberedLimb1->cLimb != pDismemberedLimb2->cLimb) {
            return false;
        }
        if (pDismemberedLimb1->bLimbExploded != pDismemberedLimb2->bLimbExploded) {
            return false;
        }
        if (pDismemberedLimb1->bObjectArrayIdentical != pDismemberedLimb2->bObjectArrayIdentical) {
            return false;
        }
        if (pDismemberedLimb1->bLimbRemoved != pDismemberedLimb2->bLimbRemoved) {
            return false;
        }
        if (!pDismemberedLimb1->ObjectArray.CompareArrays(pDismemberedLimb2->ObjectArray)) {
            return false;
        }
    }
    return true;
}

ExtraFactionChanges::ExtraFactionChanges() : BSExtraData(94), pFactionChanges(new BSSimpleList<FACTION_RANK*>) {}

ExtraFactionChanges::~ExtraFactionChanges() {
    pFactionChanges->RemoveAll();
    delete pFactionChanges;
}

void ExtraFactionChanges::ExpelFromFaction(TESFaction* apFaction) {
    for (FACTION_RANK* pFacRank : pFactionChanges->ItemsUntilNull()) {
        if (apFaction == pFacRank->pFaction) {
            pFacRank->cRank = -1;
            return;
        }
    }
    if (!pFactionChanges) {
        pFactionChanges = new BSSimpleList<FACTION_RANK*>;
    }
    FACTION_RANK* pFacRank = static_cast<FACTION_RANK*>(ourMalloc(sizeof(FACTION_RANK)));
    pFacRank->pFaction = apFaction;
    pFacRank->cRank = -1;
    pFactionChanges->AddHead(pFacRank);
}

// cRank is sign-extended, so the compare against -1 is signed.
bool ExtraFactionChanges::GetIsInFaction(TESFaction* apFaction) {
    for (BSSimpleList<FACTION_RANK*>* pIter = pFactionChanges; pIter && pIter->GetItem(); pIter = pIter->GetNext()) {
        FACTION_RANK* pRank = pIter->GetItem();
        if (apFaction == pRank->pFaction && static_cast<int8_t>(pRank->cRank) > -1) {
            return true;
        }
    }
    return false;
}

// cRank is sign-extended, so the compare against -1 is signed.
bool ExtraFactionChanges::GetIsExpelled(TESFaction* apFaction) {
    for (BSSimpleList<FACTION_RANK*>* pIter = pFactionChanges; pIter && pIter->GetItem(); pIter = pIter->GetNext()) {
        FACTION_RANK* pRank = pIter->GetItem();
        if (apFaction == pRank->pFaction && static_cast<int8_t>(pRank->cRank) == -1) {
            return true;
        }
    }
    return false;
}

void ExtraFactionChanges::AddToFaction(TESFaction* apFaction, int32_t aiRank) {
    for (FACTION_RANK* pFacRank : pFactionChanges->ItemsUntilNull()) {
        if (apFaction == pFacRank->pFaction) {
            pFacRank->cRank = aiRank;
            return;
        }
    }
    FACTION_RANK* pNewRank = static_cast<FACTION_RANK*>(ourMalloc(sizeof(FACTION_RANK)));
    pNewRank->cRank = aiRank;
    pNewRank->pFaction = apFaction;
    pFactionChanges->AddHead(pNewRank);
}

void ExtraFactionChanges::ExpelFromAllFactions(TESActorBase* apActorBase) {
    bool bExpel;
    using expelledmap_t = BSMap<TESFaction*, bool>;
    expelledmap_t expelledmap(37u);
    BSSimpleList<FACTION_RANK*>* pList = pFactionChanges;
    while (pList && pList->m_item) {
        FACTION_RANK* pRankData = pList->m_item;
        pList = pList->m_pkNext;
        pRankData->cRank = -1;
        expelledmap.SetAt(pRankData->pFaction, true);
    }
    if (apActorBase) {
        pList = &apActorBase->listFactions;
        while (pList && pList->m_item) {
            FACTION_RANK* pRankData = pList->m_item;
            pList = pList->m_pkNext;
            bExpel = false;
            if (!expelledmap.GetAt(pRankData->pFaction, bExpel) || !bExpel) {
                FACTION_RANK* pRank = static_cast<FACTION_RANK*>(ourMalloc(sizeof(FACTION_RANK)));
                pRank->pFaction = pRankData->pFaction;
                pRank->cRank = -1;
                pFactionChanges->AddHead(pRank);
            }
        }
    }
}

void ExtraFactionChanges::SetFactionRank(TESFaction* apFaction, int32_t aiRank) {
    for (FACTION_RANK* pFacRank : pFactionChanges->ItemsUntilNull()) {
        if (apFaction == pFacRank->pFaction) {
            pFacRank->cRank = aiRank;
            return;
        }
    }
}

ExtraRefractionProperty::ExtraRefractionProperty(float afRefractionPower)
  : BSExtraData(EXTRA_REFRACTION_PROPERTY), fRefractionPower(afRefractionPower) {}

ExtraActorCause::ExtraActorCause() : BSExtraData(0x60u), spActorCause(nullptr) {}

void ExtraActorCause::SetActorCause(ActorCause* apCause) {
    spActorCause = apCause;
}

ExtraAudioBuoyMarker::ExtraAudioBuoyMarker(AudioBuoyMarkerData* apAudioBuoyData)
  : BSExtraData(145u), pAudioBuoyData(apAudioBuoyData) {}

// Differs when apCompare is not a buoy marker, when the base data differs, or when the audio data differs.
bool ExtraAudioBuoyMarker::Compare(BSExtraData* apCompare) {
    ExtraAudioBuoyMarker* pCompare = dynamic_cast<ExtraAudioBuoyMarker*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return pAudioBuoyData->Compare(pCompare->pAudioBuoyData);
}

ExtraAudioBuoyMarker::~ExtraAudioBuoyMarker() {
    delete pAudioBuoyData;
}

ExtraCharge::ExtraCharge() : BSExtraData(40), fCharge(0.0f) {}

ExtraCharge::ExtraCharge(float afCharge) : BSExtraData(EXTRA_CHARGE), fCharge(afCharge) {}

bool ExtraCharge::Compare(BSExtraData* apCompare) {
    ExtraCharge* pCompare = dynamic_cast<ExtraCharge*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return pCompare->fCharge != fCharge;
}

ExtraFollowerSwimBreadcrumb::ExtraFollowerSwimBreadcrumb() {}

ExtraFollowerSwimBreadcrumbs::ExtraFollowerSwimBreadcrumbs()
  : BSExtraData(0x8Bu), eLeaderState(BREADCRUMB_STATE_INVALID), LeaderLocation(NiPoint3::ZERO), LeaderNavMeshID(0) {}

ExtraFollowerSwimBreadcrumbs::~ExtraFollowerSwimBreadcrumbs() {
    for (BSSimpleList<ExtraFollowerSwimBreadcrumb*>* pList = &CrumbList; pList; pList = pList->m_pkNext) {
        delete pList->m_item;
        pList->m_item = nullptr;
    }
}

const ExtraFollowerSwimBreadcrumb* ExtraFollowerSwimBreadcrumbs::GetFirstCrumb() {
    return CrumbList.GetItem();
}

void ExtraFollowerSwimBreadcrumbs::RemoveFirstCrumb() {
    ExtraFollowerSwimBreadcrumb* pCrumb = CrumbList.m_item;
    if (pCrumb) {
        delete pCrumb;
    }
    ExtraFollowerSwimBreadcrumb* pNoCrumb = nullptr;
    CrumbList.SetItem(pNoCrumb);
    CrumbList.RemoveHead();
}

bool ExtraFollowerSwimBreadcrumbs::UpdateLeaderInfo(
    BREADCRUMB_STATE aeLeaderState,
    NiPoint3 aLeaderLocation,
    uint32_t aLeaderNavMeshID
) {
    if (aeLeaderState == BREADCRUMB_STATE_INVALID) {
        return false;
    }
    bool bAddedCrumb = false;
    if (aeLeaderState != eLeaderState && eLeaderState) {
        ExtraFollowerSwimBreadcrumb* pCrumb = new ExtraFollowerSwimBreadcrumb;
        pCrumb->StartLocation = LeaderLocation;
        pCrumb->StartNavMeshID = LeaderNavMeshID;
        pCrumb->EndLocation = aLeaderLocation;
        pCrumb->EndNavMeshID = aLeaderNavMeshID;
        pCrumb->bEnteringWater = aeLeaderState == BREADCRUMB_STATE_IN_WATER;
        CrumbList.AddTail(pCrumb);
        bAddedCrumb = true;
    }

    LeaderLocation = aLeaderLocation;
    LeaderNavMeshID = aLeaderNavMeshID;
    eLeaderState = aeLeaderState;
    return bAddedCrumb;
}

ExtraFriendHits::ExtraFriendHits() : BSExtraData(69u) {}

ExtraFriendHits::~ExtraFriendHits() {}

int32_t ExtraFriendHits::GetHitCount() {
    RemoveOldHits();
    return Hits.iSize;
}

// Records a hit now unless the last one is younger than fFriendMinimumLastHitTime, so that repeated friendly fire
// inside that interval counts once. The elapsed time is kept in a double, so the comparison is a double one.
void ExtraFriendHits::AddHit() {
    RemoveOldHits();
    int32_t iCount = static_cast<int32_t>(Hits.iSize);
    if (iCount > 0) {
        CombatTimeStamp LastHit = Hits[iCount - 1];
        double dElapsedTime = LastHit.ElapsedTime();
        // Only a strictly older last hit lets a new one in; NaN counts as too recent.
        if (fFriendMinimumLastHitTime.Float() > dElapsedTime) {
            return;
        }
    }
    float fTimer = CombatTimer::GetTimer();
    CombatTimeStamp NewHit(fTimer);
    Hits.Add(NewHit);
}

// Drops the hits older than fFriendHitTimer, keeping the younger ones in order. The count is read again on every
// iteration.
void ExtraFriendHits::RemoveOldHits() {
    for (uint32_t uiIndex = 0; uiIndex < Hits.iSize; ++uiIndex) {
        CombatTimeStamp Hit = Hits[uiIndex];
        double dElapsedTime = Hit.ElapsedTime();
        // A hit goes only once the timer's age has passed it; an equal age or NaN keeps it.
        if (!(fFriendHitTimer.Float() < dElapsedTime)) {
            continue;
        }
        Hits.RemoveFast(uiIndex, 1);
        --uiIndex;
    }
}

ExtraHasNoRumors::ExtraHasNoRumors() : BSExtraData(78), bNoRumors(false) {}

ExtraHasNoRumors::ExtraHasNoRumors(bool abNoRumors) : BSExtraData(EXTRA_NO_RUMORS), bNoRumors(abNoRumors) {}

ExtraHasNoRumors::~ExtraHasNoRumors() {}

ExtraHotKey::ExtraHotKey(char acHotKey) : BSExtraData(EXTRA_HOT_KEY), chotkey(acHotKey) {}

bool ExtraHotKey::Compare(BSExtraData* apCompare) {
    ExtraHotKey* pCompare = dynamic_cast<ExtraHotKey*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return chotkey != pCompare->chotkey;
}

ExtraModdingItem::ExtraModdingItem(bool abFlag) : BSExtraData(EXTRA_WEAPON_IS_MODDING), bIsModding(abFlag) {}

bool ExtraModdingItem::Compare(BSExtraData* apCompare) {
    char* pComp = reinterpret_cast<char*>(dynamic_cast<ExtraModdingItem*>(apCompare));
    if (!pComp) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return bIsModding != pComp[12];
}

ExtraObjectHealth::ExtraObjectHealth() : BSExtraData(86), fHealth(0.0f) {}

ExtraObjectHealth::ExtraObjectHealth(float afHealth) : BSExtraData(EXTRA_OBJECT_HEALTH), fHealth(afHealth) {}

ExtraOriginalReference::ExtraOriginalReference(TESObjectREFR* apReference)
  : BSExtraData(EXTRA_ORIGINAL_REFERENCE), pReference(apReference) {}

bool ExtraOriginalReference::Compare(BSExtraData* apCompare) {
    ExtraOriginalReference* pCompare = dynamic_cast<ExtraOriginalReference*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return pReference != pCompare->pReference;
}

ExtraPackageData::ExtraPackageData() : BSExtraData(112u), pActorPackageData(nullptr) {}

ExtraPackageData::ExtraPackageData(ActorPackageData* apActorPackageData)
  : BSExtraData(EXTRA_PACKAGE_DATA), pActorPackageData(apActorPackageData) {}

ExtraPatrolRefInUseData::ExtraPatrolRefInUseData(TESObjectREFR* apUser)
  : BSExtraData(EXTRA_PATROL_REF_IN_USE_DATA), User(apUser ? apUser->TESForm::GetFormID() : 0) {}

ExtraPatrolRefInUseData::~ExtraPatrolRefInUseData() {}

bool ExtraPatrolRefInUseData::Compare(BSExtraData* apCompare) {
    return User != static_cast<ExtraPatrolRefInUseData*>(apCompare)->User;
}

bool ExtraPatrolRefInUseData::GetCanBeUsedBy(const TESObjectREFR* apPotentialUser) {
    if (apPotentialUser && (apPotentialUser->TESForm::GetFormID() == User)) {
        return true;
    }
    Actor* pUser = static_cast<Actor*>(TESForm::GetFormByNumericID(User));
    if (!pUser || pUser->IsDead(false) || pUser->GetDisabledOrDeleted()) {
        User = 0;
        return true;
    }

    if (pUser->IsActor() &&
        (pUser->IsRestrained() ||
         pUser->IsUnConscious() ||
         (pUser->GetCurrentProcessType() && !pUser->GetRunsInLow()))) {
        User = 0;
        return true;
    }

    return false;
}

ExtraScale::ExtraScale(float afScale) : BSExtraData(EXTRA_SCALE), fScale(afScale) {}

bool ExtraScale::Compare(BSExtraData* apCompare) {
    ExtraScale* pCompare = dynamic_cast<ExtraScale*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return pCompare->fScale != fScale;
}

void ExtraSecuritronFace::SetPersonality(const char* asPersonality) {
    msPersonality = asPersonality;
}

void ExtraSecuritronFace::SetMood(const char* asMood) {
    msMood = asMood;
}

void ExtraSecuritronFace::ApplyFace(NiAVObject* apMesh) {
    if (apMesh) {
        NiAVObject* pFace = apMesh->GetObjectByName("Screen01:0");
        if (pFace) {
            NiPointer<NiSourceTexture> TexPtr(static_cast<NiSourceTexture*>(nullptr));
            BSStringT<char> kPersonality = GetPersonality();
            BSStringT<char> kMood = GetMood();
            if (kPersonality.GetLength() && kMood.GetLength()) {
                char cTexturePath[268];
                char cPath[268];
                BSsprintf(
                    cTexturePath,
                    260,
                    "Textures\\creatures\\Securitron\\%s_%s.dds",
                    kPersonality.String(),
                    kMood.String()
                );
                SwapPlatformLanguageTexturePath(cTexturePath, cPath, 260u);
                TES::pInstance->CreateTextureImage(cPath, TexPtr, false, false);
            }

            int32_t iType = NiShadeProperty::GetType();
            BSShaderNoLightingProperty* pFaceShader = static_cast<BSShaderNoLightingProperty*>(
                pFace->GetProperty(iType)
            );
            if (pFaceShader) {
                pFaceShader->SetBaseMap(TexPtr.m_pObject);
            }
        }
    }
}

ExtraCreatureMovementSound::ExtraCreatureMovementSound(BSSoundHandle aHandle)
  : BSExtraData(EXTRA_CREATURE_MOVEMENT_SOUND) {
    pHandle = aHandle;
}

ExtraWeaponIdleSound::ExtraWeaponIdleSound(BSSoundHandle aHandle) : BSExtraData(EXTRA_WEAPON_IDLE_SOUND) {
    pHandle = aHandle;
}

ExtraActivateLoopSound::ExtraActivateLoopSound(BSSoundHandle aHandle) : BSExtraData(EXTRA_ACTIVATE_LOOP_SOUND) {
    pHandle = aHandle;
}

ExtraTalkingActor::ExtraTalkingActor() : BSExtraData(EXTRA_TALKING_ACTOR), pTalkObject(nullptr) {}

ExtraTalkingActor::ExtraTalkingActor(BGSTalkingActivator* apBaseForm, TESObjectREFR* apActor)
  : BSExtraData(EXTRA_TALKING_ACTOR) {
    pTalkObject = CreateLocalMobileObjectCopy(apBaseForm, apActor);
    // The stand-in is put where the actor is.
    pTalkObject->SetLocationOnReference(apActor->GetLocationOnReference());
}

MobileObject* ExtraTalkingActor::CreateLocalMobileObjectCopy(BGSTalkingActivator* apTalkact, TESObjectREFR* apRef) {
    MobileObject* pNewActor = nullptr;
    if (apRef) {
        TESObjectCELL* pSaveCell;
        pNewActor = new MobileObject;
        pNewActor->SetObjectReference(apTalkact);
        pNewActor->SetRefPersists(apRef->GetRefPersists());
        pNewActor->SetLocationOnReference(*apRef->QLocationOnReference());
        pNewActor->SetAngleOnReference(apRef->GetAngleOnReference());
        pNewActor->SetTalkingActivatorRef(apRef);
        pNewActor->SetTalkingActivator(true);
        pNewActor->UpdateTopicInfoSayOnce();
        if ((apRef->GetParentCell() && apRef->GetParentCell()->GetInterior()) ||
            (pSaveCell = apRef->GetSaveParentCell()) == nullptr) {
            pNewActor->SetParentCell(apRef->GetParentCell());
        }
        else {
            pNewActor->m_Extra.SetPersistentCell(pSaveCell);
        }
        BaseProcess* pProc = pPlayer->pCurrentProcess;
        if (pProc) {
            // A new process of the player's process level.
            BaseProcess* pCopyProc = nullptr;
            switch (pProc->eLevel) {
                case PROCESS_TYPE_HIGH: {
                    pCopyProc = new HighProcess;
                    break;
                }
                case PROCESS_TYPE_MIDDLE_HIGH: {
                    pCopyProc = new MiddleHighProcess;
                    break;
                }
                case PROCESS_TYPE_MIDDLE_LOW: {
                    pCopyProc = new MiddleLowProcess;
                    break;
                }
                case PROCESS_TYPE_LOW: {
                    pCopyProc = new LowProcess;
                    break;
                }
                default: {
                    break;
                }
            }
            pNewActor->SetCurrentAIProcess(pCopyProc);
            ReferenceProcessLists.AddReference(pNewActor, pCopyProc->eLevel, false, false, nullptr);
        }
    }
    return pNewActor;
}

ExtraTerminalState::ExtraTerminalState() : BSExtraData(EXTRA_TERMINALSTATE), cState(0), cLockOverride(0xFF) {}

ExtraTerminalState::~ExtraTerminalState() {}

bool ExtraTerminalState::IsLockedState() {
    return !cState && static_cast<int8_t>(cLockOverride) == -1;
}

ExtraTimeLeft::ExtraTimeLeft(float afTime) : BSExtraData(EXTRA_TIMELEFT), fTime(afTime) {}

bool ExtraTimeLeft::Compare(BSExtraData* apCompare) {
    ExtraTimeLeft* pCompare = dynamic_cast<ExtraTimeLeft*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return pCompare->fTime != fTime;
}

ExtraTresPassPackage::ExtraTresPassPackage(TrespassPackage* apPack)
  : BSExtraData(EXTRA_TRESPASS_PACKAGE), pPack(apPack) {}

ExtraTresPassPackage::~ExtraTresPassPackage() {
    if (pPack) {
        pPack->SetIsCreated(true);
        delete pPack;
    }
}

ExtraUses::ExtraUses() : BSExtraData(38u), cUses(0) {}

ExtraUses::ExtraUses(uint8_t acUses) : BSExtraData(EXTRA_USES), cUses(acUses) {}

ExtraUses::~ExtraUses() {}

bool ExtraUses::Compare(BSExtraData* apCompare) {
    ExtraUses* pCompare = dynamic_cast<ExtraUses*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return cUses != pCompare->cUses;
}

ExtraHeadingTarget::ExtraHeadingTarget() : BSExtraData(EXTRA_HEAD_TRACK_TARGET), pHeadingTarget(nullptr) {}

ExtraHeadingTarget::ExtraHeadingTarget(TESObjectREFR* apHeadingTarget)
  : BSExtraData(EXTRA_HEAD_TRACK_TARGET), pHeadingTarget(apHeadingTarget) {}

ExtraHeadingTarget::~ExtraHeadingTarget() {
    pHeadingTarget = nullptr;
}

ExtraSeed::ExtraSeed(uint8_t aiSeed) : BSExtraData(EXTRA_SEED), iSeed(aiSeed) {}

ExtraSeed::~ExtraSeed() {}

bool ExtraSeed::Compare(BSExtraData* apCompare) {
    ExtraSeed* pCompare = dynamic_cast<ExtraSeed*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return iSeed != pCompare->iSeed;
}

ExtraInfoGeneralTopic::ExtraInfoGeneralTopic() : BSExtraData(EXTRA_INFO_GENERAL_TOPIC), pInfoGen(new MenuTopic) {}

ExtraInfoGeneralTopic::ExtraInfoGeneralTopic(MenuTopic* apItem) : BSExtraData(77u), pInfoGen(apItem) {}

ExtraInfoGeneralTopic::~ExtraInfoGeneralTopic() {
    pInfoGen->bGeneralTopic = false;
    delete pInfoGen;
}

ExtraDistantData::ExtraDistantData() : BSExtraData(19u), kNormal(0.0f, 0.0f, 0.97f) {}

ExtraRegionList::ExtraRegionList(TESRegionList* apList) : BSExtraData(4u), pList(apList) {}

ExtraWaterLightRefs::ExtraWaterLightRefs() : BSExtraData(132u) {}

bool ExtraWaterLightRefs::Compare(BSExtraData* apCompare) {
    ExtraWaterLightRefs* pComp = dynamic_cast<ExtraWaterLightRefs*>(apCompare);
    if (!pComp) {
        return true;
    }
    int32_t iCount = RefList.ItemsInList();
    if (iCount != pComp->RefList.ItemsInList()) {
        return true;
    }
    for (BSSimpleList<TESObjectREFR*>* pList = &pComp->RefList; pList && !pList->IsEmpty(); pList = pList->m_pkNext) {
        if (!RefList.IsInList(pList->m_item)) {
            return true;
        }
    }
    return false;
}

ExtraWaterLightRefs::~ExtraWaterLightRefs() {
    RefList.RemoveAll();
}

void ExtraWaterLightRefs::Copy(BSExtraData* apCopy) {
    ExtraWaterLightRefs* pCopy = dynamic_cast<ExtraWaterLightRefs*>(apCopy);
    if (pCopy) {
        ClearReferenceData();
        for (BSSimpleList<TESObjectREFR*>* pList = &pCopy->RefList; pList && !pList->IsEmpty();
             pList = pList->m_pkNext) {
            AddReference(pList->m_item);
        }
    }
}

void ExtraWaterLightRefs::AddReference(TESObjectREFR* apRef) {
    if (!RefList.IsInList(apRef)) {
        RefList.AddHead(apRef);
    }
}

void ExtraWaterLightRefs::ClearReferenceData() {
    RefList.RemoveAll();
}

ExtraMultiBound::ExtraMultiBound() : BSExtraData(97), spBound(nullptr) {}

ExtraMultiBound::~ExtraMultiBound() {}

ExtraRoom::ExtraRoom() : BSExtraData(121), spRoom(nullptr) {}

ExtraRoom::~ExtraRoom() {}

ExtraItemDropper::ExtraItemDropper() : BSExtraData(0x39u), pDropper(nullptr) {}

ExtraItemDropper::~ExtraItemDropper() {}

ExtraGhost::ExtraGhost() : BSExtraData(EXTRA_GHOST) {}

ExtraGhost::~ExtraGhost() {}

ExtraStartingWorldOrCell::ExtraStartingWorldOrCell()
  : BSExtraData(EXTRA_STARTINGWORLDORCELL), pStartingWorldOrCell(nullptr) {}

ExtraStartingWorldOrCell::~ExtraStartingWorldOrCell() {}

ExtraOpenCloseActivateRef::ExtraOpenCloseActivateRef()
  : BSExtraData(EXTRA_OPENCLOSEACTIVATE_REF), pActivateRef(nullptr) {}

ExtraOpenCloseActivateRef::~ExtraOpenCloseActivateRef() {}

ExtraAshPileRef::ExtraAshPileRef() : BSExtraData(EXTRA_ASHPILE_REF), pAshPileRef(nullptr) {}

ExtraAshPileRef::~ExtraAshPileRef() {}

ExtraWeaponAttackSound::ExtraWeaponAttackSound(BSSoundHandle apSound) : BSExtraData(134) {
    pHandle = apSound;
}

ExtraWeaponAttackSound::~ExtraWeaponAttackSound() {}

ExtraEditorRefMoveData::ExtraEditorRefMoveData(TESObjectREFR* apRef) : BSExtraData(0x4Cu) {
    if (apRef) {
        realAngle = apRef->GetAngleOnReference();
        realLocation = apRef->GetLocationOnReference();
        oldLocation = realLocation;
    }
}

ExtraEditorRefMoveData::~ExtraEditorRefMoveData() {}

// The whole pointer is copied.
NAVMESH_PORTAL::NAVMESH_PORTAL(const NAVMESH_PORTAL& aPortal)
  : pNavMesh(aPortal.pNavMesh), iTriIndex(aPortal.iTriIndex) {}

void NAVMESH_PORTAL::Endian() {
    ENDIAN(NavMeshID, false);
    ENDIAN(iTriIndex, false);
}

ExtraNavMeshPortal::ExtraNavMeshPortal(NAVMESH_PORTAL aPortal) : BSExtraData(90u), Portal(aPortal) {}

ExtraNavMeshPortal::~ExtraNavMeshPortal() {}

ExtraPortal::ExtraPortal() : BSExtraData(120), spPortal(nullptr) {}

ExtraPortal::~ExtraPortal() {}

ExtraReservedMarkers::ExtraReservedMarkers() : BSExtraData(EXTRA_RESERVEDMARKERS), iReservedMarkers(0) {}

ExtraReservedMarkers::~ExtraReservedMarkers() {}

uint16_t Animation::GetGroup(ANIM_GROUP_SECTION aeSection) {
    int32_t eSection = aeSection;
    switch (aeSection) {
        case AGS_WHOLE_BODY: eSection = AGS_MOVEMENT; break;
        case AGS_UPPER_BODY: eSection = AGS_WEAPON; break;
    }
    return this->group[eSection];
}

uint8_t BSExtraData::GetExtraType() const {
    return cEtype;
}

BSExtraData* BSExtraData::GetNext() {
    return pNext;
}

void BSExtraData::SetNext(BSExtraData* apNext) {
    pNext = apNext;
}

ExtraGuardedRefData::ExtraGuardedRefData() : BSExtraData(EXTRA_GUARDED_REF_DATA), Guards() {}

ExtraGuardedRefData::~ExtraGuardedRefData() {}

bool ExtraGuardedRefData::Compare(BSExtraData* apCompare) {
    if (!BSExtraData::Compare(apCompare)) {
        return true;
    }
    // The cast is static because BSExtraData::Compare has already checked that both extras are of one type.
    ExtraGuardedRefData* pComp = static_cast<ExtraGuardedRefData*>(apCompare);
    if (pComp->Guards.iSize != Guards.iSize) {
        return true;
    }
    for (uint32_t uiGuard : Guards) {
        if (pComp->Guards.Find(uiGuard, 0, &BSSimpleArray<uint32_t>::DefaultEqualFunctionByAddress) == -1) {
            return true;
        }
    }
    return false;
}

void ExtraGuardedRefData::AddGuard(Actor* apActor) {
    uint32_t uiNumID = apActor->iFormID;
    if (Guards.Find(uiNumID, 0, &BSSimpleArray<uint32_t>::DefaultEqualFunctionByAddress) == -1) {
        Guards.Add(uiNumID);
    }
}

void ExtraGuardedRefData::NotifyGuardsAboutActivation(TESObjectREFR* apObject, TESObjectREFR* apActionRef) {
    for (uint32_t i = 0; i < Guards.iSize; ++i) {
        Actor* pActor = static_cast<Actor*>(TESForm::GetFormByNumericID(Guards[i]));
        if (pActor && pActor->IsReference() && pActor->IsActor()) {
            BaseProcess* pProc = pActor->pCurrentProcess;
            if (pProc) {
                pProc->NotifyGuardAboutActivation(pActor, apActionRef, apObject);
            }
        }
    }
}

void ExtraGuardedRefData::operator delete(void* apMemory) {
    ourFree(apMemory);
}

BSSimpleList<FACTION_RANK*>* TESActorBaseData::GetFactionList() {
    return &listFactions;
}

bool DoorTeleportData::InitItem(TESForm* apOwner) {
    uint32_t tempID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pLinkedDoor));
    TESFile* pFile = apOwner->GetFile(-1);
    TESForm::AddCompileIndex(tempID, pFile);
    TESForm* pForm = TESForm::GetFormByNumericID(tempID);
    pLinkedDoor = dynamic_cast<TESObjectREFR*>(pForm);
    if (pLinkedDoor) {
        if (pLinkedDoor->GetObjectReference()->GetFormType() == DOOR_ID) {
            if (!std::isfinite(position.x) ||
                !std::isfinite(position.y) ||
                !std::isfinite(position.z) ||
                std::isnan(position.x) ||
                std::isnan(position.y) ||
                std::isnan(position.z)) {
                Warning("MASTERFILE: Corrupt location found in teleport data, setting to (0, 0, 0).");
                position = NiPoint3::ZERO;
            }
            if (!std::isfinite(rotation.x) ||
                !std::isfinite(rotation.y) ||
                !std::isfinite(rotation.z) ||
                std::isnan(rotation.x) ||
                std::isnan(rotation.y) ||
                std::isnan(rotation.z)) {
                Warning("MASTERFILE: Corrupt angle found in teleport data, setting to (0, 0, 0).");
                rotation = NiPoint3::ZERO;
            }
            return true;
        }

        TESBoundObject* pObject = pLinkedDoor->GetObjectReference();
        int32_t iFormID = pLinkedDoor->GetObjectReference()->iFormID;
        const char* pcEditorID = pObject->GetFormEditorID();
        int32_t iFormType = pLinkedDoor->GetObjectReference()->GetFormType();
        Warning(
            "MASTERFILE: Linked door (%08X) in teleport data points to invalid object (%s %s(%08X)).",
            tempID,
            formEnumString[iFormType].pFormString,
            pcEditorID,
            iFormID
        );
        pLinkedDoor = nullptr;
        return false;
    }

    Warning("MASTERFILE: Could not find linked door (%08X) in teleport data init.", tempID);
    return false;
}

void MobileObject::SetCurrentAIProcess(BaseProcess* appross) {
    pCurrentProcess = appross;
}

void NiFixedString::operator delete(void* apMemory) {
    ourFree(apMemory);
}

// The empty string's handle has no reference count. The count is raw memory, the untyped word GetRealBufferStart
// returns; it is decremented through the int32_t NiAtomicDecrement.
void NiGlobalStringTable::DecHandleRefCount(GlobalStringHandle& arHandle) {
    if (arHandle != NULL_STRING) {
        const char* pcBuffer = GetRealBufferStart(arHandle);
        NiAtomicDecrement(*reinterpret_cast<int32_t*>(const_cast<char*>(pcBuffer)));
    }
}
