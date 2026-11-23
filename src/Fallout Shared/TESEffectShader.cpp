#include "Fallout Shared/TESEffectShader.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSHavok/WeaponObject.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkWorld.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "BSShader/ParticleShader.h"
#include "BSShader/ParticleShaderProperty.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSDebris.h"
#include "Fallout Shared/EffectShaderData.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/Sky/SkyTextureManager.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESGrass.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout/AI/ArrowProjectile.h"
#include "Fallout/AI/BaseProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSDismemberSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"
#include "xfnv/Compiler.h"

#include <cmath>
#include <cstdint>

// The EDID buffer is initialised to "" (one zeroed byte, allocated even for an empty chunk), so an empty chunk
// (malformed data) does not leave it uninitialised.
bool TESEffectShader::Load(TESFile* apFile) {
    if (apFile->GetTESForm() != 79) {
        return false;
    }
    ClearData();
    LoadForm(apFile);
    SetInitialized(false);
    do {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        switch (uiChunk) {
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size, read up to 0x200.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize ? uiChunkSize : 1));
                // An empty chunk leaves the buffer unwritten (see above), so it is cleared first; the game does not do
                // this.
                pBuffer[0] = '\0';
                apFile->GetChunkData(pBuffer, 0x200);
                SetFormEditorID(pBuffer);
                break;
            }
            case ICO2_ID: TESTexture::LoadTextureChunk(&ParticleShaderTexture, apFile); break;
            case NAM7_ID: TESTexture::LoadTextureChunk(&BlockOutTexture, apFile); break;
            case DATA_ID:
                if (apFile->QEndian()) {
                    Data.Endian();
                }
                LoadData(apFile, reinterpret_cast<char*>(&Data), 0x134);
                // The file's record is 0x134 bytes, the addon models' 4-byte form ID at 0xF4.
                TESFile::WidenFormIDRecords(
                    &Data,
                    1,
                    0x134,
                    sizeof(Data),
                    {{0xF4, offsetof(EffectShaderData, pAddonModels)}}
                );
                if (apFile->QEndian()) {
                    Data.Endian();
                }
                break;
            case OBND_ID: LoadObjectBound(apFile); break;
            case ICON_ID: TESTexture::LoadTextureChunk(&TextureShaderTexture, apFile); break;
            default: break;
        }
    }
    while (apFile->NextChunk());
    // The float is compared with the doubles 1.0 and 0.0, and a NaN is skipped in both, as <= and > do. The product is
    // formed in double from the instance count and stored as a float.
    if (Data.fParticleFull <= 1.0 && Data.fParticleFull > 0.0) {
        Data.fParticleFull = static_cast<float>(
            ParticleShader::GetMaxInstanceSize() * static_cast<double>(Data.fParticleFull)
        );
    }
    if (Data.fParticlePersistent <= 1.0 && Data.fParticlePersistent > 0.0) {
        Data.fParticlePersistent = static_cast<float>(
            ParticleShader::GetMaxInstanceSize() * static_cast<double>(Data.fParticlePersistent)
        );
    }
    return true;
}

TESEffectShader::TESEffectShader()
  : TESForm(), Data(), TextureShaderTexture(), ParticleShaderTexture(), BlockOutTexture() {
    SetFormType(EFSH_ID);
    TESEffectShader::InitializeData();
}

TESEffectShader::~TESEffectShader() {}

void TESEffectShader::InitializeData() {
    GetData()->Initialize();
}

void TESEffectShader::Save() {
    BGSDebris* pSavedAddonModels;
    StartForm();
    if (TextureShaderTexture.GetTextureNameLength()) {
        TextureShaderTexture.Save(ICON_ID);
    }
    else {
        TESForm::AddChunkArray(
            ICON_ID,
            TextureShaderTexture.GetTextureName(),
            TextureShaderTexture.GetTextureNameLength() + 1
        );
    }
    if (ParticleShaderTexture.GetTextureNameLength()) {
        ParticleShaderTexture.Save(ICO2_ID);
    }
    else {
        TESForm::AddChunkArray(
            ICO2_ID,
            ParticleShaderTexture.GetTextureName(),
            ParticleShaderTexture.GetTextureNameLength() + 1
        );
    }
    if (BlockOutTexture.GetTextureNameLength()) {
        BlockOutTexture.Save(NAM7_ID);
    }
    else {
        TESForm::AddChunkArray(NAM7_ID, BlockOutTexture.GetTextureName(), BlockOutTexture.GetTextureNameLength() + 1);
    }
    BGSDebris* pAddonModels = Data.pAddonModels;
    if (pAddonModels) {
        // The record holds the add-on models' form ID in the pointer field while it is written.
        // Stores the uint32_t form ID in the BGSDebris* field.
        pSavedAddonModels = reinterpret_cast<BGSDebris*>(static_cast<uintptr_t>(pAddonModels->iFormID));
    }
    else {
        pSavedAddonModels = nullptr;
    }
    Data.pAddonModels = pSavedAddonModels;
    if (TESForm::QEndianSwapOnSave()) {
        Data.Endian();
    }
    // The file keeps the form IDs as 4 bytes; pack the record to its layout for the write and restore it after.
    TESFile::NarrowFormIDRecords(&Data, 1, 0x134, sizeof(Data), {{0xF4, offsetof(EffectShaderData, pAddonModels)}});
    __SaveData(&Data, 308u);
    TESFile::WidenFormIDRecords(&Data, 1, 0x134, sizeof(Data), {{0xF4, offsetof(EffectShaderData, pAddonModels)}});
    if (TESForm::QEndianSwapOnSave()) {
        Data.Endian();
    }
    Data.pAddonModels = pAddonModels;
    CloseForm();
}

