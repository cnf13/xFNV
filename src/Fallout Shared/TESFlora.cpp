#include "Fallout Shared/TESFlora.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSOpenCloseForm.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/Magic/IngredientItem.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectACTI.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESProduceForm.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Misc/Calendar.h"
#include "SDK/MSVC/rtti.h"

SettingT<GameSettingCollection> gs_sFloraSuccessMessage("sFloraSuccessMessage", "You successfully harvest %s.");
SettingT<GameSettingCollection> gs_sFloraFailureMessage("sFloraFailureMessage", "You find nothing of use.");

TESFlora::TESFlora() : TESProduceForm(), TESObjectACTI() {
    SetFormType(FLOR_ID);
    SetInitialized(true);
}

TESFlora::~TESFlora() {}

// The EDID buffer and the PFPC chances are initialised to "" and zeros (allocated even for an empty chunk), so they are
// never read uninitialised when the chunk is empty or shorter (malformed data).
bool TESFlora::Load(TESFile* apFile) {
    LoadForm(apFile);
    SetInitialized(false);
    do {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        switch (uiChunk) {
            case PFIG_ID: {
                uint32_t uiIngredientID = 0;
                apFile->GetChunkData(uiIngredientID);
                // the ingredient's form ID stands in for the ingredient until the forms are resolved
                static_cast<TESProduceForm*>(this)->SetFormIngredient(
                    reinterpret_cast<IngredientItem*>(static_cast<uintptr_t>(uiIngredientID))
                );
                break;
            }
            case OBND_ID: LoadObjectBound(apFile); break;
            case PFPC_ID: {
                // Reads all four bytes; a shorter chunk leaves the rest unwritten (see above).
                uint8_t aucChances[4] = {};
                apFile->GetChunkData(aucChances, 4);
                static_cast<TESProduceForm*>(this)->SetAllProductionChances(aucChances);
                break;
            }
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size, read up to 0x200 bytes.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
                // Clears the buffer first because an empty chunk leaves it unwritten (see above).
                pBuffer[0] = '\0';
                apFile->GetChunkData(pBuffer, 0x200);
                SetFormEditorID(pBuffer);
                break;
            }
            case MODL_ID:
            case MODT_ID:
            case MOSD_ID:
            case MODD_ID: TESModel::LoadModelChunk(static_cast<TESModel*>(this), apFile); break;
            case MODS_ID: {
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize, 128));
                apFile->GetChunkData(pBuffer, uiChunkSize);
                uint32_t uiVersion = apFile->GetFormVersion();
                static_cast<TESModelTextureSwap*>(this)->UnpackTextureSwapChunkData(pBuffer, uiChunkSize, uiVersion);
                break;
            }
            case SCRI_ID: {
                uint32_t uiScriptID = 0;
                apFile->GetChunkData(uiScriptID);
                TESScriptableForm* pScriptable = static_cast<TESScriptableForm*>(this);
                // the script's form ID stands in for the script until the forms are resolved
                pScriptable->SetFormScript(reinterpret_cast<Script*>(static_cast<uintptr_t>(uiScriptID)));
                pScriptable->InitItem(static_cast<TESForm*>(this));
                break;
            }
            case FULL_ID: TESFullName::LoadFullNameChunk(static_cast<TESFullName*>(this), apFile); break;
            case DSTD_ID:
            case DEST_ID:
                BGSDestructibleObjectForm::LoadChunk(static_cast<BGSDestructibleObjectForm*>(this), apFile);
                break;
        }
    }
    while (apFile->NextChunk());
    return true;
}

void TESFlora::Save() {
    static_cast<TESObjectACTI*>(this)->StartForm();
    TESFullName::Save();
    TESModelTextureSwap::Save(MODL_ID, MODT_ID, MODS_ID);
    TESScriptableForm::Save();
    BGSDestructibleObjectForm::Save();
    TESProduceForm::Save();
    static_cast<TESObjectACTI*>(this)->CloseForm();
}

