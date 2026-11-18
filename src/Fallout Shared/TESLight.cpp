#include "Fallout Shared/TESLight.h"

#include "BSCore/BSString.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSCloningProcess.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSMessageIcon.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESValueForm.h"
#include "Fallout Shared/TESWeightForm.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Fallout/AI/FixedStrings.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

#include <cmath>

SettingT<INISettingCollection> gs_bUseConstant_bLightAttenuation("bUseConstant:bLightAttenuation", false);
SettingT<INISettingCollection> gs_bUseLinear_bLightAttenuation("bUseLinear:bLightAttenuation", false);
SettingT<INISettingCollection> gs_bUseQuadratic_bLightAttenuation("bUseQuadratic:bLightAttenuation", true);
SettingT<INISettingCollection> gs_fFlickerMovement_bLightAttenuation("fFlickerMovement:bLightAttenuation", 8.0f);
SettingT<INISettingCollection> gs_uLinearMethod_bLightAttenuation("uLinearMethod:bLightAttenuation", 1u);
SettingT<INISettingCollection> gs_uQuadraticMethod_bLightAttenuation("uQuadraticMethod:bLightAttenuation", 2u);
SettingT<INISettingCollection> gs_fLinearValue_bLightAttenuation("fLinearValue:bLightAttenuation", 3.0f);
SettingT<INISettingCollection> gs_fQuadraticValue_bLightAttenuation("fQuadraticValue:bLightAttenuation", 16.0f);
SettingT<INISettingCollection> gs_fConstantValue_bLightAttenuation("fConstantValue:bLightAttenuation", 0.0f);
SettingT<INISettingCollection> gs_bOutQuadInLin_bLightAttenuation("bOutQuadInLin:bLightAttenuation", false);
SettingT<INISettingCollection> gs_fLinearRadiusMult_bLightAttenuation("fLinearRadiusMult:bLightAttenuation", 1.0f);
SettingT<INISettingCollection>
    gs_fQuadraticRadiusMult_bLightAttenuation("fQuadraticRadiusMult:bLightAttenuation", 1.0f);
// Whether the [LightAttenuation] settings were read.
static bool bAttenuationInitialized = GetAttenuationSettings();

void SetLightAttenuation(NiLight* apLight, uint32_t aiRadius) {
    NiPointLight* NiPointLight_ =
        static_cast<NiPointLight*>(NiObject::DynamicCastStatic(&NiPointLight::ms_RTTI, apLight));
    if (NiPointLight_) {
        if ((ATTENUATION_SETTINGS::cUsageFlags & 1) != 0) {
            float fConstantValue = gs_fConstantValue_bLightAttenuation.Float();
            NiPointLight_->SetConstantAttenuation(fConstantValue);
        }
        else {
            NiPointLight_->SetConstantAttenuation(0.0);
        }
        NiPointLight_->SetLinearAttenuation(0.0);
        NiPointLight_->SetQuadraticAttenuation(0.0);
        if ((ATTENUATION_SETTINGS::cUsageFlags & 2) != 0 ||
            (TES::pInstance->GetInterior() &&
             static_cast<SettingT<INISettingCollection>*>(&gs_bOutQuadInLin_bLightAttenuation)
                 ->SettingT<INISettingCollection>::operator==(1))) {
            float fLinearRadius = gs_fLinearRadiusMult_bLightAttenuation.Float() * static_cast<double>(aiRadius);
            uint32_t uLinearMethod = gs_uLinearMethod_bLightAttenuation.UInt();
            if (uLinearMethod) {
                float v11;
                if (uLinearMethod == 2) {
                    double v10;
                    if (fLinearRadius == 0.0) {
                        v10 = 0.009999999776482582;
                    }
                    else {
                        v10 = gs_fLinearValue_bLightAttenuation.Float() / (fLinearRadius * fLinearRadius);
                    }
                    v11 = v10;
                    NiPointLight_->SetLinearAttenuation(v11);
                }
                else {
                    double v12;
                    if (fLinearRadius == 0.0) {
                        v12 = 0.009999999776482582;
                    }
                    else {
                        v12 = gs_fLinearValue_bLightAttenuation.Float() * (1.0 / fLinearRadius);
                    }
                    v11 = v12;
                    NiPointLight_->SetLinearAttenuation(v11);
                }
            }
            else {
                float* fLinearValue = &gs_fLinearValue_bLightAttenuation.Float();
                NiPointLight_->SetLinearAttenuation(*fLinearValue);
            }
        }
        if ((ATTENUATION_SETTINGS::cUsageFlags & 4) != 0 ||
            (!TES::pInstance->GetInterior() &&
             static_cast<SettingT<INISettingCollection>*>(&gs_bOutQuadInLin_bLightAttenuation)
                 ->SettingT<INISettingCollection>::operator==(1))) {
            float fQuadraticRadius = gs_fQuadraticRadiusMult_bLightAttenuation.Float() * static_cast<double>(aiRadius);
            uint32_t uQuadraticMethod = gs_uQuadraticMethod_bLightAttenuation.UInt();
            if (uQuadraticMethod) {
                float v6;
                if (uQuadraticMethod == 1) {
                    double v7;
                    if (fQuadraticRadius == 0.0) {
                        v7 = 0.009999999776482582;
                    }
                    else {
                        v7 = gs_fQuadraticValue_bLightAttenuation.operator float() *
                            (1.0 / static_cast<double>(aiRadius));
                    }
                    v6 = v7;
                    NiPointLight_->SetQuadraticAttenuation(v6);
                }
                else {
                    double v5;
                    if (fQuadraticRadius == 0.0) {
                        v5 = 0.009999999776482582;
                    }
                    else {
                        v5 = gs_fQuadraticValue_bLightAttenuation.operator float() /
                            (fQuadraticRadius * fQuadraticRadius);
                    }
                    v6 = v5;
                    NiPointLight_->SetQuadraticAttenuation(v6);
                }
            }
            else {
                float* fQuadraticValue = &gs_fQuadraticValue_bLightAttenuation.Float();
                NiPointLight_->SetQuadraticAttenuation(*fQuadraticValue);
            }
        }
    }
}

