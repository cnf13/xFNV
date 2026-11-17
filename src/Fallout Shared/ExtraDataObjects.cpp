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

pLockLevelNames_t pLockLevelNames{&sLockLevelNameVeryEasy, &sLockLevelNameEasy,     &sLockLevelNameAverage,
                                  &sLockLevelNameHard,     &sLockLevelNameVeryHard, &sLockLevelNameImpossible};
LeveledCreatureMultiplierSettings pLevCreaModMult{
    &fLeveledActorMultEasy, &fLeveledActorMultMedium, &fLeveledActorMultHard, &fLeveledActorMultBoss
};

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

ExtraAction::ExtraAction() : BSExtraData(EXTRA_ACTION), eAction((uint8_t)1), pActionRef(nullptr) {}

bool ExtraAction::Compare(BSExtraData* apCompare) {
    ExtraAction* v4 = dynamic_cast<ExtraAction*>(apCompare);
    if (!v4) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return (bool)((uint8_t)(eAction != v4->eAction));
}

ExtraActivateRef::ExtraActivateRef() : BSExtraData(83), cActivateFlags(0) {
    ActivateTextOverride.Set("", 0u);
}

bool ExtraActivateRef::Compare(BSExtraData* apCompare) {
    ExtraActivateRef* pcomp = dynamic_cast<ExtraActivateRef*>(apCompare);
    if (!pcomp) {
        return true;
    }
    int32_t v3 = ParentList.ItemsInList();
    if (v3 != pcomp->ParentList.ItemsInList()) {
        return true;
    }
    for (BSSimpleList<REF_ACTIVATE_DATA*>* plist = &ParentList; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
        REF_ACTIVATE_DATA** v4 = &plist->m_item;
        REF_ACTIVATE_DATA* pdata = *v4;
        REF_ACTIVATE_DATA* pcompdata = pcomp->GetData((*v4)->pActivateRef);
        if (!pcompdata || std::memcmp(pcompdata, pdata, sizeof(REF_ACTIVATE_DATA))) {
            return true;
        }
    }
    if (cActivateFlags != pcomp->cActivateFlags) {
        return true;
    }
    if (!ActivateTextOverride.GetLength() && !pcomp->ActivateTextOverride.GetLength()) {
        return false;
    }
    if (!ActivateTextOverride.GetLength() || !pcomp->ActivateTextOverride.GetLength()) {
        return true;
    }
    return BSstrcmp(ActivateTextOverride.pString, pcomp->ActivateTextOverride.pString) != 0;
}

ExtraActivateRef::~ExtraActivateRef() {
    FreeActivateList(&ParentList);
}

REF_ACTIVATE_DATA* ExtraActivateRef::GetData(TESObjectREFR* apRef) {
    return GetActivateDataFromList(apRef, &ParentList);
}

void ExtraActivateRef::Copy(BSExtraData* apCopy) {
    ExtraActivateRef* pcopy = dynamic_cast<ExtraActivateRef*>(apCopy);
    if (pcopy) {
        FreeActivateList(&ParentList);
        BSSimpleList<REF_ACTIVATE_DATA*>* plist = &pcopy->ParentList;
        while (plist && !plist->IsEmpty()) {
            REF_ACTIVATE_DATA* pcopydata;
            REF_ACTIVATE_DATA* pdata = plist->m_item;
            plist = plist->m_pkNext;
            REF_ACTIVATE_DATA* apMemory = static_cast<REF_ACTIVATE_DATA*>(ourMalloc(sizeof(REF_ACTIVATE_DATA)));
            if (apMemory) {
                pcopydata = ::new (apMemory) REF_ACTIVATE_DATA;
            }
            else {
                pcopydata = nullptr;
            }
            REF_ACTIVATE_DATA* pcopydata_2 = pcopydata;
            ourMemcpy(pcopydata, pdata, sizeof(REF_ACTIVATE_DATA));
            ParentList.AddHead(pcopydata_2);
        }
        cActivateFlags = pcopy->cActivateFlags;
        ActivateTextOverride.Set(pcopy->ActivateTextOverride);
    }
}

REF_ACTIVATE_DATA::REF_ACTIVATE_DATA() : pActivateRef(nullptr), fActivateDelay(0.0) {}

void REF_ACTIVATE_DATA::Endian() {
    ENDIAN(pActivateRef, false);
    ENDIAN(fActivateDelay, false);
}

REF_ACTIVATE_DATA* GetActivateDataFromList(TESObjectREFR* apRef, BSSimpleList<REF_ACTIVATE_DATA*>* apList) {
    REF_ACTIVATE_DATA* v5 = nullptr;
    while (apList && !apList->IsEmpty()) {
        REF_ACTIVATE_DATA* v3 = apList->m_item;
        if (v3->pActivateRef == apRef) {
            return v3;
        }
        apList = apList->m_pkNext;
    }
    return v5;
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

uint32_t getLockLevelName(int32_t a1) {
    uint32_t name = 0;
    switch (a1) {
        case 0: {
            name = (&::iLockLevelMaxVeryEasy)->UInt();
            break;
        }
        case 1: {
            name = (&::iLockLevelMaxEasy)->UInt();
            break;
        }
        case 2: {
            name = (&::iLockLevelMaxAverage)->UInt();
            break;
        }
        case 3: {
            name = (&::iLockLevelMaxHard)->UInt();
            break;
        }
        case 4: {
            name = (&::iLockLevelMaxVeryHard)->UInt();
            break;
        }
        case 5: {
            name = (&::iLockLevelMaxImpossible)->UInt();
            break;
        }
        default: {
            return name;
        }
    }
    return name;
}

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
    ExtraAmmo* v4 = dynamic_cast<ExtraAmmo*>(apCompare);
    if (!v4) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    if (((uint32_t)iCount) == ((uint32_t)v4->iCount)) {
        return (bool)((uint8_t)(pAmmo != v4->pAmmo));
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
    ExtraCombatStyle* v4 = dynamic_cast<ExtraCombatStyle*>(apCompare);
    if (!v4) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return (bool)((uint8_t)(pCombatStyle != v4->pCombatStyle));
}

ExtraContainerChanges::ExtraContainerChanges(InventoryChanges* apChanges)
  : BSExtraData(EXTRA_CONTAINER_CHANGES), pChanges(apChanges) {}

ExtraContainerChanges::~ExtraContainerChanges() {
    delete pChanges;
}

ExtraCount::ExtraCount() : BSExtraData(36), iCount(0) {}

ExtraCount::ExtraCount(int16_t aiCount) : BSExtraData(EXTRA_COUNT), iCount(aiCount) {}

bool ExtraCount::Compare(BSExtraData* apCompare) {
    ExtraCount* v4 = dynamic_cast<ExtraCount*>(apCompare);
    if (!v4) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return (bool)((uint8_t)(iCount != v4->iCount));
}

ExtraDecalRefs::ExtraDecalRefs() : BSExtraData(87u) {}

bool ExtraDecalRefs::Compare(BSExtraData* apCompare) {
    ExtraDecalRefs* pcomp = dynamic_cast<ExtraDecalRefs*>(apCompare);
    if (!pcomp) {
        return true;
    }
    int32_t v3 = DecalRefList.ItemsInList();
    if (v3 != pcomp->DecalRefList.ItemsInList()) {
        return true;
    }
    for (BSSimpleList<REF_DECAL_DATA*>* plist = &pcomp->DecalRefList; plist && !plist->IsEmpty();
         plist = plist->m_pkNext) {
        REF_DECAL_DATA* pcompdata = plist->m_item;
        REF_DECAL_DATA* pdata = GetRefDecalData(pcompdata->pDecalRef);
        if (!pdata) {
            return true;
        }
        if (std::memcmp(pdata, pcompdata, sizeof(REF_DECAL_DATA))) {
            return true;
        }
    }
    return false;
}

void ExtraDecalRefs::InitItem(TESForm* apOwner) {
    BSSimpleList<REF_DECAL_DATA*>* preflist = &DecalRefList;
    BSSimpleList<REF_DECAL_DATA*>* plastlist = nullptr;
    while (preflist && !preflist->IsEmpty()) {
        bool badvanced = false;
        REF_DECAL_DATA* pdata = preflist->m_item;
        uint32_t tempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pdata->pDecalRef));
        TESFile* File = apOwner->GetFile(-1);
        TESForm::AddCompileIndex(tempint, File);
        TESForm* FormByNumericID = TESForm::GetFormByNumericID(tempint);
        TESObjectREFR* pdecal = dynamic_cast<TESObjectREFR*>(FormByNumericID);
        if (pdecal) {
            pdata->pDecalRef = pdecal;
        }
        else {
            Warning("MASTERFILE: Unable to find decal reference %08X.", tempint);
            if (plastlist) {
                plastlist->Remove(pdata);
                preflist = plastlist->m_pkNext;
            }
            else {
                preflist->RemoveHead();
            }
            ourFree(pdata);
            pdata = nullptr;
            badvanced = true;
        }
        if (!badvanced) {
            plastlist = preflist;
            preflist = preflist->m_pkNext;
        }
    }
}