void TESEffectShader::InitItem() {
    if (!GetInitialized()) {
        // Until the form is initialised the pointer field holds the add-on models' form ID as an integer.
        // The BGSDebris* in Data.pAddonModels holds a form ID (uint32_t) here.
        uint32_t uiAddonModelsID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Data.pAddonModels));
        if (uiAddonModelsID) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(uiAddonModelsID, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(uiAddonModelsID);
            Data.pAddonModels = dynamic_cast<BGSDebris*>(pForm);
        }
        SetInitialized(true);
    }
}

void TESEffectShader::Copy(TESForm* apCopy) {
    TESEffectShader* pCopy = dynamic_cast<TESEffectShader*>(apCopy);
    if (pCopy) {
        CopyAllComponents(pCopy);
        ourMemcpy(&Data, &pCopy->Data, sizeof(EffectShaderData));
        // Copies each texture through its vtable.
        TESTexture* pTextureShaderTexture = pCopy->GetTextureShaderTexture();
        BaseFormComponent* pComponent = &TextureShaderTexture;
        pComponent->CopyComponent(pTextureShaderTexture);
        pComponent = &ParticleShaderTexture;
        pComponent->CopyComponent(&pCopy->ParticleShaderTexture);
        TESTexture* pBlockOutTexture = pCopy->GetBlockOutTexture();
        pComponent = &BlockOutTexture;
        pComponent->CopyComponent(pBlockOutTexture);
    }
}

bool TESEffectShader::Compare(TESForm* apCompare) {
    TESEffectShader* pComp = dynamic_cast<TESEffectShader*>(apCompare);
    if (!pComp) {
        return true;
    }
    if (CompareAllComponents(pComp)) {
        return true;
    }
    // Compares each texture through its vtable.
    TESTexture* pTextureShaderTexture = pComp->GetTextureShaderTexture();
    BaseFormComponent* pComponent = &TextureShaderTexture;
    if (pComponent->CompareComponent(pTextureShaderTexture)) {
        return true;
    }
    pComponent = &ParticleShaderTexture;
    if (pComponent->CompareComponent(&pComp->ParticleShaderTexture)) {
        return true;
    }
    TESTexture* pBlockOutTexture = pComp->GetBlockOutTexture();
    pComponent = &BlockOutTexture;
    return pComponent->CompareComponent(pBlockOutTexture) ||
        std::memcmp(&Data, &pComp->Data, sizeof(EffectShaderData)) != 0;
}

