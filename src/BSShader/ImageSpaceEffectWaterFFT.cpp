#include "BSShader/ImageSpaceEffectWaterFFT.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceEffectWaterDisplacement.h"
#include "BSShader/ImageSpaceManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9TextureData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

#include <bit>
#include <cmath>

float randnormal(float mean, float stdev) {
	double n;
	int32_t i = 0;
	float v9 = static_cast<double>(rt::rand()) / 32767.0;
	double v3 = v9;
	if (!(v9 >= 0.5f)) {
		float v10 = 1.0 - v3;
		if (kNormalDistributionTable[0][1] < static_cast<double>(v10)) {
			double v6;
			do {
				v6 = kNormalDistributionTable[++i][1];
			} while (v6 < v10);
		}
		// The x of the entry before the one found; with no step (i == 0) the exe reads the dword before the table,
		// as this does.
		n = 1.0 - kNormalDistributionTable[i - 1][0];
	} else {
		if (kNormalDistributionTable[0][1] < v3) {
			double v4;
			do {
				v4 = kNormalDistributionTable[++i][1];
			} while (v4 < v3);
		}
		n = kNormalDistributionTable[i - 1][0];
	}
	float n_2 = n;
	return n_2 * stdev + mean;
}

ImageSpaceEffectWaterFFT::ImageSpaceEffectWaterFFT() : fTime(0.0f), fPassNumber(0.0f) {
	pTextures.SetSize(10u);
	fFrequency[3] = 5.0f;
	pInstance = this;
	fFrequency[2] = 90.0f;
	fFrequency[1] = 0.5f;
	fFrequency[0] = 1.0f;
	if (WaterHighResolution) {
		iGridResolution = 256;
		iLog2GridResolution = 8;
	} else {
		iGridResolution = 128;
		iLog2GridResolution = 7;
	}
	bNormals = true;
	fInvResolution = 1.0 / static_cast<double>(iGridResolution);
}

