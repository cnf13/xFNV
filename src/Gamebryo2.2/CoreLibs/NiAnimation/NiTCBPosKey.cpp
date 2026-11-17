#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTCBPosKey.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBezRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/string.h"

#include <cmath>
#include <stdint.h>

void NiTCBPosKey::CalculateDVals(const NiPoint3& ptSub1, const NiPoint3& ptPlus1, float fPreLen, float fNextLen) {
	NiPoint3 v40;
	NiPoint3 v41;
	float v16 = m_Pos.x - ptSub1.x;
	float v17 = m_Pos.y - ptSub1.y;
	float v18 = m_Pos.z - ptSub1.z;
	float v19 = ptPlus1.x - m_Pos.x;
	float v21 = ptPlus1.y - m_Pos.y;
	float v23 = ptPlus1.z - m_Pos.z;
	float ptPlus1a = m_fBias + 1.0;
	float v13 = 1.0 - m_fBias;
	float v11 = (1.0 - m_fTension) * 0.5;
	float ptSub1a = m_fContinuity + 1.0;
	double v5 = v11;
	float v12 = ptSub1a * v11;
	float ptSub1b = 1.0 - m_fContinuity;
	float ptSub1c = v5 * ptSub1b;
	double v6 = v13;
	float v14 = v12 * v13;
	double v7 = v19;
	float v25 = v19 * v14;
	double v8 = v21;
	float v27 = v21 * v14;
	double v9 = v23;
	float v29 = v14 * v23;
	float v15 = ptSub1c * ptPlus1a;
	float v20 = v15 * v16;
	float v22 = v17 * v15;
	float v24 = v15 * v18;
	v41.x = v20 + v25;
	v41.y = v22 + v27;
	v41.z = v24 + v29;
	m_DS = v41;
	float ptSub1d = v6 * ptSub1c;
	float v26 = v7 * ptSub1d;
	float v28 = v8 * ptSub1d;
	float v30 = ptSub1d * v9;
	float ptSub1e = v12 * ptPlus1a;
	v41.x = v16 * ptSub1e;
	v41.y = v17 * ptSub1e;
	v41.z = v18 * ptSub1e;
	v40.x = v41.x + v26;
	v40.y = v41.y + v28;
	v40.z = v41.z + v30;
	m_DD = v40;
	float ptSub1f = 2.0 / (fPreLen + fNextLen);
	double v10 = ptSub1f;
	float ptSub1g = fPreLen * ptSub1f;
	m_DS.x = m_DS.x * ptSub1g;
	m_DS.y = m_DS.y * ptSub1g;
	m_DS.z = ptSub1g * m_DS.z;
	float ptSub1h = v10 * fNextLen;
	m_DD.x = m_DD.x * ptSub1h;
	m_DD.y = m_DD.y * ptSub1h;
	m_DD.z = ptSub1h * m_DD.z;
}

void NiTCBPosKey::LoadBinary(NiStream& stream) {
	uint32_t v10;
	NiStream* v2 = &stream;
	NiPosKey::LoadBinary(stream);
	NiBinaryStream* m_pkIstr = v2->Istr();
	uint32_t (*m_pfnRead)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = m_pkIstr->m_pfnRead;
	v10 = 4;
	m_pkIstr->m_uiAbsoluteCurrentPos += m_pfnRead(m_pkIstr, &m_fTension, 4, &v10, 1);
	NiBinaryStream* v6 = v2->Istr();
	uint32_t (*v7)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v6->m_pfnRead;
	v10 = 4;
	v6->m_uiAbsoluteCurrentPos += v7(v6, &m_fContinuity, 4, &v10, 1);
	NiBinaryStream* v8 = v2->Istr();
	uint32_t (*v9)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v8->m_pfnRead;
	v10 = 4;
	v8->m_uiAbsoluteCurrentPos += v9(v8, &m_fBias, 4, &v10, 1);
}

void NiTCBPosKey::Copy(NiAnimationKey* pkNewKey, const NiAnimationKey* pkOrigKey) {
	NiTCBPosKey* pkNewTCB = static_cast<NiTCBPosKey*>(pkNewKey);
	const NiTCBPosKey* pkOrigTCB = static_cast<const NiTCBPosKey*>(pkOrigKey);

	pkNewTCB->m_fTime = pkOrigTCB->m_fTime;
	pkNewTCB->m_Pos = pkOrigTCB->m_Pos;
	pkNewTCB->m_fTension = pkOrigTCB->m_fTension;
	pkNewTCB->m_fContinuity = pkOrigTCB->m_fContinuity;
	pkNewTCB->m_fBias = pkOrigTCB->m_fBias;
	pkNewTCB->m_DS = pkOrigTCB->m_DS;
	pkNewTCB->m_DD = pkOrigTCB->m_DD;
	pkNewTCB->m_A = pkOrigTCB->m_A;
	pkNewTCB->m_B = pkOrigTCB->m_B;
}