NiLight* FindLight(NiNode* apNode) {
    NiLight* plight = nullptr;
    if (!apNode) {
        return plight;
    }
    for (uint32_t n = 0;; ++n) {
        uint32_t ArrayCount = apNode->GetArrayCount();
        if (n >= ArrayCount) {
            break;
        }
        NiPointer<NiAVObject> spchild(apNode->GetAt(n));
        if (spchild.m_pObject) {
            NiAVObject* v3 = spchild.m_pObject;
            if (v3->IsNode()) {
                plight = FindLight(static_cast<NiNode*>(spchild.m_pObject));
                if (plight) {
                    NiLight* m_pObject = plight;
                    return m_pObject;
                }
            }
            else {
                if (NiObject::IsKindOf_0(&NiLight::ms_RTTI, spchild.m_pObject)) {
                    NiLight* v4 = static_cast<NiLight*>(spchild.m_pObject);
                    return v4;
                }
            }
        }
    }
    return plight;
}

bool GetAttenuationSettings() {
    ATTENUATION_SETTINGS::cUsageFlags = 0;
    if (gs_bUseConstant_bLightAttenuation.operator==(1)) {
        ATTENUATION_SETTINGS::cUsageFlags |= 1u;
    }
    if (gs_bUseLinear_bLightAttenuation.operator==(1)) {
        ATTENUATION_SETTINGS::cUsageFlags |= 2u;
    }
    if (gs_bUseQuadratic_bLightAttenuation.operator==(1)) {
        ATTENUATION_SETTINGS::cUsageFlags |= 4u;
    }
    if (!ATTENUATION_SETTINGS::cUsageFlags) {
        ATTENUATION_SETTINGS::cUsageFlags = 4;
    }
    if ((gs_uLinearMethod_bLightAttenuation.Int()) &&
        (gs_uLinearMethod_bLightAttenuation.Int() != 1 && gs_uLinearMethod_bLightAttenuation.Int() != 2)) {
        gs_uLinearMethod_bLightAttenuation.operator=(1);
    }

    if ((gs_uQuadraticMethod_bLightAttenuation.Int()) &&
        (gs_uQuadraticMethod_bLightAttenuation.Int() != 1 && gs_uQuadraticMethod_bLightAttenuation.Int() != 2)) {
        gs_uQuadraticMethod_bLightAttenuation.operator=(2);
    }

    if (gs_fLinearRadiusMult_bLightAttenuation.Float() == 0.0) {
        gs_fLinearRadiusMult_bLightAttenuation.operator=(1.0f);
    }
    if (gs_fQuadraticRadiusMult_bLightAttenuation.Float() == 0.0) {
        gs_fQuadraticRadiusMult_bLightAttenuation.operator=(1.0f);
    }
    return true;
}

