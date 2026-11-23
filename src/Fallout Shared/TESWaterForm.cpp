#include "Fallout Shared/TESWaterForm.h"

#include "BSAudio/BSAudio.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/SettingCollection.h"
#include "BSShader/BSRenderedTexture.h"
#include "BSShader/BSShaderManager.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSDecalManager.h"
#include "Fallout Shared/BGSEntryPoint.h"
#include "Fallout Shared/BGSLoadedReferenceCollection.h"
#include "Fallout Shared/BGSNote.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/BGSPlaceableWater.h"
#include "Fallout Shared/BGSQuestObjective.h"
#include "Fallout Shared/BGSRadiation.h"
#include "Fallout Shared/BGSTalkingActivator.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/FOCollisionListener.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Magic/EffectSettingCollection.h"
#include "Fallout Shared/Magic/SpellItem.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageEventAction.h"
#include "Fallout Shared/Pathfinding/PathBuilder.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/ScreenSplatter.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/Sky/SkyTextureManager.h"
#include "Fallout Shared/SpeedTree/BSTreeManager.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESAttackDamageForm.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESConditionFunctions.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESIdleManager.h"
#include "Fallout Shared/TESImageSpaceModifier.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESQuestTarget.h"
#include "Fallout Shared/TESRace.h"
#include "Fallout Shared/TESRaceForm.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESScriptCompiler.h"
#include "Fallout Shared/TESScriptFunctions.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout Shared/WaterShaderData.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/CharacterProgression.h"
#include "Fallout/AI/Combat/CombatCoverMap.h"
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
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSTexturePalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"
#include "xfnv/Compiler.h"

#include <cstdint>

SettingT<INISettingCollection> TESWaterForm::iWaterNoiseResolution("iWaterNoiseResolution:Water", 256);

// The MNAM and EDID buffers are each initialised to "" (one zeroed byte, allocated only for an empty chunk) so that an
// empty chunk (malformed data) is not read uninitialised. The record's chunks: the name, the bound, the editor ID, the
// alpha and flags, the shader data (whose height scales and amplitudes are set to their defaults when the chunk carries
// the whole structure), the material name, the noise texture, the sound, the drink effect and the three weather control
// waters.
bool TESWaterForm::Load(TESFile* apFile) {
    if (static_cast<uint8_t>(apFile->GetTESForm()) != 78) {
        return false;
    }
    LoadForm(apFile);
    SetInitialized(false);
    do {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        if (uiChunk > FNAM_ID) {
            if (uiChunk == GNAM_ID) {
                apFile->GetChunkData(pWaterWeatherControl, 0xC);
                // The file keeps a 4-byte form ID for each water form, resolved by InitItem.
                TESFile::WidenFormIDRecords(pWaterWeatherControl, 3, 4, sizeof(TESWaterForm*), {{0, 0}});
                if (apFile->QEndian()) {
                    for (TESWaterForm*& pControl : pWaterWeatherControl) {
                        ENDIAN(pControl, false);
                    }
                }
            }
            else if (uiChunk == MNAM_ID) {
                // A 16-byte aligned stack buffer of the chunk's size.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize ? uiChunkSize : 1));
                // An empty chunk leaves the buffer unwritten (see above), so it is terminated first; the game does not
                // do this.
                pBuffer[0] = '\0';
                apFile->GetChunkData(pBuffer, 0);
                cMaterialID.Set(pBuffer, 0);
            }
            else if (uiChunk == NNAM_ID) {
                TESTexture::LoadTextureChunk(&NoiseTexture, apFile);
            }
            else if (uiChunk == SNAM_ID) {
                uint32_t uiSoundID = 0;
                apFile->GetChunkData(uiSoundID);
                // The chunk holds the sound's form ID, which is kept in the pointer until InitItem resolves it.
                SetWaterSound(reinterpret_cast<TESSound*>(static_cast<uintptr_t>(uiSoundID)));
            }
            else if (uiChunk == XNAM_ID) {
                uint32_t uiDrinkEffectID = 0;
                apFile->GetChunkData(uiDrinkEffectID);
                SetSpellItem(reinterpret_cast<SpellItem*>(static_cast<uintptr_t>(uiDrinkEffectID)));
            }
        }
        else if (uiChunk == FNAM_ID) {
            apFile->GetChunkData(&cFlags, 1);
        }
        else if (uiChunk > FULL_ID) {
            if (uiChunk == ANAM_ID) {
                apFile->GetChunkData(&cAlpha, 1);
            }
            else if (uiChunk == DNAM_ID) {
                apFile->GetChunkData(&Data, 0xC4);
                if (apFile->QEndian()) {
                    Data.Endian();
                }
            }
        }
        else if (uiChunk == FULL_ID) {
            TESFullName::LoadFullNameChunk(this, apFile);
        }
        else if (uiChunk == DATA_ID) {
            if (apFile->GetChunkSize() - GetDataSizeAdded()) {
                // LoadData reads the chunk's bytes into an untyped buffer, which is how the shader data is passed.
                LoadData(apFile, reinterpret_cast<char*>(&Data), sizeof(Data));
                if (apFile->QEndian()) {
                    Data.Endian();
                }
                Data.fHeightUVScale0 = 100.0f;
                Data.fHeightUVScale1 = 100.0f;
                Data.fHeightUVScale2 = 100.0f;
                Data.fAmplitude0 = 1.0f;
                Data.fAmplitude1 = 0.5f;
                Data.fAmplitude2 = 0.25f;
                TESAttackDamageForm::SetDamage(0);
            }
            else {
                LoadData(apFile, nullptr, 0);
                if (apFile->QEndian()) {
                    Data.Endian();
                }
            }
        }
        else if (uiChunk == EDID_ID) {
            // A 16-byte aligned stack buffer of the chunk's size.
            uint32_t uiChunkSize = apFile->GetChunkSize();
            char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize ? uiChunkSize : 1));
            // An empty chunk leaves the buffer unwritten (see above), so it is terminated first; the game does not do
            // this.
            pBuffer[0] = '\0';
            apFile->GetChunkData(pBuffer, 0x200);
            SetFormEditorID(pBuffer);
        }
        else if (uiChunk == OBND_ID) {
            LoadObjectBound(apFile);
        }
    }
    while (apFile->NextChunk());
    return true;
}

