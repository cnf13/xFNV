#include "Fallout Shared/Script.h"

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSCore/TLSData.h"
#include "BSDevices/Controls.h"
#include "BSDevices/XPrefSettings.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkShape.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSShader/BSShaderManager.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSEncounterZone.h"
#include "Fallout Shared/BGSListForm.h"
#include "Fallout Shared/BGSMenuIcon.h"
#include "Fallout Shared/BGSMessage.h"
#include "Fallout Shared/BGSOpenCloseForm.h"
#include "Fallout Shared/BGSRagdoll.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/FOScriptFunctions.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/SCRIPT_PARAMETER.h"
#include "Fallout Shared/ScriptCompileData.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESIdleManager.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESRace.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESScriptFunctions.h"
#include "Fallout Shared/TESScriptRunManager.h"
#include "Fallout Shared/TESScriptVariable.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout Shared/TESWeather.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/HitData.h"
#include "Fallout/AI/Combat/VATS.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/FixedStrings.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Interface/FOHUDEffectManager.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "SDK/MSVC/rtti.h"

#include <cmath>
#include <cstdint>
#include <cstring>

class TESQuest;

bool Cmd_GetIsCurrentWeather(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    afReturnValue = 0.0;
    TESForm* v6 = nullptr;
    if ((apParam1) && (((int32_t)static_cast<TESForm*>(apParam1)->GetFormType()) == 53)) {
        v6 = static_cast<TESForm*>(apParam1);
    }

    Sky* instance = Sky::GetInstance();
    if (static_cast<const void*>(v6) == static_cast<const void*>(instance->GetCurrentWeather())) {
        afReturnValue = 1.0;
    }
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("GetIsCurrentWeather >> %0.2f", afReturnValue);
    }
    return true;
}

bool Cmd_IsWeaponSkillType(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    TESObjectWEAP* v6 = nullptr;
    afReturnValue = 0.0;
    TESBoundObject* objectReference = apRefObject->TESObjectREFR::GetObjectReference();
    if (((int32_t)objectReference->GetFormType()) == 40) {
        v6 = static_cast<TESObjectWEAP*>(apRefObject->TESObjectREFR::GetObjectReference());
    }
    if (v6 || (reinterpret_cast<intptr_t>(apParam1) != 45)) {
        if (!v6) {
            return false;
        }
    }
    else {
        afReturnValue = 1.0;
    }
    if ((v6) && (reinterpret_cast<intptr_t>(apParam1) == v6->TESObjectWEAP::GetWeaponSkill())) {
        afReturnValue = 1.0;
    }

    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("Is Weapon Skill Type >> %0.2f", afReturnValue);
    }
    return true;
}

bool Cmd_SameFactionAsPC(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    return Cmd_SameFaction(apRefObject, pPlayer, nullptr, afReturnValue);
}

bool Cmd_SameFaction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    afReturnValue = 0.0;
    Actor* pActor = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        pActor = static_cast<Actor*>(apRefObject);
    }

    Actor* pOther = static_cast<Actor*>(apParam1);
    if (!pActor || !pOther) {
        return true;
    }
    TESActorBase* pActorBase = pActor->GetActorBase();
    TESActorBase* pOtherBase = pOther->GetActorBase();
    if ((pActorBase) && (pOtherBase)) {
        for (BSSimpleList<FACTION_RANK*>* pNode = pActorBase->GetFactionList();
             pNode && !pNode->IsEmpty() && 1.0 != afReturnValue; pNode = pNode->GetNext()) {
            TESFaction* pFaction = pNode->GetItem()->pFaction;
            if ((pFaction) && (pOtherBase->GetFactionRank(pFaction, pOther == pPlayer) != -1)) {
                afReturnValue = 1.0;
            }
        }
    }

    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("SameFaction >> %0.2f", afReturnValue);
    }
    return true;
}

bool Cmd_SetArgTo0AndReturnTrue(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    afReturnValue = 0.0;
    return true;
}

bool Cmd_SameRaceAsPC(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    return Cmd_SameRace(apRefObject, pPlayer, nullptr, afReturnValue);
}

bool Cmd_SameSexAsPC(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    return Cmd_SameSex(apRefObject, pPlayer, nullptr, afReturnValue);
}

bool Cmd_SameRace(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    afReturnValue = 0.0;
    TESNPC* v8 = nullptr;
    if (apRefObject) {
        TESBoundObject* ObjectReference = apRefObject->GetObjectReference();
        if (ObjectReference->GetFormType() == 42) {
            v8 = static_cast<TESNPC*>(apRefObject->GetObjectReference());
        }
    }
    TESNPC* v9 = nullptr;
    if (apParam1) {
        TESBoundObject* v5 = static_cast<TESObjectREFR*>(apParam1)->GetObjectReference();
        if (v5->GetFormType() == 42) {
            v9 = static_cast<TESNPC*>(static_cast<TESObjectREFR*>(apParam1)->GetObjectReference());
        }
    }
    if ((v8) && (v9)) {
        TESRace* Race = v8->GetRace();
        if (Race == v9->GetRace()) {
            afReturnValue = 1.0;
        }
    }

    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("SameRace >> %0.2f", afReturnValue);
    }
    return true;
}

bool Cmd_SameSex(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    afReturnValue = 0.0;
    TESActorBase* v8 = nullptr;
    if (apRefObject) {
        TESBoundObject* ObjectReference = apRefObject->GetObjectReference();
        if (ObjectReference->GetFormType() == 42) {
            v8 = static_cast<TESActorBase*>(apRefObject->GetObjectReference());
        }
    }
    TESActorBase* v9 = nullptr;
    if (apParam1) {
        TESBoundObject* v5 = static_cast<TESObjectREFR*>(apParam1)->GetObjectReference();
        if (v5->GetFormType() == 42) {
            v9 = static_cast<TESActorBase*>(static_cast<TESObjectREFR*>(apParam1)->GetObjectReference());
        }
    }
    if ((v8) && (v9)) {
        TESActorBase::SEX Sex = v8->GetSex();
        if (Sex == v9->GetSex()) {
            afReturnValue = 1.0;
        }
    }

    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("SameSex >> %0.2f", afReturnValue);
    }
    return true;
}

// The process, item and list getters are trivial accessors.
bool Cmd_IsWeaponInList(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    afReturnValue = 0.0;
    BGSListForm* pList = static_cast<BGSListForm*>(apParam1);
    Actor* pActor = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        pActor = static_cast<Actor*>(apRefObject);
    }

    if ((pActor) && (pActor->pCurrentProcess) && (pList)) {
        BaseProcess* pProcess = pActor->pCurrentProcess;
        ItemChange* pWeaponItem = pProcess->GetCurrentWeapon();
        TESForm* pWeapon = nullptr;
        if ((pWeaponItem) && (pWeaponItem->pContainerObj) && (pWeaponItem->pContainerObj->GetFormType() == WEAP_ID)) {
            pWeapon = pWeaponItem->pContainerObj;
        }

        if (!pWeapon) {
            pWeapon = pFists;
        }
        if (pWeapon) {
            TESForm* pItem = pWeapon;
            if (pList->GetFormList()->IsInList(pItem)) {
                afReturnValue = 1.0;
            }
        }
    }

    if (GetThreadTLSData()->bSystemOutput) {
        Interface::PrintLine("IsWeaponInList >> %0.2f", afReturnValue);
    }
    return true;
}

bool Cmd_GetDefaultOpen(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    if (!apRefObject) {
        return false;
    }
    afReturnValue = 0.0;
    if (apRefObject->GetAction(OBJECT_ACTION_OPEN_BY_DEFAULT)) {
        afReturnValue = 1.0;
    }
    return true;
}

