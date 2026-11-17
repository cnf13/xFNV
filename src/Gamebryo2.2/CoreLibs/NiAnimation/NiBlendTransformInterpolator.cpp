#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendTransformInterpolator.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendBoolInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiQuatTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <cfloat>
#include <cmath>

NiRTTI NiBlendTransformInterpolator::ms_RTTI("NiBlendTransformInterpolator", &NiBlendInterpolator::ms_RTTI);

// The start of the blended rotation sum. Nothing in the exe writes it, so it stays all zero.
static NiQuaternion kBlendRotateStart{};

NiBlendTransformInterpolator::NiBlendTransformInterpolator() : NiBlendInterpolator() {}

NiBlendTransformInterpolator::NiBlendTransformInterpolator(int32_t abManagerControlled, float afWeightThreshold,
	int32_t aucArraySize)
	: NiBlendInterpolator(abManagerControlled, afWeightThreshold, aucArraySize) {}

NiBlendTransformInterpolator::~NiBlendTransformInterpolator() {}

NiObject* NiBlendTransformInterpolator::CreateClone(NiCloningProcess& kCloning) {
	void* v3 = NiMemObject::operator new(sizeof(NiBlendTransformInterpolator));
	if (v3) {
		NiBlendInterpolator* v4 = ::new (v3) NiBlendTransformInterpolator;
		NiBlendInterpolator::CopyMembers(v4, kCloning);
		return v4;
	} else {
		NiBlendInterpolator::CopyMembers(nullptr, kCloning);
		return nullptr;
	}
}

// Files before 10.1.0.110 store a transform here that is no longer used; it is read and dropped.
void NiBlendTransformInterpolator::LoadBinary(NiStream& kStream) {
	NiBlendInterpolator::LoadBinary(kStream);
	if (kStream.GetFileVersion() < NiStream::GetVersion(10, 1, 0, 110)) {
		NiQuatTransform kUnusedValue;
		kUnusedValue.m_kTranslate = NiQuatTransform::INVALID_TRANSLATE;
		kUnusedValue.m_kRotate = NiQuatTransform::INVALID_ROTATE;
		kUnusedValue.m_fScale = -FLT_MAX;
		kUnusedValue.LoadBinary(kStream);
	}
}

void NiBlendTransformInterpolator::LinkObject(NiStream& kStream) {
	NiBlendInterpolator::LinkObject(kStream);
}

// Also the RegisterStreamables of the other blend interpolators but NiBlendAccumTransformInterpolator.
bool NiBlendTransformInterpolator::RegisterStreamables(NiStream& kStream) {
	return NiBlendInterpolator::RegisterStreamables(kStream);
}

// Also BSBlendTreadTransfInterpolator::SaveBinary.
void NiBlendTransformInterpolator::SaveBinary(NiStream& kStream) {
	NiBlendInterpolator::SaveBinary(kStream);
}

bool NiBlendTransformInterpolator::IsEqual(NiObject* pkObject) {
	return NiBlendInterpolator::IsEqual(pkObject);
}

void NiBlendTransformInterpolator::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiBlendInterpolator::GetViewerStrings(pkStrings);
	pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

bool NiBlendTransformInterpolator::Update(float fTime, NiObjectNET* pkInterpTarget, NiQuatTransform& kValue) {
	bool bReturnValue = false;
	if (m_ucInterpCount == 1) {
		bReturnValue = StoreSingleValue(fTime, pkInterpTarget, kValue);
	} else if (m_ucInterpCount > 0) {
		ComputeNormalizedWeights();
		bReturnValue = BlendValues(fTime, pkInterpTarget, kValue);
	}
	m_fLastTime = fTime;
	return bReturnValue;
}

// Also the InterpolatorIsCorrectType of NiBlendAccumTransformInterpolator and BSBlendTreadTransfInterpolator.
bool NiBlendTransformInterpolator::InterpolatorIsCorrectType(NiInterpolator* pkInterpolator) const {
	return pkInterpolator->IsTransformValueSupported();
}

void NiBlendTransformInterpolator::CopyMembers(NiBlendTransformInterpolator* pkDest, NiCloningProcess& kCloning) {
	NiBlendInterpolator::CopyMembers(pkDest, kCloning);
}

bool NiBlendTransformInterpolator::StoreSingleValue(float fTime, NiObjectNET* pkInterpTarget, NiQuatTransform& kValue) {
	if (GetSingleUpdateTime(fTime) && m_pkSingleInterpolator->Update(fTime, pkInterpTarget, kValue)) {
		if (kValue.IsRotateValid()) {
			NiQuaternion kRotate = kValue.m_kRotate;
			kRotate.Normalize();
			kValue.m_kRotate = kRotate;
		}
		return true;
	}
	kValue.m_kTranslate.x = -FLT_MAX;
	kValue.m_kRotate.m_fX = -FLT_MAX;
	kValue.m_fScale = -FLT_MAX;
	return false;
}