void TESEffectShader::SetupParticleShader(ParticleShaderProperty* apProperty, NiSourceTexture* apTex) {
    apProperty->SetTexture(apTex);
    apProperty->SetLifetime(Data.fParticleLifetime);
    apProperty->SetLifetimeVar(Data.fParticleLifeVar);
    apProperty->SetNormalSpeed(Data.fParticleNormalSpeed);
    apProperty->SetNormalAcceleration(Data.fParticleNormalAcc);
    apProperty->SetNormalSpeedVar(Data.fParticleNormalSpeedVar);
    apProperty->SetVelocity(Data.ParticleVelocity);
    apProperty->SetAcceleration(Data.ParticleAcceleration);
    apProperty->SetRotate(Data.fParticleRotate);
    apProperty->SetRotateVar(Data.fParticleRotateVar);
    apProperty->SetRotateSpeed(Data.fParticleRotateSpeed);
    apProperty->SetRotateSpeedVar(Data.fParticleRotateSpeedVar);
    apProperty->SetTextureCountX(static_cast<float>(Data.iParticleTextureCountX));
    apProperty->SetTextureCountY(static_cast<float>(Data.iParticleTextureCountY));
    apProperty->SetExplosionWindSpeed(Data.fExplosionSpeed);
    float fB = static_cast<float>(static_cast<uint8_t>(Data.iParticleColor1 >> 16));
    float fG = static_cast<float>(static_cast<uint8_t>(static_cast<uint16_t>(Data.iParticleColor1) >> 8));
    float fR = static_cast<float>(static_cast<uint8_t>(Data.iParticleColor1));
    NiColorA color1 = NiColorA(fR, fG, fB, 0.0f) / 255.0f;
    fB = static_cast<float>(static_cast<uint8_t>(Data.iParticleColor2 >> 16));
    fG = static_cast<float>(static_cast<uint8_t>(static_cast<uint16_t>(Data.iParticleColor2) >> 8));
    fR = static_cast<float>(static_cast<uint8_t>(Data.iParticleColor2));
    NiColorA color2 = NiColorA(fR, fG, fB, 0.0f) / 255.0f;
    fB = static_cast<float>(static_cast<uint8_t>(Data.iParticleColor3 >> 16));
    fG = static_cast<float>(static_cast<uint8_t>(static_cast<uint16_t>(Data.iParticleColor3) >> 8));
    fR = static_cast<float>(static_cast<uint8_t>(Data.iParticleColor3));
    NiColorA color3 = NiColorA(fR, fG, fB, 0.0f) / 255.0f;
    color1.a = Data.fParticleColor1Alpha;
    color2.a = Data.fParticleColor2Alpha;
    color3.a = Data.fParticleColor3Alpha;
    apProperty->SetColor1(color1);
    apProperty->SetColor2(color2);
    apProperty->SetColor3(color3);
    apProperty->SetColorTime1(Data.fParticleColor1Time);
    apProperty->SetColorTime2(Data.fParticleColor2Time);
    apProperty->SetColorTime3(Data.fParticleColor3Time);
    apProperty->SetScale1(Data.fParticleScale1);
    apProperty->SetScale2(Data.fParticleScale2);
    apProperty->SetScaleTime1(Data.fParticleScale1Time);
    apProperty->SetScaleTime2(Data.fParticleScale2Time);
    apProperty->eSrcBlendMode = Data.eParticleBlendModeSource;
    apProperty->eDestBlendMode = Data.eParticleBlendModeDest;
    apProperty->SetBlendOp(Data.eParticleBlendOperation);
    apProperty->SetZTestFunc(Data.eParticleZTestFunction);
}

void TESEffectShader::StartTextureShader(
    NiAVObject* apObject,
    BSShaderPPLightingProperty::TextureEffectData* apTexEffectData
) {
    if (apObject) {
        if (apObject->IsGeometry()) {
            int32_t bIsLighting;
            NiShadeProperty* pProp = static_cast<NiShadeProperty*>(apObject->GetProperty(3));
            if (pProp) {
                bIsLighting = pProp->iShaderPropertyType >= 8 && pProp->iShaderPropertyType <= 12;
            }
            else {
                bIsLighting = 0;
            }
            // Shader property types 8 to 12 are lighting properties.
            BSShaderPPLightingProperty* pLightingProp = nullptr;
            if (bIsLighting) {
                pLightingProp = static_cast<BSShaderPPLightingProperty*>(pProp);
            }
            if (pLightingProp) {
                bool bApply = true;
                if (IsFlagSet(TESEffectShader::SKIN_ONLY)) {
                    NiMaterialProperty* pMat = static_cast<NiMaterialProperty*>(apObject->GetProperty(2));
                    if (!pMat->m_kName.GetHandleString() || BSstrcmp(pMat->m_kName.GetHandleString(), "skin")) {
                        bApply = false;
                    }
                }
                if (bApply) {
                    pLightingProp->SetTexEffectData(apTexEffectData);
                }
            }
        }
        else {
            NiNode* pNode = apObject->IsNode();
            if (pNode) {
                for (uint32_t i = 0; i < pNode->GetArrayCount(); ++i) {
                    StartTextureShader(pNode->GetAt(i), apTexEffectData);
                }
            }
        }
    }
}

bool TESEffectShader::IsFlagSet(EffectShaderFlags aeFlag) {
    return static_cast<uint8_t>(aeFlag & GetData()->cFlags) != 0;
}

