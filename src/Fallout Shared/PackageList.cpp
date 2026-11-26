#include "Fallout Shared/PackageList.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSDecalManager.h"
#include "Fallout Shared/BGSLoadedReferenceCollection.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/BGSQuestObjective.h"
#include "Fallout Shared/BGSTalkingActivator.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/Date.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/FOScriptFunctions.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/FOCollisionListener.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Magic/EffectSettingCollection.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageEventAction.h"
#include "Fallout Shared/PackageSchedule.h"
#include "Fallout Shared/PackageTarget.h"
#include "Fallout Shared/Pathfinding/PathBuilder.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/ScriptCompileData.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/SpeedTree/BSTreeManager.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESConditionFunctions.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESIdleManager.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESQuestTarget.h"
#include "Fallout Shared/TESRaceForm.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESScriptCompiler.h"
#include "Fallout Shared/TESScriptFunctions.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/Combat/CombatTargetMap.h"
#include "Fallout/AI/Combat/CombatThreatMap.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/Combat/HitData.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/FleePackage.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Audio/FalloutRadio.h"
#include "Fallout/Audio/ImpactMixer.h"
#include "Fallout/Debug/DebugTextManager.h"
#include "Fallout/Dialogue/DialogueResponse.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/LoadingMenu.h"
#include "Fallout/Interface/Menus/VATSMenu.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Misc/Calendar.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadManager.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdio.h"

#include <cstdint>

bool TestHour(int32_t aitime, int32_t aiduration, float afhour) {
    int32_t iCurrentHour = static_cast<int32_t>(afhour);
    int32_t iEndHour = aiduration + aitime;
    if (iCurrentHour == aitime) {
        return true;
    }
    if (iEndHour <= 23) {
        if (iCurrentHour > aitime && iCurrentHour < iEndHour) {
            return true;
        }
    }
    else if ((iCurrentHour > aitime || iCurrentHour < iEndHour - 24) && iCurrentHour < iEndHour) {
        return true;
    }
    return false;
}

void PackageList::InitItem(TESForm* apOwner) {
    BSSimpleList<TESPackage*>* pList = &this->listPackages;
    BSSimpleList<TESPackage*>* pLastList = nullptr;
    TESFile* pOwnerFile = apOwner ? apOwner->GetFile(-1) : nullptr;
    while (pList && !pList->IsEmpty()) {
        uint32_t uiTempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pList->m_item));
        if (apOwner) {
            TESForm::AddCompileIndex(uiTempInt, pOwnerFile);
        }
        TESForm* pForm = TESForm::GetFormByNumericID(uiTempInt);
        TESPackage* pPack = dynamic_cast<TESPackage*>(pForm);
        if (pPack) {
            pList->SetItem(pPack);
            pLastList = pList;
            pList = pList->m_pkNext;
        }
        else {
            BSStringT<char> kTempString;
            if (apOwner) {
                apOwner->GetFormDetailedString(kTempString);
            }
            else {
                kTempString.Set("UNKNOWN form", 0);
            }
            Warning("MASTERFILE: Could not find Package (%08X) for %s.", uiTempInt, kTempString.pString);
            if (pLastList) {
                // The list holds the form ID in the item's pointer until the package is found.
                TESPackage* pItem = reinterpret_cast<TESPackage*>(static_cast<uintptr_t>(uiTempInt));
                pLastList->Remove(pItem);
                pList = pLastList->m_pkNext;
            }
            else {
                pList->RemoveHead();
            }
        }
    }
}

bool PackageList::Compare(PackageList* apCompare) {
    BSSimpleList<TESPackage*>* pList = &listPackages;
    BSSimpleList<TESPackage*>* pCompareList = &apCompare->listPackages;
    if (pList->IsEmpty()) {
        if (!pCompareList->IsEmpty()) {
            return true;
        }
    }
    else {
        while (pList && pList->m_item && pCompareList && pCompareList->m_item) {
            if (pList->m_item != pCompareList->m_item) {
                return true;
            }
            pList = pList->GetNext();
            pCompareList = pCompareList->GetNext();
        }
        if (pList || pCompareList) {
            return true;
        }
    }
    return false;
}