// Frees the twelve sub-effects' shader parameters; the texture members and the base destroy themselves. The slots
// are cleared in place, as the object is going.
ImageSpaceEffectWaterFFT::~ImageSpaceEffectWaterFFT() {
	for (uint32_t i = 0; i < 12; ++i) {
		ImageSpaceShaderParam* pParam = pEffectParamList.m_pBase[i].m_pObject;
		if (pParam) {
			delete pParam;
			pEffectParamList.m_pBase[i].m_pObject = nullptr;
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void ImageSpaceEffectWaterFFT::Render(NiScreenElements* apGeometry, NiDX9Renderer* apRenderer,
	ImageSpaceEffectParam* apParam, int32_t abEndFrame) {
	ImageSpaceEffect* peffect_2;
	ImageSpaceEffect* peffect_3;
	ImageSpaceEffect* peffect_4;
	ImageSpaceEffect* peffect_5;
	ImageSpaceEffect* peffect_6;
	ImageSpaceEffect* v26;
	ImageSpaceShaderParam* pparam_4;
	ImageSpaceTexture pdst;
	ImageSpaceTexture psrc;
	ImageSpaceTexture tmpptr;
	ImageSpaceTexture pFourierSpectrum;
	if (!bLoaded) {
		InitializeUtilityBuffers();
	}
	int32_t iLog2Resolution = ImageSpaceEffectWaterFFT::iLog2GridResolution;
	if (bParamsChanged) {
		bParamsChanged = UpdateParams(apParam);
	}
	int32_t v6 = 0;
	// The exe inlines each texture's constructor: three bytes, then the texture, filter and clamp dwords.
	pFourierSpectrum.bForceAniso = 0;
	pFourierSpectrum.bIsRenderedTexture = 0;
	pFourierSpectrum.bIsTextureBorrowed = 0;
	pFourierSpectrum.spTexture = nullptr;
	pFourierSpectrum.eFilterMode = 0;
	pFourierSpectrum.eClampMode = 0;
	psrc.bForceAniso = 0;
	psrc.bIsRenderedTexture = 0;
	psrc.bIsTextureBorrowed = 0;
	psrc.spTexture = nullptr;
	psrc.eFilterMode = 0;
	psrc.eClampMode = 0;
	pdst.bForceAniso = 0;
	pdst.bIsRenderedTexture = 0;
	pdst.bIsTextureBorrowed = 0;
	pdst.spTexture = nullptr;
	pdst.eFilterMode = 0;
	pdst.eClampMode = 0;
	pFourierSpectrum.BorrowRenderedTexture(BSTextureManager::TEX_TESWATER_FFT);
	pdst.BorrowRenderedTexture(BSTextureManager::TEX_TESWATER_FFT);
	psrc.BorrowRenderedTexture(BSTextureManager::TEX_TESWATER_FFT);
	tmpptr.bForceAniso = 0;
	tmpptr.bIsRenderedTexture = 0;
	tmpptr.bIsTextureBorrowed = 0;
	tmpptr.spTexture = nullptr;
	tmpptr.eFilterMode = 0;
	tmpptr.eClampMode = 0;
	ImageSpaceEffect* peffect = pEffectList.m_pBase->m_pObject;
	if (!peffect || peffect == reinterpret_cast<ImageSpaceEffect*>(static_cast<uintptr_t>(108))) {
		peffect_2 = 0;
	} else {
		peffect_2 = peffect;
	}
	ImageSpaceShaderParam* pparam = pEffectParamList.m_pBase->m_pObject;
	pparam->SetPixelConstant(0, fTime, 0.0f, 0.0f, 0.0f);
	peffect_2->SetTexture(0, &pFourierSpectrum, NiTexturingProperty::FILTER_NEAREST);
	peffect_2->SetTexture(1, &pHighAmplitude0Tex, NiTexturingProperty::FILTER_NEAREST);
	peffect_2->SetTexture(2, &pHighWTex, NiTexturingProperty::FILTER_NEAREST);
	peffect_2->Render(apGeometry, apRenderer, pparam, 1);
	ImageSpaceEffect* m_pObject = pEffectList.m_pBase[3].m_pObject;
	if (!m_pObject || m_pObject == reinterpret_cast<ImageSpaceEffect*>(static_cast<uintptr_t>(108))) {
		peffect_3 = 0;
	} else {
		peffect_3 = pEffectList.m_pBase[3].m_pObject;
	}
	ImageSpaceShaderParam* v12 = pEffectParamList.m_pBase[3].m_pObject;
	peffect_3->SetTexture(0, &pdst, NiTexturingProperty::FILTER_NEAREST);
	peffect_3->SetTexture(1, &pScrambleMap, NiTexturingProperty::FILTER_NEAREST);
	peffect_3->SetTexture(2, &pFourierSpectrum, NiTexturingProperty::FILTER_NEAREST);
	peffect_3->Render(apGeometry, apRenderer, v12, 1);
	tmpptr.operator=(psrc);
	psrc.operator=(pdst);
	pdst.operator=(tmpptr);
	ImageSpaceEffect* v13 = pEffectList.m_pBase[1].m_pObject;
	if (!v13 || v13 == reinterpret_cast<ImageSpaceEffect*>(static_cast<uintptr_t>(108))) {
		peffect_4 = 0;
	} else {
		peffect_4 = pEffectList.m_pBase[1].m_pObject;
	}
	ImageSpaceShaderParam* pparam_2 = pEffectParamList.m_pBase[1].m_pObject;
	int32_t apParams = 0;
	if (iLog2Resolution > 0) {
		do {
			float apParamsa = static_cast<double>(apParams) * 0.0625 + 0.03125;
			fPassNumber = apParamsa;
			pparam_2->SetPixelConstant(1, apParamsa, 4.0f, 0.0f, 0.0f);
			peffect_4->SetTexture(0, &pdst, NiTexturingProperty::FILTER_NEAREST);
			peffect_4->SetTexture(1, &pIFFTButterflyMap, NiTexturingProperty::FILTER_NEAREST);
			peffect_4->SetTexture(2, &psrc, NiTexturingProperty::FILTER_NEAREST);
			peffect_4->Render(apGeometry, apRenderer, pparam_2, 1);
			tmpptr.operator=(psrc);
			psrc.operator=(pdst);
			pdst.operator=(tmpptr);
			apParams = ++v6;
		} while (v6 < iLog2Resolution);
	}
	ImageSpaceEffect* v16 = pEffectList.m_pBase[4].m_pObject;
	if (!v16 || v16 == reinterpret_cast<ImageSpaceEffect*>(static_cast<uintptr_t>(108))) {
		peffect_5 = 0;
	} else {
		peffect_5 = pEffectList.m_pBase[4].m_pObject;
	}
	ImageSpaceShaderParam* v18 = pEffectParamList.m_pBase[4].m_pObject;
	peffect_5->SetTexture(0, &pdst, NiTexturingProperty::FILTER_NEAREST);
	peffect_5->SetTexture(1, &pScrambleMap, NiTexturingProperty::FILTER_NEAREST);
	peffect_5->SetTexture(2, &psrc, NiTexturingProperty::FILTER_NEAREST);
	NiDX9Renderer* v19 = apRenderer;
	peffect_5->Render(apGeometry, apRenderer, v18, 1);
	tmpptr.operator=(psrc);
	psrc.operator=(pdst);
	pdst.operator=(tmpptr);
	ImageSpaceEffect* v20 = pEffectList.m_pBase[2].m_pObject;
	if (!v20 || v20 == reinterpret_cast<ImageSpaceEffect*>(static_cast<uintptr_t>(108))) {
		peffect_6 = 0;
	} else {
		peffect_6 = pEffectList.m_pBase[2].m_pObject;
	}
	int32_t v22 = 0;
	ImageSpaceShaderParam* pparam_3 = pEffectParamList.m_pBase[2].m_pObject;
	int32_t v30 = 0;
	if (iLog2Resolution > 0) {
		int32_t v23 = iLog2Resolution - 1;
		do {
			float v31 = static_cast<double>(v30) * 0.0625 + 0.03125;
			fPassNumber = v31;
			pparam_3->SetPixelConstant(1, v31, 4.0f, 0.0f, 0.0f);
			if (v22 == v23) {
				peffect_6->SetTexture(0, pTextures.m_pBase[1], NiTexturingProperty::FILTER_NEAREST);
			} else {
				peffect_6->SetTexture(0, &pdst, NiTexturingProperty::FILTER_NEAREST);
			}
			peffect_6->SetTexture(1, &pIFFTButterflyMap, NiTexturingProperty::FILTER_NEAREST);
			peffect_6->SetTexture(2, &psrc, NiTexturingProperty::FILTER_NEAREST);
			peffect_6->Render(apGeometry, apRenderer, pparam_3, 1);
			if (v22 != v23) {
				tmpptr.operator=(psrc);
				psrc.operator=(pdst);
				pdst.operator=(tmpptr);
			}
			v30 = ++v22;
		} while (v22 < iLog2Resolution);
		v19 = apRenderer;
	}
	NiPointer<ImageSpaceEffect>* m_pBase = pEffectList.m_pBase;
	if (ImageSpaceEffectWaterFFT::bNormals) {
		ImageSpaceEffect* v25 = m_pBase[5].m_pObject;
		if (!v25 || v25 == reinterpret_cast<ImageSpaceEffect*>(static_cast<uintptr_t>(108))) {
			v26 = 0;
		} else {
			v26 = v25;
		}
		pparam_4 = pEffectParamList.m_pBase[5].m_pObject;
		pparam_4->SetPixelConstant(2, ImageSpaceEffectWaterFFT::fInvResolution, 4.0f, 0.0f, 0.0f);
	} else {
		ImageSpaceEffect* v28 = m_pBase[6].m_pObject;
		if (!v28 || v28 == reinterpret_cast<ImageSpaceEffect*>(static_cast<uintptr_t>(108))) {
			v26 = 0;
		} else {
			v26 = v28;
		}
		pparam_4 = pEffectParamList.m_pBase[6].m_pObject;
	}
	v26->SetTexture(0, *pTextures.m_pBase, NiTexturingProperty::FILTER_NEAREST);
	v26->SetTexture(1, pTextures.m_pBase[1], NiTexturingProperty::FILTER_NEAREST);
	v26->Render(apGeometry, v19, pparam_4, 1);
	if (pFourierSpectrum.GetTexture()) {
		pFourierSpectrum.ReturnRenderedTexture();
	}
	if (pdst.GetTexture()) {
		pdst.ReturnRenderedTexture();
	}
	if (psrc.GetTexture()) {
		psrc.ReturnRenderedTexture();
	}
}

// The exe's cleanup-only C++ EH frame is left out. The parameter list's SetAt (0xBA3AA0) stores the pointer without
// reference counting, and the exe inlines spParam's constructor and destructor, which touch no reference count.
void ImageSpaceEffectWaterFFT::Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) {
	ImageSpaceShaderParam* pparam;
	ImageSpaceShaderParam* pparam_2;
	ImageSpaceShaderParam* pparam_3;
	ImageSpaceShaderParam* pparam_4;
	ImageSpaceShaderParam* pparam_5;
	ImageSpaceShaderParam* pparam_6;
	ImageSpaceShaderParam* pparam_7;
	FNVD_INLINED_LOCAL(NiPointer<ImageSpaceShaderParam>, spParam, std::nullopt, std::nullopt);
	SetEffectCount(12);
	pTextures.SetSize(10u);
	ImageSpaceShaderParam* v4 =
		static_cast<ImageSpaceShaderParam*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam)));
	spParam.m_pObject = v4;
	if (v4) {
		pparam = ::new (v4) ImageSpaceShaderParam;
	} else {
		pparam = 0;
	}
	pparam->InitConstantMap(2, 3);
	NiTPrimitiveArray<NiPointer<ImageSpaceShaderParam>>* p_pEffectParamList = &pEffectParamList;
	pparam->bUseTexOffset[1] = 0;
	spParam.m_pObject = pparam;
	pEffectParamList.SetAt(0, spParam);
	SetEffect(0, apManager->pEffectList.m_pBase[93], pparam, 0);
	ImageSpaceShaderParam* v8 =
		static_cast<ImageSpaceShaderParam*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam)));
	spParam.m_pObject = v8;
	if (v8) {
		pparam_2 = ::new (v8) ImageSpaceShaderParam;
	} else {
		pparam_2 = 0;
	}
	pparam_2->InitConstantMap(2, 3);
	pparam_2->bUseTexOffset[1] = 0;
	spParam.m_pObject = pparam_2;
	p_pEffectParamList->SetAt(1u, spParam);
	SetEffect(1, apManager->pEffectList.m_pBase[94], pparam_2, 0);
	ImageSpaceShaderParam* v10 =
		static_cast<ImageSpaceShaderParam*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam)));
	spParam.m_pObject = v10;
	if (v10) {
		pparam_3 = ::new (v10) ImageSpaceShaderParam;
	} else {
		pparam_3 = 0;
	}
	pparam_3->InitConstantMap(2, 3);
	pparam_3->bUseTexOffset[1] = 0;
	spParam.m_pObject = pparam_3;
	p_pEffectParamList->SetAt(2u, spParam);
	SetEffect(2, apManager->pEffectList.m_pBase[95], pparam_3, 0);
	ImageSpaceShaderParam* v12 =
		static_cast<ImageSpaceShaderParam*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam)));
	spParam.m_pObject = v12;
	if (v12) {
		pparam_4 = ::new (v12) ImageSpaceShaderParam;
	} else {
		pparam_4 = 0;
	}
	pparam_4->InitConstantMap(2, 3);
	pparam_4->bUseTexOffset[1] = 0;
	spParam.m_pObject = pparam_4;
	p_pEffectParamList->SetAt(3u, spParam);
	SetEffect(3, apManager->pEffectList.m_pBase[96], pparam_4, 0);
	ImageSpaceShaderParam* v14 =
		static_cast<ImageSpaceShaderParam*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam)));
	spParam.m_pObject = v14;
	if (v14) {
		pparam_5 = ::new (v14) ImageSpaceShaderParam;
	} else {
		pparam_5 = 0;
	}
	pparam_5->InitConstantMap(2, 3);
	pparam_5->bUseTexOffset[1] = 0;
	spParam.m_pObject = pparam_5;
	p_pEffectParamList->SetAt(4u, spParam);
	SetEffect(4, apManager->pEffectList.m_pBase[97], pparam_5, 0);
	ImageSpaceShaderParam* v16 =
		static_cast<ImageSpaceShaderParam*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam)));
	spParam.m_pObject = v16;
	if (v16) {
		pparam_6 = ::new (v16) ImageSpaceShaderParam;
	} else {
		pparam_6 = 0;
	}
	pparam_6->InitConstantMap(2, 3);
	pparam_6->bUseTexOffset[1] = 0;
	spParam.m_pObject = pparam_6;
	p_pEffectParamList->SetAt(5u, spParam);
	SetEffect(5, apManager->pEffectList.m_pBase[98], pparam_6, 0);
	ImageSpaceShaderParam* v18 =
		static_cast<ImageSpaceShaderParam*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceShaderParam)));
	spParam.m_pObject = v18;
	if (v18) {
		pparam_7 = ::new (v18) ImageSpaceShaderParam;
	} else {
		pparam_7 = 0;
	}
	pparam_7->InitConstantMap(2, 3);
	pparam_7->bUseTexOffset[1] = 0;
	spParam.m_pObject = pparam_7;
	p_pEffectParamList->SetAt(6u, spParam);
	SetEffect(6, apManager->pEffectList.m_pBase[99], pparam_7, 0);
}