// One animated shader value at afTime: fading (abFade) lowers afCurrentValue by a rate of at least 0.33 per
// instance-size step towards 0, and stops fading before afFadeIn + afFullTime; otherwise it ramps up to afFullValue
// over afFadeIn, holds it for afFullTime, then falls towards afPersistentValue over afFadeOut. afTimeDelta / afFadeOut
// (and 1 / afFadeOut) are kept as a double, and GetMaxInstanceSize's result is widened as unsigned.
float TESEffectShader::AnimateValue(
    float afCurrentValue,
    float afTimeDelta,
    float afTime,
    bool abFade,
    float afFadeIn,
    float afFadeOut,
    float afFullTime,
    float afFullValue,
    float afPersistentValue
) {
    if (abFade) {
        if (afTime < afFadeIn + afFullTime) {
            abFade = false;
        }
        else {
            float fRate = 0.0f;
            if (afFadeOut > 0.0 && afFullValue - afPersistentValue != 0.0) {
                double dInvFadeOut = 1.0 / afFadeOut;
                float fRange = afFullValue - afPersistentValue;
                fRate = static_cast<float>(dInvFadeOut * std::fabs(fRange));
            }
            if (fRate < 0.33000001311302185) {
                fRate = 0.33f;
            }
            fRate = static_cast<float>(static_cast<double>(ParticleShader::GetMaxInstanceSize()) * afTimeDelta * fRate);
            float fValue = afCurrentValue - fRate;
            afCurrentValue = BSMAX(0.0f, fValue);
        }
    }
    if (!abFade) {
        if (afFadeIn > afTime) {
            float fValue = afTimeDelta / afFadeIn * afFullValue + afCurrentValue;
            afCurrentValue = BSMIN(afFullValue, fValue);
        }
        else if (afFullTime > afTime - afFadeIn) {
            afCurrentValue = afFullValue;
        }
        else if (afFadeOut > afTime - afFadeIn - afFullTime) {
            double dStep = afTimeDelta / afFadeOut;
            float fRange = afFullValue - afPersistentValue;
            float fValue = static_cast<float>(afCurrentValue - dStep * std::fabs(fRange));
            afCurrentValue = BSMAX(afPersistentValue, fValue);
        }
        else {
            afCurrentValue = afPersistentValue;
        }
    }
    return afCurrentValue;
}

float TESEffectShader::GetEdgeAlphaFadeOutTime() {
    return Data.fEdgeAlphaFadeOutTime;
}

TESTexture* TESEffectShader::GetTextureShaderTexture() {
    return &TextureShaderTexture;
}

ParticleShaderProperty* TESEffectShader::StartParticleShader(
    NiAVObject* apRootObject,
    NiAVObject* apTargetObject,
    NiSourceTexture* apTex,
    float afTime
) {
    if (!apTex) {
        return nullptr;
    }
    ParticleShaderProperty* pProperty = ParticleShader::Start(static_cast<NiNode*>(apRootObject), apTargetObject);
    SetupParticleShader(pProperty, apTex);
    pProperty->InitParticleData(afTime);
    return pProperty;
}

void TESEffectShader::StopParticleShader(NiAVObject* apTargetObject, ParticleShaderProperty* apProp) {
    ParticleShader::Stop(apTargetObject, apProp);
}

float TESEffectShader::AnimateEdgeEffectAlpha(float afCurrentAlpha, float afTimeDelta, float afTime, bool abFade) {
    float fEdgeAlphaPersistentPercent = Data.fEdgeAlphaPersistentPercent;
    float fEdgeAlphaFullPercent = Data.fEdgeAlphaFullPercent;
    float fEdgeAlphaFullTime = Data.fEdgeAlphaFullTime;
    float fEdgeAlphaFadeOutTime = TESEffectShader::GetEdgeAlphaFadeOutTime();
    float fEdgeAlphaFadeInTime = Data.fEdgeAlphaFadeInTime;
    return AnimateValue(
        afCurrentAlpha,
        afTimeDelta,
        afTime,
        abFade,
        fEdgeAlphaFadeInTime,
        fEdgeAlphaFadeOutTime,
        fEdgeAlphaFullTime,
        fEdgeAlphaFullPercent,
        fEdgeAlphaPersistentPercent
    );
}

void TESEffectShader::StopTextureShader(
    NiAVObject* apObject,
    BSShaderPPLightingProperty::TextureEffectData* apTexEffectData
) {
    if (!apObject) {
        return;
    }
    if (apObject->IsGeometry()) {
        NiShadeProperty* pShade = static_cast<NiShadeProperty*>(apObject->GetProperty(NiProperty::SHADE));
        // A lighting shader property: shader property types 8 to 12.
        bool bIsLighting = pShade && pShade->GetShaderPropertyType() >= 8 && pShade->GetShaderPropertyType() <= 12;
        BSShaderPPLightingProperty* pProperty = bIsLighting ? static_cast<BSShaderPPLightingProperty*>(pShade)
                                                            : nullptr;
        if (pProperty && pProperty->GetTextureEffectData() == apTexEffectData) {
            pProperty->SetTexEffectData(nullptr);
        }
    }
    else {
        NiNode* pNode = apObject->IsNode();
        if (pNode) {
            for (uint32_t i = 0; i < pNode->GetArrayCount(); ++i) {
                StopTextureShader(pNode->GetAt(i), apTexEffectData);
            }
        }
    }
}