bool Cmd_GetFactionRelation(TESObjectREFR* apRefObject, Actor* apParam1, void* apParam2, double& afReturnValue) {
    afReturnValue = 0.0;
    if ((apRefObject) && (apRefObject->IsActor())) {
        Actor* pActor = static_cast<Actor*>(apRefObject);
        bool bFromCrime = false;
        afReturnValue =
            static_cast<double>(static_cast<int32_t>(pActor->GetFactionFightReaction(apParam1, bFromCrime)));
        if (GetThreadTLSData()->bSystemOutput) {
            // The result is truncated to 64 bits and the low 32 bits are printed.
            Interface::PrintLine("Faction Relation  >> %d", static_cast<int32_t>(static_cast<int64_t>(afReturnValue)));
        }
    }

    return true;
}

bool Cmd_IsLastIdlePlayed(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    afReturnValue = 0.0;
    if ((apRefObject) && (apRefObject->IsActor())) {
        BaseProcess* CurrentAIProcess = static_cast<Actor*>(apRefObject)->pCurrentProcess;
        if (CurrentAIProcess->GetLastIdlePlayed() == apParam1) {
            afReturnValue = 1.0;
        }
        if (GetThreadTLSData()->bSystemOutput) {
            Interface::PrintLine("The last idle played was  >> %d", afReturnValue);
        }
    }

    return true;
}

void ClearPendingReferenceLists() {
    Script::ScriptRefListCrit.Enter(nullptr);
    Script::ReferencesToEnable.RemoveAll();
    Script::ReferencesToDisable.RemoveAll();
    Script::ReferencesToDelete.RemoveAll();
    Script::ReferencesToFadeDisable.RemoveAll();
    Script::ScriptRefListCrit.Leave();
}

Script::Script() {
    Script::InitializeData();
}

void Script::ClearData() {
    ourFree(m_data);
    ourFree(m_text);
    ClearVariableList();
    ClearAllReferenceObjects();
}

void Script::InitItem() {
    // the engine passes the script itself as the owner
    InitItem(this);
}

void Script::Init(ScriptCompileData* apCompileData) {
    InitItem();
    if (apCompileData->cScriptName.GetLength() != 0) {
        if (NeedsUpdate(apCompileData)) {
            SetFormEditorID(apCompileData->cScriptName.pString);
            ourMemcpy(&m_header, &apCompileData->header, sizeof(SCRIPT_HEADER));
            SetCompileData(m_header.dataSize, apCompileData->pOutput);
            ClearAllReferenceObjects();
            ClearVariableList();
            if (m_header.dataSize != 0) {
                CopyReferencedObjectList(apCompileData->GetReferencedObjectList(), &listRefObjects, this);
                CopyVariableList(apCompileData->GetVariableList(), &listVariables, this);
            }
            SetDelete(false);
            SetAltered(true);
        }
    }
    else {
        SetDelete(true);
    }
}

void Script::ClearVariableList() {
    BSSimpleList<ScriptVariable*>* node = GetVariableList();
    while (node != nullptr && !node->IsEmpty()) {
        ScriptVariable* variable = node->GetItem();
        if (variable != nullptr) {
            delete variable;
        }
        node->RemoveHead();
    }
}

void Script::SetCompileData(uint32_t auiDataSize, char* apCompileData) {
    ourFree(m_data);
    m_data = nullptr;
    m_header.dataSize = auiDataSize;
    if (auiDataSize != 0) {
        m_data = static_cast<char*>(ourCalloc(1, auiDataSize));
        ourMemcpy(m_data, apCompileData, auiDataSize);
    }
}

void Script::SetText(const char* newText) {
    if (m_text != nullptr) {
        ourFree(m_text);
    }
    if (newText != nullptr) {
        uint32_t uiLength = static_cast<uint32_t>(std::strlen(newText));
        m_text = static_cast<char*>(ourCalloc(1, uiLength + 1));
        ourMemcpy(m_text, newText, uiLength);
    }
    else {
        m_text = nullptr;
    }
}

const char* Script::GetVariableName(uint32_t auiID) {
    BSSimpleList<ScriptVariable*>* node = GetVariableList();
    while (node != nullptr) {
        ScriptVariable* variable = node->GetItem();
        if (variable == nullptr) {
            break;
        }
        node = node->GetNext();
        if (variable->data.uiID == auiID) {
            return variable->cName.String();
        }
    }
    const char* editorID = GetFormEditorID();
    Warning("SCRIPTS: Trying to access local variable %d in script '%s' -- variable not found.\r\n", auiID, editorID);
    return nullptr;
}

char Script::FindVariable(const char* varName, uint32_t* auiID) {
    BSSimpleList<ScriptVariable*>* node = GetVariableList();
    while (node != nullptr) {
        ScriptVariable* variable = node->GetItem();
        if (variable == nullptr) {
            break;
        }
        node = node->GetNext();
        if (BSstricmp(variable->cName.String(), varName) == 0) {
            *auiID = variable->data.uiID;
            return variable->data.bIsInteger ? 's' : 'f';
        }
    }
    *auiID = 0;
    return 0;
}

void Script::SetIsCompiled(bool abCompiled) {
    m_header.bIsCompiled = (bool)((uint8_t)abCompiled);
}

void Script::AddProfilerTime(float afTime) {
    fProfilerTimer = (fProfilerTimer + afTime);
}

void Script::SetOwnerQuest(TESQuest* apQuest) {
    pOwnerQuest = apQuest;
}

bool Script::GetIsCompiled() {
    return (bool)((uint8_t)m_header.bIsCompiled);
}

bool Script::GetInZoneConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    TESWorldSpace* worldSpace;
    TESObjectCELL* parentCell;
    BGSEncounterZone* encounterZone;
    afReturnValue = 0.0;
    if ((apRefObject) && (apParam1)) {
        encounterZone = nullptr;
        parentCell = apRefObject->TESObjectREFR::GetParentCell();
        if (parentCell) {
            encounterZone = parentCell->GetEncounterZone();
        }
        if (!encounterZone) {
            worldSpace = apRefObject->GetWorldSpace();
            if (worldSpace) {
                encounterZone = worldSpace->GetEncounterZone();
            }
        }
        if (encounterZone == apParam1) {
            afReturnValue = 1.0;
        }
    }

    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("GetInZone >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetAlarmedConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    const char* currentPackageName;
    afReturnValue = 0.0;
    Actor* v6 = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        v6 = static_cast<Actor*>(apRefObject);
    }

    if ((v6) && (v6->GetCurrentPackageName())) {
        currentPackageName = v6->GetCurrentPackageName();
        if (!BSstricmp(currentPackageName, "Alarm")) {
            afReturnValue = 1.0;
        }
    }

    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("GetAlarmed >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::IsWeaponOutConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    Actor* v5 = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        v5 = static_cast<Actor*>(apRefObject);
    }

    if ((v5) && (v5->IsWeaponDrawn())) {
        afReturnValue = 1.0;
    }

    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("Is Weapon Out >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetOpenStateConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (apRefObject) {
        afReturnValue = (double)BGSOpenCloseForm::GetOpenState(apRefObject);
    }
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("GetOpenState >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetRadiationLevel(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    double v7;
    double v8;
    BaseProcess* v9;
    BaseProcess* v10;
    BaseProcess* currentProcess;
    afReturnValue = 0.0;
    if (!apRefObject) {
        return true;
    }
    if (!apRefObject->IsActor()) {
        return true;
    }
    afReturnValue = 0.0;
    if (static_cast<Actor*>(apRefObject)->MobileObject::GetCurrentAIProcess()) {
        currentProcess = static_cast<Actor*>(apRefObject)->MobileObject::GetCurrentAIProcess();
        v10 = static_cast<Actor*>(apRefObject)->MobileObject::GetCurrentAIProcess();
        v9 = static_cast<Actor*>(apRefObject)->MobileObject::GetCurrentAIProcess();
        v8 = (double)v9->GetRadiationWaterDelta();
        v7 = (double)v10->GetRadiationMagicDelta();
        afReturnValue = (currentProcess->GetRadiationDelta() + v7) + v8;
    }
    if (!Script::GetThreadSystemOutput()) {
        return true;
    }
    double v6 = afReturnValue;
    const char* baseFullName = apRefObject->TESObjectREFR::GetFullName();
    Interface::PrintLine("%s radiation level is %.2f", baseFullName, v6);
    return true;
}

