#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolKey.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBezRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// The exe reads the flag as one byte into the stream argument's own stack slot: a read that fills nothing leaves the
// low byte of the stream's address there, which ucBool starts as.
void NiBoolKey::LoadBinary(NiStream& stream) {
	NiStream* v2 = &stream;
	uint32_t v8;
	uint8_t ucBool = static_cast<uint8_t>(reinterpret_cast<uintptr_t>(&stream));
	NiBinaryStream* m_pkIstr = v2->Istr();
	uint32_t (*m_pfnRead)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = m_pkIstr->m_pfnRead;
	v8 = 4;
	m_pkIstr->m_uiAbsoluteCurrentPos += m_pfnRead(m_pkIstr, this, 4, &v8, 1);
	NiBinaryStream* v6 = v2->Istr();
	uint32_t (*v7)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v6->m_pfnRead;
	v8 = 1;
	v6->m_uiAbsoluteCurrentPos += v7(v6, &ucBool, 1, &v8, 1);
	m_bBool = ucBool != 0;
}

// The keys' times are in increasing order, so the search starts at uiLastIdx, the key the previous call ended on.
// The exe keeps the index in a local and stores it back only after the interpolation call.
uint8_t NiBoolKey::GenInterp(float fTime, NiBoolKey* pkKeys, NiAnimationKey::KeyType eType, uint32_t uiNumKeys,
	uint32_t& uiLastIdx, uint8_t ucSize) {
	if (uiNumKeys == 1) {
		return pkKeys->GetBool();
	}
	// The exe compares fTime with -FLT_MAX as a double (fcom qword 0x10241B0).
	if (fTime == -3.4028234663852886e+38) {
		return pkKeys->GetBool();
	}

	uint32_t uiNumKeysM1 = uiNumKeys - 1;
	uint32_t uiLastIdxCopy = uiLastIdx;
	float fLastTime = pkKeys->GetKeyAt(uiLastIdxCopy, ucSize)->GetTime();
	if (fLastTime > fTime) {
		uiLastIdxCopy = 0;
		fLastTime = pkKeys->GetTime();
	}

	uint32_t uiNextIdx;
	float fNextTime = 0.0f;
	for (uiNextIdx = uiLastIdxCopy + 1; uiNextIdx <= uiNumKeysM1; uiNextIdx++) {
		fNextTime = pkKeys->GetKeyAt(uiNextIdx, ucSize)->GetTime();
		// The exe's test (fcom; test ah, 1; je) stops only when the key is not before fTime and the compare is
		// ordered: C0 is set for an unordered compare too, which goes on searching.
		if (fNextTime >= fTime) {
			break;
		}
		uiLastIdxCopy++;
		fLastTime = fNextTime;
	}

	// The interpolation time, normalised to [0, 1], is passed as a float.
	float fNormTime = (fTime - fLastTime) / (fNextTime - fLastTime);
	bool bResult;
	NiAnimationKey::ms_interps[static_cast<uint32_t>(BOOLKEY) * NUMKEYTYPES + eType](fNormTime,
		pkKeys->GetKeyAt(uiLastIdxCopy, ucSize), pkKeys->GetKeyAt(uiNextIdx, ucSize), &bResult);
	uiLastIdx = uiLastIdxCopy;
	return bResult;
}

uint8_t NiBoolKey::GenInterpDefault(float fTime, NiBoolKey* pkKeys, KeyType eType, uint32_t uiNumKeys, uint8_t ucSize) {
	if (!uiNumKeys)
		return ms_bDefault;
	if (pkKeys->m_fTime > fTime)
		return pkKeys->m_bBool;
	NiBoolKey* pkLast = pkKeys->GetKeyAt(uiNumKeys - 1, ucSize);
	if (pkLast->m_fTime < fTime)
		return pkLast->m_bBool;
	// The exe reuses uiNumKeys's own stack slot as the zeroed last index GenInterp updates.
	uint32_t uiCount = uiNumKeys;
	uiNumKeys = 0;
	return GenInterp(fTime, pkKeys, eType, uiCount, uiNumKeys, ucSize);
}

// NiAnimationKey::Equal (the times), then the flags.
bool NiBoolKey::Equal(const NiAnimationKey& key0, const NiAnimationKey& key1) {
	// NiAnimationKey::Equal (the times); the exe tests its low byte.
	if (!static_cast<uint8_t>(NiAnimationKey::Equal(&key0.m_fTime, &key1.m_fTime))) {
		return false;
	}
	const NiBoolKey& bool0 = static_cast<const NiBoolKey&>(key0);
	const NiBoolKey& bool1 = static_cast<const NiBoolKey&>(key1);
	return bool0.m_bBool == bool1.m_bBool;
}

// The source's Set*Function calls, inlined: the exe stores its own functions' addresses.
void NiBoolKey::RegisterSupportedFunctions(KeyContent eContent, KeyType eType) {
	uint32_t uiIndex = static_cast<uint32_t>(eContent) * NUMKEYTYPES + eType;
	ms_curves[uiIndex] = &NiBoolKey::ComputeMaxCurvature;
	ms_interps[uiIndex] = &NiBoolKey::Interpolate;
	ms_interpd1s[uiIndex] = &NiBoolKey::Interpolate;
	ms_interpd2s[uiIndex] = &NiBoolKey::Interpolate;
	ms_equals[uiIndex] = &NiBoolKey::Equal;
	ms_fillderived[uiIndex] = &NiBoolKey::FillDerivedVals;
	ms_inserts[uiIndex] = nullptr;
}

int32_t NiBoolKey::RegisterLoader() {
	static char bRegistered;
	if (bRegistered) {
		return 0;
	}
	bRegistered = 1;
	ms_loaders[static_cast<uint32_t>(BOOLKEY) * NUMKEYTYPES + NOINTERP] = &NiBoolKey::CreateFromStream;
	ms_saves[static_cast<uint32_t>(BOOLKEY) * NUMKEYTYPES + NOINTERP] = &NiBoolKey::SaveToStream;
	ms_copies[static_cast<uint32_t>(BOOLKEY) * NUMKEYTYPES + NOINTERP] = &NiBoolKey::Copy;
	ms_arrays[static_cast<uint32_t>(BOOLKEY) * NUMKEYTYPES + NOINTERP] = &NiBoolKey::CreateArray;
	ms_deletes[static_cast<uint32_t>(BOOLKEY) * NUMKEYTYPES + NOINTERP] = &NiBoolKey::DeleteArray;
	ms_keysizes[static_cast<uint32_t>(BOOLKEY) * NUMKEYTYPES + NOINTERP] = sizeof(NiBoolKey);
	RegisterSupportedFunctions(BOOLKEY, NOINTERP);
	return 1;
}

void NiBoolKey::Interpolate(float, const NiAnimationKey*, const NiAnimationKey*, void* pResult) {
	*static_cast<bool*>(pResult) = NiBoolKey::ms_bDefault;
}

float NiBoolKey::ComputeMaxCurvature(NiAnimationKey*, uint32_t) {
	return 0.0f;
}
