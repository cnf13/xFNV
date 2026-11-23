#include "Fallout Shared/Script.h"

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
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
#include "Fallout/AI/Movement/ActorMover.h"
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
    TESForm* pWeather = nullptr;
    if (apParam1 && static_cast<TESForm*>(apParam1)->GetFormType() == WTHR_ID) {
        pWeather = static_cast<TESForm*>(apParam1);
    }

    if (static_cast<const void*>(pWeather) == static_cast<const void*>(Sky::GetInstance()->GetCurrentWeather())) {
        afReturnValue = 1.0;
    }
    if (Script::bSystemOutput) {
        Interface::PrintLine("GetIsCurrentWeather >> %0.2f", afReturnValue);
    }
    return true;
}

bool Cmd_IsWeaponSkillType(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    TESObjectWEAP* pWeapon = nullptr;
    afReturnValue = 0.0;
    TESBoundObject* pObject = apRefObject->TESObjectREFR::GetObjectReference();
    if (pObject->GetFormType() == WEAP_ID) {
        pWeapon = static_cast<TESObjectWEAP*>(apRefObject->TESObjectREFR::GetObjectReference());
    }
    if (pWeapon || reinterpret_cast<intptr_t>(apParam1) != 45) {
        if (!pWeapon) {
            return false;
        }
    }
    else {
        afReturnValue = 1.0;
    }
    if (pWeapon && reinterpret_cast<intptr_t>(apParam1) == pWeapon->TESObjectWEAP::GetWeaponSkill()) {
        afReturnValue = 1.0;
    }

    if (Script::bSystemOutput) {
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
    if (apRefObject && apRefObject->IsActor()) {
        pActor = static_cast<Actor*>(apRefObject);
    }

    Actor* pOther = static_cast<Actor*>(apParam1);
    if (!pActor || !pOther) {
        return true;
    }
    TESActorBase* pActorBase = pActor->GetActorBase();
    TESActorBase* pOtherBase = pOther->GetActorBase();
    if (pActorBase && pOtherBase) {
        for (BSSimpleList<FACTION_RANK*>* pNode = pActorBase->GetFactionList();
             pNode && !pNode->IsEmpty() && 1.0 != afReturnValue;
             pNode = pNode->GetNext()) {
            TESFaction* pFaction = pNode->GetItem()->pFaction;
            if (pFaction && pOtherBase->GetFactionRank(pFaction, pOther == pPlayer) != -1) {
                afReturnValue = 1.0;
            }
        }
    }

    if (Script::bSystemOutput) {
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
    TESNPC* pNPC = nullptr;
    if (apRefObject) {
        TESBoundObject* pObject = apRefObject->GetObjectReference();
        if (pObject->GetFormType() == NPC__ID) {
            pNPC = static_cast<TESNPC*>(apRefObject->GetObjectReference());
        }
    }
    TESNPC* pOtherNPC = nullptr;
    if (apParam1) {
        TESBoundObject* pOtherObject = static_cast<TESObjectREFR*>(apParam1)->GetObjectReference();
        if (pOtherObject->GetFormType() == NPC__ID) {
            pOtherNPC = static_cast<TESNPC*>(static_cast<TESObjectREFR*>(apParam1)->GetObjectReference());
        }
    }
    if (pNPC && pOtherNPC) {
        TESRace* pRace = pNPC->GetRace();
        if (pRace == pOtherNPC->GetRace()) {
            afReturnValue = 1.0;
        }
    }

    if (Script::bSystemOutput) {
        Interface::PrintLine("SameRace >> %0.2f", afReturnValue);
    }
    return true;
}

bool Cmd_SameSex(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    afReturnValue = 0.0;
    TESActorBase* pActorBase = nullptr;
    if (apRefObject) {
        TESBoundObject* pObject = apRefObject->GetObjectReference();
        if (pObject->GetFormType() == NPC__ID) {
            pActorBase = static_cast<TESActorBase*>(apRefObject->GetObjectReference());
        }
    }
    TESActorBase* pOtherBase = nullptr;
    if (apParam1) {
        TESBoundObject* pOtherObject = static_cast<TESObjectREFR*>(apParam1)->GetObjectReference();
        if (pOtherObject->GetFormType() == NPC__ID) {
            pOtherBase = static_cast<TESActorBase*>(static_cast<TESObjectREFR*>(apParam1)->GetObjectReference());
        }
    }
    if (pActorBase && pOtherBase) {
        TESActorBase::SEX eSex = pActorBase->GetSex();
        if (eSex == pOtherBase->GetSex()) {
            afReturnValue = 1.0;
        }
    }

    if (Script::bSystemOutput) {
        Interface::PrintLine("SameSex >> %0.2f", afReturnValue);
    }
    return true;
}

// The process, item and list getters are trivial accessors.
bool Cmd_IsWeaponInList(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    afReturnValue = 0.0;
    BGSListForm* pList = static_cast<BGSListForm*>(apParam1);
    Actor* pActor = nullptr;
    if (apRefObject && apRefObject->IsActor()) {
        pActor = static_cast<Actor*>(apRefObject);
    }

    if (pActor && pActor->pCurrentProcess && pList) {
        BaseProcess* pProcess = pActor->pCurrentProcess;
        ItemChange* pWeaponItem = pProcess->GetCurrentWeapon();
        TESForm* pWeapon = nullptr;
        if (pWeaponItem && pWeaponItem->pContainerObj && pWeaponItem->pContainerObj->GetFormType() == WEAP_ID) {
            pWeapon = pWeaponItem->pContainerObj;
        }

        if (!pWeapon) {
            pWeapon = pFists;
        }
        if (pWeapon) {
            if (pList->GetFormList()->IsInList(pWeapon)) {
                afReturnValue = 1.0;
            }
        }
    }

    if (Script::bSystemOutput) {
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
    if (apRefObject && apRefObject->IsActor()) {
        Actor* pActor = static_cast<Actor*>(apRefObject);
        bool bFromCrime = false;
        afReturnValue = static_cast<double>(
            static_cast<int32_t>(pActor->GetFactionFightReaction(apParam1, bFromCrime))
        );
        if (Script::bSystemOutput) {
            // The result is truncated to 64 bits and the low 32 bits are printed.
            Interface::PrintLine("Faction Relation  >> %d", static_cast<int32_t>(static_cast<int64_t>(afReturnValue)));
        }
    }

    return true;
}

bool Cmd_IsLastIdlePlayed(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    afReturnValue = 0.0;
    if (apRefObject && apRefObject->IsActor()) {
        BaseProcess* pProcess = static_cast<Actor*>(apRefObject)->pCurrentProcess;
        if (pProcess->GetLastIdlePlayed() == apParam1) {
            afReturnValue = 1.0;
        }
        if (Script::bSystemOutput) {
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
    Warning(
        "SCRIPTS: Trying to access local variable %d in script '%s' -- variable not found.\r\n",
        auiID,
        GetFormEditorID()
    );
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
    m_header.bIsCompiled = abCompiled;
}

void Script::AddProfilerTime(float afTime) {
    fProfilerTimer = fProfilerTimer + afTime;
}

void Script::SetOwnerQuest(TESQuest* apQuest) {
    pOwnerQuest = apQuest;
}

bool Script::GetIsCompiled() {
    return m_header.bIsCompiled;
}

bool Script::GetInZoneConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (apRefObject && apParam1) {
        BGSEncounterZone* pEncounterZone = nullptr;
        TESObjectCELL* pParentCell = apRefObject->TESObjectREFR::GetParentCell();
        if (pParentCell) {
            pEncounterZone = pParentCell->GetEncounterZone();
        }
        if (!pEncounterZone) {
            TESWorldSpace* pWorldSpace = apRefObject->GetWorldSpace();
            if (pWorldSpace) {
                pEncounterZone = pWorldSpace->GetEncounterZone();
            }
        }
        if (pEncounterZone == apParam1) {
            afReturnValue = 1.0;
        }
    }

    if (bSystemOutput) {
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
    afReturnValue = 0.0;
    Actor* pActor = nullptr;
    if (apRefObject && apRefObject->IsActor()) {
        pActor = static_cast<Actor*>(apRefObject);
    }

    if (pActor && pActor->GetCurrentPackageName()) {
        const char* pPackageName = pActor->GetCurrentPackageName();
        if (!BSstricmp(pPackageName, "Alarm")) {
            afReturnValue = 1.0;
        }
    }

    if (bSystemOutput) {
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
    Actor* pActor = nullptr;
    if (apRefObject && apRefObject->IsActor()) {
        pActor = static_cast<Actor*>(apRefObject);
    }

    if (pActor && pActor->IsWeaponDrawn()) {
        afReturnValue = 1.0;
    }

    if (bSystemOutput) {
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
        afReturnValue = BGSOpenCloseForm::GetOpenState(apRefObject);
    }
    if (bSystemOutput) {
        Interface::PrintLine("GetOpenState >> %0.2f", afReturnValue);
    }
    return true;
}

bool Script::GetRadiationLevel(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    afReturnValue = 0.0;
    if (!apRefObject) {
        return true;
    }
    if (!apRefObject->IsActor()) {
        return true;
    }
    afReturnValue = 0.0;
    if (static_cast<Actor*>(apRefObject)->MobileObject::GetCurrentAIProcess()) {
        BaseProcess* pProcess = static_cast<Actor*>(apRefObject)->MobileObject::GetCurrentAIProcess();
        BaseProcess* pMagicProcess = static_cast<Actor*>(apRefObject)->MobileObject::GetCurrentAIProcess();
        BaseProcess* pWaterProcess = static_cast<Actor*>(apRefObject)->MobileObject::GetCurrentAIProcess();
        double dWaterDelta = pWaterProcess->GetRadiationWaterDelta();
        double dMagicDelta = pMagicProcess->GetRadiationMagicDelta();
        afReturnValue = pProcess->GetRadiationDelta() + dMagicDelta + dWaterDelta;
    }
    if (!bSystemOutput) {
        return true;
    }
    double dRadiationLevel = afReturnValue;
    const char* pName = apRefObject->TESObjectREFR::GetFullName();
    Interface::PrintLine("%s radiation level is %.2f", pName, dRadiationLevel);
    return true;
}

bool Script::IsCombatTargetConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (apRefObject &&
        apRefObject->IsActor() &&
        static_cast<Actor*>(apRefObject)->IsInCombatWithActor(static_cast<Actor*>(apParam1))) {
        afReturnValue = 1.0;
    }

    if (bSystemOutput) {
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
    afReturnValue = static_cast<int32_t>(ReferenceProcessLists.GetNumberOfActorsInHigh());
    if (bSystemOutput) {
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
    Actor* pActor = nullptr;
    if (apRefObject && apRefObject->IsActor()) {
        pActor = static_cast<Actor*>(apRefObject);
    }

    if (!pActor) {
        return true;
    }
    afReturnValue = pActor->GetFatiguePercentage();
    if (bSystemOutput) {
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
    Actor* pActor = nullptr;
    if (apRefObject && apRefObject->IsActor()) {
        pActor = static_cast<Actor*>(apRefObject);
    }

    if (!pActor) {
        return true;
    }
    afReturnValue = pActor->GetHealthPercentage();
    if (bSystemOutput) {
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
    if (apRefObject) {
        afReturnValue = apRefObject->fRefScale;
        if (bSystemOutput) {
            double dScale = apRefObject->GetScale();
            Interface::PrintLine("GetScale >> %0.2f (with base %0.2f)", afReturnValue, dScale);
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
    afReturnValue = 0.0;
    if (apRefObject) {
        LockData* pLock = apRefObject->GetLock();
        if (pLock) {
            afReturnValue = pLock->IsBroken();
        }
    }
    if (bSystemOutput) {
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
    if (bSystemOutput) {
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
    afReturnValue = 0.0;
    TESBoundObject* pGold = static_cast<TESBoundObject*>(TESForm::GetFormByNumericID(15));
    if (apRefObject && pGold && apRefObject->HasContainer()) {
        InventoryChanges* pInventoryChanges = InventoryChanges::GetInventoryChanges(apRefObject);
        if (pInventoryChanges) {
            afReturnValue = pInventoryChanges->GetObjectCount(pGold);
        }
    }

    if (bSystemOutput) {
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
    afReturnValue = 0.0;
    Actor* pActor = nullptr;
    if (apRefObject && apRefObject->IsActor()) {
        pActor = static_cast<Actor*>(apRefObject);
    }

    if (pActor) {
        BaseProcess* pProcess = pActor->MobileObject::GetCurrentAIProcess();
        afReturnValue = pProcess->GetFurnitureMarkerID();
    }
    if (bSystemOutput) {
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
    Sky* pSky = Sky::GetInstance();
    double dCurrentPercent = 0.0;
    if (pSky->Sky::GetCurrentWeather() && pSky->Sky::GetCurrentWeather()->IsPleasant()) {
        dCurrentPercent = pSky->GetWeatherPercent();
    }
    afReturnValue = dCurrentPercent;
    double dLastPercent = 0.0;
    if (pSky->Sky::GetLastWeather() && pSky->Sky::GetLastWeather()->IsPleasant()) {
        dLastPercent = 1.0 - pSky->GetWeatherPercent();
    }
    afReturnValue = afReturnValue + dLastPercent;
    if (bSystemOutput) {
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
    Sky* pSky = Sky::GetInstance();
    double dCurrentPercent = 0.0;
    if (pSky->Sky::GetCurrentWeather() && pSky->Sky::GetCurrentWeather()->IsCloudy()) {
        dCurrentPercent = pSky->GetWeatherPercent();
    }
    afReturnValue = dCurrentPercent;
    double dLastPercent = 0.0;
    if (pSky->Sky::GetLastWeather() && pSky->Sky::GetLastWeather()->IsCloudy()) {
        dLastPercent = 1.0 - pSky->GetWeatherPercent();
    }
    afReturnValue = afReturnValue + dLastPercent;
    if (bSystemOutput) {
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
    afReturnValue = Sky::GetInstance()->IsRaining() ? 1.0f : 0.0f;
    if (bSystemOutput) {
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
    afReturnValue = Sky::GetInstance()->IsSnowing() ? 1.0f : 0.0f;
    if (bSystemOutput) {
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
    afReturnValue = Sky::GetInstance()->GetWeatherPercent();
    if (bSystemOutput) {
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
    Actor* pActor = nullptr;
    if (apRefObject && apRefObject->IsActor()) {
        pActor = static_cast<Actor*>(apRefObject);
    }

    if (pActor) {
        afReturnValue = pActor->HasBeenAttacked();
    }
    if (bSystemOutput) {
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
    if (apRefObject && apRefObject->IsChild(false)) {
        afReturnValue = 1.0;
    }

    if (bSystemOutput) {
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
    afReturnValue = TESIdleManager::GetUsedItemLevel();
    if (bSystemOutput) {
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
    if (bSystemOutput) {
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
    afReturnValue = 0.0;
    Actor* pActor = nullptr;
    if (apRefObject && apRefObject->IsActor()) {
        pActor = static_cast<Actor*>(apRefObject);
    }

    if (pActor) {
        if (pActor->IsRestrained()) {
            afReturnValue = 1.0;
        }
        if (bSystemOutput) {
            if (0.0 == afReturnValue) {
                const char* pName = pActor->TESObjectREFR::GetFullName();
                Interface::PrintLine("%s is not restrained", pName);
            }
            else {
                const char* pName = pActor->TESObjectREFR::GetFullName();
                Interface::PrintLine("%s is restrained", pName);
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
    Actor* pActor = nullptr;
    if (apRefObject && apRefObject->IsActor()) {
        pActor = static_cast<Actor*>(apRefObject);
    }

    if (pActor) {
        afReturnValue = pActor->GetLevel();
    }
    if (bSystemOutput) {
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
    Actor* pActor = nullptr;
    if (apRefObject && apRefObject->IsActor()) {
        pActor = static_cast<Actor*>(apRefObject);
    }

    if (pActor) {
        afReturnValue = pActor->GetAlert();
        if (bSystemOutput) {
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
    afReturnValue = 0.0;
    TESObjectREFR* pActor = nullptr;
    if (apRefObject && apRefObject->IsActor()) {
        pActor = apRefObject;
    }

    if (pActor) {
        NiNode* pRoot = pActor->TESObjectREFR::Get3D();
        NiFixedString* pBip01Spine = FixedStrings::GetBip01_Spine();
        NiNode* pSpine = static_cast<NiNode*>(pActor->GetActorBone(pRoot, *pBip01Spine));
        if (!pSpine) {
            pSpine = static_cast<NiNode*>(pActor->GetActorBone(pRoot, "Bip01 Spine01"));
        }
        if (!pSpine || bhkNiCollisionObject::GetFaceUp(pSpine, nullptr)) {
            afReturnValue = 1.0;
        }
    }
    if (bSystemOutput) {
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
    afReturnValue = 0.0;
    TESObjectREFR* pActor = nullptr;
    if (apRefObject && apRefObject->IsActor()) {
        pActor = apRefObject;
    }

    if (pActor) {
        NiNode* pRoot = pActor->TESObjectREFR::Get3D();
        NiFixedString* pBip01Spine = FixedStrings::GetBip01_Spine();
        NiAVObject* pSpine = pActor->GetActorBone(pRoot, *pBip01Spine);
        if (!pSpine || NiNode::IsLeftFacingUp(pSpine, nullptr)) {
            afReturnValue = 1.0;
        }
    }
    if (bSystemOutput) {
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
    afReturnValue = 0.0;
    Actor* pActor = nullptr;
    if (apRefObject && apRefObject->IsActor()) {
        pActor = static_cast<Actor*>(apRefObject);
    }

    if (pActor && pActor->MobileObject::GetCurrentAIProcess() && pActor->MobileObject::GetCurrentPackage()) {
        TESPackage* pPackage = pActor->MobileObject::GetCurrentPackage();
        switch (pPackage->GetPackType()) {
            case PACKAGE_TYPE_EXPLORE: {
                afReturnValue = 0.0;
                break;
            }
            case PACKAGE_TYPE_FOLLOW: {
                afReturnValue = 1.0;
                break;
            }
            case PACKAGE_TYPE_ESCORT: {
                afReturnValue = 2.0;
                break;
            }
            case PACKAGE_TYPE_EAT: {
                afReturnValue = 3.0;
                break;
            }
            case PACKAGE_TYPE_SLEEP: {
                afReturnValue = 4.0;
                break;
            }
            case PACKAGE_TYPE_WANDER: {
                afReturnValue = 13.0;
                break;
            }
            case PACKAGE_TYPE_TRAVEL: {
                afReturnValue = 14.0;
                break;
            }
            case PACKAGE_TYPE_ACCOMPANY: {
                afReturnValue = 15.0;
                break;
            }
            case PACKAGE_TYPE_USE_ITEM_AT: {
                afReturnValue = 16.0;
                break;
            }
            case PACKAGE_TYPE_AMBUSH: {
                afReturnValue = 17.0;
                break;
            }
            case PACKAGE_TYPE_FLEE_NON_COMBAT: {
                afReturnValue = 18.0;
                break;
            }
            case PACKAGE_TYPE_CAST_MAGIC: {
                afReturnValue = 19.0;
                break;
            }
            case PACKAGE_TYPE_SANDBOX: {
                afReturnValue = 36.0;
                break;
            }
            case PACKAGE_TYPE_PATROL: {
                afReturnValue = 37.0;
                break;
            }
            case PACKAGE_TYPE_GUARD: {
                afReturnValue = 35.0;
                break;
            }
            case PACKAGE_TYPE_DIALOGUE: {
                afReturnValue = 34.0;
                break;
            }
            case PACKAGE_TYPE_USE_WEAPON: {
                afReturnValue = 33.0;
                break;
            }
            case PACKAGE_TYPE_COMBAT: {
                afReturnValue = 5.0;
                break;
            }
            case PACKAGE_TYPE_COMBAT_LOW: {
                afReturnValue = 20.0;
                break;
            }
            case PACKAGE_TYPE_ACTIVATE: {
                afReturnValue = 7.0;
                break;
            }
            case PACKAGE_TYPE_ALARM: {
                afReturnValue = 8.0;
                break;
            }
            case PACKAGE_TYPE_FLEE: {
                afReturnValue = 10.0;
                break;
            }
            case PACKAGE_TYPE_TRESPASS: {
                afReturnValue = 11.0;
                break;
            }
            case PACKAGE_TYPE_SPECTATOR: {
                afReturnValue = 9.0;
                break;
            }
            case PACKAGE_TYPE_REACT_TO_DEAD: {
                afReturnValue = 12.0;
                break;
            }
            case PACKAGE_TYPE_GET_UP: {
                afReturnValue = 21.0;
                break;
            }
            case PACKAGE_TYPE_DO_NOTHING: {
                afReturnValue = 24.0;
                break;
            }
            case PACKAGE_TYPE_IN_GAME_DIALOGUE: {
                afReturnValue = 6.0;
                break;
            }
            case PACKAGE_TYPE_SURFACE: {
                afReturnValue = 28.0;
                break;
            }
            case PACKAGE_TYPE_SEARCH_FOR_ATTACKER: {
                afReturnValue = 29.0;
                break;
            }
            case PACKAGE_TYPE_AVOID_RADIATION: {
                afReturnValue = 32.0;
                break;
            }
            default: {
                afReturnValue = -1.0;
                break;
            }
        }
    }

    if (bSystemOutput) {
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
    Actor* pActor = nullptr;
    if (apRefObject && apRefObject->IsActor()) {
        pActor = static_cast<Actor*>(apRefObject);
    }

    if (pActor && pActor->IsWaiting()) {
        afReturnValue = 1.0;
    }

    if (bSystemOutput) {
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
    afReturnValue = 0.0;
    if (apRefObject) {
        Animation* pAnimation = apRefObject->GetAnimation();
        if (pAnimation && !pAnimation->SpecialIdleDonePlaying()) {
            afReturnValue = 1.0;
        }
    }
    if (bSystemOutput) {
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
    afReturnValue = pPlayer->GetAmountSoldStolen();
    if (bSystemOutput) {
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
    if (!TES::pInstance || !TES::pInstance->TES::GetSky()) {
        return false;
    }
    Sky* pSky = TES::pInstance->TES::GetSky();
    afReturnValue = pSky->GetWindSpeed();
    if (bSystemOutput) {
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
    afReturnValue = 0.0;
    PlayerCharacter* pActor = nullptr;
    if (apRefObject && apRefObject->IsActor()) {
        pActor = static_cast<PlayerCharacter*>(apRefObject);
    }

    if (pActor && pActor->TESObjectREFR::GetParentCell()) {
        TESObjectCELL* pParentCell = pActor->TESObjectREFR::GetParentCell();
        if (pParentCell->IsActorCellOwner(pActor)) {
            afReturnValue = 1.0;
        }
    }

    if (bSystemOutput) {
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
    Actor* pActor = nullptr;
    if (apRefObject && apRefObject->IsActor()) {
        pActor = static_cast<Actor*>(apRefObject);
    }

    if (pActor) {
        afReturnValue = pActor->IsContinuingPackagePCNear();
    }
    if (bSystemOutput) {
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
    afReturnValue = 0.0;
    if (apRefObject && apRefObject->Get3D()) {
        NiNode* pRoot = apRefObject->Get3D();
        if (pRoot) {
            NiNode* pNode = pRoot->IsNode();
            afReturnValue = TESObjectREFR::HasAddonNodes(pNode) ? 1.0f : 0.0f;
        }
    }

    if (bSystemOutput) {
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
    afReturnValue = 0.0;
    afReturnValue = pPlayer->IsSleeping() ? 1.0f : 0.0f;
    if (bSystemOutput) {
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
    afReturnValue = 0.0;
    afReturnValue = pPlayer->IsAMurderer() ? 1.0f : 0.0f;
    if (bSystemOutput) {
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
    Actor* pActor = nullptr;
    if (apRefObject && apRefObject->IsActor()) {
        pActor = static_cast<Actor*>(apRefObject);
    }

    if (pActor && pActor->IsSwimming()) {
        afReturnValue = 1.0;
    }

    if (bSystemOutput) {
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
    if (apRefObject && apRefObject->TESForm::GetDestroyed()) {
        afReturnValue = 1.0;
    }

    if (bSystemOutput) {
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
    afReturnValue = 0.0;
    TESObjectREFR* pActor = nullptr;
    if (apRefObject && apRefObject->IsActor()) {
        pActor = apRefObject;
    }

    if (pActor && apParam1) {
        MagicTarget* pMagicTarget = pActor->GetMagicTarget();
        if (pMagicTarget->ChecksForMagicEffect(static_cast<EffectSetting*>(apParam1))) {
            afReturnValue = 1.0;
        }
    }

    if (bSystemOutput) {
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
    afReturnValue = pVATS.eMode;
    if (bSystemOutput) {
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
        afReturnValue = pPlayer->GetSandman();
    }
    if (bSystemOutput) {
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
    if (apRefObject->IsActor() && static_cast<Actor*>(apRefObject)->GetHasBeenEaten()) {
        afReturnValue = 1.0;
    }

    if (bSystemOutput) {
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
    Actor* pActor = nullptr;
    if (apRefObject && apRefObject->IsActor()) {
        pActor = static_cast<Actor*>(apRefObject);
    }

    if (pActor && pActor->GetGhost()) {
        afReturnValue = 1.0;
    }

    if (bSystemOutput) {
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
    afReturnValue = 0.0;
    Actor* pActor = nullptr;
    if (apRefObject && apRefObject->IsActor()) {
        pActor = static_cast<Actor*>(apRefObject);
    }

    if (pActor && pActor->GetOffersServicesNow()) {
        afReturnValue = 1.0;
    }

    if (!bSystemOutput) {
        return true;
    }
    if (0.0 == afReturnValue) {
        const char* pName = pActor->TESObjectREFR::GetFullName();
        Interface::PrintLine("%s is not offering services", pName);
    }
    else {
        const char* pName = pActor->TESObjectREFR::GetFullName();
        Interface::PrintLine("%s  is offering services", pName);
    }
    return true;
}

bool Script::GetBarterGoldConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    Actor* pActor = nullptr;
    if (apRefObject && apRefObject->IsActor()) {
        pActor = static_cast<Actor*>(apRefObject);
    }

    if (pActor) {
        afReturnValue = static_cast<uint32_t>(pActor->GetBarterGold());
        if (bSystemOutput) {
            int32_t iBarterGold = pActor->GetBarterGold();
            Interface::PrintLine("%s  has %d barter gold currently", pActor, iBarterGold);
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
    afReturnValue = 0.0;
    Actor* pActor = nullptr;
    if (apRefObject && apRefObject->IsActor()) {
        pActor = static_cast<Actor*>(apRefObject);
    }

    if (pActor->IsSneaking()) {
        afReturnValue = 1.0;
    }
    if (!bSystemOutput) {
        return true;
    }
    if (0.0 == afReturnValue) {
        const char* pName = TESFullName::GetFullName(pActor);
        Interface::PrintLine("%s is not sneaking", pName);
    }
    else {
        const char* pName = TESFullName::GetFullName(pActor);
        Interface::PrintLine("%s is sneaking", pName);
    }
    return true;
}

bool Script::IsRunningConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    Actor* pActor = nullptr;
    if (apRefObject && apRefObject->IsActor()) {
        pActor = static_cast<Actor*>(apRefObject);
    }

    if (pActor->IsRunning()) {
        afReturnValue = 1.0;
    }
    if (!bSystemOutput) {
        return true;
    }
    if (0.0 == afReturnValue) {
        const char* pName = TESFullName::GetFullName(pActor);
        Interface::PrintLine("%s is not running", pName);
    }
    else {
        const char* pName = TESFullName::GetFullName(pActor);
        Interface::PrintLine("%s is running", pName);
    }
    return true;
}

bool Script::GetFriendHitConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    TESObjectREFR* pActor = nullptr;
    if (apRefObject && apRefObject->IsActor()) {
        pActor = apRefObject;
    }

    if (!pActor) {
        return true;
    }
    ExtraDataList* pExtra = pActor->TESObjectREFR::GetExtra();
    afReturnValue = pExtra->GetFriendHitCount();
    if (bSystemOutput) {
        double dHitCount = afReturnValue;
        const char* pActorName = TESFullName::GetFullName(pActor);
        const char* pPlayerName = TESFullName::GetFullName(pPlayer);
        Interface::PrintLine("%s has hit %s %0.2f times", pPlayerName, pActorName, dHitCount);
    }
    return true;
}

bool Script::IsInInteriorConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (apRefObject && apRefObject->TESObjectREFR::GetParentCell()) {
        TESObjectCELL* pParentCell = apRefObject->TESObjectREFR::GetParentCell();
        if (pParentCell->GetInterior()) {
            afReturnValue = 1.0;
        }
    }

    if (!bSystemOutput) {
        return true;
    }
    if (0.0 == afReturnValue) {
        const char* pName = TESFullName::GetFullName(apRefObject);
        Interface::PrintLine("%s is not an Interior", pName);
    }
    else {
        const char* pName = TESFullName::GetFullName(apRefObject);
        Interface::PrintLine("%s is in an Interior", pName);
    }
    return true;
}

bool Script::GetWhichServiceConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = -1.0;
    afReturnValue = Interface::iServiceFlag;
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
    afReturnValue = 0.0;
    afReturnValue = Interface::Has360Controller() ? 1.0f : 0.0f;
    if (bSystemOutput) {
        Interface::PrintLine("IsXBox >> %1.0f", afReturnValue);
    }
    return true;
}

bool Script::IsPS3ConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    afReturnValue = 0.0;
    if (bSystemOutput) {
        Interface::PrintLine("IsPS3 >> %1.0f", afReturnValue);
    }
    return true;
}

bool Script::IsPCConditionFunction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    afReturnValue = 1.0;
    afReturnValue = Interface::Has360Controller() ? 0.0f : 1.0f;
    if (bSystemOutput) {
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
    afReturnValue = pPlayer->GetLastAction();
    return true;
}

bool Script::GetLastHitCriticalConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (apRefObject && apRefObject->IsActor() && static_cast<Actor*>(apRefObject)->GetLastHitData()) {
        HitData* pLastHitData = static_cast<Actor*>(apRefObject)->GetLastHitData();
        afReturnValue = pLastHitData->IsFlagSet(HitData::HD_CRITICAL) ? 1.0f : 0.0f;
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
    int8_t movementFlags = static_cast<int8_t>(static_cast<Actor*>(apRefObject)->GetMoveMode());
    if ((movementFlags & ActorMover::MF_FORWARD) != 0) {
        afReturnValue = 1.0;
    }
    else if ((movementFlags & ActorMover::MF_BACKWARD) != 0) {
        afReturnValue = 2.0;
    }
    else if ((movementFlags & ActorMover::MF_LEFT) != 0) {
        afReturnValue = 3.0;
    }
    else if ((movementFlags & ActorMover::MF_RIGHT) != 0) {
        afReturnValue = 4.0;
    }
    return true;
}

bool Script::GetAnimActionConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = -1.0;
    if (apRefObject && apRefObject->IsActor()) {
        afReturnValue = static_cast<Actor*>(apRefObject)->GetAnimAction();
    }

    return true;
}

bool Script::GetPlantedExplosiveConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    afReturnValue = 0.0;
    if (apRefObject &&
        apRefObject->IsActor() &&
        static_cast<MobileObject*>(apRefObject)->MobileObject::GetCurrentAIProcess()) {
        BaseProcess* pProcess = static_cast<MobileObject*>(apRefObject)->MobileObject::GetCurrentAIProcess();
        afReturnValue = pProcess->GetPlantedExplosive();
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
    if (apRefObject && apRefObject->IsActor()) {
        afReturnValue = ReferenceProcessLists.IsActorTalkingThroughActivator(static_cast<Actor*>(apRefObject));
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
        afReturnValue = static_cast<Actor*>(apRefObject)->GetSpeakingEmotion();
        if (!static_cast<MobileObject*>(apRefObject)->GetShouldUseEmotionAnimation()) {
            afReturnValue = -1.0;
        }
    }
    if (bSystemOutput) {
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
    if (bSystemOutput) {
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
    TESObject* pUsedItem = TESIdleManager::GetUsedItem();
    if (pUsedItem && pUsedItem->GetWaterType()) {
        afReturnValue = 1.0;
    }

    if (bSystemOutput) {
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
    if (apRefObject && apRefObject->IsActor()) {
        if (static_cast<Actor*>(apRefObject)->bPlayerTeammate) {
            afReturnValue = 1.0;
        }
        if (bSystemOutput) {
            Interface::PrintLine("Player teammate  >> %d", static_cast<int32_t>(afReturnValue));
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
    if (apRefObject && apRefObject->IsActor()) {
        if (static_cast<const void*>(apRefObject) == static_cast<const void*>(pPlayer)) {
            afReturnValue = static_cast<int32_t>(static_cast<PlayerCharacter*>(apRefObject)->GetMinorCrimeCount());
        }
        else {
            afReturnValue = static_cast<Actor*>(apRefObject)->GetMinorCrimeCount();
        }
        if (bSystemOutput) {
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
        afReturnValue = static_cast<Actor*>(apRefObject)->GetMajorCrimesNumber();
    }
    else {
        afReturnValue = static_cast<Actor*>(apRefObject)->GetMajorCrimeCount();
    }
    if (bSystemOutput) {
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
    afReturnValue = -1.0;
    if (apRefObject) {
        afReturnValue = BGSDestructibleObjectForm::CalcDestructionStage(apRefObject);
    }
    if (bSystemOutput) {
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
        afReturnValue = pPlayer->IsHardcore();
    }
    if (bSystemOutput) {
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
        afReturnValue = pPlayer->GetAlwaysHardcore();
    }
    if (bSystemOutput) {
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
    afReturnValue = 0.0;
    if (apRefObject && apRefObject->IsActor()) {
        int8_t moveMode = static_cast<int8_t>(static_cast<Actor*>(apRefObject)->GetMoveMode());
        if ((moveMode & 16) != 0) {
            afReturnValue = 1.0;
        }
        else if ((moveMode & 32) != 0) {
            afReturnValue = 2.0;
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
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &pMessage,
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
            apParameters,
            apCompiledParams,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &afReturnValue,
            &auiOffset,
            pDescriptionText,
            pText
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
            pMessageText,
            nullptr,
            pTitle,
            &Script::HandleMessageMenu,
            0,
            0x17,
            0.0f,
            0.0f,
            apButtons[0],
            apButtons[1],
            apButtons[2],
            apButtons[3],
            apButtons[4],
            apButtons[5],
            apButtons[6],
            apButtons[7],
            apButtons[8],
            apButtons[9],
            static_cast<const char*>(nullptr)
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
        // Widens the display time to a 64-bit integer before rounding it to float.
        float fSeconds = static_cast<float>(static_cast<int64_t>(static_cast<uint64_t>(pMessage->GetDisplayTime())));
        const char* pMessageText = NavMeshInfo::GetCellOrCoords(const_cast<char*>(pText->String()), 0);
        Interface::ShowMessage(pMessageText, 0, pIconName, nullptr, fSeconds, false);
    }
    pText->~BSStringT<char>();
    return true;
}

bool Script::VerboseOutput() {
    return bSystemOutput;
}

bool Script::GetPosConditionFunction(
    TESObjectREFR* apRefObject,
    void* apParam1,
    void* apParam2,
    double& afReturnValue
) {
    if (apRefObject) {
        const NiPoint3& kLocation = apRefObject->GetLocationOnReference();
        switch (static_cast<int32_t>(reinterpret_cast<intptr_t>(apParam1))) {
            case 88: {
                afReturnValue = kLocation.x;
                break;
            }
            case 89: {
                afReturnValue = kLocation.y;
                break;
            }
            case 90: {
                afReturnValue = kLocation.z;
                break;
            }
        }
        if (bSystemOutput) {
            Interface::PrintLine(
                "GetPos: %c >> %0.2f",
                static_cast<int32_t>(reinterpret_cast<intptr_t>(apParam1)),
                afReturnValue
            );
        }
    }
    return true;
}

void Script::LoadPendingReferenceLists(BGSLoadGameBuffer* apLoadGameBuffer) {
    Script::ScriptRefListCrit.Enter(nullptr);
    Script::ReferencesToEnable.RemoveAll();
    Script::ReferencesToDisable.RemoveAll();
    Script::ReferencesToDelete.RemoveAll();
    Script::ReferencesToFadeDisable.RemoveAll();
    uint32_t uiEnableCount = apLoadGameBuffer->LoadVariableSizedValue();
    if (uiEnableCount) {
        for (uint32_t i = 0; i < uiEnableCount; ++i) {
            TESObjectREFR* pRef = new TESObjectREFR;
            TESForm* pForm = TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID());
            pRef = dynamic_cast<TESObjectREFR*>(pForm);
            Script::ReferencesToEnable.AddHead(pRef);
        }
    }
    uint32_t uiDisableCount = apLoadGameBuffer->LoadVariableSizedValue();
    if (uiDisableCount) {
        for (uint32_t i = 0; i < uiDisableCount; ++i) {
            TESObjectREFR* pRef = new TESObjectREFR;
            TESForm* pForm = TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID());
            pRef = dynamic_cast<TESObjectREFR*>(pForm);
            Script::ReferencesToDisable.AddHead(pRef);
        }
    }
    uint32_t uiDeleteCount = apLoadGameBuffer->LoadVariableSizedValue();
    if (uiDeleteCount) {
        for (uint32_t i = 0; i < uiDeleteCount; ++i) {
            TESObjectREFR* pRef = new TESObjectREFR;
            TESForm* pForm = TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID());
            pRef = dynamic_cast<TESObjectREFR*>(pForm);
            Script::ReferencesToDelete.AddHead(pRef);
        }
    }
    uint32_t uiFadeDisableCount = apLoadGameBuffer->LoadVariableSizedValue();
    if (uiFadeDisableCount) {
        for (uint32_t i = 0; i < uiFadeDisableCount; ++i) {
            TESObjectREFR* pRef = new TESObjectREFR;
            TESForm* pForm = TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID());
            pRef = dynamic_cast<TESObjectREFR*>(pForm);
            Script::ReferencesToFadeDisable.AddHead(pRef);
        }
    }
    Script::ScriptRefListCrit.Leave();
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
    Script::ScriptRefListCrit.Enter(nullptr);
    if (apRef) {
        Script::ReferencesToEnable.Remove(apRef);
        if (abFade) {
            if (!Script::IsPendingFadeDisabledReference(apRef)) {
                Script::ReferencesToFadeDisable.AddHead(apRef);
            }
        }
        else if (!Script::IsPendingDisabledReference(apRef)) {
            Script::ReferencesToDisable.AddHead(apRef);
        }
    }
    Script::ScriptRefListCrit.Leave();
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
        ScriptRunManager::Instance()->InitActionList(locals->m_pMasterScript, ownerObject, locals);
    }
}

void Script::ClearButtonPressed() {
    Script::m_cButtonValue = -1;
    Script::m_iButtonOwner = 0;
}

bool Script::GetNoRumors(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
    afReturnValue = 0.0;
    Actor* pActor = nullptr;
    if (apRefObject && apRefObject->IsActor()) {
        pActor = static_cast<Actor*>(apRefObject);
    }

    if (!pActor) {
        return true;
    }
    if (pActor->HasNoRumors()) {
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
    float fUnused;
    float fIntensity = 500.0f;
    float fDuration = 0.10000000149011612f;
    float fFrequency = 0.05000000074505806f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &fIntensity,
            &fDuration,
            &fFrequency,
            &fUnused
        )) {
        return false;
    }
    FOHUDEffectManager* pHUDEffects = Interface::GetHUDEffects();
    pHUDEffects->TriggerShudder(fIntensity, fDuration, fFrequency);
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
    if (apRefObject->IsActor()) {
        CombatController* pCombatController = static_cast<Actor*>(apRefObject)->GetCombatController();
        if (pCombatController) {
            pCombatController->DebugCombatPlan();
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
    int8_t cPrefINIPath[264];
    int8_t cCustomINIPath[264];
    int8_t cINIPath[268];
    if (pAppININame) {
        const char* pPersonalPath = BSUtilities::GetPersonalPath();
        BSstrcpy(static_cast<char*>(static_cast<void*>(cINIPath)), 0x104u, pPersonalPath);
        BSstrcat(static_cast<char*>(static_cast<void*>(cINIPath)), 0x104u, pAppININame);
        INISettingCollection* pCollection = SettingT<INISettingCollection>::GetCollection();
        pCollection->ReadSettingsFromFile(static_cast<const char*>(static_cast<const void*>(cINIPath)));
    }
    if (pAppCustomININame) {
        const char* pPersonalPath = BSUtilities::GetPersonalPath();
        BSstrcpy(static_cast<char*>(static_cast<void*>(cCustomINIPath)), 0x104u, pPersonalPath);
        BSstrcat(static_cast<char*>(static_cast<void*>(cCustomINIPath)), 0x104u, pAppCustomININame);
        INISettingCollection* pCollection = SettingT<INISettingCollection>::GetCollection();
        pCollection->ReadSettingsFromFile(static_cast<const char*>(static_cast<const void*>(cCustomINIPath)));
    }
    if (pAppPrefININame) {
        const char* pPersonalPath = BSUtilities::GetPersonalPath();
        BSstrcpy(static_cast<char*>(static_cast<void*>(cPrefINIPath)), 0x104u, pPersonalPath);
        BSstrcat(static_cast<char*>(static_cast<void*>(cPrefINIPath)), 0x104u, pAppPrefININame);
        INIPrefSettingCollection* pCollection = SettingT<INIPrefSettingCollection>::GetCollection();
        pCollection->ReadSettingsFromFile(static_cast<const char*>(static_cast<const void*>(cPrefINIPath)));
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
    int32_t iOpen = 0;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iOpen
        )) {
        return false;
    }
    if (apRefObject) {
        if (iOpen) {
            if (BGSOpenCloseForm::GetOpenState(apRefObject) > BGSOpenCloseForm::OBJECT_OPENING) {
                apRefObject->Activate(nullptr, false, nullptr, 1);
            }
            apRefObject->SetLastFinishedSequence("Open");
        }
        else {
            if (BGSOpenCloseForm::GetOpenState(apRefObject) < BGSOpenCloseForm::OBJECT_CLOSED) {
                apRefObject->Activate(nullptr, false, nullptr, 1);
            }
            apRefObject->SetLastFinishedSequence("Close");
        }
        ExtraDataList* pExtra = apRefObject->TESObjectREFR::GetExtra();
        pExtra->RemoveSavedAnimation();
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
    int32_t iActorValue = 0;
    float fValue = 0.0f;
    if (!Script::ParseParameters(
            apParameters,
            apCompiledParams,
            auiOffset,
            apRefObject,
            apContainer,
            apScript,
            apScriptLocals,
            &iActorValue,
            &fValue
        )) {
        return false;
    }
    Actor* pActor = dynamic_cast<Actor*>(apRefObject);
    if (pActor) {
        if (ActorValue::IsFlagSet(static_cast<ActorValue::Index>(iActorValue), ActorValue::NO_SCRIPT_MODAV)) {
            if (bSystemOutput) {
                const char* pName = ActorValue::GetActorValueScriptName(static_cast<ActorValue::Index>(iActorValue));
                Interface::PrintLine("Actor Value '%s' cannot be modified in scripts or the console.", pName);
            }
        }
        else {
            if (iActorValue == ActorValue::HEALTH && pActor->IsChild(false)) {
                pActor->HandleHealthDamage(nullptr, -fValue);
                return true;
            }
            float fDamage = fValue;
            if (!ActorValue::IsFlagSet(static_cast<ActorValue::Index>(iActorValue), ActorValue::DAMAGE_IS_POSITIVE)) {
                fDamage = -std::fabs(fValue);
            }
            pActor->DamageModActorValue(static_cast<ActorValue::Index>(iActorValue), fDamage, nullptr);
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
    if (bSystemOutput) {
        if (bBordersOn) {
            Interface::PrintLine("Borders -> %s", "On");
        }
        else {
            Interface::PrintLine("Borders -> %s", "Off");
        }
    }
    return true;
}