// InitializeData runs before the field stores, so they stay in the body after it.
TESWaterForm::TESWaterForm()
  : spNoiseNormalMap(nullptr), fTexScroll0{}, fTexScroll1{}, fTexScroll2{}, spNoiseTexture(nullptr) {
    cFormType = 78;
    TESWaterForm::InitializeData();
    iCurrentTextureSelect01 = 0;
    iCurrentTextureSelect02 = 0;
    fFrequencyX = 9;
    fFrequencyY = 9;
    iOctaves = 4;
    fAmplitude = 0.5f;
    fLacunarity = 2.0f;
    fBias = 0.5f;
    fGain = 0.5f;
    pSpellItem = nullptr;
    bNeedUpdate = true;
    bResetNoiseTexture = false;
    pPlaceableAutoWater = nullptr;
    pPlaceableLODWater = nullptr;
    iRadiationDamage = 0;
    iHealthRestoration = 0;
    pRemappedWaterForm = nullptr;
}

TESWaterForm::~TESWaterForm() {
    spNoiseTexture = static_cast<NiTexture*>(nullptr);
}

void TESWaterForm::InitializeData() {
    cAlpha = 75;
    cFlags = 0;
    pWaterSound = nullptr;
    cMaterialID.Set(nullptr, 0);
    WaterShaderData* visData = GetData();
    visData->Initialize();
    for (TESWaterForm*& pControl : pWaterWeatherControl) {
        pControl = nullptr;
    }
}