void OBJ_LIGH::Endian() {
    ENDIAN(time, false);
    ENDIAN(radius, false);
    ENDIAN(color, false);
    ENDIAN(flags, false);
    ENDIAN(fFallOffExponent, false);
    ENDIAN(fFOV, false);
}

TESObjectLIGH::TESObjectLIGH()
  : TESBoundAnimObject(), TESFullName(), TESModelTextureSwap(), TESIcon(), BGSMessageIcon(), TESScriptableForm(),
    TESWeightForm(), TESValueForm(), BGSDestructibleObjectForm(), EmittanceColor(0.0f, 0.0f, 0.0f) {
    SetFormType(30u);
    TESObjectLIGH::InitializeData();
}

TESObjectLIGH::~TESObjectLIGH() {}

void TESObjectLIGH::InitializeData() {
    SetSound(nullptr);
    ourMemset(&data, 0, sizeof(OBJ_LIGH));
    fFade = 1.0f;
    data.fFOV = 90.0f;
}

bool TESObjectLIGH::Load(TESFile* apFile) {
    // The form type is kept in a byte (30 is the light form type).
    uint8_t cFormType = static_cast<uint8_t>(apFile->GetTESForm());
    if (cFormType != 30) {
        return false;
    }
    LoadForm(apFile);
    SetInitialized(false);
    data.fFOV = 0.0f;
    data.fFallOffExponent = 0.0f;
    do {
        uint32_t iChunk = apFile->GetTESChunk();
        if (!iChunk) {
            break;
        }
        // The switch compares the record code signed.
        switch (static_cast<int32_t>(iChunk)) {
            case 0x44494445: { // 'EDID'
                char* pEditorID = static_cast<char*>(__builtin_alloca(apFile->GetChunkSize()));
                apFile->GetChunkData(pEditorID, 0x200);
                SetFormEditorID(pEditorID);
                break;
            }
            case 0x444E424F: // 'OBND'
                LoadObjectBound(apFile);
                break;
            case 0x4C4C5546: // 'FULL'
                TESFullName::LoadFullNameChunk(this, apFile);
                break;
            case 0x44534F4D: // 'MOSD'
            case 0x44444F4D: // 'MODD'
            case 0x4C444F4D: // 'MODL'
            case 0x54444F4D: // 'MODT'
                TESModel::LoadModelChunk(this, apFile);
                break;
            case 0x53444F4D: { // 'MODS'
                uint32_t iSize = apFile->GetChunkSize();
                char* pData = static_cast<char*>(__builtin_alloca(iSize));
                apFile->GetChunkData(pData, iSize);
                TESModelTextureSwap::UnpackTextureSwapChunkData(pData, iSize, apFile->GetFormVersion());
                break;
            }
            case 0x4E4F4349: // 'ICON'
                TESTexture::LoadTextureChunk(static_cast<TESIcon*>(this), apFile);
                break;
            case 0x4F43494D: // 'MICO'
                BGSMessageIcon::LoadMessageIcon(this, apFile);
                break;
            case 0x49524353: { // 'SCRI'
                uint32_t iScript = 0;
                apFile->GetChunkData<uint32_t>(iScript);
                // the script stays a form ID until InitItem resolves it
                TESScriptableForm::SetFormScript(reinterpret_cast<Script*>(iScript));
                TESScriptableForm::InitItem(this);
                break;
            }
            case 0x44545344: // 'DSTD'
            case 0x54534544: // 'DEST'
                BGSDestructibleObjectForm::LoadChunk(this, apFile);
                break;
            case 0x41544144: // 'DATA'
                // LoadData copies the chunk's raw bytes over the struct
                LoadData(apFile, reinterpret_cast<char*>(&data), sizeof(OBJ_LIGH));
                break;
            case 0x4D414E46: // 'FNAM'
                apFile->GetChunkData<float>(fFade);
                break;
            case 0x4D414E53: { // 'SNAM'
                uint32_t iSound = 0;
                apFile->GetChunkData<uint32_t>(iSound);
                // the sound stays a form ID until InitItem resolves it
                m_pSound = reinterpret_cast<TESSound*>(iSound);
                break;
            }
        }
        if (iChunk == 0x41544144) { // 'DATA'
            if (apFile->QEndian()) {
                data.Endian();
            }
            if (data.fFOV == 0.0) {
                data.fFOV = 90.0f;
            }
            if (data.fFallOffExponent == 0.0) {
                data.fFallOffExponent = 1.0f;
            }
        }
    }
    while (apFile->NextChunk());
    return true;
}