bool ImageSpaceEffectWaterFFT::UpdateParams(ImageSpaceEffectParam* pParam) {
	// The divisor is a double constant (fdiv qword).
	fTime = static_cast<float>(BSShaderManager::fTimerValues[0] / 25.0 * fFrequency[0]);
	return true;
}

void ImageSpaceEffectWaterFFT::FreeUpTextureMemory() {
	if (pIFFTButterflyMap.GetTexture()) {
		pIFFTButterflyMap.ReturnRenderedTexture();
	}
	if (pScrambleMap.GetTexture()) {
		pScrambleMap.ReturnRenderedTexture();
	}
	if (pHighWTex.GetTexture()) {
		pHighWTex.ReturnRenderedTexture();
	}
	if (pHighAmplitude0Tex.GetTexture()) {
		pHighAmplitude0Tex.ReturnRenderedTexture();
	}
	if (pFourierSpectrum.GetTexture()) {
		pFourierSpectrum.ReturnRenderedTexture();
	}
	bLoaded = false;
}

// Borrows the four utility textures the effect has not got yet, then builds the utility buffers (the exe tail-jumps
// into Do_InitializeUtilityBuffers).
void ImageSpaceEffectWaterFFT::InitializeUtilityBuffers() {
	if (!pScrambleMap.GetTexture()) {
		pScrambleMap.BorrowRenderedTexture(BSTextureManager::TEX_TESWATER_SCRAMBLEMAP);
	}
	if (!pIFFTButterflyMap.GetTexture()) {
		pIFFTButterflyMap.BorrowRenderedTexture(BSTextureManager::TEX_TESWATER_IFFT_BUTTERFLYMAP);
	}
	if (!pHighWTex.GetTexture()) {
		pHighWTex.BorrowRenderedTexture(BSTextureManager::TEX_TESWATER_HIGH_W);
	}
	if (!pHighAmplitude0Tex.GetTexture()) {
		pHighAmplitude0Tex.BorrowRenderedTexture(BSTextureManager::TEX_TESWATER_HIGH_AMPLITUDE);
	}
	Do_InitializeUtilityBuffers();
}