// The form's record: name, noise texture, alpha, flags, material, sound and spell IDs, the shader data (swapped for
// the write and back) and the three weather control waters' IDs.
void TESWaterForm::Save() {
    StartForm();
    TESFullName::Save();
    uint32_t uiNoiseSize = NoiseTexture.GetTextureNameLength() + 1;
    TESForm::AddChunkArray(NNAM_ID, const_cast<char*>(NoiseTexture.GetTextureName()), uiNoiseSize);
    TESForm::AddChunk(ANAM_ID, cAlpha);
    TESForm::AddChunk(FNAM_ID, cFlags);
    uint32_t uiMaterialSize = cMaterialID.GetLength() + 1;
    TESForm::AddChunkArray(MNAM_ID, const_cast<char*>(GetMaterialName()), uiMaterialSize);
    if (pWaterSound) {
        uint32_t uiSoundID = pWaterSound->GetFormID();
        TESForm::AddChunk(SNAM_ID, uiSoundID);
    }
    if (pSpellItem) {
        uint32_t uiSpellID = pSpellItem->GetFormID();
        TESForm::AddChunk(XNAM_ID, uiSpellID);
    }
    SaveData();
    if (TESForm::QEndianSwapOnSave()) {
        Data.Endian();
    }
    TESForm::__AddChunkData(DNAM_ID, &Data, sizeof(WaterShaderData));
    if (TESForm::QEndianSwapOnSave()) {
        Data.Endian();
    }
    uint32_t auiControlIDs[3];
    for (int32_t i = 0; i < 3; ++i) {
        if (pWaterWeatherControl[i]) {
            auiControlIDs[i] = pWaterWeatherControl[i]->GetFormID();
        }
        else {
            auiControlIDs[i] = 0;
        }
    }
    TESForm::AddChunkArray(GNAM_ID, auiControlIDs, 3);
    CloseForm();
}

// Saves the form's changes into the save buffer; a remapped water type is saved as its form ID.
void TESWaterForm::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    TESForm::SaveGame(apSaveGameBuffer);
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_WATER_REMAPPED)) {
        apSaveGameBuffer->SaveFormID(pRemappedWaterForm, 0);
    }
}

void TESWaterForm::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESForm::LoadGame(apLoadGameBuffer);
    BGSChangeFlags kChangeFlags = apLoadGameBuffer->GetChangeFlags();
    if (kChangeFlags.CheckFlags(CHANGE_WATER_REMAPPED)) {
        int32_t iRemappedID = static_cast<int32_t>(apLoadGameBuffer->BGSLoadGameBuffer::LoadFormID());
        TESForm* pForm = TESForm::GetFormByNumericID(iRemappedID);
        pRemappedWaterForm = dynamic_cast<TESWaterForm*>(pForm);
    }
}

void TESWaterForm::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_WATER_REMAPPED)) {
        pRemappedWaterForm = nullptr;
    }
}

void TESWaterForm::InitItem() {
    if (!GetInitialized()) {
        uint32_t uiTempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pWaterSound));
        if (uiTempInt) {
            TESForm::AddCompileIndex(uiTempInt, GetFile(-1));
            TESForm* pForm = TESForm::GetFormByNumericID(uiTempInt);
            pWaterSound = dynamic_cast<TESSound*>(pForm);
            if (!pWaterSound) {
                int32_t iThisFormID = iFormID;
                const char* pEditorID = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find sound (%08X) for water type '%s' (%08X).",
                    uiTempInt,
                    pEditorID,
                    iThisFormID
                );
            }
        }
        uiTempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pSpellItem));
        if (uiTempInt) {
            TESForm::AddCompileIndex(uiTempInt, GetFile(-1));
            TESForm* pForm = TESForm::GetFormByNumericID(uiTempInt);
            pSpellItem = dynamic_cast<SpellItem*>(pForm);
            if (!pSpellItem) {
                int32_t iThisFormID = iFormID;
                const char* pEditorID = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find sound (%08X) for spell item '%s' (%08X).",
                    uiTempInt,
                    pEditorID,
                    iThisFormID
                );
            }
            if (pSpellItem && !pSpellItem->IsEmpty()) {
                EffectItemList* pList = pSpellItem;
                EffectItem* pDamageRadiationEffect = nullptr;
                EffectItem* pRestoreHealthEffect = nullptr;
                while (pList && (!pDamageRadiationEffect || !pRestoreHealthEffect)) {
                    EffectItem* pEffectItem = pList->m_item;
                    if (pEffectItem->GetActorValueIndex() == ActorValue::RADIATION_RADS &&
                        pEffectItem->pEffectSetting->IsFlagSet(EffectSetting::DETRIMENTAL)) {
                        pDamageRadiationEffect = pEffectItem;
                        iRadiationDamage = pEffectItem->GetMagnitude();
                    }
                    else if (
                        pEffectItem->GetActorValueIndex() == ActorValue::HEALTH &&
                        !pEffectItem->pEffectSetting->IsFlagSet(EffectSetting::DETRIMENTAL)
                    ) {
                        pRestoreHealthEffect = pEffectItem;
                        iHealthRestoration = pEffectItem->GetMagnitude();
                    }
                    // The list is walked as EffectItemList nodes: each next node is downcast.
                    pList = static_cast<EffectItemList*>(pList->GetNext());
                }
            }
        }
        for (int32_t i = 0; i < 3; ++i) {
            uiTempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pWaterWeatherControl[i]));
            if (uiTempInt) {
                TESForm::AddCompileIndex(uiTempInt, GetFile(-1));
                TESForm* pForm = TESForm::GetFormByNumericID(uiTempInt);
                pWaterWeatherControl[i] = dynamic_cast<TESWaterForm*>(pForm);
                if (!pWaterWeatherControl[i]) {
                    int32_t iThisFormID = iFormID;
                    const char* pEditorID = GetFormEditorID();
                    Warning(
                        "MASTERFILE: Could not find WaterForm (%08X) for water type '%s' (%08X).",
                        uiTempInt,
                        pEditorID,
                        iThisFormID
                    );
                }
            }
        }
        SetInitialized(true);
    }
}