// NiPosKey::Equal (src's NiPosKey_Equal), then tension, continuity and bias, then the exe calls NiPoint3::operator== for
// each of m_DS, m_DD, m_A and m_B.
bool NiTCBPosKey::Equal(const NiAnimationKey& key0, const NiAnimationKey& key1) {
	if (!NiPosKey_Equal(key0, key1)) {
		return false;
	}
	const NiTCBPosKey& tcb0 = static_cast<const NiTCBPosKey&>(key0);
	const NiTCBPosKey& tcb1 = static_cast<const NiTCBPosKey&>(key1);
	return tcb0.m_fTension == tcb1.m_fTension && tcb0.m_fContinuity == tcb1.m_fContinuity &&
		tcb0.m_fBias == tcb1.m_fBias && tcb0.m_DS == tcb1.m_DS && tcb0.m_DD == tcb1.m_DD && tcb0.m_A == tcb1.m_A &&
		tcb0.m_B == tcb1.m_B;
}

void NiTCBPosKey::SaveToStream(NiStream& stream, NiAnimationKey* pkKeys, uint32_t uiNumKeys) {
	NiTCBPosKey* pkTCBPosKeys = static_cast<NiTCBPosKey*>(pkKeys);

	for (uint32_t ui = 0; ui < uiNumKeys; ui++)
		SaveBinary(stream, &pkTCBPosKeys[ui]);
}

void NiTCBPosKey::SaveBinary(NiStream& stream, NiAnimationKey* pkKey) {
	uint32_t v9;
	NiTCBPosKey* pkTCBKey = static_cast<NiTCBPosKey*>(pkKey);
	NiPosKey::SaveBinary(stream, pkKey);
	NiBinaryStream* m_pkOstr = (&stream)->Ostr();
	NiBinaryStream::NIBINARYSTREAMWRITEFN m_pfnWrite = m_pkOstr->m_pfnWrite;
	v9 = 4;
	m_pkOstr->m_uiAbsoluteCurrentPos += m_pfnWrite(m_pkOstr, &pkTCBKey->m_fTension, 4, &v9, 1);
	NiBinaryStream* v5 = (&stream)->Ostr();
	NiBinaryStream::NIBINARYSTREAMWRITEFN v6 = v5->m_pfnWrite;
	v9 = 4;
	v5->m_uiAbsoluteCurrentPos += v6(v5, &pkTCBKey->m_fContinuity, 4, &v9, 1);
	NiBinaryStream* v7 = (&stream)->Ostr();
	NiBinaryStream::NIBINARYSTREAMWRITEFN v8 = v7->m_pfnWrite;
	v9 = 4;
	v7->m_uiAbsoluteCurrentPos += v8(v7, &pkTCBKey->m_fBias, 4, &v9, 1);
}

// The exe inlines d1.Cross(d2).Length() and d1.SqrLength(): the cross product's components, its squared length, the
// square root (__CIsqrt) and the quotient are each stored as floats.
float NiTCBPosKey::ComputeMaxCurvature(NiAnimationKey* pkKeys, uint32_t uiNumKeys) {
	NiTCBPosKey* pkTCBPosKeys = static_cast<NiTCBPosKey*>(pkKeys);

	float fMaxCurvature = 0.0f;

	for (uint32_t uiI = 0; uiI < uiNumKeys - 1; uiI++) {
		NiTCBPosKey* pTCB0 = &pkTCBPosKeys[uiI];
		NiTCBPosKey* pTCB1 = &pkTCBPosKeys[uiI + 1];

		for (int32_t iJ = 0; iJ <= ms_iSampleSize; iJ++) {
			// EXE-INLINED none: NiPosKey::GetMaxCurvatureSampleDelta
			float fTime = iJ * ms_fSampleDelta;
			NiPoint3 d1;
			// The exe realigns esp to 8 and keeps d2 on an 8-byte boundary (InterpolateD2's pResult, [esp+0x40]).
			alignas(8) NiPoint3 d2;
			InterpolateD1(fTime, pTCB0, pTCB1, &d1);
			InterpolateD2(fTime, pTCB0, pTCB1, &d2);
			// EXE-INLINED 0x4B3800
			NiPoint3 kCross;
			kCross.x = d1.y * d2.z - d1.z * d2.y;
			kCross.y = d1.z * d2.x - d1.x * d2.z;
			kCross.z = d1.x * d2.y - d1.y * d2.x;
			// EXE-INLINED 0x457990
			float fCrossSqrLength = kCross.x * kCross.x + kCross.y * kCross.y + kCross.z * kCross.z;
			float fCrossLength = std::sqrt(fCrossSqrLength);
			// EXE-INLINED 0x4A7290
			float fSqrLength = d1.x * d1.x + d1.y * d1.y + d1.z * d1.z;
			float fCurvature = fCrossLength / fSqrLength;
			if (fCurvature > fMaxCurvature)
				fMaxCurvature = fCurvature;
		}
	}

	return fMaxCurvature;
}