float TESEffectShader::CalculateFillEffectPulse(float afTime) {
    // A zero, negative or NaN amplitude gives no pulse.
    if (!(GetFillAlphaPulseAmplitude() > 0.0)) {
        return 1.0f;
    }
    double dAmplitude = GetFillAlphaPulseAmplitude();
    double dScaledTime = static_cast<double>(afTime) * NI_TWO_PI;
    float fPhase = static_cast<float>(GetFillAlphaPulseFrequency() * dScaledTime);
    return static_cast<float>(std::sin(fPhase) * dAmplitude);
}

TESTexture* TESEffectShader::GetBlockOutTexture() {
    return &BlockOutTexture;
}

float TESEffectShader::GetEdgeAlphaPulseFrequency() {
    return Data.fEdgeAlphaPulseFrequency;
}

float TESEffectShader::GetEdgeAlphaPulseAmplitude() {
    return Data.fEdgeAlphaPulseAmplitude;
}

double TESEffectShader::CalculateEdgeAlphaPulse(float afTime) {
    if (!(GetEdgeAlphaPulseAmplitude() > 0.0)) {
        return 1.0;
    }
    double dEdgeAlphaPulseAmpl = GetEdgeAlphaPulseAmplitude();
    double dScaledTime = afTime * NI_TWO_PI;
    float fEdgeAlphaPulseFreq = static_cast<float>(GetEdgeAlphaPulseFrequency() * dScaledTime);
    return static_cast<float>(std::sin(fEdgeAlphaPulseFreq) * dEdgeAlphaPulseAmpl);
}

float TESEffectShader::GetFillAlphaFullTime() {
    return Data.fFillAlphaFullTime;
}

float TESEffectShader::GetFillAlphaFadeOutTime() {
    return Data.fFillAlphaFadeOutTime;
}

float TESEffectShader::GetFillAlphaFadeInTime() {
    return Data.fFillAlphaFadeInTime;
}

float TESEffectShader::GetFillAlphaFullPercent() {
    return Data.fFillAlphaFullPercent;
}

float TESEffectShader::GetFillAlphaPersistentPercent() {
    return Data.fFillAlphaPersistentPercent;
}

float TESEffectShader::AnimateFillEffectAlpha(float afCurrentAlpha, float afTimeDelta, float afTime, bool abFade) {
    float fFillTexturePersistentAlphaRatio = GetFillAlphaPersistentPercent();
    float fFillTextureFullAlphaRatio = GetFillAlphaFullPercent();
    float fFillTextureFullAlphaTime = GetFillAlphaFullTime();
    float fFillTextureAlphaFadeOutTime = GetFillAlphaFadeOutTime();
    float fFillTextureAlphaFadeInTime = GetFillAlphaFadeInTime();
    return AnimateValue(
        afCurrentAlpha,
        afTimeDelta,
        afTime,
        abFade,
        fFillTextureAlphaFadeInTime,
        fFillTextureAlphaFadeOutTime,
        fFillTextureFullAlphaTime,
        fFillTextureFullAlphaRatio,
        fFillTexturePersistentAlphaRatio
    );
}