REF_DECAL_DATA* ExtraDecalRefs::GetRefDecalData(TESObjectREFR* apRef) {
    REF_DECAL_DATA* v5 = nullptr;
    BSSimpleList<REF_DECAL_DATA*>* ecx0 = &DecalRefList;
    while (ecx0 && !ecx0->IsEmpty()) {
        REF_DECAL_DATA* v3 = static_cast<REF_DECAL_DATA*>(ecx0->m_item);
        ecx0 = ecx0->m_pkNext;
        if (v3->pDecalRef == apRef) {
            return v3;
        }
    }
    return v5;
}

// Each point is copied 4 bytes at a time, each load followed by its store, so a source overlapping the record reads the
// bytes already written; the 4-byte copies deliberately keep that order and the bits, as the game does.
void ExtraDecalRefs::SetRefDecalData(TESObjectREFR* apRef, const NiPoint3& aIntersect, const NiPoint3& aNormal) {
    REF_DECAL_DATA* pData = GetRefDecalData(apRef);
    if (!pData) {
        void* pMemory = ourMalloc(sizeof(REF_DECAL_DATA));
        pData = pMemory ? ::new (pMemory) REF_DECAL_DATA : nullptr;
        pData->pDecalRef = apRef;
        REF_DECAL_DATA* pItem = pData;
        DecalRefList.AddHead(pItem);
    }
    __builtin_memcpy(&pData->Intersect.x, &aIntersect.x, sizeof(pData->Intersect.x));
    __builtin_memcpy(&pData->Intersect.y, &aIntersect.y, sizeof(pData->Intersect.y));
    __builtin_memcpy(&pData->Intersect.z, &aIntersect.z, sizeof(pData->Intersect.z));
    __builtin_memcpy(&pData->Normal.x, &aNormal.x, sizeof(pData->Normal.x));
    __builtin_memcpy(&pData->Normal.y, &aNormal.y, sizeof(pData->Normal.y));
    __builtin_memcpy(&pData->Normal.z, &aNormal.z, sizeof(pData->Normal.z));
}

int32_t ExtraDecalRefs::ClearDecalData() {
    int32_t result;
    void* v3;
    while (true) {
        result = (int32_t)(&DecalRefList)->IsEmpty();
        if ((uint8_t)result) {
            break;
        }
        v3 = (&DecalRefList)->m_item;
        (&DecalRefList)->RemoveHead();
        ourFree(v3);
    }
    return result;
}