// Every float local is one the exe rounds with `fstp dword`; the constants are the exe's doubles (fmul/fdiv qword).
// Products of two floats are exact in double, as on the x87 stack.
float ImageSpaceEffectWaterFFT::Phillips(float kx, float ky) {
	float fWindAngle = static_cast<double>(fFrequency[2]) * 3.1415927410125732 / 180.0;
	float fAmplitude = static_cast<double>(fFrequency[1]) / 100000.0;
	float fL = static_cast<double>(fFrequency[3]) * fFrequency[3] / 9.8100004196167;
	float fSmallL = static_cast<double>(fL) / 75.0;
	float fK2 = static_cast<double>(ky) * ky + static_cast<double>(kx) * kx;
	float fCos = std::cos(fWindAngle);
	double dCosX = static_cast<double>(fCos) * kx;
	float fSin = std::sin(fWindAngle);
	float fDot = static_cast<double>(fSin) * ky + dCosX;
	if (fDot < 0.0) {
		fDot = 0.0f;
	}
	double dDot = fDot;
	double dK2 = fK2;
	float fExpArg = -1.0 / (static_cast<double>(fL) * fL * fK2);
	float fExp = std::exp(fExpArg);
	float fP = static_cast<double>(fExp) * (dDot * dDot) * fAmplitude / (dK2 * dK2 * fK2);
	float fDampArg = -static_cast<double>(fK2) * fSmallL * fSmallL;
	float fDamp = std::exp(fDampArg);
	if (0.0f > fDot) {
		fP = fP * 0.25;
	}
	float fResult = fDamp * fP;
	return fResult;
}