bool Script::IsCombatTargetConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if ((apRefObject) &&
        (apRefObject->IsActor() &&
         static_cast<Actor*>(apRefObject)->IsInCombatWithActor(static_cast<Actor*>(apParam1)))) {
        afReturnValue = 1.0;
    }

    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("IsCombatTarget >> %.2f", afReturnValue);
    }
    return true;
}

bool Script::GetActorsInHighConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    afReturnValue = (double)((int32_t)(&ReferenceProcessLists)->GetNumberOfActorsInHigh());
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("GetActorsInHigh >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::HasLoaded3DConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (!apRefObject) {
        return true;
    }
    if (apRefObject->Get3D()) {
        afReturnValue = 1.0;
    }
    return true;
}

bool Script::GetFatiguePercentageConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    Actor* v5 = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        v5 = static_cast<Actor*>(apRefObject);
    }

    if (!v5) {
        return true;
    }
    afReturnValue = (double)v5->GetFatiguePercentage();
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("GetFatiguePercentage >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetHealthPercentageConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    Actor* v5 = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        v5 = static_cast<Actor*>(apRefObject);
    }

    if (!v5) {
        return true;
    }
    afReturnValue = (double)v5->GetHealthPercentage();
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("GetHealthPercentage >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetScaleConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    double scale;
    if (apRefObject) {
        afReturnValue = (double)apRefObject->fRefScale;
        if (Script::GetThreadSystemOutput()) {
            scale = (double)apRefObject->GetScale();
            Interface::PrintLine("GetScale >> %0.2f (with base %0.2f)", afReturnValue, scale);
        }
    }
    return true;
}

bool Script::GetIsLockBrokenConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    LockData* lock;
    afReturnValue = 0.0;
    if (apRefObject) {
        lock = apRefObject->GetLock();
        if (lock) {
            afReturnValue = (double)lock->IsBroken();
        }
    }
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("GetIsLockBroken >> %0.f", afReturnValue);
    }
    return true;
}