TESPackage* PackageList::FindCurrentPackage(Actor* apActor) {
    BSSimpleList<TESPackage*>* pList = &this->listPackages;
    TESPackage* pCurrentPack = nullptr;
    if (apActor) {
        pCurrentPack = apActor->GetCurrentPackage();
    }
    if (!pList->IsEmpty()) {
        while (pList) {
            TESPackage* pPack = pList->m_item;
            if (!pPack) {
                break;
            }
            if ((pPack->IsReadyToRun(apActor) && !pPack->IsNeverToRun() && !pPack->GetOncePerDay()) ||
                (pPack->GetOncePerDay() && !apActor->m_Extra.IsInRunOnceDayPackageList(pPack))) {
                PackageSchedule* pSchedule = pPack->GetPackageSchedule();
                if (pSchedule) {
                    bool bOverlapDayOf = false;
                    bool bOverlapDay = false;
                    bool bOverlapMonth = false;
                    int32_t iStartTime = pSchedule->GetStartTime();
                    if (iStartTime + pSchedule->PSData.iDuration > 24) {
                        bOverlapDayOf = pPack->IsitDayofTheWeek(true);
                        int32_t iDay = appCalendar.GetDay() - 1;
                        if (iDay < 0) {
                            iDay = 30;
                        }
                        if (pSchedule->PSData.cDate == iDay) {
                            bOverlapDay = true;
                        }
                        int32_t iMonth = appCalendar.GetMonth() - 1;
                        if (iMonth < 0) {
                            iMonth = 12;
                        }
                        if (pSchedule->PSData.eMonth == iMonth) {
                            bOverlapMonth = true;
                        }
                    }
                    if ((pSchedule->PSData.eMonth == PSM_NONE ||
                         pSchedule->PSData.eMonth == appCalendar.GetMonth() ||
                         bOverlapMonth) &&
                        (!pSchedule->PSData.cDate || pSchedule->PSData.cDate == appCalendar.GetDay() || bOverlapDay) &&
                        (pSchedule->GetStartDayOfWeek() == PSD_NONE ||
                         pPack->IsitDayofTheWeek(false) ||
                         bOverlapDayOf)) {
                        char cStartTime;
                        int32_t iDuration;
                        float fCurrentHour;
                        if (pSchedule->GetStartTime() == -1 ||
                            (fCurrentHour = appCalendar.GetHour(),
                             iDuration = pSchedule->PSData.iDuration,
                             cStartTime = pSchedule->GetStartTime(),
                             TestHour(cStartTime, iDuration, fCurrentHour))) {
                            TESObjectREFR* pTargetRef = nullptr;
                            PackageTarget* pTarget = pPack->GetPackageTarget();
                            if (pTarget) {
                                pTargetRef = pTarget->GetTargReference();
                            }
                            if (pPack->packConditions.IsTrue(apActor, pTargetRef)) {
                                if (Interface::GetPickREFR() == apActor) {
                                    char acMessage[308];
                                    const char* pcEditorID = pPack->GetFormEditorID();
                                    PACKAGE_TYPE ePackType = pPack->GetPackType();
                                    const char* pcPackType = packageTypeToString[ePackType];
                                    std::sprintf(
                                        acMessage,
                                        "%s Picked Package %s (%s)",
                                        apActor->GetFullName(),
                                        pcPackType,
                                        pcEditorID
                                    );
                                    Interface::PrintLine(static_cast<const char*>(acMessage));
                                }
                                if (pPack->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_NONE) {
                                    TESObjectREFR* pLinkedRef = apActor->GetLinkedRef();
                                    pPack->CalculateProcedureType(pLinkedRef);
                                }
                                if (pCurrentPack &&
                                    pCurrentPack != pPack &&
                                    pCurrentPack->GetPackType() == PACKAGE_TYPE_FOLLOW) {
                                    TESObjectREFR* pFollowTarget = apActor->pCurrentProcess->GetTarget();
                                    Actor* pAct = dynamic_cast<Actor*>(pFollowTarget);
                                    if (pAct) {
                                        pAct->m_Extra.RemoveFollower(apActor);
                                    }
                                }

                                if (pCurrentPack && pCurrentPack != pPack) {
                                    Script::SetActionFlag(
                                        pCurrentPack,
                                        &apActor->m_Extra,
                                        SCRIPT_ACTION_ONPACKAGECHANGE
                                    );
                                    apActor->pCurrentProcess->ChangePackage(apActor, pCurrentPack);
                                    apActor->pCurrentProcess->SetPackageDoneOnce(false);
                                }

                                if (pPack && pPack != pCurrentPack) {
                                    Script::SetActionFlag(pPack, &apActor->m_Extra, SCRIPT_ACTION_ONPACKAGESTART);
                                }

                                return pPack;
                            }
                        }
                    }
                }
            }
            pList = pList->m_pkNext;
        }
    }
    return pCurrentPack;
}