void TESWaterForm::Copy(TESForm* apCopy) {
    TESWaterForm* pCopy = dynamic_cast<TESWaterForm*>(apCopy);
    if (pCopy) {
        CopyAllComponents(pCopy);
        cFlags = pCopy->cFlags;
        cAlpha = pCopy->cAlpha;
        WaterShaderData* pCopyData = pCopy->GetData();
        ourMemcpy(GetData(), pCopyData, sizeof(WaterShaderData));
        SetWaterSound(pCopy->pWaterSound);
        SetSpellItem(pCopy->GetSpellItem());
        ourMemcpy(pWaterWeatherControl, pCopy->pWaterWeatherControl, sizeof(pWaterWeatherControl));
        // Called through a base pointer to keep clang from devirtualising the call.
        static_cast<BaseFormComponent*>(&NoiseTexture)->CopyComponent(&pCopy->NoiseTexture);
        SetMaterialName(pCopy->GetMaterialName());
    }
}

bool TESWaterForm::Compare(TESForm* apCompare) {
    TESWaterForm* pComp = dynamic_cast<TESWaterForm*>(apCompare);
    if (!pComp) {
        return true;
    }
    if (CompareAllComponents(pComp)) {
        return true;
    }
    if (cFlags != pComp->cFlags) {
        return true;
    }
    if (cAlpha != pComp->cAlpha) {
        return true;
    }
    WaterShaderData* pCompData = pComp->GetData();
    if (std::memcmp(GetData(), pCompData, sizeof(WaterShaderData))) {
        return true;
    }
    if (NoiseTexture.CompareComponent(&pComp->NoiseTexture)) {
        return true;
    }
    if (pWaterSound != pComp->pWaterSound) {
        return true;
    }
    const char* pMaterialName = GetMaterialName();
    if (pMaterialName != pComp->GetMaterialName()) {
        return true;
    }
    SpellItem* pSpell = GetSpellItem();
    return pSpell != pComp->GetSpellItem() ||
        std::memcmp(pWaterWeatherControl, pComp->pWaterWeatherControl, sizeof(pWaterWeatherControl)) != 0;
}

// Drinking from the water: the activating reference's magic caster casts the form's spell, with an effectiveness the
// perk owner's ADJUST_DRINKING_RADIATION entry point may adjust first.
bool TESWaterForm::Activate(
    TESObjectREFR* apItemActivated,
    TESObjectREFR* apActionRef,
    bool abIdFlag,
    TESBoundObject* apObjecttoGet,
    int32_t aiCount
) {
    if (!pSpellItem || !apActionRef) {
        return false;
    }
    MagicCaster* pCaster = apActionRef->GetMagicCaster();
    if (!pCaster) {
        return false;
    }
    float fEffectivenessMult = 1.0f;
    if (apActionRef->IsActor()) {
        BGSEntryPoint::HandleEntryPoint(
            BGSEntryPoint::ADJUST_DRINKING_RADIATION,
            static_cast<Actor*>(apActionRef),
            &fEffectivenessMult
        );
    }
    MagicItem* pMagicItem = static_cast<MagicItem*>(pSpellItem);
    pCaster->CastSpellImmediate(pMagicItem, false, nullptr, fEffectivenessMult, true);
    return true;
}

void TESWaterForm::SetWaterSound(TESSound* apSound) {
    pWaterSound = apSound;
}

void TESWaterForm::SetSpellItem(SpellItem* pItem) {
    pSpellItem = pItem;
}

