#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBezFloatKey.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBezRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpScalar.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiStepPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTCBRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// The PDB's RegisterSupportedFunctions is inlined: the exe clears the slot's curvature and derivative entries before
// it sets the guard, then stores its own functions' addresses.
int32_t NiBezFloatKey_RegisterLoader() {
	if (NiBezFloatKey_RegisterLoader_bRegistered) {
		return 0;
	}
	const uint32_t uiIndex =
		static_cast<uint32_t>(NiAnimationKey::FLOATKEY) * NiAnimationKey::NUMKEYTYPES + NiAnimationKey::BEZKEY;
	NiAnimationKey::ms_curves[uiIndex] = nullptr;
	NiAnimationKey::ms_interpd1s[uiIndex] = nullptr;
	NiAnimationKey::ms_interpd2s[uiIndex] = nullptr;
	NiBezFloatKey_RegisterLoader_bRegistered = 1;
	NiAnimationKey::ms_loaders[uiIndex] = &NiBezFloatKey::CreateFromStream;
	NiAnimationKey::ms_saves[uiIndex] = &NiBezFloatKey::SaveToStream;
	NiAnimationKey::ms_copies[uiIndex] = &NiBezFloatKey::Copy;
	NiAnimationKey::ms_arrays[uiIndex] = &NiBezFloatKey::CreateArray;
	NiAnimationKey::ms_deletes[uiIndex] = &NiStepPosKey::DeleteArray;
	NiAnimationKey::ms_keysizes[uiIndex] = sizeof(NiBezFloatKey);
	NiAnimationKey::ms_interps[uiIndex] = &NiBezFloatKey::Interpolate;
	NiAnimationKey::ms_equals[uiIndex] = &NiBezFloatKey::Equal;
	NiAnimationKey::ms_fillderived[uiIndex] = &NiBezFloatKey::FillDerivedVals;
	NiAnimationKey::ms_inserts[uiIndex] = &NiBezFloatKey::Insert;
	return 1;
}

// NiNew NiBezFloatKey[uiNumKeys] is the engine's new[] on MemoryManager, inlined: the size saturates on overflow, the
// count cookie goes in the untyped word before the keys, and the keys are not constructed. Each key's LoadBinary is
// inlined: NiFloatKey's, then both tangents. The exe keeps the array in the count's argument slot and returns it from
// there; the size word both reads are handed is one local.
NiAnimationKey* NiBezFloatKey::CreateFromStream(NiStream& stream, uint32_t uiNumKeys) {
	uint32_t uiReadSize;
	NiBezFloatKey* pkBezKeys;
	size_t uiSize = uiNumKeys <= SIZE_MAX / sizeof(NiBezFloatKey) ? sizeof(NiBezFloatKey) * uiNumKeys : SIZE_MAX;
	size_t* puiCookie =
		static_cast<size_t*>(MemoryManager_Singleton.Allocate(uiSize + static_cast<uint32_t>(sizeof(size_t)) < uiSize
				? -1
				: uiSize + sizeof(size_t)));
	if (puiCookie) {
		*puiCookie = uiNumKeys;
		pkBezKeys = reinterpret_cast<NiBezFloatKey*>(puiCookie + 1);
	} else {
		pkBezKeys = 0;
	}
	for (uint32_t ui = 0; ui < uiNumKeys; ++ui) {
		// EXE-INLINED none: NiBezFloatKey::LoadBinary
		NiBezFloatKey* pkKey = &pkBezKeys[ui];
		pkKey->NiFloatKey::LoadBinary(stream);
		NiBinaryStream* pkIstr = stream.Istr();
		NiBinaryStream::NIBINARYSTREAMREADFN pfnRead = pkIstr->m_pfnRead;
		uiReadSize = 4;
		pkIstr->m_uiAbsoluteCurrentPos += pfnRead(pkIstr, &pkKey->m_fInTan, 4, &uiReadSize, 1);
		NiBinaryStream* pkIstr2 = stream.Istr();
		NiBinaryStream::NIBINARYSTREAMREADFN pfnRead2 = pkIstr2->m_pfnRead;
		uiReadSize = 4;
		pkIstr2->m_uiAbsoluteCurrentPos += pfnRead2(pkIstr2, &pkKey->m_fOutTan, 4, &uiReadSize, 1);
	}
	return pkBezKeys;
}

