#include "BSShader/ImageSpaceEffectCinematic.h"

#include "BSCore/MemoryManager.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceEffectParam.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShaderParam.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVector4.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiPoint4.h"

ImageSpaceEffectCinematic::ImageSpaceEffectCinematic()
  : bNightEye(false), pfNightEyeData(&pfDefaultNightEyeData.x), pfData(pfDefaultData), uiEnables(0) {}

// The first shader parameter is the effect's: it is deleted and its slot emptied.
ImageSpaceEffectCinematic::~ImageSpaceEffectCinematic() {
    if (pEffectParamList.m_pBase[0].m_pObject) {
        delete pEffectParamList.m_pBase[0].m_pObject;
        pEffectParamList.SetAt(0, nullptr);
    }
}

// One effect: the cinematic shader over input texture 1, with its own parameters (four pixel constants).
void ImageSpaceEffectCinematic::Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) {
    SetEffectCount(1);
    pTextures.SetSize(2);
    ImageSpaceShaderParam* pParam =
        static_cast<ImageSpaceShaderParam*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam)));
    if (pParam) {
        pParam = ::new (pParam) ImageSpaceShaderParam;
    }
    else {
        pParam = nullptr;
    }
    pParam->InitConstantMap(0, 4);
    pEffectParamList.SetAt(0, reinterpret_cast<const NiPointer<ImageSpaceShaderParam>&>(pParam));
    SetEffectInputs(0, 1, NiTexturingProperty::FILTER_NEAREST);
    SetEffect(0, apManager->pEffectList.m_pBase[ImageSpaceManager::EFFECT_CINEMATIC_SHADER], pParam, 0);
}

bool ImageSpaceEffectCinematic::IsActive() {
    return pfData && !BSShaderManager::bHDR;
}

bool ImageSpaceEffectCinematic::UpdateParams(ImageSpaceEffectParam* pParam) {
    float v12;
    NiVector4 v14;
    NiVector4 v15;
    NiVector4 v16;
    bool v2 = !bNightEye;
    float v3 = pfEnables[0][0];
    ImageSpaceShaderParam* m_pObject = pEffectParamList.m_pBase->m_pObject;
    v15.y = pfEnables[0][1];
    float v5 = pfEnables[0][3];
    v15.x = v3;
    float v6 = pfEnables[0][2];
    v15.w = v5;
    float* pfData = this->pfData;
    v15.z = v6;
    if (v2) {
        v12 = pfData[3];
        v14.x = pfData[4];
        v14.y = pfData[5];
        v14.z = pfData[6];
        v14.w = pfData[7];
    }
    else {
        float* pfNightEyeData = this->pfNightEyeData;
        float v13 = pfData[3];
        double v9 = pfNightEyeData[3];
        if (!(v13 < v9)) {
            v9 = v13;
        }
        v12 = v9;
        v14.x = *pfNightEyeData;
        v14.y = pfNightEyeData[1];
        v14.z = pfNightEyeData[2];
        v14.w = 1.0;
        v15.z = 1.0;
        v15.w = 1.0;
    }
    float* v10 = this->pfData;
    v16.x = v10[8];
    v16.y = v10[9];
    v16.z = v10[10];
    v16.w = v10[11];
    m_pObject->SetPixelConstant(0, *v10, v10[1], v10[2], v12);
    m_pObject->SetPixelConstants(1, &v14);
    m_pObject->SetPixelConstants(2, &v16);
    m_pObject->SetPixelConstants(3, &v15);
    return 0;
}

// Night eye: the brightness is the larger of the two (pfData's when unordered), the night-eye colour replaces
// pfData's and the last three enables are forced to 1.
float* ImageSpaceEffectCinematic::GetParams() {
    float fBright;
    float fRed;
    float fGreen;
    float fBlue;
    float fAlpha;
    float fEnable0 = pfEnables[0][0];
    float fEnable1 = pfEnables[0][1];
    float fEnable3 = pfEnables[0][3];
    float* pfData = this->pfData;
    float fEnable2 = pfEnables[0][2];
    if (bNightEye) {
        float* pfNightEyeData = this->pfNightEyeData;
        float fDataBright = pfData[3];
        float fNightBright = pfNightEyeData[3];
        if (!(fDataBright < fNightBright)) {
            fNightBright = fDataBright;
        }
        fBright = fNightBright;
        fRed = *pfNightEyeData;
        fGreen = pfNightEyeData[1];
        fBlue = pfNightEyeData[2];
        fAlpha = 1.0f;
        fEnable2 = 1.0f;
        fEnable3 = 1.0f;
    }
    else {
        fBright = pfData[3];
        fRed = pfData[4];
        fGreen = pfData[5];
        fBlue = pfData[6];
        fAlpha = pfData[7];
    }
    float v14 = pfData[8];
    float v15 = pfData[9];
    float v16 = pfData[10];
    float v17 = pfData[11];
    pfParams[0] = *pfData;
    pfParams[1] = pfData[1];
    pfParams[2] = pfData[2];
    pfParams[3] = fBright;
    pfParams[4] = fRed;
    pfParams[5] = fGreen;
    pfParams[6] = fBlue;
    pfParams[7] = fAlpha;
    pfParams[8] = fEnable0;
    pfParams[9] = fEnable1;
    pfParams[10] = fEnable2;
    pfParams[11] = fEnable3;
    pfParams[12] = v14;
    pfParams[13] = v15;
    pfParams[14] = v16;
    pfParams[15] = v17;
    return pfParams;
}

// Each of the low four bits of auiEnables turns one of the first four enables on.
void ImageSpaceEffectCinematic::SetParams(float* apfData, uint32_t auiEnables) {
    bParamsChanged = true;
    pfEnables[0][0] = 0.0f;
    pfEnables[0][1] = 0.0f;
    pfEnables[0][2] = 0.0f;
    pfEnables[0][3] = 0.0f;
    pfEnables[1][0] = 0.0f;
    pfEnables[1][1] = 0.0f;
    pfEnables[1][2] = 0.0f;
    pfEnables[1][3] = 0.0f;
    pfData = apfData;
    uiEnables = auiEnables;
    pfEnables[0][0] = (auiEnables & 1) ? 1.0f : 0.0f;
    pfEnables[0][1] = (auiEnables & 2) ? 1.0f : 0.0f;
    pfEnables[0][2] = (auiEnables & 4) ? 1.0f : 0.0f;
    pfEnables[0][3] = (auiEnables & 8) ? 1.0f : 0.0f;
}

void ImageSpaceEffectCinematic::SetNightEyeParams(float* apfData) {
    bParamsChanged = (bool)((uint8_t)bNightEye);
    pfNightEyeData = apfData;
}

void ImageSpaceEffectCinematic::SetNightEye(bool abNightEye) {
    bool bChanged = abNightEye != bNightEye;
    bNightEye = abNightEye;
    if (bChanged) {
        bParamsChanged = true;
    }
}