bool Script::GetDiseaseConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("UNIMPLEMENTED: GetDisease >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetGoldConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    InventoryChanges* inventoryChanges;
    afReturnValue = 0.0;
    TESBoundObject* formByNumericID = static_cast<TESBoundObject*>(TESForm::GetFormByNumericID(15));
    if ((apRefObject) && (formByNumericID) && (apRefObject->HasContainer())) {
        inventoryChanges = InventoryChanges::GetInventoryChanges(apRefObject);
        if (inventoryChanges) {
            afReturnValue = (double)inventoryChanges->GetObjectCount(formByNumericID);
        }
    }

    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("GetGold >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetFurnitureMarkerIDConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    BaseProcess* currentAIProcess;
    afReturnValue = 0.0;
    Actor* v6 = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        v6 = static_cast<Actor*>(apRefObject);
    }

    if (v6) {
        currentAIProcess = v6->MobileObject::GetCurrentAIProcess();
        afReturnValue = (double)currentAIProcess->GetFurnitureMarkerID();
    }
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("GetFurnitureMarkerID >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetIsPleasantConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    void* thread;
    TESWeather* time;
    double v7;
    double weatherPercent;
    Sky* instance = Sky::GetInstance();
    if (instance->Sky::GetCurrentWeather() &&
        ((thread = instance->Sky::GetCurrentWeather()), static_cast<TESWeather*>(thread)->IsPleasant())) {
        weatherPercent = (double)instance->GetWeatherPercent();
    }
    else {
        weatherPercent = 0.0;
    }
    afReturnValue = weatherPercent;
    if (instance->Sky::GetLastWeather() && ((time = instance->Sky::GetLastWeather()), time->IsPleasant())) {
        v7 = 1.0 - instance->GetWeatherPercent();
    }
    else {
        v7 = 0.0;
    }
    afReturnValue = afReturnValue + v7;
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("GetIsPleasant >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetIsCloudyConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    void* thread;
    TESWeather* time;
    double v7;
    double weatherPercent;
    Sky* instance = Sky::GetInstance();
    if (instance->Sky::GetCurrentWeather() &&
        ((thread = instance->Sky::GetCurrentWeather()), static_cast<TESWeather*>(thread)->IsCloudy())) {
        weatherPercent = (double)instance->GetWeatherPercent();
    }
    else {
        weatherPercent = 0.0;
    }
    afReturnValue = weatherPercent;
    if (instance->Sky::GetLastWeather() && ((time = instance->Sky::GetLastWeather()), time->IsCloudy())) {
        v7 = 1.0 - instance->GetWeatherPercent();
    }
    else {
        v7 = 0.0;
    }
    afReturnValue = afReturnValue + v7;
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("GetIsCloudy >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetIsRainingConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    float v5;
    Sky* instance = Sky::GetInstance();
    if (instance->IsRaining()) {
        v5 = (float)1.0;
    }
    else {
        v5 = (float)0.0;
    }
    afReturnValue = (double)v5;
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("GetIsRaining >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetIsSnowingConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    float v5;
    Sky* instance = Sky::GetInstance();
    if (instance->IsSnowing()) {
        v5 = (float)1.0;
    }
    else {
        v5 = (float)0.0;
    }
    afReturnValue = (double)v5;
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("GetIsSnowing >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetWeatherPercentConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    Sky* instance = Sky::GetInstance();
    afReturnValue = (double)instance->GetWeatherPercent();
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("GetCurrentWeatherPercent >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetAttackedConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    Actor* v5 = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        v5 = static_cast<Actor*>(apRefObject);
    }

    if (v5) {
        afReturnValue = (double)v5->HasBeenAttacked();
    }
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("GetAttacked >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetIsChildConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if ((apRefObject) && (apRefObject->IsChild(false))) {
        afReturnValue = 1.0;
    }

    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("IsChild >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetUsedItemLevelConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = (double)TESIdleManager::GetUsedItemLevel();
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("GetIsUsedItemLevel >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetUsedItemActivateConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (TESIdleManager::GetUsedItemActivate()) {
        afReturnValue = 1.0;
    }
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("GetIsUsedItemActivate >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetRestrainedConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    const char* v4;
    const char* fullName;
    afReturnValue = 0.0;
    Actor* v7 = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        v7 = static_cast<Actor*>(apRefObject);
    }

    if (v7) {
        if (v7->IsRestrained()) {
            afReturnValue = 1.0;
        }
        if (Script::GetThreadSystemOutput()) {
            if (0.0 == afReturnValue) {
                fullName = v7->TESObjectREFR::GetFullName();
                Interface::PrintLine("%s is not restrained", fullName);
            }
            else {
                v4 = v7->TESObjectREFR::GetFullName();
                Interface::PrintLine("%s is restrained", v4);
            }
        }
    }
    return true;
}

bool Script::GetLevelConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    Actor* v5 = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        v5 = static_cast<Actor*>(apRefObject);
    }

    if (v5) {
        afReturnValue = (double)v5->GetLevel();
    }
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("GetLevel >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetAlertConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    Actor* v5 = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        v5 = static_cast<Actor*>(apRefObject);
    }

    if (v5) {
        afReturnValue = (double)v5->GetAlert();
        if (Script::GetThreadSystemOutput()) {
            Interface::PrintLine("GetIsAlerted: %0.2f", afReturnValue);
        }
    }
    return true;
}

bool Script::IsFacingUpConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    NiFixedString* pBip01Spine;
    NiNode* pSpine;
    NiNode* pRoot;
    afReturnValue = 0.0;
    TESObjectREFR* pActor = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        pActor = apRefObject;
    }

    if (pActor) {
        pRoot = pActor->TESObjectREFR::Get3D();
        pBip01Spine = FixedStrings::GetBip01_Spine();
        pSpine = static_cast<NiNode*>(pActor->GetActorBone(pRoot, *pBip01Spine));
        if (!pSpine) {
            pSpine = static_cast<NiNode*>(pActor->GetActorBone(pRoot, "Bip01 Spine01"));
        }
        if ((!pSpine) || bhkNiCollisionObject::GetFaceUp(pSpine, nullptr)) {
            afReturnValue = 1.0;
        }
    }
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("Is Facing Up >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::IsLeftUpConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    NiFixedString* bip01_Spine;
    NiAVObject* niObject;
    NiNode* apNode;
    afReturnValue = 0.0;
    TESObjectREFR* v8 = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        v8 = apRefObject;
    }

    if (v8) {
        apNode = v8->TESObjectREFR::Get3D();
        bip01_Spine = FixedStrings::GetBip01_Spine();
        niObject = v8->GetActorBone(apNode, *bip01_Spine);
        if ((!niObject) || ((uint8_t)NiNode::IsLeftFacingUp(niObject, nullptr))) {
            afReturnValue = 1.0;
        }
    }
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("Is Left Up >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetCurrentAIPackageConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    TESPackage* currentPackage;
    afReturnValue = 0.0;
    Actor* v6 = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        v6 = static_cast<Actor*>(apRefObject);
    }

    if ((v6) && (v6->MobileObject::GetCurrentAIProcess()) && (v6->MobileObject::GetCurrentPackage())) {
        currentPackage = v6->MobileObject::GetCurrentPackage();
        switch ((int32_t)currentPackage->GetPackType()) {
            case 0: {
                afReturnValue = 0.0;
                break;
            }
            case 1: {
                afReturnValue = 1.0;
                break;
            }
            case 2: {
                afReturnValue = 2.0;
                break;
            }
            case 3: {
                afReturnValue = 3.0;
                break;
            }
            case 4: {
                afReturnValue = 4.0;
                break;
            }
            case 5: {
                afReturnValue = 13.0;
                break;
            }
            case 6: {
                afReturnValue = 14.0;
                break;
            }
            case 7: {
                afReturnValue = 15.0;
                break;
            }
            case 8: {
                afReturnValue = 16.0;
                break;
            }
            case 9: {
                afReturnValue = 17.0;
                break;
            }
            case 10: {
                afReturnValue = 18.0;
                break;
            }
            case 11: {
                afReturnValue = 19.0;
                break;
            }
            case 12: {
                afReturnValue = 36.0;
                break;
            }
            case 13: {
                afReturnValue = 37.0;
                break;
            }
            case 14: {
                afReturnValue = 35.0;
                break;
            }
            case 15: {
                afReturnValue = 34.0;
                break;
            }
            case 16: {
                afReturnValue = 33.0;
                break;
            }
            case 18: {
                afReturnValue = 5.0;
                break;
            }
            case 19: {
                afReturnValue = 20.0;
                break;
            }
            case 20: {
                afReturnValue = 7.0;
                break;
            }
            case 21: {
                afReturnValue = 8.0;
                break;
            }
            case 22: {
                afReturnValue = 10.0;
                break;
            }
            case 23: {
                afReturnValue = 11.0;
                break;
            }
            case 24: {
                afReturnValue = 9.0;
                break;
            }
            case 25: {
                afReturnValue = 12.0;
                break;
            }
            case 26: {
                afReturnValue = 21.0;
                break;
            }
            case 27: {
                afReturnValue = 24.0;
                break;
            }
            case 28: {
                afReturnValue = 6.0;
                break;
            }
            case 29: {
                afReturnValue = 28.0;
                break;
            }
            case 30: {
                afReturnValue = 29.0;
                break;
            }
            case 31: {
                afReturnValue = 32.0;
                break;
            }
            default: {
                afReturnValue = (-1.0);
                break;
            }
        }
    }

    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("Current Process >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::IsWaitingConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    Actor* v5 = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        v5 = static_cast<Actor*>(apRefObject);
    }

    if ((v5) && (v5->IsWaiting())) {
        afReturnValue = 1.0;
    }

    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("Is Waiting >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::IsIdlePlayingConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    Animation* v5;
    afReturnValue = 0.0;
    if (apRefObject) {
        v5 = apRefObject->GetAnimation();
        if ((v5) && (!v5->SpecialIdleDonePlaying())) {
            afReturnValue = 1.0;
        }
    }
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("Is Idle Playing >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetAmountStolenSoldConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    afReturnValue = (double)pPlayer->GetAmountSoldStolen();
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("Amount stolen the player sold >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetWindSpeedConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if ((!TES::pInstance) || (!TES::pInstance->TES::GetSky())) {
        return false;
    }
    Sky* currentAIProcess = TES::pInstance->TES::GetSky();
    afReturnValue = (double)currentAIProcess->GetWindSpeed();
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("GetWindSpeed >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::IsInMyOwnedCellConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    TESObjectCELL* parentCell;
    afReturnValue = 0.0;
    PlayerCharacter* v6 = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        v6 = static_cast<PlayerCharacter*>(apRefObject);
    }

    if ((v6) && (v6->TESObjectREFR::GetParentCell())) {
        parentCell = v6->TESObjectREFR::GetParentCell();
        if (parentCell->IsActorCellOwner(v6)) {
            afReturnValue = 1.0;
        }
    }

    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("Is in owned cell value %0.2f", afReturnValue);
    }
    return true;
}

bool Script::IsContinuingPackagePCNearConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    Actor* v5 = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        v5 = static_cast<Actor*>(apRefObject);
    }

    if (v5) {
        afReturnValue = (double)v5->IsContinuingPackagePCNear();
    }
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("is continuing package near PC value %0.2f", afReturnValue);
    }
    return true;
}

bool Script::HasFlamesConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    NiNode* v4;
    float fResult;
    NiNode* v7;
    afReturnValue = 0.0;
    if ((apRefObject) && (apRefObject->Get3D())) {
        v7 = apRefObject->Get3D();
        if (v7) {
            v4 = v7->IsNode();
            if (TESObjectREFR::HasAddonNodes(v4)) {
                fResult = (float)1.0;
            }
            else {
                fResult = (float)0.0;
            }
            afReturnValue = (double)fResult;
        }
    }

    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("HasFlames >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::IsPCSleepingConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    float v5;
    afReturnValue = 0.0;
    if (pPlayer->IsSleeping()) {
        v5 = (float)1.0;
    }
    else {
        v5 = (float)0.0;
    }
    afReturnValue = (double)v5;
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("IsPlayerSleeping >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::IsPCAMurdererConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    float v5;
    afReturnValue = 0.0;
    if (pPlayer->IsAMurderer()) {
        v5 = (float)1.0;
    }
    else {
        v5 = (float)0.0;
    }
    afReturnValue = (double)v5;
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("IsPlayerAMurderer >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::IsSwimmingConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    Actor* v5 = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        v5 = static_cast<Actor*>(apRefObject);
    }

    if ((v5) && (v5->IsSwimming())) {
        afReturnValue = 1.0;
    }

    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("Is Swimming >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetDestroyedConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if ((apRefObject) && (apRefObject->TESForm::GetDestroyed())) {
        afReturnValue = 1.0;
    }

    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("GetDestroyed >> %.0f", afReturnValue);
    }
    return true;
}