void NiTCBPosKey::InterpolateD1(float fTime, const NiAnimationKey* pKey0, const NiAnimationKey* pKey1, void* pResult) {
	double v4 = fTime;
	float fTimea = 3.0 * fTime;
	float v9 = pKey0[16].GetTime() * fTimea;
	float v11 = pKey0[17].GetTime() * fTimea;
	float v13 = fTimea * pKey0[18].GetTime();
	float v6 = pKey0[13].GetTime() * 2.0;
	float v7 = pKey0[14].GetTime() * 2.0;
	float v8 = 2.0 * pKey0[15].GetTime();
	float v15 = v6 + v9;
	float v17 = v7 + v11;
	float v19 = v8 + v13;
	float v10 = v15 * v4;
	float v12 = v17 * v4;
	float v14 = v4 * v19;
	float v16 = pKey0[10].GetTime() + v10;
	float v18 = pKey0[11].GetTime() + v12;
	double v5 = pKey0[12].GetTime() + v14;
	*static_cast<float*>(pResult) = v16;
	*(static_cast<float*>(pResult) + 1) = v18;
	float v20 = v5;
	*(static_cast<float*>(pResult) + 2) = v20;
}

void NiTCBPosKey::InterpolateD2(float fTime, const NiAnimationKey* pKey0, const NiAnimationKey* pKey1, void* pResult) {
	float fTimea = fTime * 6.0;
	float v7 = pKey0[16].GetTime() * fTimea;
	float v8 = pKey0[17].GetTime() * fTimea;
	float v9 = fTimea * pKey0[18].GetTime();
	float v4 = pKey0[13].GetTime() * 2.0;
	float v5 = pKey0[14].GetTime() * 2.0;
	float v6 = 2.0 * pKey0[15].GetTime();
	float v10 = v4 + v7;
	*static_cast<float*>(pResult) = v10;
	float v11 = v8 + v5;
	*(static_cast<float*>(pResult) + 1) = v11;
	float v12 = v6 + v9;
	*(static_cast<float*>(pResult) + 2) = v12;
}