BSShaderPPLightingProperty::TextureEffectData*
TESEffectShader::CreateTextureShaderData(NiAVObject* apObject, NiSourceTexture* apTex, NiSourceTexture* apBlockOutTex) {
    BSShaderPPLightingProperty::TextureEffectData* pData = new BSShaderPPLightingProperty::TextureEffectData;
    int32_t iFillTextureRGB = Data.iFillColor;
    int32_t iEdgeTextureRGB = Data.iEdgeColor;
    pData->spTexture = static_cast<NiTexture*>(nullptr);
    float fB = static_cast<float>(static_cast<uint8_t>(iFillTextureRGB >> 16));
    float fG = static_cast<float>(static_cast<uint8_t>(iFillTextureRGB >> 8));
    float fR = static_cast<float>(static_cast<uint8_t>(iFillTextureRGB));
    pData->FillColor = NiColorA(fR, fG, fB, 0.0f) / 255.0f;
    fB = static_cast<float>(static_cast<uint8_t>(iEdgeTextureRGB >> 16));
    fG = static_cast<float>(static_cast<uint8_t>(iEdgeTextureRGB >> 8));
    fR = static_cast<float>(static_cast<uint8_t>(iEdgeTextureRGB));
    pData->RimColor = NiColorA(fR, fG, fB, 0.0f) / 255.0f;
    pData->fTextureUOffset = 0.0f;
    pData->fTextureVOffset = 0.0f;
    double dEdgeExponent = Data.fEdgeExponentValue;
    pData->fEdgeExponent = dEdgeExponent;
    double dRadius = apObject->GetWorldBound().m_fRadius;
    pData->fBoundDiameter = dRadius + dRadius;
    int32_t iSrcBlendMode = Data.eTextureBlendModeSource;
    pData->eSrcBlendMode = static_cast<D3DBLEND>(iSrcBlendMode);
    pData->eDestBlendMode = Data.eTextureBlendModeDest;
    pData->eBlendOperation = Data.eTextureBlendOperation;
    pData->eZTestFunction = Data.eTextureZTestFunction;
    pData->spTexture = static_cast<NiTexture*>(apTex);
    pData->spBlockOutTexture = static_cast<NiTexture*>(apBlockOutTex);
    pData->iAlphaTest = static_cast<int64_t>(Data.fAlphaTestStartValue);
    if (IsFlagSet(TESEffectShader::EDGECOLOR_SUBTRACTIVE)) {
        pData->RimColor.r = pData->RimColor.r + -1.0;
        pData->RimColor.g = pData->RimColor.g + -1.0;
        pData->RimColor.b = pData->RimColor.b + -1.0;
    }
    return pData;
}

double
TESEffectShader::AnimateParticleEffectBirthrate(float afCurrentValue, float afTimeDelta, float afTime, bool abFade) {
    float fParticleBirthPersistRatio = Data.fParticlePersistent;
    float fParticleBirthFullRatio = Data.fParticleFull;
    float fParticleBirthFullTime = Data.fParticleFullTime;
    float fParticleBirthRampDownTime = Data.fParticleFadeOutTime;
    float fParticleBirthRampUpTime = Data.fParticleFadeInTime;
    return AnimateValue(
        afCurrentValue,
        afTimeDelta,
        afTime,
        abFade,
        fParticleBirthRampUpTime,
        fParticleBirthRampDownTime,
        fParticleBirthFullTime,
        fParticleBirthFullRatio,
        fParticleBirthPersistRatio
    );
}

void TESEffectShader::AnimateTextureEffect(
    BSShaderPPLightingProperty::TextureEffectData* pData,
    float afTimeDelta,
    float afTime,
    float afAge,
    bool abFade
) {
    if (pData) {
        pData->FillColor.a = AnimateFillEffectAlpha(pData->FillColor.a, afTimeDelta, afTime, abFade);
        pData->CurrentFillColor = pData->FillColor;
        pData->CurrentFillColor.a = CalculateFillEffectPulse(afAge) * pData->CurrentFillColor.a +
            pData->CurrentFillColor.a;
        float fClampedAlpha = BSMAX(0.0f, pData->CurrentFillColor.a);
        pData->CurrentFillColor.a = BSMIN(1.0f, fClampedAlpha);
        pData->RimColor.a = AnimateEdgeEffectAlpha(pData->RimColor.a, afTimeDelta, afTime, abFade);
        pData->CurrentRimColor = pData->RimColor;
        pData->CurrentRimColor.a = CalculateEdgeAlphaPulse(afAge) * pData->CurrentRimColor.a + pData->CurrentRimColor.a;
        fClampedAlpha = BSMAX(0.0f, pData->CurrentRimColor.a);
        pData->CurrentRimColor.a = BSMIN(1.0f, fClampedAlpha);
        pData->fTextureUOffset = Data.fFillTextureUAnimSpeed * afTimeDelta + pData->fTextureUOffset;
        pData->fTextureVOffset = Data.fFillTextureVAnimSpeed * afTimeDelta + pData->fTextureVOffset;
        float fClampedTime = BSMAX(Data.fAlphaTestStartTime, afAge);
        float fAlphaTestTime = BSMIN(Data.fAlphaTestEndTime, fClampedTime);
        float fHolesEndTime = Data.fAlphaTestEndTime;
        float fHolesStartTime = Data.fAlphaTestStartTime;
        float fHolesEndVal = Data.fAlphaTestEndValue;
        float fHolesStartVal = Data.fAlphaTestStartValue;
        pData->iAlphaTest = static_cast<int64_t>(
            FLerp(fHolesStartVal, fHolesEndVal, fHolesStartTime, fHolesEndTime, fAlphaTestTime)
        );
    }
}