bool Script::HasMagicEffectConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    MagicTarget* v4;
    afReturnValue = 0.0;
    TESObjectREFR* v6 = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        v6 = apRefObject;
    }

    if ((v6) && (apParam1)) {
        v4 = v6->GetMagicTarget();
        if (v4->ChecksForMagicEffect(static_cast<EffectSetting*>(apParam1))) {
            afReturnValue = 1.0;
        }
    }

    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("Has Magic Effect >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetVATSModeConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    afReturnValue = (double)(&pVATS)->eMode;
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("VATS Mode >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetSandmanConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (static_cast<const void*>(apRefObject) == static_cast<const void*>(pPlayer)) {
        afReturnValue = (double)pPlayer->GetSandman();
    }
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("Player is in Sandman action >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::HasBeenEatenConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if ((apRefObject->IsActor()) && (static_cast<Actor*>(apRefObject)->GetHasBeenEaten())) {
        afReturnValue = 1.0;
    }

    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("Actor has been eaten >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetGhostConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    Actor* v5 = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        v5 = static_cast<Actor*>(apRefObject);
    }

    if ((v5) && (v5->GetGhost())) {
        afReturnValue = 1.0;
    }

    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("GetGhost >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetOffersServiceConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    const char* v4;
    const char* baseFormFullName;
    afReturnValue = 0.0;
    Actor* pActor = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        pActor = static_cast<Actor*>(apRefObject);
    }

    if ((pActor) && (pActor->GetOffersServicesNow())) {
        afReturnValue = 1.0;
    }

    if (!Script::GetThreadSystemOutput()) {
        return true;
    }
    if (0.0 == afReturnValue) {
        baseFormFullName = pActor->TESObjectREFR::GetFullName();
        Interface::PrintLine("%s is not offering services", baseFormFullName);
    }
    else {
        v4 = pActor->TESObjectREFR::GetFullName();
        Interface::PrintLine("%s  is offering services", v4);
    }
    return true;
}

bool Script::GetBarterGoldConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    int32_t barterGold;
    Actor* v6 = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        v6 = static_cast<Actor*>(apRefObject);
    }

    if (v6) {
        afReturnValue = (double)((uint32_t)v6->GetBarterGold());
        if (Script::GetThreadSystemOutput()) {
            barterGold = v6->GetBarterGold();
            Interface::PrintLine("%s  has %d barter gold currently", v6, barterGold);
        }
    }
    return true;
}

bool Script::IsActorUsingATorchConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    if (apRefObject) {
        apRefObject->IsActor();
    }
    afReturnValue = 0.0;
    return true;
}

bool Script::IsSneakingConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    const char* v4;
    const char* fullName;
    afReturnValue = 0.0;
    Actor* v7 = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        v7 = static_cast<Actor*>(apRefObject);
    }

    if (v7->IsSneaking()) {
        afReturnValue = 1.0;
    }
    if (!Script::GetThreadSystemOutput()) {
        return true;
    }
    if (0.0 == afReturnValue) {
        fullName = TESFullName::GetFullName(v7);
        Interface::PrintLine("%s is not sneaking", fullName);
    }
    else {
        v4 = TESFullName::GetFullName(v7);
        Interface::PrintLine("%s is sneaking", v4);
    }
    return true;
}

bool Script::IsRunningConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    const char* v4;
    const char* fullName;
    afReturnValue = 0.0;
    Actor* v7 = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        v7 = static_cast<Actor*>(apRefObject);
    }

    if (v7->IsRunning()) {
        afReturnValue = 1.0;
    }
    if (!Script::GetThreadSystemOutput()) {
        return true;
    }
    if (0.0 == afReturnValue) {
        fullName = TESFullName::GetFullName(v7);
        Interface::PrintLine("%s is not running", fullName);
    }
    else {
        v4 = TESFullName::GetFullName(v7);
        Interface::PrintLine("%s is running", v4);
    }
    return true;
}

bool Script::GetFriendHitConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    const char* v6;
    const char* fullName;
    double v8;
    afReturnValue = 0.0;
    TESObjectREFR* apObj = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        apObj = apRefObject;
    }

    if (!apObj) {
        return true;
    }
    ExtraDataList* extra = apObj->TESObjectREFR::GetExtra();
    afReturnValue = (double)extra->GetFriendHitCount();
    if (Script::GetThreadSystemOutput()) {
        v8 = afReturnValue;
        fullName = TESFullName::GetFullName(apObj);
        v6 = TESFullName::GetFullName(pPlayer);
        Interface::PrintLine("%s has hit %s %0.2f times", v6, fullName, v8);
    }
    return true;
}

bool Script::IsInInteriorConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    TESObjectCELL* parentCell;
    const char* v5;
    const char* fullName;
    afReturnValue = 0.0;
    if ((apRefObject) && (apRefObject->TESObjectREFR::GetParentCell())) {
        parentCell = apRefObject->TESObjectREFR::GetParentCell();
        if (parentCell->GetInterior()) {
            afReturnValue = 1.0;
        }
    }

    if (!Script::GetThreadSystemOutput()) {
        return true;
    }
    if (0.0 == afReturnValue) {
        fullName = TESFullName::GetFullName(apRefObject);
        Interface::PrintLine("%s is not an Interior", fullName);
    }
    else {
        v5 = TESFullName::GetFullName(apRefObject);
        Interface::PrintLine("%s is in an Interior", v5);
    }
    return true;
}

bool Script::GetWhichServiceConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = (-1.0);
    afReturnValue = (double)Interface::iServiceFlag;
    return true;
}

bool Script::IsActorRidingHorseConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (apRefObject) {
        apRefObject->IsActor();
    }
    return true;
}

bool Script::IsXboxConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    float v5;
    afReturnValue = 0.0;
    if (Interface::Has360Controller()) {
        v5 = (float)1.0;
    }
    else {
        v5 = (float)0.0;
    }
    afReturnValue = (double)v5;
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("IsXBox >> %1.0f", afReturnValue);
    }
    return true;
}

bool Script::IsPS3ConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    afReturnValue = 0.0;
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("IsPS3 >> %1.0f", afReturnValue);
    }
    return true;
}

bool Script::IsPCConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    float v5;
    afReturnValue = 1.0;
    if (Interface::Has360Controller()) {
        v5 = (float)0.0;
    }
    else {
        v5 = (float)1.0;
    }
    afReturnValue = (double)v5;
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("IsPC >> %1.0f", afReturnValue);
    }
    return true;
}

bool Script::IsEssentialConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (apRefObject->IsActor() && static_cast<Actor*>(apRefObject)->GetEssential()) {
        afReturnValue = 1.0;
    }
    return true;
}

bool Script::IsActorConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (apRefObject->IsActor()) {
        afReturnValue = 1.0;
    }
    return true;
}

bool Script::IsPlayerMovingIntoNewSpaceConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (pPlayer->btransporting) {
        afReturnValue = 1.0;
    }
    return true;
}

bool Script::GetLastPlayerActionConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    afReturnValue = (double)pPlayer->GetLastAction();
    return true;
}

bool Script::GetLastHitCriticalConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    HitData* lastHitData;
    float v6;
    afReturnValue = 0.0;
    if ((apRefObject) && (apRefObject->IsActor()) && (static_cast<Actor*>(apRefObject)->GetLastHitData())) {
        lastHitData = static_cast<Actor*>(apRefObject)->GetLastHitData();
        if (lastHitData->IsFlagSet(HitData::HD_CRITICAL)) {
            v6 = (float)1.0;
        }
        else {
            v6 = (float)0.0;
        }
        afReturnValue = (double)v6;
    }

    return true;
}