// EXE-INLINED none: NiNew NiTCBPosKey[uiNumKeys + 1] (the overflow-clamped size, the count cookie and
// MemoryManager::Allocate; the keys have no constructor to run) and NiDelete[] of the old array (Deallocate of its
// cookie). The block is raw memory to the exe: a uint32_t count cookie, then the keys. NiMemcpy is the exe's memcpy.
bool NiTCBPosKey::Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys) {
	NiTCBPosKey* pkTCBPosKeys = static_cast<NiTCBPosKey*>(pkKeys);

	uint32_t uiInsertAt;
	if (!OkayToInsert(fTime, pkTCBPosKeys, uiNumKeys, uiInsertAt, sizeof(NiTCBPosKey)))
		return false;

	uint32_t uiNewNumKeys = uiNumKeys + 1;
	uint64_t ullBytes = static_cast<uint64_t>(uiNewNumKeys) * sizeof(NiTCBPosKey);
	uint32_t uiBytes = (ullBytes >> 32) != 0 ? 0xFFFFFFFFu : static_cast<uint32_t>(ullBytes);
	uint32_t uiAllocSize = uiBytes + sizeof(size_t) < uiBytes ? 0xFFFFFFFFu : uiBytes + sizeof(size_t);
	size_t* puiCookie = static_cast<size_t*>(MemoryManager_Singleton.Allocate(uiAllocSize));
	NiTCBPosKey* pkNewKeys;
	if (puiCookie) {
		*puiCookie = uiNewNumKeys;
		pkNewKeys = reinterpret_cast<NiTCBPosKey*>(puiCookie + 1);
	} else {
		pkNewKeys = nullptr;
	}
	rt::memcpy(pkNewKeys, pkTCBPosKeys, static_cast<uint32_t>(uiInsertAt * sizeof(NiTCBPosKey)));
	if (uiNumKeys > uiInsertAt) {
		rt::memcpy(&pkNewKeys[uiInsertAt + 1], &pkTCBPosKeys[uiInsertAt],
			static_cast<uint32_t>((uiNumKeys - uiInsertAt) * sizeof(NiTCBPosKey)));
	}

	NiPoint3 kPt = NiPosKey::GenInterpDefault(fTime, pkTCBPosKeys, TCBKEY, uiNumKeys, sizeof(NiTCBPosKey));

	NiTCBPosKey* pkNewKey = &pkNewKeys[uiInsertAt];
	pkNewKey->m_fTime = fTime;
	pkNewKey->m_Pos = kPt;
	pkNewKey->m_fTension = 0.0f;
	pkNewKey->m_fContinuity = 0.0f;
	pkNewKey->m_fBias = 0.0f;

	uiNumKeys++;
	if (pkTCBPosKeys)
		MemoryManager_Singleton.Deallocate(fnvd::arrayBlock(pkTCBPosKeys));
	pkKeys = pkNewKeys;
	FillDerivedVals(pkNewKeys, uiNumKeys, sizeof(NiTCBPosKey));
	return true;
}

NiAnimationKey* NiTCBPosKey::CreateFromStream(NiStream& stream, uint32_t uiNumKeys) {
	// EXE-INLINED none: NiNew NiTCBPosKey[uiNumKeys] (the overflow-clamped size, the count cookie and
	// MemoryManager::Allocate; the keys have no constructor to run). The block is raw memory to the exe: a uint32_t
	// count cookie, then the keys; the cast below steps over that cookie.
	uint64_t ullBytes = static_cast<uint64_t>(uiNumKeys) * sizeof(NiTCBPosKey);
	uint32_t uiBytes = (ullBytes >> 32) != 0 ? 0xFFFFFFFFu : static_cast<uint32_t>(ullBytes);
	uint32_t uiAllocSize = uiBytes + sizeof(size_t) < uiBytes ? 0xFFFFFFFFu : uiBytes + sizeof(size_t);
	size_t* puiCookie = static_cast<size_t*>(MemoryManager_Singleton.Allocate(uiAllocSize));
	NiTCBPosKey* pkTCBPosKeys;
	if (puiCookie) {
		*puiCookie = uiNumKeys;
		pkTCBPosKeys = reinterpret_cast<NiTCBPosKey*>(puiCookie + 1);
	} else {
		pkTCBPosKeys = nullptr;
	}
	for (uint32_t ui = 0; ui < uiNumKeys; ui++) {
		pkTCBPosKeys[ui].LoadBinary(stream);
	}
	return pkTCBPosKeys;
}

// The exe stores its own functions' addresses (Equal, Interpolate and FillDerivedVals are still src's sub_
// functions), as the engine's Set*Function calls do inline.
void NiTCBPosKey::RegisterSupportedFunctions(KeyContent eContent, KeyType eType) {
	uint32_t uiIndex = static_cast<uint32_t>(eContent) * NUMKEYTYPES + eType;
	ms_curves[uiIndex] = &NiTCBPosKey::ComputeMaxCurvature;
	ms_interps[uiIndex] = &NiTCBPosKey::Interpolate;
	ms_interpd1s[uiIndex] = &NiTCBPosKey::InterpolateD1;
	ms_interpd2s[uiIndex] = &NiTCBPosKey::InterpolateD2;
	ms_equals[uiIndex] = &NiTCBPosKey::Equal;
	ms_fillderived[uiIndex] = &NiTCBPosKey::FillDerivedVals;
	ms_inserts[uiIndex] = &NiTCBPosKey::Insert;
}