void TESObjectLIGH::Save() {
    uint32_t v2;
    TESForm::StartForm();
    static_cast<TESModelTextureSwap*>(this)->Save(MODL_ID, MODT_ID, MODS_ID);
    static_cast<TESScriptableForm*>(this)->Save();
    static_cast<BGSDestructibleObjectForm*>(this)->Save();
    if (((int32_t)(data.flags & 2)) != 0) {
        static_cast<TESFullName*>(this)->Save();
        static_cast<TESTexture*>(this)->Save(ICON_ID);
        static_cast<BGSMessageIcon*>(this)->Save(static_cast<CHUNK_ID>(1329809741));
    }
    else {
        data.flags &= (~0x20u);
        data.time = (-1);
    }
    if (TESForm::QEndianSwapOnSave()) {
        (&data)->Endian();
    }
    TESForm::__SaveData(&data, 0x18u);
    if (TESForm::QEndianSwapOnSave()) {
        (&data)->Endian();
    }
    TESForm::AddChunk(FNAM_ID, fFade);
    if (m_pSound) {
        v2 = m_pSound->TESForm::GetFormID();
        TESForm::AddChunk(SNAM_ID, v2);
    }
    TESForm::CloseForm();
}

uint16_t TESObjectLIGH::GetSaveSize(uint32_t aiFlags) {
    uint16_t saveSize = TESForm::GetSaveSize(aiFlags);
    return (uint16_t)(static_cast<TESValueForm*>(this)->TESValueForm::GetSaveSize(aiFlags) + saveSize);
}

void TESObjectLIGH::SaveGame(uint32_t aiFlags) {
    TESForm::SaveGame(aiFlags);
    static_cast<TESValueForm*>(this)->TESValueForm::SaveGame(aiFlags);
}

void TESObjectLIGH::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    TESForm::SaveGame(apSaveGameBuffer);
    static_cast<TESValueForm*>(this)->SaveGame(apSaveGameBuffer);
}

void TESObjectLIGH::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    TESForm::LoadGame(aiFlags, aiCurrentFlags);
    static_cast<TESValueForm*>(this)->TESValueForm::LoadGame(aiFlags, aiCurrentFlags);
}

void TESObjectLIGH::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESForm::LoadGame(apLoadGameBuffer);
    static_cast<TESValueForm*>(this)->LoadGame(apLoadGameBuffer);
}

void TESObjectLIGH::InitItem() {
    if (GetInitialized()) {
        return;
    }
    TESScriptableForm::InitItem(this);
    BGSDestructibleObjectForm::InitItem(this);
    TESModelTextureSwap::InitItem(this);
    if (m_pSound) {
        // the loaded sound is a form ID until it is resolved here
        uint32_t aiID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(m_pSound));
        TESFile* pFile = GetFile(-1);
        TESForm::AddCompileIndex(aiID, pFile);
        m_pSound = dynamic_cast<TESSound*>(TESForm::GetFormByNumericID(aiID));
        if (!m_pSound) {
            Warning("MASTERFILE: Unable to find sound (%08X) on object '%s'.", aiID, GetFormEditorID());
        }
    }
    SetInitialized(true);
}

NiColor* TESObjectLIGH::GetEmittanceColor() {
    return &EmittanceColor;
}

void TESObjectLIGH::Copy(TESForm* apCopy) {
    TESObjectLIGH* pcopy = dynamic_cast<TESObjectLIGH*>(apCopy);
    if (pcopy) {
        CopyAllComponents(apCopy);
        data = pcopy->data;
        SetFade(pcopy->GetFade());
        SetSound(pcopy->m_pSound);
    }
}

