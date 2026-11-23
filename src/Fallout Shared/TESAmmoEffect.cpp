#include "Fallout Shared/TESAmmoEffect.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"
#include "xfnv/Compiler.h"

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<GameSettingCollection> sAmmoEffectDAM("sAmmoEffectDAM", "DAM");
SettingT<GameSettingCollection> sAmmoEffectDR("sAmmoEffectDR", "Target DR");
SettingT<GameSettingCollection> sAmmoEffectDT("sAmmoEffectDT", "Target DT");
SettingT<GameSettingCollection> sAmmoEffectSpread("sAmmoEffectSpread", "Gun Spread");
SettingT<GameSettingCollection> sAmmoEffectCondition("sAmmoEffectCondition", "Gun CND");
SettingT<GameSettingCollection> sAmmoEffectFatigue("sAmmoEffectFatigue", "Target Fatigue");

void AMMO_EFFECT_DATA::Endian() {
    ENDIAN(iAmmoEffectType, false);
    ENDIAN(iOperation, false);
    ENDIAN(fValue, false);
}

TESAmmoEffect::TESAmmoEffect() : data{} {
    SetFormType(AMEF_ID);
}

TESAmmoEffect::~TESAmmoEffect() {}

bool TESAmmoEffect::Load(TESFile* apFile) {
    uint8_t cFormType = static_cast<uint8_t>(apFile->GetTESForm());
    if (cFormType != AMEF_ID) {
        return false;
    }
    LoadForm(apFile);
    SetInitialized(false);
    do {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        switch (uiChunk) {
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                apFile->GetChunkData(pBuffer, 0x200);
                static_cast<TESForm*>(this)->SetFormEditorID(pBuffer);
                break;
            }
            case OBND_ID: static_cast<TESForm*>(this)->LoadObjectBound(apFile); break;
            case FULL_ID: TESFullName::LoadFullNameChunk(static_cast<TESFullName*>(this), apFile); break;
            case DATA_ID:
                // LoadData takes the raw bytes of the chunk; the DATA chunk is the AMMO_EFFECT_DATA struct.
                LoadData(apFile, reinterpret_cast<char*>(&data), sizeof(data));
                if (apFile->QEndian()) {
                    data.Endian();
                }
                break;
        }
    }
    while (apFile->NextChunk());
    return true;
}

void TESAmmoEffect::Save() {
    TESForm::StartForm();
    static_cast<TESFullName*>(this)->Save();
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    TESForm::__SaveData(&data, sizeof(data));
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    TESForm::CloseForm();
}

void TESAmmoEffect::Copy(TESForm* apCopy) {
    TESAmmoEffect* pAmmoEffect = dynamic_cast<TESAmmoEffect*>(apCopy);
    if (pAmmoEffect) {
        ourMemcpy(&data, &pAmmoEffect->data, sizeof(data));
        TESForm::CopyAllComponents(apCopy);
    }
}

bool TESAmmoEffect::Compare(TESForm* apCompare) {
    TESAmmoEffect* pAmmoEffect = dynamic_cast<TESAmmoEffect*>(apCompare);
    if (!pAmmoEffect) {
        return true;
    }
    if (!std::memcmp(&data, &pAmmoEffect->data, sizeof(data))) {
        return CompareAllComponents(pAmmoEffect);
    }
    return true;
}

// An effect type past AMMO_EFFECT_FATIGUE takes the switch's default, and BSsprintf then reads cEffectName, which
// nothing wrote on that path; this uninitialised read is deliberate.
void TESAmmoEffect::BuildMenuString(char* astrResult, uint32_t auiResultLen) {
    char cEffectName[64];
    char cOperation[4];
    cOperation[0] = 0;
    int32_t iOperation = data.iOperation;
    if (iOperation) {
        if (iOperation == 1) {
            BSstrcpy(cOperation, 2, "x");
        }
        else if (iOperation == 2) {
            BSstrcpy(cOperation, 2, "-");
        }
    }
    else {
        BSstrcpy(cOperation, 2, "+");
    }
    switch (data.iAmmoEffectType) {
        case AMMO_EFFECT_DAM: {
            BSstrcpy(cEffectName, 64, sAmmoEffectDAM.String());
            break;
        }
        case AMMO_EFFECT_DR: {
            BSstrcpy(cEffectName, 64, sAmmoEffectDR.String());
            break;
        }
        case AMMO_EFFECT_DT: {
            BSstrcpy(cEffectName, 64, sAmmoEffectDT.String());
            break;
        }
        case AMMO_EFFECT_SPREAD: {
            BSstrcpy(cEffectName, 64, sAmmoEffectSpread.String());
            break;
        }
        case AMMO_EFFECT_CONDITION: {
            BSstrcpy(cEffectName, 64, sAmmoEffectCondition.String());
            break;
        }
        case AMMO_EFFECT_FATIGUE: {
            BSstrcpy(cEffectName, 64, sAmmoEffectFatigue.String());
            break;
        }
        default: {
            break;
        }
    }
    BSsprintf(astrResult, static_cast<int32_t>(auiResultLen), "%s %s %.2f", cEffectName, cOperation, data.fValue);
}

float TESAmmoEffect::ApplyAmmoEffect(
    AMMO_EFFECT_TYPE aiEffectType,
    BSSimpleList<TESAmmoEffect*>* pEffectList,
    float afOriginalValue
) {
    float fReturnValue = afOriginalValue;
    if (pEffectList) {
        BSSimpleList<TESAmmoEffect*>* pEffectsLocal = new BSSimpleList<TESAmmoEffect*>;
        while (pEffectList && pEffectList->m_item) {
            if (pEffectList->m_item->data.iOperation == AMMO_OPERATION_TIMES) {
                pEffectsLocal->AddHead(pEffectList->m_item);
            }
            else {
                pEffectsLocal->AddTail(pEffectList->m_item);
            }
            pEffectList = pEffectList->m_pkNext;
        }
        while (pEffectsLocal && pEffectsLocal->m_item) {
            TESAmmoEffect* pEffect = pEffectsLocal->m_item;
            pEffectsLocal = pEffectsLocal->m_pkNext;
            AMMO_EFFECT_TYPE iEffect = pEffect->data.iAmmoEffectType;
            AMMO_OPERATION_TYPE iOperation = pEffect->data.iOperation;
            float fEffectModifier = pEffect->data.fValue;
            if (iEffect == aiEffectType) {
                if (iOperation) {
                    if (iOperation == AMMO_OPERATION_TIMES) {
                        fReturnValue = fReturnValue * fEffectModifier;
                    }
                    else if (iOperation == AMMO_OPERATION_MINUS) {
                        fReturnValue = fReturnValue - fEffectModifier;
                    }
                }
                else {
                    fReturnValue = fReturnValue + fEffectModifier;
                }
            }
        }
        // Deliberately deletes the walking pointer, not the list it allocated: null, or the node
        // whose item ended the walk.
        if (pEffectsLocal) {
            delete pEffectsLocal;
        }
        return fReturnValue;
    }

    return afOriginalValue;
}