uint32_t TESWaterForm::GetNoiseTextureNameLength() {
    return NoiseTexture.GetTextureNameLength();
}

const char* TESWaterForm::GetNoiseTextureName() {
    return NoiseTexture.GetTextureName();
}

void TESWaterForm::SetNoiseTexture(NiTexture* apTexture) {
    spNoiseTexture = apTexture;
}

void TESWaterForm::GetNoiseTexture(NiPointer<NiTexture>& aspTexture) {
    aspTexture = spNoiseTexture;
}

float TESWaterForm::GetLightRadius() {
    return Data.fLightRadius;
}

float TESWaterForm::GetLightBrightness() {
    return Data.fLightBrightness;
}

WaterShaderData* TESWaterForm::GetData() {
    return &Data;
}

float TESWaterForm::GetAlpha() {
    return static_cast<float>(cAlpha / 100.0);
}

int32_t TESWaterForm::GetRadiationDamage() {
    return iRadiationDamage;
}

double TESWaterForm::GetFogScale() {
    return Data.fFogAmount;
}

bool TESWaterForm::ShouldResetNoiseTextures() {
    return bResetNoiseTexture;
}

bool TESWaterForm::IsDangerous() {
    return (cFlags & FLAG_DANGEROUS) != 0;
}

double TESWaterForm::GetDepthY() {
    return Data.fDepthFalloffEnd;
}

SpellItem* TESWaterForm::GetSpellItem() {
    return reinterpret_cast<SpellItem*>(reinterpret_cast<uintptr_t>(pSpellItem));
}

void TESWaterForm::SetMaterialName(const char* apMaterial) {
    cMaterialID.Set(apMaterial, 0);
}

const char* TESWaterForm::GetMaterialName() {
    if (cMaterialID.pString) {
        return cMaterialID.pString;
    }

    return "";
}

void TESWaterForm::SetRemappedWaterForm(TESWaterForm* apWater) {
    if (this == apWater) {
        pRemappedWaterForm = nullptr;
        RemoveChange(CHANGE_WATER_REMAPPED);
    }
    else {
        pRemappedWaterForm = apWater;
        AddChange(CHANGE_WATER_REMAPPED);
    }
}

BGSPlaceableWater* TESWaterForm::GetPlaceableAutoWater() {
    if (!pPlaceableAutoWater) {
        char cEditorID[268];
        pPlaceableAutoWater = new BGSPlaceableWater;
        pPlaceableAutoWater->SetTemporary();
        pPlaceableAutoWater->SetAutomaticWater(true);
        pPlaceableAutoWater->SetRefractsActors(true);
        pPlaceableAutoWater->SetReflectsLODLand(true);
        pPlaceableAutoWater->SetReflectsLODBuildings(true);
        pPlaceableAutoWater->SetReflectsLODTrees(true);
        BSsprintf(cEditorID, 260, "PlaceableAutoWater");
        pPlaceableAutoWater->SetFormEditorID(cEditorID);
        pPlaceableAutoWater->Data.pWaterType = this;
    }
    return pPlaceableAutoWater;
}

BGSPlaceableWater* TESWaterForm::GetPlaceableLODWater() {
    if (!pPlaceableLODWater) {
        char cEditorID[268];
        pPlaceableLODWater = new BGSPlaceableWater;
        pPlaceableLODWater->SetTemporary();
        pPlaceableLODWater->SetLODWater(true);
        pPlaceableLODWater->SetReflectionEnabled(true);
        pPlaceableLODWater->SetRefractionEnabled(false);
        pPlaceableLODWater->SetReflectsActors(false);
        pPlaceableLODWater->SetReflectsLand(false);
        pPlaceableLODWater->SetReflectsLODLand(true);
        pPlaceableLODWater->SetReflectsLODBuildings(true);
        pPlaceableLODWater->SetReflectsLODTrees(false);
        pPlaceableLODWater->SetReflectsSky(true);
        pPlaceableLODWater->SetReflectsNoReference(false);
        pPlaceableLODWater->SetReflectsDeadActors(false);
        BSsprintf(cEditorID, 260, "PlaceableLODWater");
        pPlaceableLODWater->SetFormEditorID(cEditorID);
        pPlaceableLODWater->Data.pWaterType = this;
    }
    return pPlaceableLODWater;
}

WaterShaderData::WaterShaderData() {
    Initialize();
}