bool TESObjectLIGH::Compare(TESForm* apCompare) {
    TESObjectLIGH* pComp = dynamic_cast<TESObjectLIGH*>(apCompare);
    if (!pComp) {
        return true;
    }
    if (CompareAllComponents(pComp)) {
        return true;
    }
    if (m_pSound != pComp->m_pSound) {
        return true;
    }
    void* Children = &pComp->data;
    if (std::memcmp(&data, Children, sizeof(data))) {
        return true;
    }
    double LastFadePercent = fFade;
    return LastFadePercent != pComp->fFade;
}

bool TESObjectLIGH::Activate(
    TESObjectREFR* apItemActivated,
    TESObjectREFR* apActionRef,
    bool abIdFlag,
    TESBoundObject* apObjecttoGet,
    int32_t aiCount
) {
    return (bool)((uint8_t)(GetCanCarry() &&
                            TESBoundObject::Activate(apItemActivated, apActionRef, abIdFlag, apObjecttoGet, aiCount)));
}

void TESObjectLIGH::UnClone3D(TESObjectREFR* pRequester) {
    const char* v2;
    if (static_cast<TESModelTextureSwap*>(this)->TESModel::GetModelLength()) {
        v2 = static_cast<TESModelTextureSwap*>(this)->GetModel();
        pModelLoader->ReleaseFile(v2);
    }
}

NiAVObject* TESObjectLIGH::LoadGraphics(TESObjectREFR* apRef) {
    NiAVObject* pGraphics;
    if (BSstrlen(GetModel())) {
        pGraphics = TESObject::LoadGraphics(apRef);
    }
    else {
        void* pMemory = NiMemObject::operator new(sizeof(NiNode));
        pGraphics = pMemory ? ::new (pMemory) NiNode(0) : nullptr;
    }
    if (pGraphics && !pGraphics->IsNode()) {
        void* pMemory = NiMemObject::operator new(sizeof(NiNode));
        NiNode* pNode = pMemory ? ::new (pMemory) NiNode(0) : nullptr;
        pNode->AttachChild(pGraphics, true);
        pGraphics = pNode;
    }
    return pGraphics;
}

// Loads the light's graphics and returns a clone of them for pRequester, scaled by its scale: the model cloned, or, for
// a light without a model, the loaded fade node renamed to the editor ID (a new BSFadeNode when it is none), with the
// light attached through GenDynamic. The unused NiPointer (constructed from null) and the graphics pointer are released
// in the game's order by their destructors on each return.
NiAVObject* TESObjectLIGH::Clone3D(TESObjectREFR* pRequester) {
    NiAVObject* pClone = nullptr;
    NiPointer<NiAVObject> spUnused(nullptr);
    NiPointer<NiAVObject> spGraphics(LoadGraphics(pRequester));
    if (!spGraphics.operator->()) {
        return nullptr;
    }
    uint32_t uiBufferSize = GetFormEditorIDLength() + 16;
    void* pBuffer = __builtin_alloca_with_align(uiBufferSize, 128);
    (void)pBuffer;
    if (!static_cast<TESModelTextureSwap*>(this)->TESModel::GetModelLength()) {
        NiAVObject* pGraphics = spGraphics.operator->();
        pClone = pGraphics->IsFadeNode();
        if (pClone) {
            NiFixedString kName(GetFormEditorID());
            pClone->SetName(kName);
        }
    }
    if (GetNoCollision() && pRequester) {
        pRequester->SetNoCollision(true);
    }
    float fScale = 1.0f;
    if (pRequester) {
        fScale = pRequester->GetScale();
    }
    BSCloningProcess kCloning;
    kCloning.~BSCloningProcess();
    ::new (&kCloning) BSCloningProcess(fScale);
    if (static_cast<TESModelTextureSwap*>(this)->TESModel::GetModelLength()) {
        pClone = static_cast<NiAVObject*>(spGraphics.operator->()->Clone(kCloning));
    }
    else if (!pClone) {
        void* pMemory = NiMemObject::operator new(sizeof(BSFadeNode));
        pClone = pMemory ? ::new (pMemory) BSFadeNode : nullptr;
    }
    GenDynamic(pRequester, static_cast<BSFadeNode*>(pClone), false);
    pClone->UpdateProperties();
    NiAVObject* pResult = pClone;
    return pResult;
}