// The schedule's month, date and start-time are each read as a sign-extended byte.
void PackageList::FindMissedPackages(
    Actor* apActor,
    BSSimpleList<TESPackage*>* apPacks,
    float afStartdate,
    float afhour
) {
    TESPackage* pCurrentPack = nullptr;
    int32_t iDay = apActor->pCurrentProcess->GetProcessedDay();
    int32_t iMonth = apActor->pCurrentProcess->GetProcessedMonth();
    float fHour = afhour;
    int32_t iEndDay = appCalendar.GetDay();
    int32_t iEndMonth = appCalendar.GetMonth();
    while (iDay) {
        BSSimpleList<TESPackage*>* pList = &this->listPackages;
        if (apActor) {
            pCurrentPack = apActor->GetCurrentPackage();
        }
        if (pList->IsEmpty()) {
            iDay = 0;
        }
        else {
            TESPackage* pPack = nullptr;
            PackageSchedule* pSchedule = nullptr;
            while (pList) {
                pPack = pList->m_item;
                if (!pPack) {
                    break;
                }
                if (pPack->GetMustComplete() && pPack->HasActorCompletedPackage(apActor)) {
                    pSchedule = pPack->GetPackageSchedule();
                    if (pSchedule &&
                        (pSchedule->PSData.eMonth == PSM_NONE || pSchedule->PSData.eMonth <= iMonth) &&
                        (!pSchedule->PSData.cDate || pSchedule->PSData.cDate <= iDay)) {
                        PSDAYOFWEEK eStartDayOfWeek;
                        if (pSchedule->GetStartDayOfWeek() == PSD_NONE ||
                            (eStartDayOfWeek = pSchedule->GetStartDayOfWeek(),
                             static_cast<int32_t>(eStartDayOfWeek) <= appCalendar.GetDayOfTheWeek())) {
                            if ((pSchedule->GetStartTime() == -1 ||
                                 fHour > static_cast<double>(pSchedule->GetStartTime())) &&
                                pPack != pCurrentPack &&
                                !apPacks->IsInList(pPack)) {
                                if (pPack->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_NONE) {
                                    TESObjectREFR* pLinkedRef = apActor->GetLinkedRef();
                                    pPack->CalculateProcedureType(pLinkedRef);
                                }
                                apPacks->AddTail(pPack);
                            }
                        }
                    }
                }
                pList = pList->m_pkNext;
            }
            fHour = fHour + 1.0;
            if (fHour > 23.0) {
                ++iDay;
                fHour = 0.0f;
                uint16_t sDaysInMonth = Date::GetDaysInMonth(static_cast<CALENDAR_MONTH>(static_cast<char>(iMonth)));
                if (iDay > sDaysInMonth) {
                    iDay -= sDaysInMonth;
                    if (++iMonth >= 12) {
                        iMonth -= 12;
                    }
                }
            }
            if (iDay == iEndDay && iMonth == iEndMonth) {
                int32_t iHour = static_cast<int32_t>(fHour);
                if (iHour >= static_cast<int32_t>(appCalendar.GetHour())) {
                    iDay = 0;
                }
            }
        }
    }
    BSSimpleList<TESPackage*>* pFirst = apPacks;
    if (apPacks) {
        int32_t iCount = pFirst->ItemsInList();
        bool bSwappedItems = true;
        while (iCount && bSwappedItems) {
            bSwappedItems = false;
            for (BSSimpleList<TESPackage*>* pSecond = pFirst; pSecond; pSecond = pSecond->m_pkNext) {
                TESPackage* pFirstData = pFirst->m_item;
                TESPackage* pSecondData = pSecond->m_item;
                PackageSchedule* pSchedule = pSecondData->GetPackageSchedule();
                if (pFirstData->ComparePackageSchedule(pSchedule) == 1) {
                    pFirst->SetItem(pSecondData);
                    pSecond->SetItem(pFirstData);
                    bSwappedItems = true;
                }
            }
            pFirst = pFirst->m_pkNext;
            --iCount;
        }
    }
}