bool Script::GetIsMovingConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (!apRefObject) {
        return true;
    }
    if (!apRefObject->IsActor()) {
        return true;
    }
    int8_t movementFlags = (int8_t)static_cast<Actor*>(apRefObject)->GetMoveMode();
    if ((movementFlags & 1) != 0) {
        afReturnValue = 1.0;
    }
    else {
        if ((movementFlags & 2) != 0) {
            afReturnValue = 2.0;
        }
        else {
            if ((movementFlags & 4) != 0) {
                afReturnValue = 3.0;
            }
            else {
                if ((movementFlags & 8) != 0) {
                    afReturnValue = 4.0;
                }
            }
        }
    }
    return true;
}

bool Script::GetAnimActionConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = (-1.0);
    if ((apRefObject) && (apRefObject->IsActor())) {
        afReturnValue = (double)static_cast<Actor*>(apRefObject)->GetAnimAction();
    }

    return true;
}

bool Script::GetPlantedExplosiveConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    BaseProcess* currentAIProcess;
    afReturnValue = 0.0;
    if ((apRefObject) && (apRefObject->IsActor()) &&
        (static_cast<MobileObject*>(apRefObject)->MobileObject::GetCurrentAIProcess())) {
        currentAIProcess = static_cast<MobileObject*>(apRefObject)->MobileObject::GetCurrentAIProcess();
        afReturnValue = (double)currentAIProcess->GetPlantedExplosive();
    }

    return true;
}

bool Script::IsActorTalkingThroughActivatorConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if ((apRefObject) && (apRefObject->IsActor())) {
        afReturnValue =
            (double)(&ReferenceProcessLists)->IsActorTalkingThroughActivator(static_cast<Actor*>(apRefObject));
    }

    return true;
}

bool Script::GetDialogueEmotionConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (apRefObject->IsMobileObject()) {
        afReturnValue = (double)static_cast<Actor*>(apRefObject)->GetSpeakingEmotion();
        if (!static_cast<MobileObject*>(apRefObject)->GetShouldUseEmotionAnimation()) {
            afReturnValue = (-1.0);
        }
    }
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("GetEmotion >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetDialogueEmotionValueConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (apRefObject->IsMobileObject()) {
        static_cast<Actor*>(apRefObject)->GetSpeakingEmotionValue();
    }
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("GetEmotionValue >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::IsWaterObjectConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    TESObject* usedItem = TESIdleManager::GetUsedItem();
    if ((usedItem) && (usedItem->GetWaterType())) {
        afReturnValue = 1.0;
    }

    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("IsWaterActivator >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetPlayerTeammateConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if ((apRefObject) && (apRefObject->IsActor())) {
        if (static_cast<Actor*>(apRefObject)->bPlayerTeammate) {
            afReturnValue = 1.0;
        }
        if (Script::GetThreadSystemOutput()) {
            Interface::PrintLine("Player teammate  >> %d", (int32_t)afReturnValue);
        }
    }

    return true;
}

bool Script::GetMinorCrimeCountConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if ((apRefObject) && (apRefObject->IsActor())) {
        if (static_cast<const void*>(apRefObject) == static_cast<const void*>(pPlayer)) {
            afReturnValue = (double)((int32_t)static_cast<PlayerCharacter*>(apRefObject)->GetMinorCrimeCount());
        }
        else {
            afReturnValue = (double)static_cast<Actor*>(apRefObject)->GetMinorCrimeCount();
        }
        if (Script::GetThreadSystemOutput()) {
            Interface::PrintLine("The number of minor crimes is %.2f", afReturnValue);
        }
    }

    return true;
}

bool Script::GetMajorCrimeCountConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (!apRefObject) {
        return true;
    }
    if (!apRefObject->IsActor()) {
        return true;
    }
    if (static_cast<const void*>(apRefObject) == static_cast<const void*>(pPlayer)) {
        afReturnValue = (double)static_cast<Actor*>(apRefObject)->GetMajorCrimesNumber();
    }
    else {
        afReturnValue = (double)static_cast<Actor*>(apRefObject)->GetMajorCrimeCount();
    }
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("The number of major crimes is %.2f", afReturnValue);
    }
    return true;
}

bool Script::GetDestructionStageConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = (-1.0);
    if (apRefObject) {
        afReturnValue = (double)BGSDestructibleObjectForm::CalcDestructionStage(apRefObject);
    }
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("GetDestructionStage >> %.2f", afReturnValue);
    }
    return true;
}

bool Script::IsHardcoreConditionalFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    if (pPlayer) {
        afReturnValue = (double)pPlayer->IsHardcore();
    }
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("IsHardcore? >> %f", afReturnValue);
    }
    return true;
}

bool Script::IsAlwaysHardcoreConditionalFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    if (pPlayer) {
        afReturnValue = (double)((uint8_t)pPlayer->GetAlwaysHardcore());
    }
    if (Script::GetThreadSystemOutput()) {
        Interface::PrintLine("GetAlwaysHardcore? >> %f", afReturnValue);
    }
    return true;
}

bool Script::GetIsTurningConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    int8_t moveMode;
    afReturnValue = 0.0;
    if ((apRefObject) && (apRefObject->IsActor())) {
        moveMode = (int8_t)static_cast<Actor*>(apRefObject)->GetMoveMode();
        if ((moveMode & 16) != 0) {
            afReturnValue = 1.0;
        }
        else {
            if ((moveMode & 32) != 0) {
                afReturnValue = 2.0;
            }
        }
    }

    return true;
}

bool Script::ShowMessage(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    BGSMessage* pMessage = nullptr;
    void* pUnused = nullptr;
    if (!ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &pMessage,
            &pUnused
        )) {
        return false;
    }
    if (!pMessage) {
        return false;
    }

    alignas(BSStringT<char>) uint8_t aTextStorage[sizeof(BSStringT<char>)];
    BSStringT<char>* pText = ::new (aTextStorage) BSStringT<char>;
    // The chunk is 'DESC' as a little-endian constant.
    TESDescription* pDescription = pMessage;
    const char* pDescriptionText = pDescription->GetDescription(nullptr, DESC_ID);
    if (!ParseString(
            apParameters, apCompiledParams, apRefObject, apContainer, apScript, apScriptLocals, &afReturnValue,
            &auiOffset, pDescriptionText, pText
        )) {
        pText->~BSStringT<char>();
        return false;
    }

    ClearButtonPressed();
    if (apRefObject && !apRefObject->GetTemporary()) {
        m_iButtonOwner = apRefObject->GetFormID();
    }
    else if (apScript) {
        m_iButtonOwner = apScript->GetFormID();
    }
    else {
        pText->~BSStringT<char>();
        return false;
    }

    int32_t iButton = 0;
    BSSimpleList<MESSAGEBOX_BUTTON*>* pNode = pMessage->GetButtonList();
    bool bNoButtonShown = true;
    const char* apButtons[10];
    ourMemset(apButtons, 0, sizeof(apButtons));
    while (pNode && pNode->GetItem() && iButton < 10) {
        bool bDispositionFailure;
        if (pNode->GetItem()->Conditions.IsTrueFull(pPlayer, nullptr, &bDispositionFailure, false)) {
            apButtons[iButton] = pNode->GetItem()->Text.String();
            bNoButtonShown = false;
        }
        else {
            apButtons[iButton] = " ";
        }
        ++iButton;
        pNode = pNode->GetNext();
    }
    if (bNoButtonShown) {
        apButtons[0] = "OK";
    }

    if (pMessage->GetIsMessageBox()) {
        TESFullName* pFullName = pMessage;
        const char* pTitle = pFullName->GetFullName();
        // The text is passed through an identity function, which returns it unchanged.
        const char* pMessageText = NavMeshInfo::GetCellOrCoords(const_cast<char*>(pText->String()), 0);
        Interface::CreateMessageMenu(
            pMessageText, nullptr, pTitle, &Script::HandleMessageMenu, 0, 0x17, 0.0f, 0.0f, apButtons[0], apButtons[1],
            apButtons[2], apButtons[3], apButtons[4], apButtons[5], apButtons[6], apButtons[7], apButtons[8],
            apButtons[9], static_cast<const char*>(nullptr)
        );
    }
    else if (pText->GetLength()) {
        const char* pIconName;
        if (pMessage->GetIcon()) {
            pIconName = pMessage->GetIcon()->GetTextureName();
        }
        else {
            pIconName = nullptr;
        }
        // Widens the display time to a 64-bit integer before rounding it to float, as the game does.
        float fSeconds = static_cast<float>(static_cast<int64_t>(static_cast<uint64_t>(pMessage->GetDisplayTime())));
        const char* pMessageText = NavMeshInfo::GetCellOrCoords(const_cast<char*>(pText->String()), 0);
        Interface::ShowMessage(pMessageText, 0, pIconName, nullptr, fSeconds, false);
    }
    pText->~BSStringT<char>();
    return true;
}

