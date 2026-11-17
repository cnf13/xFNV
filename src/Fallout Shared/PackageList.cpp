#include "Fallout Shared/PackageList.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSCore/RTTI.h"
#include "BSCore/TLSData.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/SettingCollection.h"
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
#include "Reconstruction.hpp"
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
    else {
        if ((iCurrentHour > aitime || iCurrentHour < iEndHour - 24) && iCurrentHour < iEndHour) {
            return true;
        }
    }
    return false;
}

void PackageList::InitItem(TESForm* apOwner) {
    TESFile* pownerFile;
    BSSimpleList<TESPackage*>* plist = &this->listPackages;
    BSSimpleList<TESPackage*>* plastlist = nullptr;
    if (apOwner) {
        pownerFile = apOwner->GetFile(-1);
    }
    else {
        pownerFile = nullptr;
    }
    while (plist && !plist->IsEmpty()) {
        uint32_t tempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(plist->m_item));
        if (apOwner) {
            TESForm::AddCompileIndex(tempint, pownerFile);
        }
        TESForm* FormByNumericID = TESForm::GetFormByNumericID(tempint);
        TESPackage* ppack = dynamic_cast<TESPackage*>(FormByNumericID);
        if (ppack) {
            plist->SetItem(ppack);
            plastlist = plist;
            plist = plist->m_pkNext;
        }
        else {
            BSStringT<char> tempstring;
            if (apOwner) {
                apOwner->GetFormDetailedString(tempstring);
            }
            else {
                tempstring.Set("UNKNOWN form", 0);
            }
            char* v3 = tempstring.pString;
            Warning("MASTERFILE: Could not find Package (%08X) for %s.", tempint, v3);
            if (plastlist) {
                // The list holds the form ID in the item's pointer until the package is found.
                TESPackage* pItem = reinterpret_cast<TESPackage*>(static_cast<uintptr_t>(tempint));
                plastlist->Remove(pItem);
                plist = plastlist->m_pkNext;
            }
            else {
                plist->RemoveHead();
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
    TESPackage* apCurrentPack = nullptr;
    if (apActor) {
        apCurrentPack = apActor->GetCurrentPackage();
    }
    if (!pList->IsEmpty()) {
        while (pList) {
            TESPackage* pPack = pList->m_item;
            if (!pPack) {
                break;
            }
            if ((pPack->IsReadyToRun(apActor) && (!pPack->IsNeverToRun() && !pPack->GetOncePerDay())) ||
                (pPack->GetOncePerDay() && !(&apActor->m_Extra)->IsInRunOnceDayPackageList(pPack))) {
                PackageSchedule* pSchedule_2 = pPack->GetPackageSchedule();
                if (pSchedule_2) {
                    bool bOverlapdayof = false;
                    bool bOverlapday = false;
                    bool bOverlapmonth = false;
                    int32_t StartTime = pSchedule_2->GetStartTime();
                    if (StartTime + pSchedule_2->PSData.iDuration > 24) {
                        bOverlapdayof = pPack->IsitDayofTheWeek(true);
                        int32_t day = appCalendar.GetDay() - 1;
                        if (day < 0) {
                            day = 30;
                        }
                        if (pSchedule_2->PSData.cDate == day) {
                            bOverlapday = true;
                        }
                        int32_t imonth = appCalendar.GetMonth() - 1;
                        if (imonth < 0) {
                            imonth = 12;
                        }
                        if (pSchedule_2->PSData.eMonth == imonth) {
                            bOverlapmonth = true;
                        }
                    }
                    if (pSchedule_2->PSData.eMonth == PSM_NONE ||
                        pSchedule_2->PSData.eMonth == appCalendar.GetMonth() || bOverlapmonth) {
                        if (!pSchedule_2->PSData.cDate || pSchedule_2->PSData.cDate == appCalendar.GetDay() ||
                            bOverlapday) {
                            if (pSchedule_2->GetStartDayOfWeek() == PSD_NONE ||
                                (pPack->IsitDayofTheWeek(false) || bOverlapdayof)) {
                                char v5;
                                int32_t iDuration;
                                float fCurrentHour;
                                if (pSchedule_2->GetStartTime() == -1 ||
                                    (fCurrentHour = appCalendar.GetHour(),
                                     (iDuration = pSchedule_2->PSData.iDuration,
                                      (v5 = pSchedule_2->GetStartTime(), TestHour(v5, iDuration, fCurrentHour))))) {
                                    TESObjectREFR* pTargetRef = nullptr;
                                    PackageTarget* pTarget = pPack->GetPackageTarget();
                                    if (pTarget) {
                                        pTargetRef = pTarget->GetTargReference();
                                    }
                                    if ((&pPack->packConditions)->IsTrue(apActor, pTargetRef)) {
                                        if (Interface::GetConsoleSelection() == apActor) {
                                            char Format[308];
                                            const char* v6 = pPack->GetFormEditorID();
                                            const void* fCurrentHoura = v6;
                                            PACKAGE_TYPE PackType = pPack->GetPackType();
                                            const void* CellOrCoords_0 = packageTypeToString[PackType];
                                            std::sprintf(
                                                Format, "%s Picked Package %s (%s)", apActor->GetFullName(),
                                                CellOrCoords_0, fCurrentHoura
                                            );
                                            Interface::PrintLine(static_cast<const char*>(Format));
                                        }
                                        if (pPack->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_NONE) {
                                            TESObjectREFR* LinkedRef = apActor->GetLinkedRef();
                                            pPack->CalculateProcedureType(LinkedRef);
                                        }
                                        if (apCurrentPack) {
                                            if (apCurrentPack != pPack &&
                                                apCurrentPack->GetPackType() == PACKAGE_TYPE_FOLLOW) {
                                                BaseProcess* CurrentAIProcess = apActor->pCurrentProcess;
                                                TESObjectREFR* v9 = CurrentAIProcess->GetTarget();
                                                Actor* pact = dynamic_cast<Actor*>(v9);
                                                if (pact) {
                                                    (&pact->m_Extra)->RemoveFollower(apActor);
                                                }
                                            }
                                        }
                                        if (apCurrentPack) {
                                            if (apCurrentPack != pPack) {
                                                ExtraDataList* Extra = &apActor->m_Extra;
                                                Script::SetActionFlag(
                                                    apCurrentPack, Extra, SCRIPT_ACTION_ONPACKAGECHANGE
                                                );
                                                BaseProcess* v18 = apActor->pCurrentProcess;
                                                v18->ChangePackage(apActor, apCurrentPack);
                                                BaseProcess* v17 = apActor->pCurrentProcess;
                                                v17->SetPackageDoneOnce(false);
                                            }
                                        }
                                        if (pPack) {
                                            if (pPack != apCurrentPack) {
                                                ExtraDataList* v14 = &apActor->m_Extra;
                                                Script::SetActionFlag(pPack, v14, SCRIPT_ACTION_ONPACKAGESTART);
                                            }
                                        }
                                        return pPack;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            pList = pList->m_pkNext;
        }
    }
    return apCurrentPack;
}

// The schedule's month, date and start-time are each read as a sign-extended byte.
void PackageList::FindMissedPackages(
    Actor* apActor,
    BSSimpleList<TESPackage*>* apPacks,
    float afStartdate,
    float afhour
) {
    TESPackage* apCurrentPack = nullptr;
    BaseProcess* CurrentAIProcess = apActor->pCurrentProcess;
    int32_t iday = CurrentAIProcess->GetProcessedDay();
    BaseProcess* v13 = apActor->pCurrentProcess;
    int32_t imonth = v13->GetProcessedMonth();
    BaseProcess* v12 = apActor->pCurrentProcess;
    int32_t iyear = v12->GetProcessedYear();
    float fhour = afhour;
    int32_t iendday = appCalendar.GetDay();
    int32_t iendmonth = appCalendar.GetMonth();
    while (iday) {
        BSSimpleList<TESPackage*>* pList = &this->listPackages;
        if (apActor) {
            apCurrentPack = apActor->GetCurrentPackage();
        }
        if (pList->IsEmpty()) {
            iday = 0;
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
                    if (pSchedule) {
                        if (pSchedule->PSData.eMonth == PSM_NONE || pSchedule->PSData.eMonth <= imonth) {
                            if (!pSchedule->PSData.cDate || pSchedule->PSData.cDate <= iday) {
                                PSDAYOFWEEK StartDayOfWeek;
                                if (pSchedule->GetStartDayOfWeek() == PSD_NONE ||
                                    (StartDayOfWeek = pSchedule->GetStartDayOfWeek(),
                                     static_cast<int32_t>(StartDayOfWeek) <= appCalendar.GetDayOfTheWeek())) {
                                    int32_t StartTime;
                                    if (pSchedule->GetStartTime() == -1 ||
                                        (StartTime = pSchedule->GetStartTime(),
                                         fhour > static_cast<double>(StartTime))) {
                                        if (pPack != apCurrentPack && !apPacks->IsInList(pPack)) {
                                            if (pPack->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_NONE) {
                                                TESObjectREFR* LinkedRef = apActor->GetLinkedRef();
                                                pPack->CalculateProcedureType(LinkedRef);
                                            }
                                            apPacks->AddTail(pPack);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                pList = pList->m_pkNext;
            }
            fhour = fhour + 1.0;
            if (fhour > 23.0) {
                ++iday;
                fhour = 0.0f;
                uint16_t sDaysInMonth = Date::GetDaysInMonth(static_cast<CALENDAR_MONTH>(static_cast<char>(imonth)));
                if (iday > sDaysInMonth) {
                    iday -= sDaysInMonth;
                    if (++imonth >= 12) {
                        imonth -= 12;
                        ++iyear;
                    }
                }
            }
            if (iday == iendday && imonth == iendmonth) {
                int32_t v9 = static_cast<int32_t>(fhour);
                if (v9 >= static_cast<int32_t>(appCalendar.GetHour())) {
                    iday = 0;
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
                PackageSchedule* PackageSchedule_ = pSecondData->GetPackageSchedule();
                if (pFirstData->ComparePackageSchedule(PackageSchedule_) == 1) {
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