// Fills the scramble map with a bit-reversed ramp and the IFFT butterfly map with the twiddle factors, through the D3D
// textures of the effect's borrowed rendered textures. The exe's cleanup-only EH frame is left out, and it leaves the
// thread's TLS block pointer in eax. The allocations are the exe's array new: a size that overflows 32 bits becomes
// 0xFFFFFFFF (0xBA1B23, 0xBA1B46, 0xBA1CCF); the memset and memcpy sizes wrap.
TLSData* ImageSpaceEffectWaterFFT::FillFFTUtilityTextures(int32_t iResolution, int32_t iLog2Resolution) {
	TLSData* pTLS = GetThreadTLSData();
	MEM_CONTEXT eMemContext = pTLS->eMemContext;
	pTLS->eMemContext = MEM_CONTEXT_WATER;
	uint64_t ullDataSize = static_cast<uint64_t>(static_cast<uint32_t>(iResolution)) * sizeof(float);
	float* pfData =
		static_cast<float*>(MemoryManager_Singleton.Allocate(ullDataSize >> 32 ? 0xFFFFFFFF
																			   : static_cast<uint32_t>(ullDataSize)));
	uint64_t ullBlockSize = static_cast<uint64_t>(static_cast<uint32_t>(iResolution)) * sizeof(uint16_t);
	uint16_t* pusBlock = static_cast<uint16_t*>(MemoryManager_Singleton.Allocate(ullBlockSize >> 32
			? 0xFFFFFFFF
			: static_cast<uint32_t>(ullBlockSize)));
	std::memset(pfData, 0, static_cast<uint32_t>(iResolution) * sizeof(float));
	NiTexture* pTexture = pScrambleMap.GetTexture();
	IDirect3DTexture9* pD3DTexture =
		static_cast<IDirect3DTexture9*>(static_cast<NiDX9TextureData*>(pTexture->m_pkRendererData)->GetD3DTexture());
	// A ramp of (i + 0.5) / resolution, then the bit-reversal permutation the FFT reads it in.
	if (iResolution > 0) {
		float fResolution = static_cast<float>(iResolution);
		for (int32_t i = 0; i < iResolution; ++i) {
			float fRamp = static_cast<float>(i + 0.5);
			pfData[i] = fRamp / fResolution;
		}
	}
	int32_t i4 = 0;
	int32_t i10 = 0;
	int32_t i48 = 0;
	if (iResolution > 0) {
		int32_t j = iResolution >> 1;
		for (;;) {
			if (i10 > i4) {
				float fTemp = pfData[i4];
				pfData[i4] = pfData[i10];
				pfData[i10] = fTemp;
			}
			while (i4 >= j && j >= 2) {
				i4 -= j;
				j >>= 1;
			}
			i10 = i48 + 1;
			i4 += j;
			if (++i48 >= iResolution)
				break;
			j = iResolution >> 1;
		}
	}
	// _ftol2_sse converts each ramp value; only its low 16 bits are stored.
	for (int32_t k = 0; k < iResolution; ++k)
		pusBlock[k] = static_cast<uint16_t>(static_cast<int32_t>(pfData[k] * 65535.0));
	D3DLOCKED_RECT kLockedRect;
	pD3DTexture->LockRect(0, &kLockedRect, nullptr, 0);
	rt::memcpy(kLockedRect.pBits, pusBlock, 2 * iResolution);
	pD3DTexture->UnlockRect(0);
	MemoryManager_Singleton.Deallocate(pfData);
	MemoryManager_Singleton.Deallocate(pusBlock);
	NiTexture* pButterflyTexture = pIFFTButterflyMap.GetTexture();
	IDirect3DTexture9* pButterflyD3DTexture =
		static_cast<IDirect3DTexture9*>(static_cast<NiDX9TextureData*>(pButterflyTexture->m_pkRendererData)
				->GetD3DTexture());
	uint32_t uiDataSize = static_cast<uint32_t>(iResolution) << 8;
	// The exe sizes the allocation as (resolution << 6) floats, saturated; the memset and memcpy use the wrapped size.
	uint64_t ullButterflySize = static_cast<uint64_t>(static_cast<uint32_t>(iResolution) << 6) * sizeof(float);
	float* pfButterfly = static_cast<float*>(MemoryManager_Singleton.Allocate(ullButterflySize >> 32
			? 0xFFFFFFFF
			: static_cast<uint32_t>(ullButterflySize)));
	std::memset(pfButterfly, 0, uiDataSize);
	if (iLog2Resolution > 0) {
		int32_t i17 = 1;
		int32_t iBlock = 0;
		for (int32_t iCount = iLog2Resolution; iCount != 0; --iCount) {
			int32_t i19 = i17;
			int32_t i76 = 2 * i17;
			float f49 = static_cast<float>(-6.283185482025146 / (-2 * i17));
			float f61 = static_cast<float>(0.5 * f49);
			float f62 = std::sin(f61);
			float f67 = static_cast<float>(f62 * (-2.0 * f62));
			float f63 = std::sin(f49);
			int32_t i20 = 0;
			float f59 = 1.0f;
			float f52 = 0.0f;
			if (i19 > 0) {
				do {
					float f23 = f59;
					float f24 = f52;
					int32_t i25 = i20;
					int32_t i50 = i20;
					if (i20 < iResolution) {
						float f26 = f52;
						float fRowStep = static_cast<float>(iResolution);
						float* pRow0 = pfButterfly + 4 * (iBlock + i20) + 2;
						float* pRow1 = pfButterfly + 4 * (iBlock + i20 + i19) + 2;
						int32_t i36 = i50;
						for (;;) {
							int32_t i34 = i19 + i25;
							i25 += i76;
							// The exe divides i34 + 0.5 unrounded (0xBA1E0B) and rounds only the stores.
							double f35 = (i34 + 0.5) / fRowStep;
							pRow1[-2] = -f35;
							int32_t i37 = i36;
							i36 = i25;
							float f68 = (i37 + 0.5) / fRowStep;
							pRow1[-1] = f68;
							pRow0[-1] = f68;
							pRow0[-2] = f35;
							pRow1[0] = f23;
							pRow1[1] = f26;
							pRow1 += 4 * i76;
							pRow0[0] = f23;
							pRow0[1] = f26;
							pRow0 += 4 * i76;
							if (i25 >= iResolution)
								break;
						}
					}
					++i20;
					float f54 = f23;
					f59 = static_cast<float>(static_cast<double>(f23) * f67 - static_cast<double>(f24) * f63 + f23);
					f52 = static_cast<float>(static_cast<double>(f24) * f67 + static_cast<double>(f63) * f54 + f24);
				} while (i20 < i19);
			}
			iBlock += iResolution;
			i17 = i76;
		}
	}
	pButterflyD3DTexture->LockRect(0, &kLockedRect, nullptr, 0);
	rt::memcpy(kLockedRect.pBits, pfButterfly, uiDataSize);
	pButterflyD3DTexture->UnlockRect(0);
	MemoryManager_Singleton.Deallocate(pfButterfly);
	pTLS->eMemContext = eMemContext;
	return pTLS;
}