// Translation and scale are weighted averages over the interpolators that have them; the rotations are summed
// weighted, each after the first turned into the hemisphere of the sum, and the sum normalized.
// DEVIATION (kNVSE's BlendValuesFixFloatingPointError, its "spider hands fix"): the exe starts each channel's total
// weight at 1.0f and subtracts the weight of every interpolator that skips, fails or lacks the channel. The float
// subtractions leave a rounding error that is large next to a small remaining weight (and a nonzero residue where none
// should remain), so the weighted sum divided by it throws the bone far off. Here each channel's total is the sum, in
// double, of the weights that went into it, and a channel whose total is not above 1e-10 is left invalid.
bool NiBlendTransformInterpolator::BlendValues(float fTime, NiObjectNET* pkInterpTarget, NiQuatTransform& kValue) {
	constexpr double dEpsilon = 1e-10;
	double dTotalTransWeight = 0.0;
	double dTotalRotWeight = 0.0;
	double dTotalScaleWeight = 0.0;
	NiPoint3 kFinalTranslate = NiPoint3_ZERO;
	NiQuaternion kFinalRotate = kBlendRotateStart;
	float fFinalScale = 0.0f;
	bool bFirstRotation = true;
	for (uint8_t uc = 0; uc < m_ucArraySize; uc++) {
		InterpArrayItem& kItem = m_pkInterpArray[uc];
		if (!kItem.m_spInterpolator || !(kItem.m_fNormalizedWeight > 0.0f))
			continue;
		float fUpdateTime = fTime;
		if (!GetUpdateTimeForItem(fUpdateTime, kItem))
			continue;
		NiQuatTransform kTransform;
		kTransform.m_kTranslate = NiQuatTransform::INVALID_TRANSLATE;
		kTransform.m_kRotate = NiQuatTransform::INVALID_ROTATE;
		kTransform.m_fScale = -FLT_MAX;
		if (!kItem.m_spInterpolator->Update(fUpdateTime, pkInterpTarget, kTransform))
			continue;

		float fWeight = kItem.m_fNormalizedWeight;
		if (kTransform.IsTranslateValid()) {
			kFinalTranslate.x += kTransform.m_kTranslate.x * fWeight;
			kFinalTranslate.y += kTransform.m_kTranslate.y * fWeight;
			kFinalTranslate.z += kTransform.m_kTranslate.z * fWeight;
			dTotalTransWeight += fWeight;
		}

		if (kTransform.IsRotateValid()) {
			NiQuaternion kRotate = kTransform.m_kRotate;
			if (bFirstRotation) {
				bFirstRotation = false;
			} else {
				float fDot = kRotate.m_fX * kFinalRotate.m_fX + kRotate.m_fW * kFinalRotate.m_fW +
					kRotate.m_fY * kFinalRotate.m_fY + kRotate.m_fZ * kFinalRotate.m_fZ;
				if (fDot < 0.0f)
					kRotate = -kRotate;
			}
			kFinalRotate.m_fW += kRotate.m_fW * fWeight;
			kFinalRotate.m_fX += kRotate.m_fX * fWeight;
			kFinalRotate.m_fY += kRotate.m_fY * fWeight;
			kFinalRotate.m_fZ += kRotate.m_fZ * fWeight;
			dTotalRotWeight += fWeight;
		}

		if (kTransform.IsScaleValid()) {
			fFinalScale += kTransform.m_fScale * fWeight;
			dTotalScaleWeight += fWeight;
		}
	}

	kValue.m_kTranslate.x = -FLT_MAX;
	kValue.m_kRotate.m_fX = -FLT_MAX;
	kValue.m_fScale = -FLT_MAX;
	if (dTotalTransWeight > dEpsilon) {
		float fInvTotal = static_cast<float>(1.0 / dTotalTransWeight);
		kValue.m_kTranslate.x = kFinalTranslate.x * fInvTotal;
		kValue.m_kTranslate.y = kFinalTranslate.y * fInvTotal;
		kValue.m_kTranslate.z = kFinalTranslate.z * fInvTotal;
	}
	if (dTotalRotWeight > dEpsilon) {
		float fLengthSquared = kFinalRotate.m_fX * kFinalRotate.m_fX + kFinalRotate.m_fW * kFinalRotate.m_fW +
			kFinalRotate.m_fY * kFinalRotate.m_fY + kFinalRotate.m_fZ * kFinalRotate.m_fZ;
		float fInvLength = 1.0f / static_cast<float>(std::sqrt(fLengthSquared));
		kValue.m_kRotate.m_fW = kFinalRotate.m_fW * fInvLength;
		kValue.m_kRotate.m_fX = kFinalRotate.m_fX * fInvLength;
		kValue.m_kRotate.m_fY = kFinalRotate.m_fY * fInvLength;
		kValue.m_kRotate.m_fZ = kFinalRotate.m_fZ * fInvLength;
	}
	if (dTotalScaleWeight > dEpsilon)
		kValue.m_fScale = fFinalScale * static_cast<float>(1.0 / dTotalScaleWeight);
	return !kValue.IsTransformInvalid();
}

void NiBlendTransformInterpolator::SetAlwaysBlend(bool abTF) {
	bsAlwaysBlend = abTF;
}

NiObject* NiBlendTransformInterpolator::CreateObject() {
	void* v0 = NiMemObject::operator new(sizeof(NiBlendTransformInterpolator));
	if (!v0) {
		return 0;
	}
	return ::new (v0) NiBlendTransformInterpolator;
}