bool Script::VerboseOutput() {
    return GetThreadSystemOutput();
}

// A per-thread flag: CompileAndRun sets it, and the condition and script functions that read it echo their result to
// the console.
bool& Script::GetThreadSystemOutput() {
    return GetThreadTLSData()->bSystemOutput;
}

Script*& Script::GetThreadLastRefSearchScript() {
    static thread_local Script* pLastRefSearchScript = nullptr;
    return pLastRefSearchScript;
}

ScriptLocals*& Script::GetThreadLastScriptLocals() {
    static thread_local ScriptLocals* pLastScriptLocals = nullptr;
    return pLastScriptLocals;
}

uint32_t& Script::GetThreadLastRefSearchIndex() {
    static thread_local uint32_t iLastRefSearchIndex = 0;
    return iLastRefSearchIndex;
}

Script::RefVariable*& Script::GetThreadLastRefObject() {
    static thread_local RefVariable* pLastRefObject = nullptr;
    return pLastRefObject;
}

bool Script::GetPosConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    const NiPoint3* v4;
    if (apRefObject) {
        v4 = &apRefObject->GetLocationOnReference();
        switch (static_cast<int32_t>(reinterpret_cast<intptr_t>(apParam1))) {
            case 88: {
                afReturnValue = (double)v4->x;
                break;
            }
            case 89: {
                afReturnValue = (double)v4->y;
                break;
            }
            case 90: {
                afReturnValue = (double)v4->z;
                break;
            }
        }
        if (Script::GetThreadSystemOutput()) {
            Interface::PrintLine(
                "GetPos: %c >> %0.2f", static_cast<int32_t>(reinterpret_cast<intptr_t>(apParam1)), afReturnValue
            );
        }
    }
    return true;
}

void Script::LoadPendingReferenceLists(BGSLoadGameBuffer* apLoadGameBuffer) {
    uint32_t formID_0;
    TESForm* formByNumericID;
    uint32_t v3;
    TESForm* v4;
    uint32_t v5;
    TESForm* v6;
    uint32_t v7;
    TESForm* v8;
    TESObjectREFR* v9;
    TESObjectREFR* v10;
    TESObjectREFR* v11;
    TESObjectREFR* v12;
    void* v13;
    void* v14;
    void* v15;
    void* apMemory;
    TESObjectREFR* v17;
    uint32_t m;
    TESObjectREFR* v20;
    uint32_t k;
    TESObjectREFR* item;
    uint32_t j;
    TESObjectREFR* a2;
    uint32_t i;
    int32_t v29;
    (&Script::ScriptRefListCrit)->Enter(nullptr);
    (&Script::ReferencesToEnable)->RemoveAll();
    (&Script::ReferencesToDisable)->RemoveAll();
    (&Script::ReferencesToDelete)->RemoveAll();
    (&Script::ReferencesToFadeDisable)->RemoveAll();
    uint32_t variableSizedValue = apLoadGameBuffer->LoadVariableSizedValue();
    if (variableSizedValue) {
        for (i = 0; i < variableSizedValue; ++i) {
            apMemory = ourMalloc(sizeof(TESObjectREFR));
            v29 = 0;
            if (apMemory) {
                v12 = ::new (apMemory) TESObjectREFR;
            }
            else {
                v12 = nullptr;
            }
            v29 = (-1);
            a2 = v12;
            formID_0 = apLoadGameBuffer->LoadFormID();
            formByNumericID = TESForm::GetFormByNumericID(formID_0);
            a2 = dynamic_cast<TESObjectREFR*>(formByNumericID);
            (&Script::ReferencesToEnable)->AddHead(a2);
        }
    }
    uint32_t v25 = apLoadGameBuffer->LoadVariableSizedValue();
    if (v25) {
        for (j = 0; j < v25; ++j) {
            v15 = ourMalloc(sizeof(TESObjectREFR));
            v29 = 1;
            if (v15) {
                v11 = ::new (v15) TESObjectREFR;
            }
            else {
                v11 = nullptr;
            }
            v29 = (-1);
            item = v11;
            v3 = apLoadGameBuffer->LoadFormID();
            v4 = TESForm::GetFormByNumericID(v3);
            item = dynamic_cast<TESObjectREFR*>(v4);
            (&Script::ReferencesToDisable)->AddHead(item);
        }
    }
    uint32_t v22 = apLoadGameBuffer->LoadVariableSizedValue();
    if (v22) {
        for (k = 0; k < v22; ++k) {
            v14 = ourMalloc(sizeof(TESObjectREFR));
            v29 = 2;
            if (v14) {
                v10 = ::new (v14) TESObjectREFR;
            }
            else {
                v10 = nullptr;
            }
            v29 = (-1);
            v20 = v10;
            v5 = apLoadGameBuffer->LoadFormID();
            v6 = TESForm::GetFormByNumericID(v5);
            v20 = dynamic_cast<TESObjectREFR*>(v6);
            (&Script::ReferencesToDelete)->AddHead(v20);
        }
    }
    uint32_t v19 = apLoadGameBuffer->LoadVariableSizedValue();
    if (v19) {
        for (m = 0; m < v19; ++m) {
            v13 = ourMalloc(sizeof(TESObjectREFR));
            v29 = 3;
            if (v13) {
                v9 = ::new (v13) TESObjectREFR;
            }
            else {
                v9 = nullptr;
            }
            v29 = (-1);
            v17 = v9;
            v7 = apLoadGameBuffer->LoadFormID();
            v8 = TESForm::GetFormByNumericID(v7);
            v17 = dynamic_cast<TESObjectREFR*>(v8);
            (&Script::ReferencesToFadeDisable)->AddHead(v17);
        }
    }
    (&Script::ScriptRefListCrit)->Leave();
}

bool Script::GetProcessScripts() {
    return bIsScriptProcessing;
}

void Script::PutNumericIDInDouble(const uint32_t& aiID, double& adVal) {
    memcpy(&adVal, &aiID, sizeof(aiID));
}

bool Script::SetActionFlag(TESForm* apActionForm, ExtraDataList* apExtra, uint32_t aiFlag) {
    if (apExtra == nullptr) {
        return false;
    }
    if (apExtra->GetScriptLocals() == nullptr) {
        return false;
    }
    ScriptLocals* locals = apExtra->GetScriptLocals();
    return locals->SetActionFlag(apActionForm, static_cast<int32_t>(aiFlag));
}

void Script::AddPendingDisabledReference(TESObjectREFR* apRef, bool abFade) {
    (&Script::ScriptRefListCrit)->Enter(nullptr);
    if (apRef) {
        (&Script::ReferencesToEnable)->Remove(apRef);
        if (abFade) {
            if (!Script::IsPendingFadeDisabledReference(apRef)) {
                (&Script::ReferencesToFadeDisable)->AddHead(apRef);
            }
        }
        else {
            if (!Script::IsPendingDisabledReference(apRef)) {
                (&Script::ReferencesToDisable)->AddHead(apRef);
            }
        }
    }
    (&Script::ScriptRefListCrit)->Leave();
}