// The exe inlines Gamebryo's NiPoint3 arithmetic (2.0f * m_Pos - ..., the coefficient sums) and stores every
// component of each step as a float, as the float locals below do.
void NiTCBPosKey::FillDerivedVals(NiAnimationKey* pkKeys, uint32_t uiNumKeys, uint8_t ucSize) {
	NiTCBPosKey* pkTCBPosKeys = static_cast<NiTCBPosKey*>(pkKeys);
	uint32_t uiNumKeysM1 = uiNumKeys - 1;
	NiPoint3 kPt;

	if (uiNumKeys >= 2) {
		// The first key's tangents, with its mirrored neighbour 2 * P0 - P1.
		// EXE-INLINED 0x45BB20
		float fX2 = pkTCBPosKeys[0].m_Pos.x * 2.0f;
		float fY2 = pkTCBPosKeys[0].m_Pos.y * 2.0f;
		float fZ2 = 2.0f * pkTCBPosKeys[0].m_Pos.z;
		// EXE-INLINED 0x439EF0
		kPt.x = fX2 - pkTCBPosKeys[1].m_Pos.x;
		kPt.y = fY2 - pkTCBPosKeys[1].m_Pos.y;
		kPt.z = fZ2 - pkTCBPosKeys[1].m_Pos.z;
		pkTCBPosKeys[0].CalculateDVals(kPt, pkTCBPosKeys[1].m_Pos, 1.0f, 1.0f);

		for (uint32_t i = 1; i < uiNumKeysM1; i++) {
			NiTCBPosKey* pkTCBM = &pkTCBPosKeys[i - 1];
			NiTCBPosKey* pkTCBZ = &pkTCBPosKeys[i];
			NiTCBPosKey* pkTCBP = &pkTCBPosKeys[i + 1];
			pkTCBZ->CalculateDVals(pkTCBM->m_Pos, pkTCBP->m_Pos, pkTCBZ->m_fTime - pkTCBM->m_fTime,
				pkTCBP->m_fTime - pkTCBZ->m_fTime);
		}

		// The last key's tangents, with its mirrored neighbour 2 * Pn-1 - Pn-2.
		NiTCBPosKey* pkLast = &pkTCBPosKeys[uiNumKeysM1];
		NiTCBPosKey* pkPrev = &pkTCBPosKeys[uiNumKeys - 2];
		// EXE-INLINED 0x45BB20
		fX2 = pkLast->m_Pos.x * 2.0f;
		fY2 = pkLast->m_Pos.y * 2.0f;
		fZ2 = 2.0f * pkLast->m_Pos.z;
		// EXE-INLINED 0x439EF0
		kPt.x = fX2 - pkPrev->m_Pos.x;
		kPt.y = fY2 - pkPrev->m_Pos.y;
		kPt.z = fZ2 - pkPrev->m_Pos.z;
		pkLast->CalculateDVals(pkPrev->m_Pos, kPt, 1.0f, 1.0f);
	}

	for (uint32_t i = 0; i < uiNumKeysM1; i++) {
		NiTCBPosKey* pTCB0 = &pkTCBPosKeys[i];
		NiTCBPosKey* pTCB1 = &pkTCBPosKeys[i + 1];

		// m_A = 3 * (P1 - P0) - (2 * DD0 + DS1).
		float fDD2X = pTCB0->m_DD.x * 2.0f;
		float fDD2Y = pTCB0->m_DD.y * 2.0f;
		float fDD2Z = pTCB0->m_DD.z * 2.0f;
		// EXE-INLINED 0x439E90
		float fSumX = fDD2X + pTCB1->m_DS.x;
		float fSumY = fDD2Y + pTCB1->m_DS.y;
		float fSumZ = fDD2Z + pTCB1->m_DS.z;
		float fDiffX = pTCB1->m_Pos.x - pTCB0->m_Pos.x;
		float fDiffY = pTCB1->m_Pos.y - pTCB0->m_Pos.y;
		float fDiffZ = pTCB1->m_Pos.z - pTCB0->m_Pos.z;
		float fDiff3X = fDiffX * 3.0f;
		float fDiff3Y = fDiffY * 3.0f;
		float fDiff3Z = fDiffZ * 3.0f;
		pTCB0->m_A.x = fDiff3X - fSumX;
		pTCB0->m_A.y = fDiff3Y - fSumY;
		pTCB0->m_A.z = fDiff3Z - fSumZ;

		// m_B = (DD0 + DS1) - 2 * (P1 - P0); the exe computes the difference again.
		fDiffX = pTCB1->m_Pos.x - pTCB0->m_Pos.x;
		fDiffY = pTCB1->m_Pos.y - pTCB0->m_Pos.y;
		fDiffZ = pTCB1->m_Pos.z - pTCB0->m_Pos.z;
		float fDiff2X = fDiffX * 2.0f;
		float fDiff2Y = fDiffY * 2.0f;
		float fDiff2Z = fDiffZ * 2.0f;
		fSumX = pTCB0->m_DD.x + pTCB1->m_DS.x;
		fSumY = pTCB1->m_DS.y + pTCB0->m_DD.y;
		fSumZ = pTCB1->m_DS.z + pTCB0->m_DD.z;
		kPt.x = fSumX - fDiff2X;
		pTCB0->m_B.x = kPt.x;
		kPt.y = fSumY - fDiff2Y;
		pTCB0->m_B.y = kPt.y;
		kPt.z = fSumZ - fDiff2Z;
		pTCB0->m_B.z = kPt.z;
	}
}