void NiBezFloatKey::Interpolate(float fTime, const NiAnimationKey* pkKey0, const NiAnimationKey* pkKey1,
	void* pResult) {
	const NiBezFloatKey* pkBezKey0 = static_cast<const NiBezFloatKey*>(pkKey0);
	const NiBezFloatKey* pkBezKey1 = static_cast<const NiBezFloatKey*>(pkKey1);
	float* pfResult = static_cast<float*>(pResult);
	*pfResult = NiInterpScalar::Bezier(fTime, pkBezKey0->m_fValue, pkBezKey0->m_fOutTan, pkBezKey1->m_fValue,
		pkBezKey1->m_fInTan);
}

// NiNew NiBezFloatKey[uiNumKeys] inlined: the count cookie dword, then the keys (no constructors). The block is
// untyped memory the exe fills by hand, hence the reinterpret_cast to the keys past the cookie.
NiAnimationKey* NiBezFloatKey::CreateArray(uint32_t uiNumKeys) {
	size_t v1 = uiNumKeys <= SIZE_MAX / sizeof(NiBezFloatKey) ? sizeof(NiBezFloatKey) * uiNumKeys : SIZE_MAX;
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

// NiNew NiBezFloatKey[uiNumKeys + 1] and NiDelete[] of the old array are inlined: MemoryManager, an untyped count
// cookie dword before the keys (the casts below step over it), no constructors or destructors. Between two keys the
// new key's tangents and the neighbours' facing tangents are fitted by NiInterpScalar::AdjustBezier (src's
// NiInterpScalar::AdjustBezier).
bool NiBezFloatKey::Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys) {
	NiBezFloatKey* pkOldKeys = static_cast<NiBezFloatKey*>(pkKeys);
	uint32_t uiInsertAt;
	if (!NiAnimationKey::OkayToInsert(fTime, pkOldKeys, uiNumKeys, uiInsertAt, sizeof(NiBezFloatKey))) {
		return false;
	}
	uint32_t uiNewNumKeys = uiNumKeys + 1;
	size_t uiSize = uiNewNumKeys <= SIZE_MAX / sizeof(NiBezFloatKey) ? sizeof(NiBezFloatKey) * uiNewNumKeys : SIZE_MAX;
	size_t* puiCookie =
		static_cast<size_t*>(MemoryManager_Singleton.Allocate(uiSize + static_cast<uint32_t>(sizeof(size_t)) < uiSize
				? -1
				: uiSize + sizeof(size_t)));
	NiBezFloatKey* pkNewKeys;
	if (puiCookie) {
		*puiCookie = uiNewNumKeys;
		pkNewKeys = reinterpret_cast<NiBezFloatKey*>(puiCookie + 1);
	} else {
		pkNewKeys = nullptr;
	}
	rt::memcpy(pkNewKeys, pkOldKeys, uiInsertAt * sizeof(NiBezFloatKey));
	if (uiNumKeys > uiInsertAt) {
		rt::memcpy(&pkNewKeys[uiInsertAt + 1], &pkOldKeys[uiInsertAt],
			(uiNumKeys - uiInsertAt) * sizeof(NiBezFloatKey));
	}
	float fValue =
		NiFloatKey::GenInterpDefault(fTime, pkOldKeys, NiAnimationKey::BEZKEY, uiNumKeys, sizeof(NiBezFloatKey));
	NiBezFloatKey* pkNewKey = &pkNewKeys[uiInsertAt];
	pkNewKey->m_fTime = fTime;
	pkNewKey->m_fValue = fValue;
	pkNewKey->m_fInTan = 0.0f;
	pkNewKey->m_fOutTan = 0.0f;
	if (uiInsertAt != 0 && uiInsertAt != uiNumKeys) {
		NiBezFloatKey* pkLastKey = &pkNewKeys[uiInsertAt - 1];
		NiBezFloatKey* pkNextKey = &pkNewKeys[uiInsertAt + 1];
		float fLastValue = pkLastKey->m_fValue;
		float fLastTime = pkLastKey->m_fTime;
		float fLastOut = pkLastKey->m_fOutTan;
		float fNextValue = pkNextKey->m_fValue;
		float fNextTime = pkNextKey->m_fTime;
		float fNextIn = pkNextKey->m_fInTan;
		// The exe hands the callee the slots of the last key's value and time for the new key's in and out tangents.
		NiInterpScalar::AdjustBezier(fLastValue, fLastTime, &fLastOut, fNextValue, fNextTime, &fNextIn, fTime, fValue,
			&fLastValue, &fLastTime);
		pkLastKey->m_fOutTan = fLastOut;
		pkNewKey->m_fInTan = fLastValue;
		pkNewKey->m_fOutTan = fLastTime;
		pkNextKey->m_fInTan = fNextIn;
	}
	++uiNumKeys;
	if (pkOldKeys) {
		MemoryManager_Singleton.Deallocate(fnvd::arrayBlock(pkOldKeys));
	}
	pkKeys = pkNewKeys;
	return true;
}

