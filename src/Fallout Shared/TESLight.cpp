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
#include "xfnv/Compiler.h"

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
    NiPointLight* pPointLight = static_cast<NiPointLight*>(
        NiObject::DynamicCastStatic(&NiPointLight::ms_RTTI, apLight)
    );
    if (pPointLight) {
        if ((ATTENUATION_SETTINGS::cUsageFlags & ATTENUATION_SETTINGS::USE_CONSTANT) != 0) {
            float fConstantValue = gs_fConstantValue_bLightAttenuation.Float();
            pPointLight->SetConstantAttenuation(fConstantValue);
        }
        else {
            pPointLight->SetConstantAttenuation(0.0);
        }
        pPointLight->SetLinearAttenuation(0.0);
        pPointLight->SetQuadraticAttenuation(0.0);
        if ((ATTENUATION_SETTINGS::cUsageFlags & ATTENUATION_SETTINGS::USE_LINEAR) != 0 ||
            (TES::pInstance->GetInterior() && gs_bOutQuadInLin_bLightAttenuation == 1)) {
            float fLinearRadius = gs_fLinearRadiusMult_bLightAttenuation.Float() * static_cast<double>(aiRadius);
            uint32_t uLinearMethod = gs_uLinearMethod_bLightAttenuation.UInt();
            if (uLinearMethod) {
                float fAttenuation;
                double dLinear;
                if (uLinearMethod == 2) {
                    if (fLinearRadius == 0.0) {
                        dLinear = 0.009999999776482582;
                    }
                    else {
                        dLinear = gs_fLinearValue_bLightAttenuation.Float() / (fLinearRadius * fLinearRadius);
                    }
                    fAttenuation = dLinear;
                    pPointLight->SetLinearAttenuation(fAttenuation);
                }
                else {
                    if (fLinearRadius == 0.0) {
                        dLinear = 0.009999999776482582;
                    }
                    else {
                        dLinear = gs_fLinearValue_bLightAttenuation.Float() * (1.0 / fLinearRadius);
                    }
                    fAttenuation = dLinear;
                    pPointLight->SetLinearAttenuation(fAttenuation);
                }
            }
            else {
                pPointLight->SetLinearAttenuation(gs_fLinearValue_bLightAttenuation.Float());
            }
        }
        if ((ATTENUATION_SETTINGS::cUsageFlags & ATTENUATION_SETTINGS::USE_QUADRATIC) != 0 ||
            (!TES::pInstance->GetInterior() && gs_bOutQuadInLin_bLightAttenuation == 1)) {
            float fQuadraticRadius = gs_fQuadraticRadiusMult_bLightAttenuation.Float() * static_cast<double>(aiRadius);
            uint32_t uQuadraticMethod = gs_uQuadraticMethod_bLightAttenuation.UInt();
            if (uQuadraticMethod) {
                float fAttenuation;
                double dQuadratic;
                if (uQuadraticMethod == 1) {
                    if (fQuadraticRadius == 0.0) {
                        dQuadratic = 0.009999999776482582;
                    }
                    else {
                        dQuadratic = static_cast<float>(gs_fQuadraticValue_bLightAttenuation) *
                            (1.0 / static_cast<double>(aiRadius));
                    }
                    fAttenuation = dQuadratic;
                    pPointLight->SetQuadraticAttenuation(fAttenuation);
                }
                else {
                    if (fQuadraticRadius == 0.0) {
                        dQuadratic = 0.009999999776482582;
                    }
                    else {
                        dQuadratic = static_cast<float>(gs_fQuadraticValue_bLightAttenuation) /
                            (fQuadraticRadius * fQuadraticRadius);
                    }
                    fAttenuation = dQuadratic;
                    pPointLight->SetQuadraticAttenuation(fAttenuation);
                }
            }
            else {
                pPointLight->SetQuadraticAttenuation(gs_fQuadraticValue_bLightAttenuation.Float());
            }
        }
    }
}