NiLight* TESObjectLIGH::GenDynamic(TESObjectREFR* pRequester, NiAVObject* pModel, bool abForceDynamic) {
    XFNV_INLINED_LOCAL(NiFixedString, kName, std::nullopt, std::nullopt);
    if (!pModel) {
        return nullptr;
    }
    if (pRequester) {
        const NiPoint3* ppos;
        TESForm* pObj = pRequester->GetObjectReference();
        if ((data.flags & LF_OFF_BY_DEFAULT) != 0 ||
            (pRequester->GetParentCell() &&
             (ppos = &pRequester->GetLocationOnReference(),
              static_cast<double>(ppos->z) < pRequester->GetRelevantWaterHeight()))) {
            if (pObj->GetFormType() == LIGH_ID && TESObjectREFR::HasAddonNodes(static_cast<NiNode*>(pModel))) {
                pRequester->SetFireOff(true);
                TESObjectLIGH::StopParticlesRecursive(static_cast<NiNode*>(pModel));
            }
            if (!pRequester->IsActor() && (data.flags & LF_OFF_BY_DEFAULT) != 0) {
                return nullptr;
            }
        }
    }
    NiLight* plight = nullptr;
    NiColor kColor(0.0f, 0.0f, 0.0f);
    kColor.r = static_cast<double>(static_cast<uint8_t>(data.color)) / 255.0;
    kColor.g = static_cast<double>(static_cast<uint8_t>(static_cast<uint16_t>(data.color) >> 8)) / 255.0;
    kColor.b = static_cast<double>(static_cast<uint8_t>(data.color >> 16)) / 255.0;
    if ((data.flags & LF_NEGATIVE) != 0) {
        kColor = -kColor;
    }
    NiNode* pattachnode = nullptr;
    NiAVObject* pobj = pModel->GetObjectByName(FixedStrings::QAttachLight());
    if (pobj) {
        pattachnode = pobj->IsNode();
    }
    if (!pattachnode) {
        pattachnode = static_cast<NiNode*>(pModel);
    }
    plight = FindLight(pattachnode);
    if (!plight) {
        NiPointLight* pnewLight;
        char acName[264];
        BSsprintf(acName, 260, "%s PtLight", GetFormEditorID());
        void* pMemory = NiMemObject::operator new(sizeof(NiPointLight));
        if (pMemory) {
            pnewLight = ::new (pMemory) NiPointLight;
        }
        else {
            pnewLight = nullptr;
        }
        plight = pnewLight;
        NiFixedString* pname = ::new (static_cast<void*>(&kName)) NiFixedString(static_cast<const char*>(acName));
        plight->SetName(*pname);
        kName.~NiFixedString();
        pattachnode->AttachChild(plight, 1);
    }
    if (pRequester) {
        pRequester->SetLight(plight);
    }
    plight->SetAmbientColor(NiColor_BLACK);
    plight->SetSpecularColor(NiColor_BLACK);
    float fRadius = static_cast<float>(data.radius);
    if (pRequester) {
        fRadius = pRequester->GetRadius();
    }
    NiColor kSpecular(fRadius, fRadius, fRadius);
    plight->SetSpecularColor(kSpecular);
    plight->SetDiffuseColor(kColor);
    for (NiTimeController* pkCtrl = pModel->GetControllers(); pkCtrl; pkCtrl = pkCtrl->GetNext()) {
        if (pkCtrl->IsTransformController()) {
            SetDynamic(true);
            break;
        }
    }
    if (!pRequester ||
        (!(&pRequester->m_Extra)->GetLitWaterRefs() ||
         ((&pRequester->m_Extra)->GetLitWaterRefs() && (&pRequester->m_Extra)->GetLitWaterRefs()->IsEmpty()))) {
        bool isDynamic = abForceDynamic || GetDynamic();
        ShadowSceneNode* pshadowSceneNode = BSShaderManager::GetShadowSceneNode(0);
        pshadowSceneNode->AddLight(plight, isDynamic);
    }
    if (plight) {
        SetLightAttenuation(plight, static_cast<int64_t>(fRadius));
        plight->SetDimmer(fFade);
    }
    return plight;
}