// The exe stores its own functions' addresses, as the engine's Set*Function calls do inline, then calls
// RegisterSupportedFunctions for the rest.
int32_t NiTCBPosKey::RegisterLoader() {
	static char bRegistered;
	if (bRegistered) {
		return 0;
	}
	bRegistered = 1;
	ms_loaders[static_cast<uint32_t>(POSKEY) * NUMKEYTYPES + TCBKEY] = &NiTCBPosKey::CreateFromStream;
	ms_saves[static_cast<uint32_t>(POSKEY) * NUMKEYTYPES + TCBKEY] = &NiTCBPosKey::SaveToStream;
	ms_copies[static_cast<uint32_t>(POSKEY) * NUMKEYTYPES + TCBKEY] = &NiTCBPosKey::Copy;
	ms_arrays[static_cast<uint32_t>(POSKEY) * NUMKEYTYPES + TCBKEY] = &NiTCBPosKey::CreateArray;
	ms_deletes[static_cast<uint32_t>(POSKEY) * NUMKEYTYPES + TCBKEY] = &NiStepPosKey::DeleteArray;
	ms_keysizes[static_cast<uint32_t>(POSKEY) * NUMKEYTYPES + TCBKEY] = sizeof(NiTCBPosKey);
	RegisterSupportedFunctions(POSKEY, TCBKEY);
	return 1;
}

// NiNew NiTCBPosKey[uiNumKeys] inlined: the count cookie dword, then the keys (no constructors). The block is
// untyped memory the exe fills by hand, hence the reinterpret_cast to the keys past the cookie.
NiAnimationKey* NiTCBPosKey::CreateArray(uint32_t uiNumKeys) {
	size_t v1 = uiNumKeys <= SIZE_MAX / sizeof(NiTCBPosKey) ? sizeof(NiTCBPosKey) * uiNumKeys : SIZE_MAX;
	size_t* v2 =
		static_cast<size_t*>(MemoryManager_Singleton.Allocate((static_cast<uint32_t>(static_cast<uint32_t>(v1) +
																   static_cast<uint32_t>(sizeof(size_t))) <
																  static_cast<uint32_t>(v1))
				? -1
				: v1 + sizeof(size_t)));
	if (!v2) {
		return 0;
	}
	*v2 = uiNumKeys;
	return reinterpret_cast<NiAnimationKey*>(v2 + 1);
}

// The cubic ((m_B * t + m_A) * t + m_DD) * t + m_Pos of pkKey0 at fTime; pkKey1 is unused.
void NiTCBPosKey::Interpolate(float fTime, const NiAnimationKey* pkKey0, const NiAnimationKey*, void* pResult) {
	const NiTCBPosKey* pkTCB = static_cast<const NiTCBPosKey*>(pkKey0);
	NiPoint3* pkResult = static_cast<NiPoint3*>(pResult);
	float v6 = pkTCB->m_B.x * fTime;
	float v9 = pkTCB->m_B.y * fTime;
	float v12 = pkTCB->m_B.z * fTime;
	float v15 = pkTCB->m_A.x + v6;
	float v18 = pkTCB->m_A.y + v9;
	float v21 = pkTCB->m_A.z + v12;
	float v7 = v15 * fTime;
	float v10 = v18 * fTime;
	float v13 = v21 * fTime;
	float v16 = pkTCB->m_DD.x + v7;
	float v19 = pkTCB->m_DD.y + v10;
	float v22 = pkTCB->m_DD.z + v13;
	float v8 = v16 * fTime;
	float v11 = v19 * fTime;
	float v14 = fTime * v22;
	float v17 = pkTCB->m_Pos.x + v8;
	float v20 = pkTCB->m_Pos.y + v11;
	double v5 = (double)(pkTCB->m_Pos.z + v14);
	pkResult->x = v17;
	pkResult->y = v20;
	float v23 = (float)v5;
	pkResult->z = v23;
}