void Script::InitActionList(TESObjectREFR* ownerObject, ExtraDataList* apExtra) {
    if (apExtra == nullptr) {
        return;
    }
    if (apExtra->GetScriptLocals() == nullptr) {
        return;
    }
    ScriptLocals* locals = apExtra->GetScriptLocals();
    if (locals->m_pActionList == nullptr) {
        ScriptRunManager* manager = ScriptRunManager::Instance();
        manager->InitActionList(locals->m_pMasterScript, ownerObject, locals);
    }
}

void Script::ClearButtonPressed() {
    Script::m_cButtonValue = (-1);
    Script::m_iButtonOwner = 0;
}

bool Script::GetNoRumors(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    afReturnValue = 0.0;
    Actor* v5 = nullptr;
    if ((apRefObject) && (apRefObject->IsActor())) {
        v5 = static_cast<Actor*>(apRefObject);
    }

    if (!v5) {
        return true;
    }
    if (v5->HasNoRumors()) {
        afReturnValue = 1.0;
    }
    return true;
}

bool Script::ToggleMaterialGeometryFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    bhkShape::bMaterialColor = true;
    TES::pInstance->ToggleCollisionGeometry();
    bhkShape::bMaterialColor = false;
    return true;
}

bool Script::TriggerHUDShudder(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    float v15;
    float a8 = 500.0f;
    float v14 = 0.10000000149011612f;
    float v13 = 0.05000000074505806f;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v14,
            &v13, &v15
        )) {
        return false;
    }
    float a4 = v13;
    float a3 = v14;
    float a2 = a8;
    FOHUDEffectManager* screenShaderThingy = Interface::GetHUDEffects();
    screenShaderThingy->TriggerShudder(a2, a3, a4);
    return true;
}

bool Script::GetWeaponAnimType(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    if (apRefObject) {
        return Script::GetWeaponAnimTypeConditionFunction(apRefObject, nullptr, nullptr, afReturnValue);
    }

    return true;
}

bool Script::DebugCombatPlan(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    CombatController* v4;
    if (apRefObject->IsActor()) {
        v4 = static_cast<Actor*>(apRefObject)->GetCombatController();
        if (v4) {
            v4->DebugCombatPlan();
        }
    }
    return true;
}

bool Script::DumpNiUpdates(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    Set(1);
    Interface::CloseConsole();
    return true;
}

bool Script::RefreshINI(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    const char* personalPath;
    INISettingCollection* collection;
    const char* v2;
    INISettingCollection* v3;
    const char* v4;
    INIPrefSettingCollection* v5;
    int8_t apDest[264];
    int8_t a2[264];
    int8_t dst[268];
    if (pAppININame) {
        personalPath = BSUtilities::GetPersonalPath();
        BSstrcpy(static_cast<char*>(static_cast<void*>(dst)), 0x104u, personalPath);
        BSstrcat(static_cast<char*>(static_cast<void*>(dst)), 0x104u, pAppININame);
        collection = SettingT<INISettingCollection>::GetCollection();
        collection->ReadSettingsFromFile(static_cast<const char*>(static_cast<const void*>(dst)));
    }
    if (pAppCustomININame) {
        v2 = BSUtilities::GetPersonalPath();
        BSstrcpy(static_cast<char*>(static_cast<void*>(a2)), 0x104u, v2);
        BSstrcat(static_cast<char*>(static_cast<void*>(a2)), 0x104u, pAppCustomININame);
        v3 = SettingT<INISettingCollection>::GetCollection();
        v3->ReadSettingsFromFile(static_cast<const char*>(static_cast<const void*>(a2)));
    }
    if (pAppPrefININame) {
        v4 = BSUtilities::GetPersonalPath();
        BSstrcpy(static_cast<char*>(static_cast<void*>(apDest)), 0x104u, v4);
        BSstrcat(static_cast<char*>(static_cast<void*>(apDest)), 0x104u, pAppPrefININame);
        v5 = SettingT<INIPrefSettingCollection>::GetCollection();
        v5->ReadSettingsFromFile(static_cast<const char*>(static_cast<const void*>(apDest)));
    }
    Interface::PrintLine("The in-game settings have been refreshed from the Fallout.ini file.");
    return true;
}

bool Script::SetOpenState(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    ExtraDataList* extra;
    int32_t a8 = 0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8
        )) {
        return false;
    }
    if (apRefObject) {
        if (a8) {
            if (((int32_t)BGSOpenCloseForm::GetOpenState(apRefObject)) > 2) {
                apRefObject->Activate(nullptr, false, nullptr, 1);
            }
            apRefObject->SetLastFinishedSequence("Open");
        }
        else {
            if (((int32_t)BGSOpenCloseForm::GetOpenState(apRefObject)) < 3) {
                apRefObject->Activate(nullptr, false, nullptr, 1);
            }
            apRefObject->SetLastFinishedSequence("Close");
        }
        extra = apRefObject->TESObjectREFR::GetExtra();
        extra->RemoveSavedAnimation();
    }
    return true;
}

bool Script::DamageActorValueFunction(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    const char* actorValueScriptName;
    float a1;
    float v11;
    int32_t a8 = 0;
    float v14 = (float)0.0;
    if (!Script::ParseParameters(
            apParameters, apCompiledParams, auiOffset, apRefObject, apContainer, apScript, apScriptLocals, &a8, &v14
        )) {
        return false;
    }
    Actor* v12 = dynamic_cast<Actor*>(apRefObject);
    if (v12) {
        if ((uint8_t)ActorValue::IsFlagSet(static_cast<ActorValue::Index>(a8), ActorValue::NO_SCRIPT_MODAV)) {
            if (Script::GetThreadSystemOutput()) {
                actorValueScriptName = ActorValue::GetActorValueScriptName(static_cast<ActorValue::Index>(a8));
                Interface::PrintLine(
                    "Actor Value '%s' cannot be modified in scripts or the console.", actorValueScriptName
                );
            }
        }
        else {
            if ((a8 == 16) && v12->IsChild(false)) {
                a1 = -v14;
                v12->HandleHealthDamage(nullptr, a1);
                return true;
            }
            v11 = v14;
            if (!((uint8_t)ActorValue::IsFlagSet(static_cast<ActorValue::Index>(a8), ActorValue::DAMAGE_IS_POSITIVE))) {
                v11 = -std::fabs(v14);
            }
            v12->DamageModActorValue(static_cast<ActorValue::Index>(a8), v11, nullptr);
        }
    }
    return true;
}

bool Script::RefreshShaders(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    BSShaderManager::ShaderErrorCallback pOldCallback = BSShaderManager::GetShaderErrorCallback();
    BSShaderManager::SetShaderErrorCallback(&ConsoleShaderError);
    BSShaderManager::ReloadShaders();
    BSShaderManager::SetShaderErrorCallback(pOldCallback);
    return true;
}

bool Script::ToggleBorders(
    const SCRIPT_PARAMETER* apParameters,
    const char* apCompiledParams,
    TESObjectREFR* apRefObject,
    TESObjectREFR* apContainer,
    Script* apScript,
    ScriptLocals* apScriptLocals,
    double& afReturnValue,
    uint32_t& auiOffset
) {
    bBordersOn ^= 1u;
    if (!TES::pInstance->TES::GetInterior()) {
        TES::pInstance->ShowLandBorders(bBordersOn);
    }
    if (Script::GetThreadSystemOutput()) {
        if (bBordersOn) {
            Interface::PrintLine("Borders -> %s", "On");
        }
        else {
            Interface::PrintLine("Borders -> %s", "Off");
        }
    }
    return true;
}