NiLight* FindLight(NiNode* apNode) {
    NiLight* plight = nullptr;
    if (!apNode) {
        return plight;
    }
    for (uint32_t n = 0; n < apNode->GetArrayCount(); ++n) {
        NiPointer<NiAVObject> spchild(apNode->GetAt(n));
        if (spchild.m_pObject) {
            if (spchild->IsNode()) {
                plight = FindLight(static_cast<NiNode*>(spchild.m_pObject));
                if (plight) {
                    return plight;
                }
            }
            else if (NiObject::IsKindOf_0(&NiLight::ms_RTTI, spchild.m_pObject)) {
                return static_cast<NiLight*>(spchild.m_pObject);
            }
        }
    }
    return plight;
}

bool GetAttenuationSettings() {
    ATTENUATION_SETTINGS::cUsageFlags = 0;
    if (gs_bUseConstant_bLightAttenuation == 1) {
        ATTENUATION_SETTINGS::cUsageFlags |= ATTENUATION_SETTINGS::USE_CONSTANT;
    }
    if (gs_bUseLinear_bLightAttenuation == 1) {
        ATTENUATION_SETTINGS::cUsageFlags |= ATTENUATION_SETTINGS::USE_LINEAR;
    }
    if (gs_bUseQuadratic_bLightAttenuation == 1) {
        ATTENUATION_SETTINGS::cUsageFlags |= ATTENUATION_SETTINGS::USE_QUADRATIC;
    }
    if (!ATTENUATION_SETTINGS::cUsageFlags) {
        ATTENUATION_SETTINGS::cUsageFlags = ATTENUATION_SETTINGS::USE_QUADRATIC;
    }
    if (gs_uLinearMethod_bLightAttenuation.Int() &&
        gs_uLinearMethod_bLightAttenuation.Int() != 1 &&
        gs_uLinearMethod_bLightAttenuation.Int() != 2) {
        gs_uLinearMethod_bLightAttenuation = 1;
    }

    if (gs_uQuadraticMethod_bLightAttenuation.Int() &&
        gs_uQuadraticMethod_bLightAttenuation.Int() != 1 &&
        gs_uQuadraticMethod_bLightAttenuation.Int() != 2) {
        gs_uQuadraticMethod_bLightAttenuation = 2;
    }

    if (gs_fLinearRadiusMult_bLightAttenuation.Float() == 0.0) {
        gs_fLinearRadiusMult_bLightAttenuation = 1.0f;
    }
    if (gs_fQuadraticRadiusMult_bLightAttenuation.Float() == 0.0) {
        gs_fQuadraticRadiusMult_bLightAttenuation = 1.0f;
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
                char* pEditorID = static_cast<char*>(XFNV_ALLOCA(apFile->GetChunkSize()));
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
                char* pData = static_cast<char*>(XFNV_ALLOCA(iSize));
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
    TESForm::StartForm();
    static_cast<TESModelTextureSwap*>(this)->Save(MODL_ID, MODT_ID, MODS_ID);
    static_cast<TESScriptableForm*>(this)->Save();
    static_cast<BGSDestructibleObjectForm*>(this)->Save();
    if ((data.flags & LF_CAN_CARRY) != 0) {
        static_cast<TESFullName*>(this)->Save();
        static_cast<TESTexture*>(this)->Save(ICON_ID);
        static_cast<BGSMessageIcon*>(this)->Save(MICO_ID);
    }
    else {
        data.flags &= ~LF_OFF_BY_DEFAULT;
        data.time = -1;
    }
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    TESForm::__SaveData(&data, sizeof(data));
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    TESForm::AddChunk(FNAM_ID, fFade);
    if (m_pSound) {
        uint32_t uiSoundID = m_pSound->TESForm::GetFormID();
        TESForm::AddChunk(SNAM_ID, uiSoundID);
    }
    TESForm::CloseForm();
}

uint16_t TESObjectLIGH::GetSaveSize(uint32_t aiFlags) {
    uint16_t saveSize = TESForm::GetSaveSize(aiFlags);
    return static_cast<uint16_t>(static_cast<TESValueForm*>(this)->TESValueForm::GetSaveSize(aiFlags) + saveSize);
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
        uint32_t uiSoundID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(m_pSound));
        TESFile* pFile = GetFile(-1);
        TESForm::AddCompileIndex(uiSoundID, pFile);
        m_pSound = dynamic_cast<TESSound*>(TESForm::GetFormByNumericID(uiSoundID));
        if (!m_pSound) {
            Warning("MASTERFILE: Unable to find sound (%08X) on object '%s'.", uiSoundID, GetFormEditorID());
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
    if (std::memcmp(&data, &pComp->data, sizeof(data))) {
        return true;
    }
    double dFade = fFade;
    return dFade != pComp->fFade;
}

bool TESObjectLIGH::Activate(
    TESObjectREFR* apItemActivated,
    TESObjectREFR* apActionRef,
    bool abIdFlag,
    TESBoundObject* apObjecttoGet,
    int32_t aiCount
) {
    return GetCanCarry() && TESBoundObject::Activate(apItemActivated, apActionRef, abIdFlag, apObjecttoGet, aiCount);
}

void TESObjectLIGH::UnClone3D(TESObjectREFR* pRequester) {
    if (static_cast<TESModelTextureSwap*>(this)->TESModel::GetModelLength()) {
        const char* pModel = static_cast<TESModelTextureSwap*>(this)->GetModel();
        pModelLoader->ReleaseFile(pModel);
    }
}

NiAVObject* TESObjectLIGH::LoadGraphics(TESObjectREFR* apRef) {
    NiAVObject* pGraphics;
    if (BSstrlen(GetModel())) {
        pGraphics = TESObject::LoadGraphics(apRef);
    }
    else {
        pGraphics = new NiNode(0);
    }
    if (pGraphics && !pGraphics->IsNode()) {
        NiNode* pNode = new NiNode(0);
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
    if (!spGraphics) {
        return nullptr;
    }
    GetFormEditorIDLength();
    if (!static_cast<TESModelTextureSwap*>(this)->TESModel::GetModelLength()) {
        NiAVObject* pGraphics = spGraphics;
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
    BSCloningProcess kCloning(fScale);
    if (static_cast<TESModelTextureSwap*>(this)->TESModel::GetModelLength()) {
        pClone = static_cast<NiAVObject*>(spGraphics->Clone(kCloning));
    }
    else if (!pClone) {
        pClone = new BSFadeNode;
    }
    GenDynamic(pRequester, static_cast<BSFadeNode*>(pClone), false);
    pClone->UpdateProperties();
    return pClone;
}

NiLight* TESObjectLIGH::GenDynamic(TESObjectREFR* pRequester, NiAVObject* pModel, bool abForceDynamic) {
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
        char acName[264];
        BSsprintf(acName, 260, "%s PtLight", GetFormEditorID());
        NiPointLight* pnewLight = new NiPointLight;
        plight = pnewLight;
        plight->SetName(acName);
        pattachnode->AttachChild(plight, 1);
    }
    if (pRequester) {
        pRequester->SetLight(plight);
    }
    plight->SetAmbientColor(NiColor::BLACK);
    plight->SetSpecularColor(NiColor::BLACK);
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
        !pRequester->m_Extra.GetLitWaterRefs() ||
        (pRequester->m_Extra.GetLitWaterRefs() && pRequester->m_Extra.GetLitWaterRefs()->IsEmpty())) {
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
    return (data.flags & LF_CAN_CARRY) != 0;
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
        float fMovement;
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
        float fAngleX = fOffsetX + 1.7000000476837158 + (fOffsetX + 1.7000000476837158);
        float fSinX = std::sin(fAngleX);
        float fAngleY = (fOffsetY + 0.5) * 1.7000000476837158;
        float fSinY = std::sin(fAngleY);
        float fAngleZ = (fOffsetZ + 0.30000001192092896) * 2.5999999046325684;
        float fSinZ = std::sin(fAngleZ);
        float fFlickerMovement = gs_fFlickerMovement_bLightAttenuation.Float();
        float fWobble = fFlickerMovement * fSinX * fSinY / 2.0;
        if (!(fFlickerMovement + fWobble > 0.0)) {
            fMovement = 0.0f;
        }
        else {
            fMovement = fWobble;
        }
        float fTranslateZ = fSinZ * fMovement;
        float fTranslateY = fSinY * fMovement;
        float fTranslateX = fSinX * fMovement;
        aspLight->SetTranslate(fTranslateX, fTranslateY, fTranslateZ);
        if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
            NiLight* plightObject = aspLight.m_pObject;
            TaskQueueInterface::GetInstance()->QueueObjectUpdate(plightObject);
        }
        else {
            NiUpdateData arData(0.0f, false, false);
            aspLight->Update(arData);
        }
        float fDimAngleZ = (fOffsetZ + 0.30000001192092896) * 3.0;
        double dDimSinZ = std::sin(fDimAngleZ);
        float fDimAngleY = (fOffsetY + 0.5) * 1.2999999523162842;
        double dDimZ = dDimSinZ / 5.0;
        double dDimSinY = std::sin(fDimAngleY);
        float fDimAngleX = (fOffsetX + 1.7000000476837158) * 1.100000023841858;
        float fFlicker = (std::sin(fDimAngleX) + 1.0) * ((dDimSinY + 1.0) / 2.0) / 2.0 / 3.0 + dDimZ;
        float fClamped = BSMIN(1.0, fFlicker);
        fFlicker = BSMAX(-1.0f, fClamped);
        fFlicker = fFlicker * 0.5 + 0.5;
        float fNewDimmer = (fFlicker * 0.4000000059604645 + 0.6000000238418579) * fFade;
        aspLight->SetDimmer(fNewDimmer);
    }
    else if ((data.flags & LF_PULSE) != 0 || (data.flags & LF_PULSE_SLOW) != 0 || (apExtra && apExtra->fTime == 0.0)) {
        double dSteppedDimmer;
        float fCurrentDimmer = aspLight->m_fDimmer;
        float fTimeDelta = appTimer.fDelta;
        float fStep = 0.1f;
        if ((data.flags & LF_FLICKER_SLOW) != 0 || (data.flags & LF_PULSE_SLOW) != 0) {
            fStep = 0.05f;
        }
        if (!(*apWantDimmer > static_cast<double>(fCurrentDimmer))) {
            dSteppedDimmer = fCurrentDimmer - fStep * fTimeDelta;
        }
        else {
            dSteppedDimmer = fStep * fTimeDelta + fCurrentDimmer;
        }
        float fNewDimmer = dSteppedDimmer;
        aspLight->SetDimmer(fNewDimmer);
        float fDifference = fNewDimmer - *apWantDimmer;
        if (static_cast<double>(fStep) > std::fabs(fDifference)) {
            if (apExtra && apExtra->fTime == 0.0) {
                bShouldResetFadeValue = true;
            }
            else if ((data.flags & LF_FLICKER) != 0 || (data.flags & LF_FLICKER_SLOW) != 0) {
                *apWantDimmer = (static_cast<double>(Random() % 75) * 0.009999999776482582 + 0.25) * fFade;
            }
            else if ((data.flags & LF_PULSE) != 0 || (data.flags & LF_PULSE_SLOW) != 0) {
                if (!(*apWantDimmer > 0.5)) {
                    *apWantDimmer = fFade;
                }
                else {
                    *apWantDimmer = fFade * 0.25;
                }
            }
        }
    }
    return bShouldResetFadeValue;
}

bool TESObjectLIGH::GetOffByDefault() {
    return (data.flags & LF_OFF_BY_DEFAULT) != 0;
}

void TESObjectLIGH::SetDynamic(bool abVal) {
    if (abVal) {
        data.flags |= LF_DYNAMIC;
    }
    else {
        data.flags &= ~LF_DYNAMIC;
    }
}

// The target reads its node from the stack, not from ECX as a member function would.
void TESObjectLIGH::StopParticlesRecursive(NiNode* pNode) {
    for (uint32_t i = 0; i < pNode->GetArrayCount(); ++i) {
        NiAVObject* pChild = pNode->GetAt(i);
        if (pChild) {
            if (pChild->IsNode()) {
                TESObjectLIGH::StopParticlesRecursive(static_cast<NiNode*>(pChild));
            }
            else {
                NiProperty* pProperty = pChild->GetProperty(2);
                if (pProperty) {
                    static_cast<NiMaterialProperty*>(pProperty)->SetEmittance(NiColor::BLACK);
                }
            }
        }
    }
}