void ExtraDecalRefs::Copy(BSExtraData* apCopy) {
    REF_DECAL_DATA* pdata_2;
    ExtraDecalRefs* pcopy = dynamic_cast<ExtraDecalRefs*>(apCopy);
    if (pcopy) {
        ClearDecalData();
        for (BSSimpleList<REF_DECAL_DATA*>* plist = &pcopy->DecalRefList; plist && !plist->IsEmpty();
             plist = plist->m_pkNext) {
            REF_DECAL_DATA* pdata;
            REF_DECAL_DATA* pcopydata = plist->m_item;
            REF_DECAL_DATA* apMemory = static_cast<REF_DECAL_DATA*>(ourMalloc(sizeof(REF_DECAL_DATA)));
            if (apMemory) {
                pdata = ::new (apMemory) REF_DECAL_DATA;
            }
            else {
                pdata = nullptr;
            }
            pdata_2 = pdata;
            ourMemcpy(pdata, pcopydata, sizeof(REF_DECAL_DATA));
            DecalRefList.AddHead(pdata_2);
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
    ExtraEmittanceSource* v4 = dynamic_cast<ExtraEmittanceSource*>(apCompare);
    return (bool)((uint8_t)((!v4) || (pSource != v4->pSource)));
}

ExtraEnableStateChildren::ExtraEnableStateChildren() : BSExtraData(0x38u) {}

ExtraEnableStateChildren::~ExtraEnableStateChildren() {
    ChildList.RemoveAll();
}

ExtraEnableStateParent::ExtraEnableStateParent()
  : BSExtraData(EXTRA_ENABLESTATEPARENT), pParent(nullptr), cFlags((uint8_t)0) {}

bool ExtraEnableStateParent::Compare(BSExtraData* apCompare) {
    ExtraEnableStateParent* pcomp = dynamic_cast<ExtraEnableStateParent*>(apCompare);
    if (!pcomp) {
        return true;
    }
    if (pcomp->pParent == pParent) {
        return pcomp->cFlags != cFlags;
    }
    return true;
}

bool ExtraEnableStateParent::ShouldPopIn() {
    return ((cFlags & 2) != 0);
}

bool ExtraEnableStateParent::ShouldPopOut() {
    return ((cFlags & 4) != 0);
}

void REF_ENABLE_PARENT_DATA::Endian() {
    ENDIAN(pParent, false);
}

ExtraEncounterZone::ExtraEncounterZone() : BSExtraData(EXTRA_ENCOUNTERZONE), pZone(nullptr) {}

bool ExtraEncounterZone::Compare(BSExtraData* apCompare) {
    ExtraEncounterZone* v4 = dynamic_cast<ExtraEncounterZone*>(apCompare);
    if (!v4) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return (bool)((uint8_t)(pZone != v4->pZone));
}

ExtraFollower::ExtraFollower() : BSExtraData(29), pActorlist(new BSSimpleList<Actor*>) {}

ExtraFollower::~ExtraFollower() {
    pActorlist->RemoveAll();
    delete pActorlist;
}

ExtraGlobal::ExtraGlobal(TESGlobal* apGlobal) : BSExtraData(EXTRA_GLOBAL), pGlobal(apGlobal) {}

bool ExtraGlobal::Compare(BSExtraData* apCompare) {
    ExtraGlobal* v4 = dynamic_cast<ExtraGlobal*>(apCompare);
    if (!v4) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return (bool)((uint8_t)(pGlobal != v4->pGlobal));
}

ExtraHealth::ExtraHealth() : BSExtraData(37u), fHealth(0.0) {}

ExtraHealth::ExtraHealth(float afHealth) : BSExtraData(EXTRA_HEALTH), fHealth(afHealth) {}

bool ExtraHealth::Compare(BSExtraData* apCompare) {
    ExtraHealth* v4 = dynamic_cast<ExtraHealth*>(apCompare);
    if (!v4) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return (bool)((uint8_t)(v4->fHealth != fHealth));
}

ExtraHealthPerc::ExtraHealthPerc() : BSExtraData(EXTRA_HEALTH_PERC), fHealthPerc((float)0.0) {}

bool ExtraHealthPerc::Compare(BSExtraData* apCompare) {
    ExtraHealthPerc* v4 = dynamic_cast<ExtraHealthPerc*>(apCompare);
    if (!v4) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return (bool)((uint8_t)(v4->fHealthPerc != fHealthPerc));
}

ExtraIgnoredBySandbox::ExtraIgnoredBySandbox() : BSExtraData(EXTRA_IGNORED_BY_SANDBOX) {}

ExtraLastFinishedSequence::ExtraLastFinishedSequence(const char* apSequenceName) : BSExtraData(65) {
    int32_t iSeqNameLen = BSstrlen(apSequenceName) + 1;
    pLastSequenceName = static_cast<char*>(ourMalloc(iSeqNameLen));
    BSstrcpy(pLastSequenceName, iSeqNameLen, apSequenceName);
}

bool ExtraLastFinishedSequence::Compare(BSExtraData* apCompare) {
    ExtraLastFinishedSequence* v4 = dynamic_cast<ExtraLastFinishedSequence*>(apCompare);
    return ((!v4) || (BSstrcmp(pLastSequenceName, v4->pLastSequenceName) != 0));
}

ExtraLastFinishedSequence::~ExtraLastFinishedSequence() {
    ourFree(pLastSequenceName);
}

ExtraLeveledCreature::ExtraLeveledCreature()
  : BSExtraData(EXTRA_LEVELEDCREATURE), pOriginalBase(nullptr), pTemplate(nullptr) {}

ExtraLeveledItem::ExtraLeveledItem() : BSExtraData(47u), iIndex(0), bdefault(0) {}

ExtraLeveledItem::ExtraLeveledItem(int32_t aiIndex)
  : BSExtraData(EXTRA_LEVELITEM), iIndex(aiIndex), bdefault((uint8_t)1) {}

bool ExtraLeveledItem::Compare(BSExtraData* apCompare) {
    return (bool)((uint8_t)((!dynamic_cast<ExtraLeveledItem*>(apCompare)) || BSExtraData::Compare(apCompare)));
}

ExtraLight::ExtraLight(LightRefData* apLight) : BSExtraData(EXTRA_LIGHT), pLight(apLight) {}

ExtraLight::~ExtraLight() {
    delete pLight;
}

ExtraLinkedRef::ExtraLinkedRef() : BSExtraData(EXTRA_LINKED_REF), pLinkedRef(nullptr) {}

bool ExtraLinkedRef::Compare(BSExtraData* apCompare) {
    ExtraLinkedRef* v4 = dynamic_cast<ExtraLinkedRef*>(apCompare);
    return (bool)((uint8_t)((!v4) || (v4->pLinkedRef != pLinkedRef)));
}

ExtraLinkedRefChildren::ExtraLinkedRefChildren() : BSExtraData(0x52u) {}

ExtraLinkedRefChildren::~ExtraLinkedRefChildren() {
    ChildList.RemoveAll();
}

ExtraLitWaterRefs::ExtraLitWaterRefs() : BSExtraData(133u) {}

bool ExtraLitWaterRefs::Compare(BSExtraData* apCompare) {
    ExtraLitWaterRefs* pcomp = dynamic_cast<ExtraLitWaterRefs*>(apCompare);
    if (!pcomp) {
        return true;
    }
    int32_t v3 = RefList.ItemsInList();
    if (v3 != pcomp->RefList.ItemsInList()) {
        return true;
    }
    for (BSSimpleList<TESObjectREFR*>* plist = &pcomp->RefList; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
        if (!RefList.IsInList(plist->m_item)) {
            return true;
        }
    }
    return false;
}

void ExtraLitWaterRefs::InitItem(TESForm* apOwner) {
    BSSimpleList<TESObjectREFR*>* preflist = &RefList;
    BSSimpleList<TESObjectREFR*> newlist;
    while (preflist && !preflist->IsEmpty()) {
        uint32_t tempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(preflist->m_item));
        TESFile* File = apOwner->GetFile(-1);
        TESForm::AddCompileIndex(tempint, File);
        TESForm* FormByNumericID = TESForm::GetFormByNumericID(tempint);
        TESObjectREFR* pref = dynamic_cast<TESObjectREFR*>(FormByNumericID);
        if (pref && pref->GetRefPersists()) {
            (&pref->m_Extra)->SetWaterLightRef(static_cast<TESObjectREFR*>(apOwner), true);
            newlist.AddHead(pref);
        }
        else {
            if (pref) {
                Warning(
                    "MASTERFILE: Reference %08X is not persistent so cannot reflect or refract other references.",
                    tempint
                );
            }
            else {
                Warning("MASTERFILE: Unable to find lit water reference %08X.", tempint);
            }
        }
        preflist = preflist->m_pkNext;
    }
    RefList.RemoveAll();
    for (TESObjectREFR*& rpItem : newlist.ItemsUntilNull()) {
        RefList.AddHead(rpItem);
    }
    newlist.RemoveAll();
}

void ExtraLitWaterRefs::Copy(BSExtraData* apCopy) {
    ExtraLitWaterRefs* pcopy = dynamic_cast<ExtraLitWaterRefs*>(apCopy);
    if (pcopy) {
        ClearReferenceData();
        for (BSSimpleList<TESObjectREFR*>* plist = &pcopy->RefList; plist && !plist->IsEmpty();
             plist = plist->m_pkNext) {
            AddReference(plist->m_item);
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

LockData::LockData()
  : cLockLevel((uint8_t)0), pKey(nullptr), cFlags((uint8_t)0), iNumTries((uint32_t)0), iTimesUnlocked((uint32_t)0) {}

void LockData::Endian() {
    ENDIAN(pKey, false);
}

bool LockData::IsBroken() {
    uint32_t v2;
    float args = (float)0.0;
    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::IGNORE_BROKEN_LOCK, pPlayer, &args);
    if (args == 0.0) {
        v2 = 1;
    }
    else {
        v2 = 2;
    }
    return (bool)((uint8_t)(iNumTries >= v2));
}

bool LockData::IsLocked() {
    return ((cFlags & 1) != 0);
}

int32_t LockData::GetLockLevel(TESObjectREFR* apOwner) {
    return GetLockPickLevel(GetLevel(apOwner));
}

// Flag 4 marks a leveled lock.
int32_t LockData::GetLevel(TESObjectREFR* apOwner) {
    int32_t iLevel = cLockLevel;
    if (static_cast<int8_t>(cFlags) & 4) {
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
        cFlags |= 1;
    }
    else {
        cFlags &= ~1;
        iNumTries = 0;
    }
}

// The lock is freed with ourFree (~ExtraLock), so it is allocated with ourMalloc.
ExtraLock::ExtraLock() : BSExtraData(EXTRA_LOCK), pLock(new (ourMalloc(sizeof(LockData))) LockData()) {}

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
    int32_t LockPickLevel = GetLockPickLevel(pLock->cLockLevel);
    if (LockPickLevel != GetLockPickLevel(pComp->pLock->cLockLevel)) {
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
    ExtraMerchantContainer* v4 = dynamic_cast<ExtraMerchantContainer*>(apCompare);
    return (bool)((uint8_t)((!v4) || (v4->pContainer != pContainer)));
}

ExtraModelSwap::ExtraModelSwap() : BSExtraData(91), pModelSwap(nullptr), pModelSwapForm(nullptr) {}

ExtraModelSwap::ExtraModelSwap(TESModel* apModelSwap, TESForm* apModelSwapForm)
  : BSExtraData(EXTRA_MODEL_SWAP), pModelSwap(apModelSwap), pModelSwapForm(apModelSwapForm) {}

ExtraMultiBoundData::ExtraMultiBoundData() : BSExtraData(EXTRA_MULTIBOUND_DATA), pBound(nullptr) {}

bool ExtraMultiBoundData::Compare(BSExtraData* apCompare) {
    ExtraMultiBoundData* pcomp = dynamic_cast<ExtraMultiBoundData*>(apCompare);
    return !pcomp || pBound->Compare(pcomp->pBound);
}

ExtraMultiBoundData::~ExtraMultiBoundData() {
    ourFree(pBound);
}

ExtraMultiBoundRef::ExtraMultiBoundRef() : BSExtraData(EXTRA_MULTIBOUND_REF), pBoundRef(nullptr) {}

bool ExtraMultiBoundRef::Compare(BSExtraData* apCompare) {
    ExtraMultiBoundRef* v4 = dynamic_cast<ExtraMultiBoundRef*>(apCompare);
    return (bool)((uint8_t)((!v4) || (pBoundRef != v4->pBoundRef)));
}

ExtraNorthRotation::ExtraNorthRotation() : BSExtraData(EXTRA_NORTHROTATION), fNorthRot((float)0.0) {}

ExtraOwnership::ExtraOwnership(TESForm* apOwner) : BSExtraData(EXTRA_OWNERSHIP), pOwner(apOwner) {}

bool ExtraOwnership::Compare(BSExtraData* apCompare) {
    ExtraOwnership* v4 = dynamic_cast<ExtraOwnership*>(apCompare);
    if (!v4) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return (bool)((uint8_t)(pOwner != v4->pOwner));
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
  : BSExtraData(EXTRA_PACKAGE), pPack(apPack), iindex(aiIndex), pTarg(apRef),
    bActionComplete((bool)((uint8_t)abcomplete)), bActivated((bool)((uint8_t)abactivated)),
    bDoneOnce((bool)((uint8_t)abDoneOnce)) {}

ExtraRagDollData::ExtraRagDollData() : BSExtraData(EXTRA_RAGDOLLDATA), pRagDollData(nullptr) {}

bool ExtraRagDollData::Compare(BSExtraData* apCompare) {
    ExtraRagDollData* pcomp = dynamic_cast<ExtraRagDollData*>(apCompare);
    if (!pcomp) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    if (pRagDollData) {
        if (pRagDollData->Compare(pcomp->pRagDollData)) {
            return true;
        }
    }
    else {
        if (pcomp->pRagDollData) {
            return true;
        }
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
    char* v4 = static_cast<char*>(
        FNVD_DYNAMIC_CAST_VOID(ExtraPackageStartLocation)(apCompare, RTTI_BSExtraData, RTTI_ExtraPackageStartLocation)
    );
    if (!v4) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return std::memcmp(&worldLoc, v4 + 12, sizeof(worldLoc)) != 0;
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
    ExtraPrimitive* v4 = dynamic_cast<ExtraPrimitive*>(apCompare);
    return (bool)((uint8_t)((!v4) || (v4->pPrimitive != pPrimitive)));
}

ExtraPrimitive::~ExtraPrimitive() {
    delete pPrimitive;
}

ExtraRadiation::ExtraRadiation() : BSExtraData(93), fRadiation(0.0f) {}

ExtraRadiation::ExtraRadiation(float afRadiation) : BSExtraData(EXTRA_RADIATION), fRadiation(afRadiation) {}

bool ExtraRadiation::Compare(BSExtraData* apCompare) {
    ExtraRadiation* v4 = dynamic_cast<ExtraRadiation*>(apCompare);
    if (!v4) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return (bool)((uint8_t)(v4->fRadiation != fRadiation));
}

ExtraRadioData::ExtraRadioData() : BSExtraData(0x68u) {
    Data.fRadius = 0.0f;
    Data.eRangeType = static_cast<RADIO_RANGE_TYPE>(0);
    Data.fStaticPct = 0.0f;
    Data.pPositionRef = nullptr;
}

bool ExtraRadioData::Compare(BSExtraData* apCompare) {
    char* v4 =
        static_cast<char*>(FNVD_DYNAMIC_CAST_VOID(ExtraRadioData)(apCompare, RTTI_BSExtraData, RTTI_ExtraRadioData));
    if (!v4) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return std::memcmp(&Data, v4 + 12, sizeof(Data)) != 0;
}

ExtraRadius::ExtraRadius() : BSExtraData(92), fRadius(0.0f) {}

ExtraRadius::ExtraRadius(float afRadius) : BSExtraData(EXTRA_RADIUS), fRadius(afRadius) {}

bool ExtraRadius::Compare(BSExtraData* apCompare) {
    ExtraRadius* v4 = dynamic_cast<ExtraRadius*>(apCompare);
    if (!v4) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return (bool)((uint8_t)(v4->fRadius != fRadius));
}

ExtraRank::ExtraRank(int32_t aiRank) : BSExtraData(EXTRA_RANK), iRank(aiRank) {}

bool ExtraRank::Compare(BSExtraData* apCompare) {
    ExtraRank* v4 = dynamic_cast<ExtraRank*>(apCompare);
    if (!v4) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return (bool)((uint8_t)(((uint32_t)iRank) != ((uint32_t)v4->iRank)));
}

REF_REFLECTOR_DATA::REF_REFLECTOR_DATA() : pRef(nullptr), iEffectFlags(0) {}

void REF_REFLECTOR_DATA::Endian() {
    ENDIAN(pRef, false);
    ENDIAN(iEffectFlags, false);
}

REF_REFLECTED_DATA::REF_REFLECTED_DATA() : pRef(nullptr), iEffectFlags(0) {}

ExtraReflectedRefs::ExtraReflectedRefs() : BSExtraData(0x65u) {}

bool ExtraReflectedRefs::Compare(BSExtraData* apCompare) {
    ExtraReflectedRefs* pcomp = dynamic_cast<ExtraReflectedRefs*>(apCompare);
    if (!pcomp) {
        return true;
    }
    int32_t v3 = RefList.ItemsInList();
    if (v3 != pcomp->RefList.ItemsInList()) {
        return true;
    }
    for (BSSimpleList<REF_REFLECTED_DATA*>* plist = &pcomp->RefList; plist && !plist->IsEmpty();
         plist = plist->m_pkNext) {
        REF_REFLECTED_DATA** v4 = &plist->m_item;
        REF_REFLECTED_DATA* pcompdata = *v4;
        REF_REFLECTED_DATA* pdata = GetRefData((*v4)->pRef);
        if (!pdata) {
            return true;
        }
        if (std::memcmp(pdata, pcompdata, sizeof(REF_REFLECTED_DATA))) {
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
    ExtraReflectedRefs* pcopy = dynamic_cast<ExtraReflectedRefs*>(apCopy);
    if (pcopy) {
        ClearReferenceData();
        for (BSSimpleList<REF_REFLECTED_DATA*>* plist = &pcopy->RefList; plist && !plist->IsEmpty();
             plist = plist->m_pkNext) {
            REF_REFLECTED_DATA* pdata;
            REF_REFLECTED_DATA* pcopydata = plist->m_item;
            REF_REFLECTED_DATA* v4 = static_cast<REF_REFLECTED_DATA*>(ourMalloc(sizeof(REF_REFLECTED_DATA)));
            if (v4) {
                pdata = ::new (v4) REF_REFLECTED_DATA;
            }
            else {
                pdata = nullptr;
            }
            REF_REFLECTED_DATA* pdata_2 = pdata;
            ourMemcpy(pdata, pcopydata, sizeof(REF_REFLECTED_DATA));
            RefList.AddHead(pdata_2);
        }
    }
}

void ExtraReflectedRefs::RemoveReference(TESObjectREFR* apRef) {
    BSSimpleList<REF_REFLECTED_DATA*>* plist = &RefList;
    BSSimpleList<REF_REFLECTED_DATA*>* plastlist = nullptr;
    while (plist && !plist->IsEmpty()) {
        REF_REFLECTED_DATA* pdata = plist->m_item;
        if (pdata->pRef == apRef) {
            if (plastlist) {
                plastlist->Remove(pdata);
            }
            else {
                plist->RemoveHead();
            }
            ourFree(pdata);
            pdata = nullptr;
            return;
        }
        plastlist = plist;
        plist = plist->m_pkNext;
    }
}

void ExtraReflectedRefs::SetReference(TESObjectREFR* apRef, uint32_t aiEffectFlags) {
    REF_REFLECTED_DATA* pdata = GetRefData(apRef);
    if (!pdata) {
        REF_REFLECTED_DATA* pdata_2;
        REF_REFLECTED_DATA* v5 = static_cast<REF_REFLECTED_DATA*>(ourMalloc(sizeof(REF_REFLECTED_DATA)));
        if (v5) {
            pdata_2 = ::new (v5) REF_REFLECTED_DATA;
        }
        else {
            pdata_2 = nullptr;
        }
        pdata = pdata_2;
        pdata_2->pRef = apRef;
        RefList.AddHead(pdata);
    }
    pdata->iEffectFlags = aiEffectFlags;
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
    ExtraReflectorRefs* pcomp = dynamic_cast<ExtraReflectorRefs*>(apCompare);
    if (!pcomp) {
        return true;
    }
    int32_t v3 = RefList.ItemsInList();
    if (v3 != pcomp->RefList.ItemsInList()) {
        return true;
    }
    for (BSSimpleList<REF_REFLECTOR_DATA*>* plist = &pcomp->RefList; plist && !plist->IsEmpty();
         plist = plist->m_pkNext) {
        REF_REFLECTOR_DATA* pcompdata = plist->m_item;
        REF_REFLECTOR_DATA* pdata = GetRefData(pcompdata->pRef);
        if (!pdata) {
            return true;
        }
        if (std::memcmp(pdata, pcompdata, sizeof(REF_REFLECTOR_DATA))) {
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
    ExtraReflectorRefs* pcopy = dynamic_cast<ExtraReflectorRefs*>(apCopy);
    if (pcopy) {
        ClearReferenceData();
        for (BSSimpleList<REF_REFLECTOR_DATA*>* plist = &pcopy->RefList; plist && !plist->IsEmpty();
             plist = plist->m_pkNext) {
            REF_REFLECTOR_DATA* pdata;
            REF_REFLECTOR_DATA* pcopydata = plist->m_item;
            REF_REFLECTOR_DATA* v4 = static_cast<REF_REFLECTOR_DATA*>(ourMalloc(sizeof(REF_REFLECTOR_DATA)));
            if (v4) {
                pdata = ::new (v4) REF_REFLECTOR_DATA;
            }
            else {
                pdata = nullptr;
            }
            REF_REFLECTOR_DATA* pdata_2 = pdata;
            ourMemcpy(pdata, pcopydata, sizeof(REF_REFLECTOR_DATA));
            RefList.AddHead(pdata_2);
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
        void* pMemory = ourMalloc(sizeof(REF_REFLECTOR_DATA));
        pData = pMemory ? ::new (pMemory) REF_REFLECTOR_DATA : nullptr;
        pData->pRef = apRef;
        RefList.AddHead(pData);
    }
    pData->iEffectFlags = aiEffectFlags;
}

void ExtraReflectorRefs::InitItem(TESForm* apOwner) {
    BSSimpleList<REF_REFLECTOR_DATA*>* preflist = &RefList;
    BSSimpleList<REF_REFLECTOR_DATA*>* plastlist = nullptr;
    while (preflist && !preflist->IsEmpty()) {
        bool badvanced = false;
        REF_REFLECTOR_DATA* pdata = preflist->m_item;
        uint32_t tempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pdata->pRef));
        TESFile* File = apOwner->GetFile(-1);
        TESForm::AddCompileIndex(tempint, File);
        TESForm* FormByNumericID = TESForm::GetFormByNumericID(tempint);
        TESObjectREFR* pref = dynamic_cast<TESObjectREFR*>(FormByNumericID);
        if (pref && pref->GetRefPersists()) {
            if ((pdata->iEffectFlags & 1) != 0) {
                (&pref->m_Extra)->SetReflectsRef(static_cast<TESObjectREFR*>(apOwner), true);
            }
            if ((pdata->iEffectFlags & 2) != 0) {
                (&pref->m_Extra)->SetRefractsRef(static_cast<TESObjectREFR*>(apOwner), true);
            }
            pdata->pRef = pref;
        }
        else {
            if (pref) {
                Warning(
                    "MASTERFILE: Reference %08X is not persistent so cannot reflect or refract other references.",
                    tempint
                );
            }
            else {
                Warning("MASTERFILE: Unable to find water reflector reference %08X.", tempint);
            }
            if (plastlist) {
                plastlist->Remove(pdata);
                preflist = plastlist->m_pkNext;
            }
            else {
                preflist->RemoveHead();
            }
            ourFree(pdata);
            pdata = nullptr;
            badvanced = true;
        }
        if (!badvanced) {
            plastlist = preflist;
            preflist = preflist->m_pkNext;
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
    ExtraScript* v4 = dynamic_cast<ExtraScript*>(apCompare);
    if (!v4) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return (bool)((uint8_t)(pScript != v4->pScript));
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

ExtraWeaponModFlags::ExtraWeaponModFlags() : BSExtraData(EXTRA_WEAPON_MOD_SLOTS), cWeaponModsActive((uint8_t)0) {}

ExtraWeaponModFlags::ExtraWeaponModFlags(uint8_t acSlot)
  : BSExtraData(EXTRA_WEAPON_MOD_SLOTS), cWeaponModsActive(acSlot) {}

bool ExtraWeaponModFlags::Compare(BSExtraData* apCompare) {
    ExtraWeaponModFlags* v4 = dynamic_cast<ExtraWeaponModFlags*>(apCompare);
    if (!v4) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return (bool)((uint8_t)(cWeaponModsActive != v4->cWeaponModsActive));
}

bool ExtraWeaponModFlags::GetWeaponModActive(uint8_t slot) {
    return (cWeaponModsActive & slot) != 0;
}

bool ExtraWeaponModFlags::HasWeaponMods() {
    return (cWeaponModsActive != 0);
}

void ExtraWeaponModFlags::SetWeaponModFlags(uint8_t flags) {
    cWeaponModsActive = flags;
}

void ExtraWeaponModFlags::SetWeaponModActive(const uint8_t slot) {
    cWeaponModsActive |= slot;
}

ExtraXTarget::ExtraXTarget() : BSExtraData(EXTRA_XTARGET), pTarget(nullptr) {}

bool ExtraXTarget::Compare(BSExtraData* apCompare) {
    ExtraXTarget* v4 = dynamic_cast<ExtraXTarget*>(apCompare);
    return (bool)((uint8_t)((!v4) || (pTarget != v4->pTarget)));
}

ExtraUsedMarkers::ExtraUsedMarkers() : BSExtraData(EXTRA_USEDMARKERS), iUsedMarkers((uint32_t)0) {}

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
    uint8_t v3 = 0;
    if (aiMarkerIndex < 30) {
        return ((((int32_t)(1 << aiMarkerIndex)) & iUsedMarkers) != 0);
    }
    return (bool)v3;
}

ExtraSpecialRenderFlags::ExtraSpecialRenderFlags() : BSExtraData(EXTRA_SPECIAL_RENDER_FLAGS), m_flags((uint32_t)0) {}

bool ExtraSpecialRenderFlags::Compare(BSExtraData* apCompare) {
    ExtraSpecialRenderFlags* v5 = dynamic_cast<ExtraSpecialRenderFlags*>(apCompare);
    if (!v5) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    uint32_t v3 = ExtraSpecialRenderFlags::GetFlags();
    return (bool)((uint8_t)((v3 != v5->ExtraSpecialRenderFlags::GetFlags()) ||
                            (v5->m_fNodeMaxFadeDistance != m_fNodeMaxFadeDistance)));
}

void ExtraSpecialRenderFlags::Copy(BSExtraData* apCopy) {
    ExtraSpecialRenderFlags* pcopy = dynamic_cast<ExtraSpecialRenderFlags*>(apCopy);
    if (pcopy) {
        m_flags = pcopy->m_flags;
        m_fNodeMaxFadeDistance = pcopy->m_fNodeMaxFadeDistance;
    }
}

ExtraStartingPosition::ExtraStartingPosition(TESObjectREFR* apObject) : BSExtraData(EXTRA_STARTINGPOSITION) {
    startPosition = apObject->GetLocationOnReference();
    startRotation = apObject->GetAngleOnReference();
}

bool ExtraStartingPosition::Compare(BSExtraData* apCompare) {
    char* v4 = static_cast<char*>(
        FNVD_DYNAMIC_CAST_VOID(ExtraStartingPosition)(apCompare, RTTI_BSExtraData, RTTI_ExtraStartingPosition)
    );
    if (!v4) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return std::memcmp(
               &startPosition, v4 + 12, sizeof(ExtraStartingPosition) - offsetof(ExtraStartingPosition, startPosition)
           ) != 0;
}

ExtraLevCreaModifier::ExtraLevCreaModifier() : BSExtraData(EXTRA_LEVCREA_MOD), eModifier(LCM_NONE) {}

bool ExtraLevCreaModifier::Compare(BSExtraData* apCompare) {
    ExtraLevCreaModifier* v4 = dynamic_cast<ExtraLevCreaModifier*>(apCompare);
    return (bool)((uint8_t)((!v4) || (v4->eModifier != eModifier)));
}

// The multiplier comes from one of a table of game settings.
float ExtraLevCreaModifier::GetMultiplier() {
    float v3 = (float)1.0;
    if (eModifier != 4) {
        return pLevCreaModMult[eModifier]->Float();
    }
    return v3;
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
  : BSExtraData(EXTRA_SAY_TO_TOPIC_INFO), pInfo(nullptr), pTopic(nullptr), nResponseID((uint32_t)0), bVoiceDone(false) {
}

ExtraSayToTopicInfo::ExtraSayToTopicInfo(TESTopicInfo* apTopicInfo)
  : BSExtraData(EXTRA_SAY_TO_TOPIC_INFO), pInfo(apTopicInfo), pTopic(nullptr), nResponseID((uint32_t)0),
    bVoiceDone(false) {}

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
    ExtraPoison* v4 = dynamic_cast<ExtraPoison*>(apCompare);
    return (bool)((uint8_t)((!v4) || (v4->pPoison != pPoison)));
}

ExtraRandomTeleportMarker::ExtraRandomTeleportMarker() : BSExtraData(59u), pMarker(nullptr) {}

bool ExtraRandomTeleportMarker::Compare(BSExtraData* apCompare) {
    ExtraRandomTeleportMarker* v4 = dynamic_cast<ExtraRandomTeleportMarker*>(apCompare);
    return (bool)((uint8_t)((!v4) || (v4->pMarker != pMarker)));
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

float* REF_PATROL_DATA::SetTimeAtRef(float* a1) {
    fTimeAtRef = *a1;
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
    float fHighestWaterHeight = -3.4028235e+38f;
    if (abValue) {
        int32_t icount = 0;
        WaterZoneMap.GetAt(apZone, icount);
        WaterZoneMap.SetAt(apZone, 1);
        if (fHighestWaterHeight < static_cast<double>(apZone->fWaterHeight)) {
            fHighestWaterHeight = apZone->fWaterHeight;
            pHighestWaterZone = apZone;
        }
    }
    else {
        int32_t icount = 0;
        WaterZoneMap.GetAt(apZone, icount);
        if (icount > 1) {
            WaterZoneMap.SetAt(apZone, icount - 1);
        }
        else {
            WaterZoneMap.RemoveAt(apZone);
            NiTMapItem<WaterZone*, int32_t>* pos = WaterZoneMap.GetFirstPos();
            fHighestWaterHeight = -3.4028235e+38f;
            while (pos) {
                int32_t izoneCount;
                WaterZone* pzone;
                WaterZoneMap.GetNext(pos, pzone, izoneCount);
                if (fHighestWaterHeight < static_cast<double>(pzone->fWaterHeight)) {
                    fHighestWaterHeight = pzone->fWaterHeight;
                    pHighestWaterZone = pzone;
                }
            }
        }
    }
}

LightRefData::LightRefData() : spLight(nullptr) {}

bool LightRefData::IsEqual(NiPointLight** apLight) {
    return (bool)((uint8_t)(static_cast<const void*>(spLight.m_pObject) == static_cast<const void*>(*apLight)));
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
    return ((cFlags & 1) != 0);
}

bool MapMarkerData::GetTravelLoc() {
    return ((cFlags & 2) != 0);
}

void MapMarkerData::SetTravelLoc(bool abVal) {
    if (abVal) {
        cFlags |= 2u;
    }
    else {
        cFlags &= (~2u);
    }
}

TESReputation* MapMarkerData::GetReputation() {
    return pReputation;
}

void MapMarkerData::SetReputation(TESReputation* pRep) {
    pReputation = pRep;
}

int32_t MapMarkerData::GetType() {
    return (int32_t)sType;
}

void MapMarkerData::ResetFlags() {
    cFlags = cOriginalFlags;
}

void MapMarkerData::SetIsVisible(uint8_t a2) {
    if (a2) {
        cFlags |= 1u;
    }
    else {
        cFlags &= (~1u);
    }
}

AudioMarker::AudioMarker()
  : fLayer2TriggerPerc(0.33000001311302185f), fLayer3TriggerPerc(0.6700000166893005f), unk18(NiPoint3_ZERO),
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

RagDollData::RagDollData() : iBoneCount(0), pBoneArray(nullptr), Bip01Rot(NiPoint3_ZERO) {}

void RagDollData::CreateRotation(TESObjectREFR* apRef) {
    const NiTransform* localTransform;
    NiNode* v3;
    NiNode* v4;
    NiAVObject* v6;
    if (apRef && apRef->IsActor()) {
        v4 = apRef->Get3D();
    }
    else {
        v4 = nullptr;
    }
    if (v4) {
        v3 = v4->IsNode();
    }
    else {
        v3 = nullptr;
    }
    if (v3) {
        v6 = v3->GetAtChecked(0);
        if (v6) {
            localTransform = &v6->GetLocalTransform();
            (&localTransform->m_Rotate)->ToEulerAnglesXYZ(Bip01Rot.x, Bip01Rot.y, Bip01Rot.z);
        }
    }
}

ExtraDismemberedLimbs::ExtraDismemberedLimbs()
  : BSExtraData(95), sLimbs(0), eCauseofDeath(CAUSE_OF_DEATH_NONE), pDeathObject(nullptr),
    eLastHitLimb(BGSBodyPart::LIMB_NONE), bEaten(false) {}

ExtraDismemberedLimbs::~ExtraDismemberedLimbs() {
    for (uint32_t uiIndex = 0; uiIndex < DismemberedLimbs.iSize; ++uiIndex) {
        delete DismemberedLimbs[uiIndex];
    }
}

void ExtraDismemberedLimbs::RemoveLimb(BGSBodyPart::LIMB_ENUM aeLimb, bool abRemoved) {
    for (uint32_t i = 0; i < DismemberedLimbs.iSize; ++i) {
        DismemberedLimb* pdismemberedLimb = DismemberedLimbs.operator[](i);
        if (pdismemberedLimb) {
            if (pdismemberedLimb->cLimb == aeLimb) {
                pdismemberedLimb->bLimbRemoved = abRemoved;
            }
        }
    }
}

DismemberedLimb* ExtraDismemberedLimbs::GetDismemberedLimb(uint32_t aiIndex) {
    return *static_cast<DismemberedLimb**>(&DismemberedLimbs.operator[](aiIndex));
}

uint32_t ExtraDismemberedLimbs::GetDismemberedLimbCount() {
    return (&DismemberedLimbs)->BSSimpleArray<DismemberedLimb*>::GetSize();
}

bool ExtraDismemberedLimbs::Dismembered(BGSBodyPart::LIMB_ENUM aeLimb) {
    return (sLimbs & (1 << aeLimb)) != 0;
}

void ExtraDismemberedLimbs::Dismember(TESObjectREFR* a1, BGSBodyPart::LIMB_ENUM a2, bool a3) {
    if (!Dismembered(a2)) {
        DismemberedLimb* pdismemberedLimb;
        sLimbs |= 1 << a2;
        DismemberedLimb* apMemory = static_cast<DismemberedLimb*>(ourMalloc(sizeof(DismemberedLimb)));
        if (apMemory) {
            pdismemberedLimb = ::new (apMemory) DismemberedLimb;
        }
        else {
            pdismemberedLimb = nullptr;
        }
        DismemberedLimb* pdismemberedLimb_2 = pdismemberedLimb;
        uint32_t iindex = DismemberedLimbs.Add(pdismemberedLimb_2);
        pdismemberedLimb_2->cLimb = static_cast<uint8_t>(a2);
        pdismemberedLimb_2->bLimbExploded = a3;
        pdismemberedLimb_2->bObjectArrayIdentical = false;
        if (a3) {
            pdismemberedLimb_2->bObjectArrayIdentical = true;
        }
        else {
            if (a1->IsActor() && a1 != pPlayer && a1->IsNpc()) {
                TESNPC* pnpc = static_cast<TESNPC*>(static_cast<Actor*>(a1)->GetActorBase());
                BSSimpleArray<TESBoundObject*, 1024>* p_ObjectArray = &pdismemberedLimb_2->ObjectArray;
                BipedAnim* v4 = a1->GetBiped();
                pnpc->BuildObjectArray(a1, v4, p_ObjectArray);
                if (iindex) {
                    for (int32_t i = iindex - 1; i >= 0; --i) {
                        DismemberedLimb* pprevDismemberedLimb = DismemberedLimbs.operator[](i);
                        if (!pprevDismemberedLimb->bObjectArrayIdentical) {
                            if (pprevDismemberedLimb->ObjectArray.CompareArrays(pdismemberedLimb_2->ObjectArray)) {
                                pdismemberedLimb_2->bObjectArrayIdentical = true;
                                pdismemberedLimb_2->ObjectArray.Clear(true);
                            }
                            return;
                        }
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
    uint32_t FormFlags = apDismemberedLimbs->DismemberedLimbs.iSize;
    if (FormFlags != DismemberedLimbs.iSize) {
        return false;
    }
    for (uint32_t i = 0; i < DismemberedLimbs.iSize; ++i) {
        DismemberedLimb* pdismemberedLimb1 = DismemberedLimbs.operator[](i);
        DismemberedLimb* pdismemberedLimb2 = apDismemberedLimbs->DismemberedLimbs.operator[](i);
        if (pdismemberedLimb1->cLimb != pdismemberedLimb2->cLimb) {
            return false;
        }
        if (pdismemberedLimb1->bLimbExploded != pdismemberedLimb2->bLimbExploded) {
            return false;
        }
        if (pdismemberedLimb1->bObjectArrayIdentical != pdismemberedLimb2->bObjectArrayIdentical) {
            return false;
        }
        if (pdismemberedLimb1->bLimbRemoved != pdismemberedLimb2->bLimbRemoved) {
            return false;
        }
        if (!pdismemberedLimb1->ObjectArray.CompareArrays(pdismemberedLimb2->ObjectArray)) {
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
    for (FACTION_RANK* m_item : pFactionChanges->ItemsUntilNull()) {
        if (apFaction == m_item->pFaction) {
            m_item->cRank = -1;
            return;
        }
    }
    if (!pFactionChanges) {
        BSSimpleList<FACTION_RANK*>* v2;
        BSSimpleList<FACTION_RANK*>* v4 =
            static_cast<BSSimpleList<FACTION_RANK*>*>(ourMalloc(sizeof(BSSimpleList<FACTION_RANK*>)));
        if (v4) {
            v2 = ::new (v4) BSSimpleList<FACTION_RANK*>;
        }
        else {
            v2 = nullptr;
        }
        pFactionChanges = v2;
    }
    FACTION_RANK* pfacrank = static_cast<FACTION_RANK*>(ourMalloc(sizeof(FACTION_RANK)));
    pfacrank->pFaction = apFaction;
    pfacrank->cRank = -1;
    pFactionChanges->AddHead(pfacrank);
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
    for (FACTION_RANK* m_item : pFactionChanges->ItemsUntilNull()) {
        if (apFaction == m_item->pFaction) {
            m_item->cRank = aiRank;
            return;
        }
    }
    FACTION_RANK* pnewRank = static_cast<FACTION_RANK*>(ourMalloc(sizeof(FACTION_RANK)));
    pnewRank->cRank = aiRank;
    pnewRank->pFaction = apFaction;
    pFactionChanges->AddHead(pnewRank);
}

void ExtraFactionChanges::ExpelFromAllFactions(TESActorBase* apActorBase) {
    bool bexpel;
    using expelledmap_t = BSMap<TESFaction*, bool>;
    expelledmap_t expelledmap(37u);
    BSSimpleList<FACTION_RANK*>* plist = pFactionChanges;
    while (plist && plist->m_item) {
        FACTION_RANK* prankdata = plist->m_item;
        plist = plist->m_pkNext;
        prankdata->cRank = -1;
        expelledmap.SetAt(prankdata->pFaction, true);
    }
    if (apActorBase) {
        plist = &apActorBase->listFactions;
        while (plist && plist->m_item) {
            FACTION_RANK* prankdata_2 = plist->m_item;
            plist = plist->m_pkNext;
            bexpel = false;
            if (!expelledmap.GetAt(prankdata_2->pFaction, bexpel) || !bexpel) {
                FACTION_RANK* prank = static_cast<FACTION_RANK*>(ourMalloc(sizeof(FACTION_RANK)));
                prank->pFaction = prankdata_2->pFaction;
                prank->cRank = -1;
                pFactionChanges->AddHead(prank);
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
    (&spActorCause)->operator=(apCause);
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
    ExtraCharge* v4 = dynamic_cast<ExtraCharge*>(apCompare);
    if (!v4) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return (bool)((uint8_t)(v4->fCharge != fCharge));
}

ExtraFollowerSwimBreadcrumb::ExtraFollowerSwimBreadcrumb() {}

ExtraFollowerSwimBreadcrumbs::ExtraFollowerSwimBreadcrumbs()
  : BSExtraData(0x8Bu), eLeaderState(BREADCRUMB_STATE_INVALID), LeaderLocation(NiPoint3_ZERO), LeaderNavMeshID(0) {}

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
    ExtraFollowerSwimBreadcrumb* a2 = nullptr;
    CrumbList.SetItem(a2);
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
    bool baddedCrumb = false;
    if (aeLeaderState != eLeaderState) {
        if (eLeaderState) {
            ExtraFollowerSwimBreadcrumb* pcrumb;
            void* apMemory = ourMalloc(sizeof(ExtraFollowerSwimBreadcrumb));
            if (apMemory) {
                pcrumb = ::new (apMemory) ExtraFollowerSwimBreadcrumb;
            }
            else {
                pcrumb = nullptr;
            }
            ExtraFollowerSwimBreadcrumb* pcrumb_2 = pcrumb;
            pcrumb->StartLocation.x = LeaderLocation.x;
            pcrumb->StartLocation.y = LeaderLocation.y;
            pcrumb->StartLocation.z = LeaderLocation.z;
            pcrumb_2->StartNavMeshID = LeaderNavMeshID;
            pcrumb_2->EndLocation = aLeaderLocation;
            pcrumb_2->EndNavMeshID = aLeaderNavMeshID;
            pcrumb_2->bEnteringWater = aeLeaderState == BREADCRUMB_STATE_IN_WATER;
            CrumbList.AddTail(pcrumb_2);
            baddedCrumb = true;
        }
    }
    LeaderLocation = aLeaderLocation;
    LeaderNavMeshID = aLeaderNavMeshID;
    eLeaderState = aeLeaderState;
    return baddedCrumb;
}

ExtraFriendHits::ExtraFriendHits() : BSExtraData(69u) {}

ExtraFriendHits::~ExtraFriendHits() {}

int32_t ExtraFriendHits::GetHitCount() {
    RemoveOldHits();
    return (&Hits)->iSize;
}

// Records a hit now unless the last one is younger than fFriendMinimumLastHitTime, so that repeated friendly fire
// inside that interval counts once. The elapsed time is kept in a double, so the comparison is a double one.
void ExtraFriendHits::AddHit() {
    RemoveOldHits();
    int32_t iCount = static_cast<int32_t>(Hits.iSize);
    if (iCount > 0) {
        CombatTimeStamp LastHit = Hits[iCount - 1];
        double dElapsedTime = LastHit.ElapsedTime();
        // Only a strictly older last hit lets a new one in; NaN counts as too recent, as the game does.
        if (fFriendMinimumLastHitTime.Float() > dElapsedTime) {
            return;
        }
    }
    float fTimer = CombatTimer::GetTimer();
    CombatTimeStamp NewHit;
    NewHit.~CombatTimeStamp();
    ::new (&NewHit) CombatTimeStamp(fTimer);
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

ExtraHasNoRumors::ExtraHasNoRumors(bool abNoRumors)
  : BSExtraData(EXTRA_NO_RUMORS), bNoRumors((bool)((int8_t)abNoRumors)) {}

ExtraHasNoRumors::~ExtraHasNoRumors() {}

ExtraHotKey::ExtraHotKey(char acHotKey) : BSExtraData(EXTRA_HOT_KEY), chotkey((int8_t)acHotKey) {}

bool ExtraHotKey::Compare(BSExtraData* apCompare) {
    ExtraHotKey* v4 = dynamic_cast<ExtraHotKey*>(apCompare);
    if (!v4) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return (bool)((uint8_t)(chotkey != v4->chotkey));
}

ExtraModdingItem::ExtraModdingItem(bool abFlag)
  : BSExtraData(EXTRA_WEAPON_IS_MODDING), bIsModding((bool)((int8_t)abFlag)) {}

bool ExtraModdingItem::Compare(BSExtraData* apCompare) {
    char* v4 = static_cast<char*>(
        FNVD_DYNAMIC_CAST_VOID(ExtraModdingItem)(apCompare, RTTI_BSExtraData, RTTI_ExtraModdingItem)
    );
    if (!v4) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return bIsModding != v4[12];
}

ExtraObjectHealth::ExtraObjectHealth() : BSExtraData(86), fHealth(0.0f) {}

ExtraObjectHealth::ExtraObjectHealth(float afHealth) : BSExtraData(EXTRA_OBJECT_HEALTH), fHealth(afHealth) {}

ExtraOriginalReference::ExtraOriginalReference(TESObjectREFR* apReference)
  : BSExtraData(EXTRA_ORIGINAL_REFERENCE), pReference(apReference) {}

bool ExtraOriginalReference::Compare(BSExtraData* apCompare) {
    ExtraOriginalReference* v4 = dynamic_cast<ExtraOriginalReference*>(apCompare);
    if (!v4) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return (bool)((uint8_t)(pReference != v4->pReference));
}

ExtraPackageData::ExtraPackageData() : BSExtraData(112u), pActorPackageData(nullptr) {}

ExtraPackageData::ExtraPackageData(ActorPackageData* apActorPackageData)
  : BSExtraData(EXTRA_PACKAGE_DATA), pActorPackageData(apActorPackageData) {}

ExtraPatrolRefInUseData::ExtraPatrolRefInUseData(TESObjectREFR* apUser)
  : BSExtraData(EXTRA_PATROL_REF_IN_USE_DATA), User(apUser ? apUser->TESForm::GetFormID() : 0) {}

ExtraPatrolRefInUseData::~ExtraPatrolRefInUseData() {}

bool ExtraPatrolRefInUseData::Compare(BSExtraData* apCompare) {
    return (bool)((uint8_t)(User != static_cast<ExtraPatrolRefInUseData*>(apCompare)->User));
}

bool ExtraPatrolRefInUseData::GetCanBeUsedBy(const TESObjectREFR* apPotentialUser) {
    if (apPotentialUser && (apPotentialUser->TESForm::GetFormID() == User)) {
        return true;
    }
    Actor* formByNumericID = static_cast<Actor*>(TESForm::GetFormByNumericID(User));
    if ((!formByNumericID) || (formByNumericID->IsDead(false) || formByNumericID->GetDisabledOrDeleted())) {
        User = (uint32_t)0;
        return true;
    }
    else {
        if (formByNumericID->IsActor() &&
            (formByNumericID->IsRestrained() ||
             (formByNumericID->IsUnConscious() ||
              (formByNumericID->GetCurrentProcessType() && (!formByNumericID->GetRunsInLow()))))) {
            User = (uint32_t)0;
            return true;
        }
        else {
            return false;
        }
    }
}

ExtraScale::ExtraScale(float afScale) : BSExtraData(EXTRA_SCALE), fScale(afScale) {}

bool ExtraScale::Compare(BSExtraData* apCompare) {
    ExtraScale* v4 = dynamic_cast<ExtraScale*>(apCompare);
    if (!v4) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return (bool)((uint8_t)(v4->fScale != fScale));
}

void ExtraSecuritronFace::SetPersonality(const char* asPersonality) {
    msPersonality = asPersonality;
}

void ExtraSecuritronFace::SetMood(const char* asMood) {
    msMood = asMood;
}

void ExtraSecuritronFace::ApplyFace(NiAVObject* apMesh) {
    FNVD_INLINED_LOCAL(NiFixedString, apString, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSStringT<char>, Personality, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSStringT<char>, Mood, std::nullopt, std::nullopt);
    if (apMesh) {
        NiFixedString* v4 = ::new (static_cast<void*>(&apString)) NiFixedString("Screen01:0");
        NiAVObject* pFace = apMesh->GetObjectByName(*v4);
        apString.~NiFixedString();
        if (pFace) {
            NiPointer<NiSourceTexture> TexPtr(static_cast<NiSourceTexture*>(nullptr));
            GetPersonality(&Personality);
            GetMood(&Mood);
            if (Personality.GetLength()) {
                if (Mood.GetLength()) {
                    char apDest[268];
                    char path[268];
                    BSsprintf(
                        apDest, 260, "Textures\\creatures\\Securitron\\%s_%s.dds", Personality.String(), Mood.String()
                    );
                    SwapPlatformLanguageTexturePath(apDest, path, 260u);
                    TES::pInstance->CreateTextureImage(path, TexPtr, false, false);
                }
            }
            int32_t Type = NiShadeProperty::GetType();
            BSShaderNoLightingProperty* pFaceShader =
                static_cast<BSShaderNoLightingProperty*>(pFace->GetProperty(Type));
            if (pFaceShader) {
                NiSourceTexture* v3 = TexPtr.m_pObject;
                pFaceShader->SetBaseMap(v3);
            }
            Mood.~BSStringT<char>();
            Personality.~BSStringT<char>();
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
    MobileObject* pnewactor = nullptr;
    if (apRef) {
        MobileObject* pnewactor_2;
        TESObjectCELL* psavecell;
        MobileObject* apMemory = static_cast<MobileObject*>(ourMalloc(sizeof(MobileObject)));
        if (apMemory) {
            pnewactor_2 = ::new (apMemory) MobileObject;
        }
        else {
            pnewactor_2 = nullptr;
        }
        pnewactor = pnewactor_2;
        pnewactor_2->SetObjectReference(apTalkact);
        bool RefPersists = apRef->GetRefPersists();
        pnewactor_2->SetRefPersists(RefPersists);
        pnewactor_2->SetLocationOnReference(*apRef->QLocationOnReference());
        pnewactor_2->SetAngleOnReference(apRef->GetAngleOnReference());
        pnewactor_2->SetTalkingActivatorRef(apRef);
        pnewactor_2->SetTalkingActivator(true);
        pnewactor_2->UpdateTopicInfoSayOnce();
        if ((apRef->GetParentCell() && apRef->GetParentCell()->GetInterior()) ||
            (psavecell = apRef->GetSaveParentCell()) == nullptr) {
            pnewactor_2->SetParentCell(apRef->GetParentCell());
        }
        else {
            (&pnewactor_2->m_Extra)->SetPersistentCell(psavecell);
        }
        BaseProcess* pproc = pPlayer->pCurrentProcess;
        if (pproc) {
            // A new process of the player's process level.
            BaseProcess* pcopyproc = nullptr;
            switch (pproc->eLevel) {
                case PROCESS_TYPE_HIGH: {
                    HighProcess* pMemory = static_cast<HighProcess*>(ourMalloc(sizeof(HighProcess)));
                    pcopyproc = pMemory ? ::new (pMemory) HighProcess : nullptr;
                    break;
                }
                case PROCESS_TYPE_MIDDLE_HIGH: {
                    MiddleHighProcess* pMemory = static_cast<MiddleHighProcess*>(ourMalloc(sizeof(MiddleHighProcess)));
                    pcopyproc = pMemory ? ::new (pMemory) MiddleHighProcess : nullptr;
                    break;
                }
                case PROCESS_TYPE_MIDDLE_LOW: {
                    MiddleLowProcess* pMemory = static_cast<MiddleLowProcess*>(ourMalloc(sizeof(MiddleLowProcess)));
                    pcopyproc = pMemory ? ::new (pMemory) MiddleLowProcess : nullptr;
                    break;
                }
                case PROCESS_TYPE_LOW: {
                    LowProcess* pMemory = static_cast<LowProcess*>(ourMalloc(sizeof(LowProcess)));
                    pcopyproc = pMemory ? ::new (pMemory) LowProcess : nullptr;
                    break;
                }
                default: {
                    break;
                }
            }
            pnewactor_2->SetCurrentAIProcess(pcopyproc);
            PROCESS_TYPE ProcessLevel = pcopyproc->eLevel;
            ReferenceProcessLists.AddReference(pnewactor_2, ProcessLevel, false, false, nullptr);
        }
    }
    return pnewactor;
}

ExtraTerminalState::ExtraTerminalState()
  : BSExtraData(EXTRA_TERMINALSTATE), cState((uint8_t)0), cLockOverride((uint8_t)(-1)) {}

ExtraTerminalState::~ExtraTerminalState() {}

bool ExtraTerminalState::IsLockedState() {
    return (bool)((uint8_t)((!cState) && (((int8_t)cLockOverride) == (-1))));
}

ExtraTimeLeft::ExtraTimeLeft(float afTime) : BSExtraData(EXTRA_TIMELEFT), fTime(afTime) {}

bool ExtraTimeLeft::Compare(BSExtraData* apCompare) {
    ExtraTimeLeft* v4 = dynamic_cast<ExtraTimeLeft*>(apCompare);
    if (!v4) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return (bool)((uint8_t)(v4->fTime != fTime));
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
    ExtraUses* v4 = dynamic_cast<ExtraUses*>(apCompare);
    if (!v4) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return (bool)((uint8_t)(cUses != v4->cUses));
}

ExtraHeadingTarget::ExtraHeadingTarget() : BSExtraData(EXTRA_HEAD_TRACK_TARGET), pHeadingTarget(nullptr) {}

ExtraHeadingTarget::ExtraHeadingTarget(TESObjectREFR* apHeadingTarget)
  : BSExtraData(EXTRA_HEAD_TRACK_TARGET), pHeadingTarget(apHeadingTarget) {}

ExtraHeadingTarget::~ExtraHeadingTarget() {
    pHeadingTarget = nullptr;
}

ExtraSeed::ExtraSeed(uint8_t aiSeed) : BSExtraData(EXTRA_SEED), iSeed((uint8_t)((int8_t)aiSeed)) {}

ExtraSeed::~ExtraSeed() {}

bool ExtraSeed::Compare(BSExtraData* apCompare) {
    ExtraSeed* v4 = dynamic_cast<ExtraSeed*>(apCompare);
    if (!v4) {
        return true;
    }
    if (BSExtraData::Compare(apCompare)) {
        return true;
    }
    return (bool)((uint8_t)(iSeed != v4->iSeed));
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
    ExtraWaterLightRefs* pcomp = dynamic_cast<ExtraWaterLightRefs*>(apCompare);
    if (!pcomp) {
        return true;
    }
    int32_t v3 = RefList.ItemsInList();
    if (v3 != pcomp->RefList.ItemsInList()) {
        return true;
    }
    for (BSSimpleList<TESObjectREFR*>* plist = &pcomp->RefList; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
        if (!RefList.IsInList(plist->m_item)) {
            return true;
        }
    }
    return false;
}

ExtraWaterLightRefs::~ExtraWaterLightRefs() {
    RefList.RemoveAll();
}

void ExtraWaterLightRefs::Copy(BSExtraData* apCopy) {
    ExtraWaterLightRefs* pcopy = dynamic_cast<ExtraWaterLightRefs*>(apCopy);
    if (pcopy) {
        ClearReferenceData();
        for (BSSimpleList<TESObjectREFR*>* plist = &pcopy->RefList; plist && !plist->IsEmpty();
             plist = plist->m_pkNext) {
            AddReference(plist->m_item);
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

ExtraReservedMarkers::ExtraReservedMarkers() : BSExtraData(0x82u), iReservedMarkers((uint32_t)0) {}

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
    ExtraGuardedRefData* pcomp = static_cast<ExtraGuardedRefData*>(apCompare);
    if (pcomp->Guards.iSize != Guards.iSize) {
        return true;
    }
    for (uint32_t i = 0; i < Guards.iSize; ++i) {
        if (pcomp->Guards.Find(Guards.operator[](i), 0, &BSSimpleArray<uint32_t>::DefaultEqualFunctionByAddress) ==
            -1) {
            return true;
        }
    }
    return false;
}

void ExtraGuardedRefData::AddGuard(Actor* apActor) {
    uint32_t inumID = apActor->iFormID;
    if (Guards.Find(inumID, 0, &BSSimpleArray<uint32_t>::DefaultEqualFunctionByAddress) == -1) {
        Guards.Add(inumID);
    }
}

void ExtraGuardedRefData::NotifyGuardsAboutActivation(TESObjectREFR* apObject, TESObjectREFR* apActionRef) {
    for (uint32_t i = 0; i < Guards.iSize; ++i) {
        Actor* pactor = static_cast<Actor*>(TESForm::GetFormByNumericID(Guards.operator[](i)));
        if (pactor) {
            if (pactor->IsReference() && pactor->IsActor()) {
                BaseProcess* pproc = pactor->pCurrentProcess;
                if (pproc) {
                    pproc->NotifyGuardAboutActivation(pactor, apActionRef, apObject);
                }
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
    TESFile* File = apOwner->GetFile(-1);
    TESForm::AddCompileIndex(tempID, File);
    TESForm* FormByNumericID = TESForm::GetFormByNumericID(tempID);
    pLinkedDoor = dynamic_cast<TESObjectREFR*>(FormByNumericID);
    if (pLinkedDoor) {
        if (pLinkedDoor->GetObjectReference()->GetFormType() == DOOR_ID) {
            if (!std::isfinite(position.x) ||
                (!std::isfinite(position.y) ||
                 (!std::isfinite(position.z) ||
                  (std::isnan(position.x) || (std::isnan(position.y) || std::isnan(position.z)))))) {
                Warning("MASTERFILE: Corrupt location found in teleport data, setting to (0, 0, 0).");
                position = NiPoint3_ZERO;
            }
            if (!std::isfinite(rotation.x) ||
                (!std::isfinite(rotation.y) ||
                 (!std::isfinite(rotation.z) ||
                  (std::isnan(rotation.x) || (std::isnan(rotation.y) || std::isnan(rotation.z)))))) {
                Warning("MASTERFILE: Corrupt angle found in teleport data, setting to (0, 0, 0).");
                rotation = NiPoint3_ZERO;
            }
            return true;
        }
        else {
            TESBoundObject* ObjectReference = pLinkedDoor->GetObjectReference();
            int32_t X_4 = pLinkedDoor->GetObjectReference()->iFormID;
            const char* X = ObjectReference->GetFormEditorID();
            int32_t FormType = pLinkedDoor->GetObjectReference()->GetFormType();
            Warning(
                "MASTERFILE: Linked door (%08X) in teleport data points to invalid object (%s %s(%08X)).", tempID,
                formEnumString[FormType].pFormString, X, X_4
            );
            pLinkedDoor = nullptr;
            return false;
        }
    }
    else {
        Warning("MASTERFILE: Could not find linked door (%08X) in teleport data init.", tempID);
        return false;
    }
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