// Fills the high-amplitude texture from HighAmplitude0 mirrored into four quadrants. The first time through it also
// fills the high-W texture from HighW and clears the flag. Every float the exe stores with `fstp dword` is a float
// copy.
void ImageSpaceEffectWaterFFT::FillWaterUtilityTextures(bool* abFirstTimeThrough) {
	TLSData* pTLS = GetThreadTLSData();
	MEM_CONTEXT eMemContext = pTLS->eMemContext;
	pTLS->eMemContext = MEM_CONTEXT_WATER;
	if (*abFirstTimeThrough) {
		NiTexture* pTexture = pHighWTex.GetTexture();
		IDirect3DTexture9* pD3DTexture =
			static_cast<IDirect3DTexture9*>(static_cast<NiDX9TextureData*>(pTexture->m_pkRendererData)
					->GetD3DTexture());
		D3DLOCKED_RECT kLockedRect;
		pD3DTexture->LockRect(0, &kLockedRect, nullptr, 0);
		float* pfDest = static_cast<float*>(kLockedRect.pBits);
		for (int32_t i = 0; i < iGridResolution; ++i) {
			for (int32_t j = 0; j < iGridResolution; ++j)
				*pfDest++ = HighW[i][j];
		}
		pD3DTexture->UnlockRect(0);
		*abFirstTimeThrough = 0;
	}
	NiTexture* pAmplitudeTexture = pHighAmplitude0Tex.GetTexture();
	IDirect3DTexture9* pAmplitudeD3DTexture =
		static_cast<IDirect3DTexture9*>(static_cast<NiDX9TextureData*>(pAmplitudeTexture->m_pkRendererData)
				->GetD3DTexture());
	D3DLOCKED_RECT kLockedRect;
	pAmplitudeD3DTexture->LockRect(0, &kLockedRect, nullptr, 0);
	float* pfDest = static_cast<float*>(kLockedRect.pBits);
	for (int32_t i = 0; i < iGridResolution; ++i) {
		for (int32_t j = 0; j < iGridResolution; ++j) {
			*pfDest++ = HighAmplitude0[iGridResolution - i][iGridResolution - j];
			*pfDest++ = HighAmplitude0[iGridResolution - i][iGridResolution - j];
			*pfDest++ = HighAmplitude0[i][j];
			*pfDest++ = HighAmplitude0[i][j];
		}
	}
	pAmplitudeD3DTexture->UnlockRect(0);
	pTLS->eMemContext = eMemContext;
}