void TESObjectLIGH::SetSound(TESSound* pSound) {
    m_pSound = pSound;
}

bool TESObjectLIGH::GetFlicker() {
    return (data.flags & LF_FLICKER) != 0;
}

bool TESObjectLIGH::GetFlickerSlow() {
    return (data.flags & LF_FLICKER_SLOW) != 0;
}

bool TESObjectLIGH::GetPulse() {
    return (data.flags & LF_PULSE) != 0;
}

bool TESObjectLIGH::GetPulseSlow() {
    return (data.flags & LF_PULSE_SLOW) != 0;
}

bool TESObjectLIGH::GetDynamic() {
    return (data.flags & LF_DYNAMIC) != 0;
}

bool TESObjectLIGH::GetCanCarry() {
    return ((((int32_t)data.flags) & 2) != 0);
}

void TESObjectLIGH::FixGravity(const NiPoint3& angle) {}

TESSound* TESObjectLIGH::GetSound() {
    return m_pSound;
}

float TESObjectLIGH::GetFade() {
    return fFade;
}

void TESObjectLIGH::SetFade(float afVal) {
    fFade = afVal;
}

bool TESObjectLIGH::Update(LightRefData* apLight, ExtraTimeLeft* apExtra) {
    return Update(apLight->spLight, &apLight->fWantDimmer, apExtra);
}