void ParticleShaderProperty::SetTexture(NiTexture* pTex) {
    spBaseTexture = pTex;
}

void ParticleShaderProperty::SetScaleTime2(float aScaleTime) {
    ScaleTime2 = aScaleTime;
}

void ParticleShaderProperty::SetVelocity(NiPoint3 aVelocity) {
    Velocity = aVelocity;
}

void ParticleShaderProperty::SetAcceleration(NiPoint3 aAcceleration) {
    Acceleration = aAcceleration;
}

void ParticleShaderProperty::SetColor1(NiColorA aColor) {
    Color1 = aColor;
}

void ParticleShaderProperty::SetColor3(NiColorA aColor) {
    Color3 = aColor;
}

EffectShaderData::EffectShaderData() {
    Initialize();
}

void EffectShaderData::Initialize() {
    NiPoint3 kAcceleration;
    NiPoint3 kVelocity;
    cFlags = 0;
    eTextureBlendModeSource = D3DBLEND_SRCALPHA;
    eTextureBlendOperation = D3DBLENDOP_ADD;
    eTextureZTestFunction = D3DCMP_EQUAL;
    iFillColor = 0;
    fFillAlphaFadeInTime = 0.0f;
    fFillAlphaFullTime = 0.0f;
    fFillAlphaFadeOutTime = 0.0f;
    fFillAlphaPersistentPercent = 0.0f;
    fFillAlphaPulseAmplitude = 0.0f;
    fFillAlphaPulseFrequency = 1.0f;
    fFillTextureUAnimSpeed = 0.0f;
    fFillTextureVAnimSpeed = 0.0f;
    fEdgeExponentValue = 1.0f;
    iEdgeColor = 0;
    fEdgeAlphaFadeInTime = 0.0f;
    fEdgeAlphaFullTime = 0.0f;
    fEdgeAlphaFadeOutTime = 0.0f;
    fEdgeAlphaPersistentPercent = 0.0f;
    fEdgeAlphaPulseAmplitude = 0.0f;
    fEdgeAlphaPulseFrequency = 1.0f;
    eTextureBlendModeDest = D3DBLEND_INVSRCALPHA;
    fFillAlphaFullPercent = 1.0f;
    fEdgeAlphaFullPercent = 1.0f;
    eParticleBlendModeSource = D3DBLEND_SRCALPHA;
    eParticleBlendModeDest = D3DBLEND_INVSRCALPHA;
    eParticleBlendOperation = D3DBLENDOP_ADD;
    eParticleZTestFunction = D3DCMP_LESSEQUAL;
    fParticleFadeInTime = 0.0f;
    fParticleFullTime = 0.0f;
    fParticleFadeOutTime = 0.0f;
    fParticleFull = 1.0f;
    fParticlePersistent = 1.0f;
    fParticleLifetime = 1.0f;
    fParticleLifeVar = 0.0f;
    fParticleNormalSpeed = 0.0f;
    fParticleNormalAcc = 0.0f;
    ParticleVelocity = *::new (&kVelocity) NiPoint3(0.0f, 0.0f, 0.0f);
    ParticleAcceleration = *::new (&kAcceleration) NiPoint3(0.0f, 0.0f, 0.0f);
    fParticleScale1 = 1.0f;
    fParticleScale2 = 1.0f;
    fParticleScale1Time = 0.0f;
    fParticleScale2Time = 1.0f;
    iParticleColor1 = 0xFFFFFF;
    iParticleColor2 = 0xFFFFFF;
    iParticleColor3 = 0xFFFFFF;
    fParticleColor1Alpha = 1.0f;
    fParticleColor2Alpha = 1.0f;
    fParticleColor3Alpha = 1.0f;
    fParticleColor1Time = 0.0f;
    fParticleColor2Time = 0.5f;
    fParticleColor3Time = 1.0f;
    fParticleNormalSpeedVar = 0.0f;
    fParticleRotate = 0.0f;
    fParticleRotateVar = 0.0f;
    fParticleRotateSpeed = 0.0f;
    fParticleRotateSpeedVar = 0.0f;
    fAlphaTestStartTime = 0.0f;
    fAlphaTestEndTime = 10.0f;
    fAlphaTestStartValue = 255.0f;
    fAlphaTestEndValue = 0.0f;
    fAlphaEdgeWidth = 0.0f;
    iAlphaEdgeColor = 0xFFFFFF;
    fExplosionSpeed = 0.0f;
    iParticleTextureCountX = 1;
    iParticleTextureCountY = 1;
    fAddOnFadeInTime = 1.0f;
    fAddOnFadeOutTime = 1.0f;
    fAddOnScaleStart = 1.0f;
    fAddOnScaleEnd = 1.0f;
    fAddOnScaleInTime = 1.0f;
    fAddOnScaleOutTime = 1.0f;
}