// Each key's SaveBinary is inlined: NiFloatKey's (src's NiFloatKey::SaveBinary), then both tangents. The exe keeps the write count
// (4) in uiNumKeys's argument slot; src keeps it in a local.
void NiBezFloatKey::SaveToStream(NiStream& kStream, NiAnimationKey* pkKeys, uint32_t uiNumKeys) {
	NiBezFloatKey* pkBezKeys = static_cast<NiBezFloatKey*>(pkKeys);
	uint32_t uiSize;
	for (uint32_t ui = 0; ui < uiNumKeys; ++ui) {
		// EXE-INLINED none: NiBezFloatKey::SaveBinary
		NiBezFloatKey* pkKey = &pkBezKeys[ui];
		NiFloatKey::SaveBinary(&kStream, pkKey);
		NiBinaryStream* pkOstr = kStream.Ostr();
		NiBinaryStream::NIBINARYSTREAMWRITEFN pfnWrite = pkOstr->m_pfnWrite;
		uiSize = 4;
		pkOstr->m_uiAbsoluteCurrentPos += pfnWrite(pkOstr, &pkKey->m_fInTan, 4, &uiSize, 1);
		NiBinaryStream* pkOstr2 = kStream.Ostr();
		NiBinaryStream::NIBINARYSTREAMWRITEFN pfnWrite2 = pkOstr2->m_pfnWrite;
		uiSize = 4;
		pkOstr2->m_uiAbsoluteCurrentPos += pfnWrite2(pkOstr2, &pkKey->m_fOutTan, 4, &uiSize, 1);
	}
}

bool NiBezFloatKey::Equal(const NiAnimationKey& key0, const NiAnimationKey& key1) {
	if (!NiFloatKey::Equal(key0, key1)) {
		return false;
	}
	const NiBezFloatKey& bez0 = static_cast<const NiBezFloatKey&>(key0);
	const NiBezFloatKey& bez1 = static_cast<const NiBezFloatKey&>(key1);
	return bez0.m_fInTan == bez1.m_fInTan && bez0.m_fOutTan == bez1.m_fOutTan;
}

void NiBezFloatKey::Copy(NiAnimationKey* pkNewKey, const NiAnimationKey* pkOrigKey) {
	NiBezFloatKey* pkNew = static_cast<NiBezFloatKey*>(pkNewKey);
	const NiBezFloatKey* pkOrig = static_cast<const NiBezFloatKey*>(pkOrigKey);
	pkNew->m_fTime = pkOrig->m_fTime;
	pkNew->m_fValue = pkOrig->m_fValue;
	pkNew->m_fInTan = pkOrig->m_fInTan;
	pkNew->m_fOutTan = pkOrig->m_fOutTan;
}
