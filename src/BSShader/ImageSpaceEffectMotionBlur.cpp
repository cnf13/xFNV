#include "BSShader/ImageSpaceEffectMotionBlur.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceEffectParam.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShaderParam.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderTargetGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"

// The game settings this file defines (PDB), with the IDs and values the exe's initializers pass, in its initialization order.
SettingT<INISettingCollection> bDoMotionBlur("bDoMotionBlur:Imagespace", true);

ImageSpaceEffectMotionBlur::ImageSpaceEffectMotionBlur() : Params(0.0f, 0.5f, 0.0f, 0.0f), VelocityBuffer() {
	pTextures.SetSize(3u);
}

// The first shader parameter is the effect's: it is deleted and its slot emptied.
ImageSpaceEffectMotionBlur::~ImageSpaceEffectMotionBlur() {
	if (pEffectParamList.m_pBase[0].m_pObject) {
		delete pEffectParamList.m_pBase[0].m_pObject;
		pEffectParamList.SetAt(0, nullptr);
	}
}

// One effect with new shader parameters (2 pixel constants, no offset on textures 1 and 2): the manager's motion
// blur shader. The exe's cleanup-only C++ EH frame is left out.
void ImageSpaceEffectMotionBlur::Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) {
	ImageSpaceShaderParam* pParam;
	SetEffectCount(1);
	void* pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam));
	if (pMemory) {
		pParam = ::new (pMemory) ImageSpaceShaderParam;
	} else {
		pParam = nullptr;
	}
	pParam->InitConstantMap(0, 2);
	pParam->bUseTexOffset[1] = false;
	pParam->bUseTexOffset[2] = false;
	pParam->SetPixelConstant(0, Params.r, 0.0f, 0.0f, 0.0f);
	// The exe passes the pointer in a stack slot as the array's element, its NiPointer structors inlined to nothing
	// (the parameter class is not a NiRefObject).
	FNVD_INLINED_LOCAL(NiPointer<ImageSpaceShaderParam>, spParam, std::nullopt, std::nullopt);
	spParam.m_pObject = pParam;
	pEffectParamList.SetAt(0, spParam);
	SetEffectInputs(0, 1, 2, NiTexturingProperty::FILTER_NEAREST, NiTexturingProperty::FILTER_BILERP);
	SetEffect(0, apManager->pEffectList.m_pBase[ImageSpaceManager::EFFECT_SHADER_MOTION_BLUR], pParam, 0);
}

void ImageSpaceEffectMotionBlur::Shutdown() {
	BSShaderManager::ReturnVelocityBuffers();
}

// Hands back the velocity buffers and clears texture slot 2.
void ImageSpaceEffectMotionBlur::ReturnTextures() {
	BSShaderManager::ReturnVelocityBuffers();
	pTextures.SetAt(2, nullptr);
}

bool ImageSpaceEffectMotionBlur::IsActive() {
	return (bool)((uint8_t)(((BSShaderManager::eShaderLevel > 4) && ::bDoMotionBlur.uValue.b) && (Params.r > 0.0)));
}

bool ImageSpaceEffectMotionBlur::UpdateParams(ImageSpaceEffectParam* apParam) {
	double r = 0.0;
	ImageSpaceShaderParam* m_pObject = pEffectParamList.m_pBase->m_pObject;
	if (ImageSpaceEffectMotionBlur::bInEditor || BSShaderManager::fTimerValues[3] > 0.0) {
		r = Params.r;
	}
	float v8 = r;
	NiRenderTargetGroup* v5 = BSShaderManager::pRenderer.m_pObject->GetDefaultRenderTargetGroup();
	double g = Params.g;
	// The exe converts the unsigned height as a signed fild plus the float 2^32 when negative; that fadd rounds at
	// the thread's precision control.
	int32_t iHeight = static_cast<int32_t>(v5->GetHeight(0));
	double dHeight = iHeight;
	if (iHeight < 0) {
		dHeight += 4294967296.0f;
	}
	float v9 = g / dHeight;
	m_pObject->SetPixelConstant(0, v8, v9, Params.b, 1.0);
	float v13 = -BSShaderManager::spCamera->m_kWorld.m_Rotate.m_pEntry[0][1];
	float v14 = -BSShaderManager::spCamera->m_kWorld.m_Rotate.m_pEntry[1][1];
	float v15 = -BSShaderManager::spCamera->m_kWorld.m_Rotate.m_pEntry[2][1];
	float v10 = v13 * BSShaderProperty::PreviousTransform::vecPrevCameraDir.x +
		BSShaderProperty::PreviousTransform::vecPrevCameraDir.y * v14 +
		BSShaderProperty::PreviousTransform::vecPrevCameraDir.z * v15;
	float v11 = BSShaderProperty::PreviousTransform::vecPrevCameraDir.z *
			BSShaderManager::spCamera->m_kWorld.m_Rotate.m_pEntry[2][2] +
		(BSShaderProperty::PreviousTransform::vecPrevCameraDir.x *
				BSShaderManager::spCamera->m_kWorld.m_Rotate.m_pEntry[0][2] +
			BSShaderProperty::PreviousTransform::vecPrevCameraDir.y *
				BSShaderManager::spCamera->m_kWorld.m_Rotate.m_pEntry[1][2]);
	m_pObject->SetPixelConstant(1, v11, v10, 0.0, 0.0);
	return 1;
}

void ImageSpaceEffectMotionBlur::SetFullScreenBlurConst(float afBlur) {
	bParamsChanged = (bool)1;
	Params.r = afBlur;
}
