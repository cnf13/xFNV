#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinFloatKey.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBezRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpScalar.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/string.h"

#include <stdint.h>

void NiLinFloatKey::Interpolate(float fTime, const NiAnimationKey* pKey0, const NiAnimationKey* pKey1, void* pResult) {
	*static_cast<float*>(pResult) = NiInterpScalar::Linear(fTime, pKey0[1].m_fTime, pKey1[1].m_fTime);
}

bool NiLinFloatKey::Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys) {
	NiAnimationKey* pkOldKeys = pkKeys;
	uint32_t uiInsertAt;
	if (!OkayToInsert(fTime, pkOldKeys, uiNumKeys, uiInsertAt, sizeof(NiLinFloatKey))) {
		return false;
	}

	// EXE-INLINED none: NiNew NiLinFloatKey[uiNewNumKeys] (the overflow-clamped size, the count cookie and
	// MemoryManager::Allocate; the keys have no constructor to run). The block is raw memory to the exe: a uint32_t
	// count cookie, then the keys; the casts below and before Deallocate step over that cookie.
	uint32_t uiNewNumKeys = uiNumKeys + 1;
	uint64_t ullBytes = static_cast<uint64_t>(uiNewNumKeys) * sizeof(NiLinFloatKey);
	uint32_t uiBytes = (ullBytes >> 32) != 0 ? 0xFFFFFFFFu : static_cast<uint32_t>(ullBytes);
	uint32_t uiAllocSize = uiBytes + sizeof(size_t) < uiBytes ? 0xFFFFFFFFu : uiBytes + sizeof(size_t);
	size_t* puiCookie = static_cast<size_t*>(MemoryManager_Singleton.Allocate(uiAllocSize));
	NiLinFloatKey* pkNewKeys;
	if (puiCookie) {
		*puiCookie = uiNewNumKeys;
		pkNewKeys = reinterpret_cast<NiLinFloatKey*>(puiCookie + 1);
	} else {
		pkNewKeys = nullptr;
	}

	NiLinFloatKey* pkOld = static_cast<NiLinFloatKey*>(pkOldKeys);
	rt::memcpy(pkNewKeys, pkOld, uiInsertAt * sizeof(NiLinFloatKey));
	if (uiNumKeys > uiInsertAt) {
		rt::memcpy(pkNewKeys + uiInsertAt + 1, pkOld + uiInsertAt, (uiNumKeys - uiInsertAt) * sizeof(NiLinFloatKey));
	}
	float fValue = GenInterpDefault(fTime, pkOld, LINKEY, uiNumKeys, sizeof(NiLinFloatKey));
	NiLinFloatKey* pkNewKey = pkNewKeys + uiInsertAt;
	pkNewKey->m_fTime = fTime;
	pkNewKey->m_fValue = fValue;
	uiNumKeys++;

	// EXE-INLINED none: NiDelete[] of the old array (no destructor calls, MemoryManager::Deallocate of the cookie)
	if (pkOldKeys) {
		MemoryManager_Singleton.Deallocate(fnvd::arrayBlock(pkOldKeys));
	}
	pkKeys = pkNewKeys;
	return true;
}

// The source's RegisterSupportedFunctions is inlined: the exe clears the slot's curvature and derivative entries
// before it sets the guard, then stores its own functions' addresses.
int32_t NiLinFloatKey::RegisterLoader() {
	static char bRegistered;
	if (bRegistered) {
		return 0;
	}
	const uint32_t uiIndex = static_cast<uint32_t>(FLOATKEY) * NUMKEYTYPES + LINKEY;
	ms_curves[uiIndex] = nullptr;
	ms_interpd1s[uiIndex] = nullptr;
	ms_interpd2s[uiIndex] = nullptr;
	bRegistered = 1;
	ms_loaders[uiIndex] = &NiAnimationKey::CreateFunction_00A2A5A0;
	ms_saves[uiIndex] = &NiAnimationKey::SaveToStream;
	ms_copies[uiIndex] = &NiLinFloatKey::Copy;
	ms_arrays[uiIndex] = &NiLinFloatKey::CreateArray;
	ms_deletes[uiIndex] = &NiStepPosKey::DeleteArray;
	ms_keysizes[uiIndex] = sizeof(NiLinFloatKey);
	ms_interps[uiIndex] = &NiLinFloatKey::Interpolate;
	ms_equals[uiIndex] = &NiLinFloatKey::Equal;
	ms_fillderived[uiIndex] = &NiLinFloatKey::FillDerivedVals;
	ms_inserts[uiIndex] = &NiLinFloatKey::Insert;
	return 1;
}

// The exe's code at this address is NiBoolKey::SaveBinary's, not NiFloatKey's: after the key's time it writes the
// byte at +4 as 0 or 1, one byte with a one-byte component.
void NiLinFloatKey::SaveBinary(NiStream& stream, NiAnimationKey* pkKey) {
	NiAnimationKey::SaveBinary(stream, pkKey);
	NiBoolKey* pkBool = static_cast<NiBoolKey*>(pkKey);
	// The exe compares the whole byte with 0 (cmp byte ptr [esi+4], 0); a bool read would copy it unchanged.
	uint8_t ucBool = static_cast<uint8_t>(pkBool->m_bBool) != 0;
	NiStreamSaveBinary(stream, ucBool);
}

void NiLinFloatKey::SaveToStream(NiStream& stream, NiAnimationKey* pkKeys, uint32_t uiNumKeys) {
	NiLinFloatKey* pkLinFloatKeys = static_cast<NiLinFloatKey*>(pkKeys);

	for (uint32_t ui = 0; ui < uiNumKeys; ui++)
		SaveBinary(stream, &pkLinFloatKeys[ui]);
}

// NiNew NiLinFloatKey[uiNumKeys] inlined: the count cookie dword, then the keys (no constructors). The block is
// untyped memory the exe fills by hand, hence the reinterpret_cast to the keys past the cookie.
NiAnimationKey* NiLinFloatKey::CreateArray(uint32_t uiNumKeys) {
	size_t v1 = uiNumKeys <= SIZE_MAX / sizeof(NiLinFloatKey) ? sizeof(NiLinFloatKey) * uiNumKeys : SIZE_MAX;
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

bool NiLinFloatKey::Equal(const NiAnimationKey& key0, const NiAnimationKey& key1) {
	return NiFloatKey::Equal(key0, key1);
}

void NiLinFloatKey::Copy(NiAnimationKey* pkNewKey, const NiAnimationKey* pkOrigKey) {
	NiLinFloatKey* pkNew = static_cast<NiLinFloatKey*>(pkNewKey);
	const NiLinFloatKey* pkOrig = static_cast<const NiLinFloatKey*>(pkOrigKey);
	pkNew->m_fTime = pkOrig->m_fTime;
	pkNew->m_fValue = pkOrig->m_fValue;
}