// The exe's cleanup-only C++ EH frame is left out. Each row's clear sizes are the exe's: sizeof(float) times the
// resolution, plus one byte for the first two.
void ImageSpaceEffectWaterFFT::FillWaterUtilityBuffers() {
	int32_t v5;
	int32_t v6;
	int32_t v11;
	bool bFirstTimeThrough;
	float ky_2;
	float fAmplitude;
	float ky;
	double v23;
	TLSData* v2 = GetThreadTLSData();
	int32_t j = v2->eMemContext;
	v2->eMemContext = MEM_CONTEXT_WATER;
	int32_t v3 = ImageSpaceEffectWaterFFT::iGridResolution;
	int32_t v4 = 0;
	for (bFirstTimeThrough = 1; v4 < v3 + 1; ++v4) {
		std::memset(HighW[v4], 0, sizeof(float) * v3 + 1);
		std::memset(HighAmplitude0[v4], 0, sizeof(float) * ImageSpaceEffectWaterFFT::iGridResolution + 1);
		v3 = ImageSpaceEffectWaterFFT::iGridResolution;
		if (v4 < ImageSpaceEffectWaterFFT::iGridResolution) {
			std::memset(HighW[v4], 0, sizeof(float) * ImageSpaceEffectWaterFFT::iGridResolution);
			v3 = ImageSpaceEffectWaterFFT::iGridResolution;
		}
	}
	v5 = 0;
	float kx = 0.0f;
	if (WaterHighResolution) {
		if (v3 >= 0) {
			while (1) {
				v6 = 0;
				ky_2 = 0.0f;
				kx = (static_cast<double>(std::bit_cast<int32_t>(kx)) -
						 static_cast<double>(ImageSpaceEffectWaterFFT::iGridResolution) * 0.5) *
					0.04908738657832146;
				if (v3 >= 0) {
					break;
				}
			LABEL_27:;
				++v5;
				kx = std::bit_cast<float>(v5);
				if (v5 > v3) {
					goto LABEL_38;
				}
			}
			v23 = kx * kx;
			while (1) {
				int32_t v7;
				ky_2 = (static_cast<double>(std::bit_cast<int32_t>(ky_2)) -
						   static_cast<double>(ImageSpaceEffectWaterFFT::iGridResolution) * 0.5) *
					0.04908738657832146;
				fAmplitude = ky_2 * ky_2 + v23;
				fAmplitude = std::sqrt((double)fAmplitude);
				fAmplitude = fAmplitude * 9.8100004196167;
				fAmplitude = std::sqrt((double)fAmplitude);
				HighW[v5][v6] = fAmplitude;
				if (v5 < 32 && v6 < 32) {
					goto LABEL_26;
				}
				v7 = ImageSpaceEffectWaterFFT::iGridResolution - 32;
				if (v5 <= ImageSpaceEffectWaterFFT::iGridResolution - 32 || v6 >= 32) {
					float* v8;
					double v9;
					if (v5 < 32 && v6 > v7) {
						v8 = HighAmplitude0[v5];
						v9 = v8[v6 - ImageSpaceEffectWaterFFT::iGridResolution + 64];
					LABEL_25:;
						v8[v6] = v9;
						goto LABEL_26;
					}
					if (v5 <= v7 || v6 <= v7) {
						double v10 = 0.0;
						if (kx == 0.0f && 0.0f == ky_2) {
							ky_2 = 0.0f;
						} else {
							fAmplitude = std::sqrt((double)Phillips(kx, ky_2));
							ky_2 = fAmplitude;
							v10 = 0.0;
						}
						fAmplitude = randnormal(v10, 1.0f);
						ky = static_cast<double>(rt::rand()) / 32767.0;
						ky = (ky + ky) * 3.1415927410125732;
						ky = std::sin((double)ky);
						v8 = HighAmplitude0[v5];
						ky = fAmplitude * ky_2 * ky;
						v9 = ky;
						goto LABEL_25;
					}
					HighAmplitude0[v5][v6] = (&HighAmplitude0[64 - ImageSpaceEffectWaterFFT::iGridResolution])[v5][v6 +
						(64 - ImageSpaceEffectWaterFFT::iGridResolution)];
				} else {
					HighAmplitude0[v5][v6] = HighAmplitude0[v5 - ImageSpaceEffectWaterFFT::iGridResolution + 64][v6];
				}
			LABEL_26:;
				v3 = ImageSpaceEffectWaterFFT::iGridResolution;
				++v6;
				ky_2 = std::bit_cast<float>(v6);
				if (v6 > ImageSpaceEffectWaterFFT::iGridResolution) {
					goto LABEL_27;
				}
			}
		}
	} else {
		if (v3 >= 0) {
			do {
				v11 = 0;
				ky_2 = 0.0f;
				kx = (static_cast<double>(std::bit_cast<int32_t>(kx)) -
						 static_cast<double>(ImageSpaceEffectWaterFFT::iGridResolution) * 0.5) *
					0.04908738657832146;
				if (v3 >= 0) {
					v23 = kx * kx;
					do {
						ky_2 = (static_cast<double>(std::bit_cast<int32_t>(ky_2)) -
								   static_cast<double>(ImageSpaceEffectWaterFFT::iGridResolution) * 0.5) *
							0.04908738657832146;
						ky = ky_2 * ky_2 + v23;
						ky = std::sqrt((double)ky);
						ky = ky * 9.8100004196167;
						ky = std::sqrt((double)ky);
						HighW[v5][v11] = ky;
						double v12 = 0.0;
						if (kx == 0.0f && 0.0f == ky_2) {
							ky_2 = 0.0f;
						} else {
							ky = std::sqrt((double)Phillips(kx, ky_2));
							ky_2 = ky;
							v12 = 0.0;
						}
						fAmplitude = randnormal(v12, 1.0f);
						ky = static_cast<double>(rt::rand()) / 32767.0;
						ky = (ky + ky) * 3.1415927410125732;
						ky = std::sin((double)ky);
						float* v13 = HighAmplitude0[v5];
						++v11;
						double v14 = fAmplitude * ky_2;
						ky_2 = std::bit_cast<float>(v11);
						ky = v14 * ky;
						v13[v11 - 1] = ky;
						v3 = ImageSpaceEffectWaterFFT::iGridResolution;
					} while (v11 <= ImageSpaceEffectWaterFFT::iGridResolution);
				}
				++v5;
				kx = std::bit_cast<float>(v5);
			} while (v5 <= v3);
		}
	}
LABEL_38:;
	FillWaterUtilityTextures(&bFirstTimeThrough);
	v2->eMemContext = static_cast<MEM_CONTEXT>(j);
}