// The default shader parameters of a new water form.
void WaterShaderData::Initialize() {
    fWindVelocity = 0.1f;
    fWindDirection = 90.0f;
    fWaveAmplitude = 0.5f;
    fWaveFrequency = 1.0f;
    fSunPower = 50.0f;
    fReflectionAmount = 0.5f;
    fFresnelAmount = 0.025f;
    fWaveSpeed = 0.0f;
    fFogDistNear = 0.0f;
    fFogDistFar = 0.0f;
    fFogAmount = 1.0f;
    fUnderwaterFogDistNear = 0.0f;
    fUnderwaterFogDistFar = 1000.0f;
    fUnderwaterFogAmount = 1.0f;
    iShallowWaterColor = 0x808000;
    iDeepWaterColor = 0x190000;
    iReflectionWaterColor = 0xFFFFFF;
    fRainForce = 0.1f;
    fRainVelocity = 0.6f;
    fRainFalloff = 0.985f;
    fRainDampener = 2.0f;
    fRainSize = 0.01f;
    fDisplacementForce = 0.4f;
    fDisplacementVelocity = 0.6f;
    fDisplacementFalloff = 0.985f;
    fDisplacementDampener = 10.0f;
    fDisplacementSize = 0.05f;
    fDepthFalloffStart = 0.0f;
    fDepthFalloffEnd = 0.0f;
    fNoiseScale = 1.0f;
    fUVScale = 500.0f;
    fHeightUVScale0 = 100.0f;
    fHeightUVScale1 = 100.0f;
    fHeightUVScale2 = 100.0f;
    fRefractivityAmount = 250.0f;
    fShininess = 100.0f;
    fHDRMultiplier = 1.0f;
    fLightRadius = 10000.0f;
    fLightBrightness = 1.0f;
}

// Swaps the fields read from a big-endian file, in field order. The blend byte needs no swap.
void WaterShaderData::Endian() {
    ENDIAN(fWindVelocity, false);
    ENDIAN(fWindDirection, false);
    ENDIAN(fWaveAmplitude, false);
    ENDIAN(fWaveFrequency, false);
    ENDIAN(fSunPower, false);
    ENDIAN(fReflectionAmount, false);
    ENDIAN(fFresnelAmount, false);
    ENDIAN(fWaveSpeed, false);
    ENDIAN(fFogDistNear, false);
    ENDIAN(fFogDistFar, false);
    ENDIAN(iShallowWaterColor, false);
    ENDIAN(iDeepWaterColor, false);
    ENDIAN(iReflectionWaterColor, false);
    ENDIAN(fRainForce, false);
    ENDIAN(fRainVelocity, false);
    ENDIAN(fRainFalloff, false);
    ENDIAN(fRainDampener, false);
    ENDIAN(fRainSize, false);
    ENDIAN(fDisplacementForce, false);
    ENDIAN(fDisplacementVelocity, false);
    ENDIAN(fDisplacementFalloff, false);
    ENDIAN(fDisplacementDampener, false);
    ENDIAN(fDisplacementSize, false);
    ENDIAN(fNoiseScale, false);
    ENDIAN(fNoiseWindDirection0, false);
    ENDIAN(fNoiseWindDirection1, false);
    ENDIAN(fNoiseWindDirection2, false);
    ENDIAN(fNoiseWindSpeed0, false);
    ENDIAN(fNoiseWindSpeed1, false);
    ENDIAN(fNoiseWindSpeed2, false);
    ENDIAN(fDepthFalloffStart, false);
    ENDIAN(fDepthFalloffEnd, false);
    ENDIAN(fFogAmount, false);
    ENDIAN(fUVScale, false);
    ENDIAN(fUnderwaterFogAmount, false);
    ENDIAN(fUnderwaterFogDistNear, false);
    ENDIAN(fUnderwaterFogDistFar, false);
    ENDIAN(fRefractivityAmount, false);
    ENDIAN(fShininess, false);
    ENDIAN(fHDRMultiplier, false);
    ENDIAN(fLightRadius, false);
    ENDIAN(fLightBrightness, false);
    ENDIAN(fHeightUVScale0, false);
    ENDIAN(fHeightUVScale1, false);
    ENDIAN(fHeightUVScale2, false);
    ENDIAN(fAmplitude0, false);
    ENDIAN(fAmplitude1, false);
    ENDIAN(fAmplitude2, false);
}