// Swaps every 32-bit field of a DATA chunk read from a big-endian file, in field order. The flags byte and its
// padding and the addon debris pointer (a form ID at load time, resolved later) are left alone.
void EffectShaderData::Endian() {
    ENDIAN(eTextureBlendModeSource, false);
    ENDIAN(eTextureBlendOperation, false);
    ENDIAN(eTextureZTestFunction, false);
    ENDIAN(iFillColor, false);
    ENDIAN(fFillAlphaFadeInTime, false);
    ENDIAN(fFillAlphaFullTime, false);
    ENDIAN(fFillAlphaFadeOutTime, false);
    ENDIAN(fFillAlphaPersistentPercent, false);
    ENDIAN(fFillAlphaPulseAmplitude, false);
    ENDIAN(fFillAlphaPulseFrequency, false);
    ENDIAN(fFillTextureUAnimSpeed, false);
    ENDIAN(fFillTextureVAnimSpeed, false);
    ENDIAN(fEdgeExponentValue, false);
    ENDIAN(iEdgeColor, false);
    ENDIAN(fEdgeAlphaFadeInTime, false);
    ENDIAN(fEdgeAlphaFullTime, false);
    ENDIAN(fEdgeAlphaFadeOutTime, false);
    ENDIAN(fEdgeAlphaPersistentPercent, false);
    ENDIAN(fEdgeAlphaPulseAmplitude, false);
    ENDIAN(fEdgeAlphaPulseFrequency, false);
    ENDIAN(fFillAlphaFullPercent, false);
    ENDIAN(fEdgeAlphaFullPercent, false);
    ENDIAN(eTextureBlendModeDest, false);
    ENDIAN(eParticleBlendModeSource, false);
    ENDIAN(eParticleBlendOperation, false);
    ENDIAN(eParticleZTestFunction, false);
    ENDIAN(eParticleBlendModeDest, false);
    ENDIAN(fParticleFadeInTime, false);
    ENDIAN(fParticleFullTime, false);
    ENDIAN(fParticleFadeOutTime, false);
    ENDIAN(fParticleFull, false);
    ENDIAN(fParticlePersistent, false);
    ENDIAN(fParticleLifetime, false);
    ENDIAN(fParticleLifeVar, false);
    ENDIAN(fParticleNormalSpeed, false);
    ENDIAN(fParticleNormalAcc, false);
    ENDIAN(ParticleVelocity.x, false);
    ENDIAN(ParticleVelocity.y, false);
    ENDIAN(ParticleVelocity.z, false);
    ENDIAN(ParticleAcceleration.x, false);
    ENDIAN(ParticleAcceleration.y, false);
    ENDIAN(ParticleAcceleration.z, false);
    ENDIAN(fParticleScale1, false);
    ENDIAN(fParticleScale2, false);
    ENDIAN(fParticleScale1Time, false);
    ENDIAN(fParticleScale2Time, false);
    ENDIAN(iParticleColor1, false);
    ENDIAN(iParticleColor2, false);
    ENDIAN(iParticleColor3, false);
    ENDIAN(fParticleColor1Alpha, false);
    ENDIAN(fParticleColor2Alpha, false);
    ENDIAN(fParticleColor3Alpha, false);
    ENDIAN(fParticleColor1Time, false);
    ENDIAN(fParticleColor2Time, false);
    ENDIAN(fParticleColor3Time, false);
    ENDIAN(fParticleNormalSpeedVar, false);
    ENDIAN(fParticleRotate, false);
    ENDIAN(fParticleRotateVar, false);
    ENDIAN(fParticleRotateSpeed, false);
    ENDIAN(fParticleRotateSpeedVar, false);
    ENDIAN(fAlphaTestStartTime, false);
    ENDIAN(fAlphaTestEndTime, false);
    ENDIAN(fAlphaTestStartValue, false);
    ENDIAN(fAlphaTestEndValue, false);
    ENDIAN(fAlphaEdgeWidth, false);
    ENDIAN(iAlphaEdgeColor, false);
    ENDIAN(fExplosionSpeed, false);
    ENDIAN(iParticleTextureCountX, false);
    ENDIAN(iParticleTextureCountY, false);
    ENDIAN(fAddOnFadeInTime, false);
    ENDIAN(fAddOnFadeOutTime, false);
    ENDIAN(fAddOnScaleStart, false);
    ENDIAN(fAddOnScaleEnd, false);
    ENDIAN(fAddOnScaleInTime, false);
    ENDIAN(fAddOnScaleOutTime, false);
}