// The buffers are (iGridResolution + 1) rows of iGridResolution + 1 floats, made with new[] (MemoryManager): its size
// is 0xFFFFFFFF when the product overflows. They live only while the utility textures are filled.
void ImageSpaceEffectWaterFFT::Do_InitializeUtilityBuffers() {
	uint64_t ullSize = static_cast<uint64_t>(static_cast<uint32_t>(iGridResolution + 1)) * sizeof(float*);
	HighW = static_cast<float**>(MemoryManager_Singleton.Allocate(ullSize >> 32 ? 0xFFFFFFFF
																				: static_cast<uint32_t>(ullSize)));
	ullSize = static_cast<uint64_t>(static_cast<uint32_t>(iGridResolution + 1)) * sizeof(float*);
	HighAmplitude0 =
		static_cast<float**>(MemoryManager_Singleton.Allocate(ullSize >> 32 ? 0xFFFFFFFF
																			: static_cast<uint32_t>(ullSize)));
	for (int32_t i = 0; i < iGridResolution + 1; ++i) {
		ullSize = static_cast<uint64_t>(static_cast<uint32_t>(iGridResolution + 1)) * sizeof(float);
		HighW[i] =
			static_cast<float*>(MemoryManager_Singleton.Allocate(ullSize >> 32 ? 0xFFFFFFFF
																			   : static_cast<uint32_t>(ullSize)));
		ullSize = static_cast<uint64_t>(static_cast<uint32_t>(iGridResolution + 1)) * sizeof(float);
		HighAmplitude0[i] =
			static_cast<float*>(MemoryManager_Singleton.Allocate(ullSize >> 32 ? 0xFFFFFFFF
																			   : static_cast<uint32_t>(ullSize)));
	}
	FillWaterUtilityBuffers();
	FillFFTUtilityTextures(iGridResolution, iLog2GridResolution);
	for (int32_t i = 0; i < iGridResolution + 1; ++i) {
		MemoryManager_Singleton.Deallocate(HighW[i]);
		MemoryManager_Singleton.Deallocate(HighAmplitude0[i]);
	}
	MemoryManager_Singleton.Deallocate(HighW);
	MemoryManager_Singleton.Deallocate(HighAmplitude0);
	HighW = nullptr;
	HighAmplitude0 = nullptr;
	bLoaded = true;
}

ImageSpaceEffectWaterFFT* ImageSpaceEffectWaterFFT::GetInstance() {
	return ImageSpaceEffectWaterFFT::pInstance;
}