bool TESObjectLIGH::Update(NiPointer<NiLight> aspLight, float* apWantDimmer, ExtraTimeLeft* apExtra) {
    NiPointLight* pLight = static_cast<NiPointLight*>(aspLight.m_pObject);
    bool bShouldResetFadeValue = false;
    if ((data.flags & LF_FLICKER) != 0 || (data.flags & LF_FLICKER_SLOW) != 0) {
        float v20;
        float fTimeDelta = appTimer.fDelta;
        float fOffsetX = 1.0f;
        float fOffsetY = 0.0f;
        float fOffsetZ = 0.0f;
        if (pLight) {
            fOffsetX = pLight->m_fAtten0;
            fOffsetY = pLight->m_fAtten1;
            fOffsetZ = pLight->m_fAtten2;
        }
        if ((data.flags & LF_FLICKER_SLOW) != 0) {
            fTimeDelta = fTimeDelta / 2.0;
        }
        int32_t v38 = -1;
        fOffsetX = (static_cast<double>(Random()) / 357913952.0 + 1.100000023841858) * fTimeDelta + fOffsetX;
        fOffsetY = (static_cast<double>(Random()) / 357913952.0 + 1.2000000476837158) * fTimeDelta + fOffsetY;
        fOffsetZ = (static_cast<double>(Random()) / 238609296.0 + 1.2999999523162842) * fTimeDelta + fOffsetZ;
        if (fOffsetX > 6.2831854820251465) {
            fOffsetX = fOffsetX - 6.2831854820251465;
        }
        if (fOffsetY > 6.2831854820251465) {
            fOffsetY = fOffsetY - 6.2831854820251465;
        }
        if (fOffsetZ > 6.2831854820251465) {
            fOffsetZ = fOffsetZ - 6.2831854820251465;
        }
        if (pLight) {
            pLight->SetConstantAttenuation(fOffsetX);
            pLight->SetLinearAttenuation(fOffsetY);
            pLight->SetQuadraticAttenuation(fOffsetZ);
        }
        float v23 = fOffsetX + 1.7000000476837158 + (fOffsetX + 1.7000000476837158);
        float v36 = std::sin(v23);
        float v22 = (fOffsetY + 0.5) * 1.7000000476837158;
        float v31 = std::sin(v22);
        float v21 = (fOffsetZ + 0.30000001192092896) * 2.5999999046325684;
        float v35 = std::sin(v21);
        float v37 = gs_fFlickerMovement_bLightAttenuation.Float();
        float v41 = v37 * v36 * v31 / 2.0;
        if (!(v37 + v41 > 0.0)) {
            v20 = 0.0f;
        }
        else {
            v20 = v41;
        }
        v37 = v20;
        float v19 = v35 * v20;
        float v18 = v31 * v20;
        float v17 = v36 * v20;
        aspLight.m_pObject->SetTranslate(v17, v18, v19);
        if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
            NiLight* a2 = aspLight.m_pObject;
            TaskQueueInterface* Instance = TaskQueueInterface::GetInstance();
            Instance->QueueObjectUpdate(a2);
        }
        else {
            NiUpdateData arData(0.0f, false, false);
            aspLight.m_pObject->Update(arData);
        }
        float v16 = (fOffsetZ + 0.30000001192092896) * 3.0;
        double v5 = std::sin(v16);
        float v15 = (fOffsetY + 0.5) * 1.2999999523162842;
        double v14 = v5 / 5.0;
        double v6 = std::sin(v15);
        float v13 = (fOffsetX + 1.7000000476837158) * 1.100000023841858;
        float v33 = (std::sin(v13) + 1.0) * ((v6 + 1.0) / 2.0) / 2.0 / 3.0 + v14;
        float a2a = BSMIN(1.0, v33);
        v33 = BSMAX(-1.0f, a2a);
        v33 = v33 * 0.5 + 0.5;
        float v12 = (v33 * 0.4000000059604645 + 0.6000000238418579) * fFade;
        aspLight.m_pObject->SetDimmer(v12);
    }
    else {
        if ((data.flags & LF_PULSE) != 0 || ((data.flags & LF_PULSE_SLOW) != 0 || (apExtra && apExtra->fTime == 0.0))) {
            double v7;
            float DodgeDuringAttackMult = aspLight.m_pObject->m_fDimmer;
            float Seconds = appTimer.fDelta;
            float v28 = 0.1f;
            if ((data.flags & LF_FLICKER_SLOW) != 0 || (data.flags & LF_PULSE_SLOW) != 0) {
                v28 = 0.05f;
            }
            if (!(*apWantDimmer > static_cast<double>(DodgeDuringAttackMult))) {
                v7 = DodgeDuringAttackMult - v28 * Seconds;
            }
            else {
                v7 = v28 * Seconds + DodgeDuringAttackMult;
            }
            float v27 = v7;
            aspLight.m_pObject->SetDimmer(v27);
            float v11 = v27 - *apWantDimmer;
            if (static_cast<double>(v28) > std::fabs(v11)) {
                if (apExtra && apExtra->fTime == 0.0) {
                    bShouldResetFadeValue = true;
                }
                else {
                    if ((data.flags & LF_FLICKER) != 0 || (data.flags & LF_FLICKER_SLOW) != 0) {
                        *apWantDimmer = (static_cast<double>(Random() % 75) * 0.009999999776482582 + 0.25) * fFade;
                    }
                    else {
                        if ((data.flags & LF_PULSE) != 0 || (data.flags & LF_PULSE_SLOW) != 0) {
                            if (!(*apWantDimmer > 0.5)) {
                                *apWantDimmer = fFade;
                            }
                            else {
                                *apWantDimmer = fFade * 0.25;
                            }
                        }
                    }
                }
            }
        }
    }
    return bShouldResetFadeValue;
}

bool TESObjectLIGH::GetOffByDefault() {
    return ((data.flags & 32) != 0);
}

void TESObjectLIGH::SetDynamic(bool abVal) {
    uint32_t flags;
    uint32_t v3;
    if (abVal) {
        data.flags |= 1u;
        flags = data.flags;
    }
    else {
        data.flags &= (~1u);
        v3 = data.flags;
    }
    uint32_t v4 = data.flags;
}

// The target reads its node from the stack, not from ECX as a member function would.
void TESObjectLIGH::StopParticlesRecursive(NiNode* pNode) {
    NiProperty* property;
    NiAVObject* v2;
    uint32_t a1;
    for (a1 = 0; a1 < pNode->GetArrayCount(); ++a1) {
        v2 = pNode->GetAt(a1);
        if (v2) {
            if (v2->IsNode()) {
                TESObjectLIGH::StopParticlesRecursive(static_cast<NiNode*>(v2));
            }
            else {
                property = v2->GetProperty(2);
                if (property) {
                    static_cast<NiMaterialProperty*>(property)->SetEmittance(NiColor_BLACK);
                }
            }
        }
    }
}