void TESFlora::InitItem() {
    if (!static_cast<TESObjectACTI*>(this)->GetInitialized()) {
        TESObjectACTI* a2;
        static_cast<TESObjectACTI*>(this)->TESObjectACTI::InitItem();
        if (this == nullptr) {
            a2 = nullptr;
        }
        else {
            a2 = static_cast<TESObjectACTI*>(this);
        }
        TESProduceForm::InitItem(a2);
        static_cast<TESObjectACTI*>(this)->SetInitialized(true);
    }
}

void TESFlora::Copy(TESForm* apCopy) {
    if (dynamic_cast<TESFlora*>(apCopy)) {
        CopyAllComponents(apCopy);
    }
}

bool TESFlora::Compare(TESForm* apCompare) {
    TESFlora* pcomp = dynamic_cast<TESFlora*>(apCompare);
    return !pcomp || CompareAllComponents(pcomp);
}

bool TESFlora::Activate(
    TESObjectREFR* apItemActivated,
    TESObjectREFR* apActionRef,
    bool abIdFlag,
    TESBoundObject* apObjectToGet,
    int32_t aiCount
) {
    FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
    Actor* pactor = dynamic_cast<Actor*>(apActionRef);
    if ((apItemActivated) && (pactor)) {
        bool bsuccess = false;
        if ((!apItemActivated->GetEmpty()) && (pFormIngredient)) {
            SEASON Season = appCalendar.GetSeason();
            uint8_t ichance = GetProductionChance(Season);
            if (ichance) {
                bsuccess = Random() % 100 < ichance;
            }
        }

        if (bsuccess) {
            IngredientItem* pingr = pFormIngredient;
            Script* pScript = TESScriptableForm::GetFormScript(pingr);
            if (pScript) {
                ExtraDataList* pExtra;
                BaseExtraList* v25 = nullptr;
                ExtraDataList* pExtraList = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
                if (pExtraList) {
                    pExtra = ::new (pExtraList) ExtraDataList;
                }
                else {
                    pExtra = nullptr;
                }
                pExtra->SetCount(1);
                if ((pExtra) && (!pExtra->GetScript())) {
                    TESObjectREFR* pref;
                    pExtra->SetScript(pScript);
                    Script* Script_ = pExtra->GetScript();
                    ScriptLocals* v8 = Script_->CloneLocals();
                    pExtra->SetScriptLocals(v8);
                    void* apMemory = ourMalloc(sizeof(TESObjectREFR));
                    if (apMemory) {
                        pref = ::new (apMemory) TESObjectREFR;
                    }
                    else {
                        pref = nullptr;
                    }
                    ScriptLocals* ScriptLocals_ = pExtra->GetScriptLocals();
                    pScript->Run(pref, ScriptLocals_, nullptr, false);
                }

                pactor->AddObjecttoContainer(pingr, pExtra, 1);
            }
            else {
                pactor->AddBoundObjectItem(pingr, nullptr, 1);
            }
            if (pactor == pPlayer) {
                BSStringT<char> v23;
                v23.SPrintF(gs_sFloraSuccessMessage.String(), pingr->GetFullName());
                Interface::ShowMessage(v23.pString, 0, nullptr, nullptr, 2.0f, false);
                BSSoundHandle soundHand;
                BSAudio* v10 = &BSAudio::QInstance();
                BSSoundHandle* arHandle = v10->GetSoundHandleByName(&a1, "ITMPickupOrganic", 288u);
                soundHand.operator=(*arHandle);
                soundHand.Play(false);
            }
        }
        else {
            if (pactor == pPlayer) {
                const char* v11 = gs_sFloraFailureMessage.String();
                Interface::ShowMessage(v11, 0, nullptr, nullptr, 2.0f, false);
                Interface::PlayMenuSound(29);
            }
        }
        apItemActivated->SetEmpty(true);
    }

    return true;
}

// Whether form flag 0x2000 (a harvested form) is set.
bool TESForm::GetEmpty() const {
    return (iFormFlags & 0x2000) != 0;
}

uint8_t TESProduceForm::GetProductionChance(SEASON aSeason) {
    return iProduceChance[aSeason];
}
