#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinColorKey.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBezRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiColorKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/string.h"

#include <stdint.h>

bool NiLinColorKey::Equal(const NiAnimationKey& key0, const NiAnimationKey& key1) {
	return NiColorKey::Equal(key0, key1);
}

void NiLinColorKey::Copy(NiAnimationKey* pkNewKey, const NiAnimationKey* pkOrigKey) {
	NiLinColorKey* pkNewColor = static_cast<NiLinColorKey*>(pkNewKey);
	const NiLinColorKey* pkOrigColor = static_cast<const NiLinColorKey*>(pkOrigKey);
	pkNewColor->m_fTime = pkOrigColor->m_fTime;
	pkNewColor->m_Color = pkOrigColor->m_Color;
}

NiAnimationKey* NiLinColorKey::CreateArray(uint32_t uiNumKeys) {
	// EXE-INLINED none: NiNew NiLinColorKey[uiNumKeys] (the overflow-clamped size, the count cookie,
	// MemoryManager::Allocate and the vector constructor iterator). The block is raw memory to the exe: a uint32_t
	// count cookie, then the keys.
	uint64_t ullBytes = static_cast<uint64_t>(uiNumKeys) * sizeof(NiLinColorKey);
	uint32_t uiBytes = (ullBytes >> 32) != 0 ? 0xFFFFFFFFu : static_cast<uint32_t>(ullBytes);
	uint32_t uiAllocSize = uiBytes + sizeof(size_t) < uiBytes ? 0xFFFFFFFFu : uiBytes + sizeof(size_t);
	size_t* puiCookie = static_cast<size_t*>(MemoryManager_Singleton.Allocate(uiAllocSize));
	if (!puiCookie) {
		return nullptr;
	}
	*puiCookie = uiNumKeys;
	NiLinColorKey* pkKeys = reinterpret_cast<NiLinColorKey*>(puiCookie + 1);
	rt::eh_vector_constructor_iterator(pkKeys, sizeof(NiLinColorKey), uiNumKeys,
		&fnvd::constructElement<NiLinColorKey>);
	return pkKeys;
}

NiAnimationKey* NiLinColorKey::CreateFromStream(NiStream& kStream, uint32_t uiNumKeys) {
	// EXE-INLINED none: NiNew NiLinColorKey[uiNumKeys], as in CreateArray (raw memory to the exe: the count cookie,
	// then the keys).
	uint64_t ullBytes = static_cast<uint64_t>(uiNumKeys) * sizeof(NiLinColorKey);
	uint32_t uiBytes = (ullBytes >> 32) != 0 ? 0xFFFFFFFFu : static_cast<uint32_t>(ullBytes);
	uint32_t uiAllocSize = uiBytes + sizeof(size_t) < uiBytes ? 0xFFFFFFFFu : uiBytes + sizeof(size_t);
	size_t* puiCookie = static_cast<size_t*>(MemoryManager_Singleton.Allocate(uiAllocSize));
	NiLinColorKey* pkLinColorKeys;
	if (puiCookie) {
		*puiCookie = uiNumKeys;
		pkLinColorKeys = reinterpret_cast<NiLinColorKey*>(puiCookie + 1);
		rt::eh_vector_constructor_iterator(pkLinColorKeys, sizeof(NiLinColorKey), uiNumKeys,
			&fnvd::constructElement<NiLinColorKey>);
	} else {
		pkLinColorKeys = nullptr;
	}
	for (uint32_t ui = 0; ui < uiNumKeys; ui++) {
		pkLinColorKeys[ui].LoadBinary(kStream);
	}
	return pkLinColorKeys;
}

bool NiLinColorKey::Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys) {
	NiAnimationKey* pkOldKeys = pkKeys;
	uint32_t uiInsertAt;
	if (!OkayToInsert(fTime, pkOldKeys, uiNumKeys, uiInsertAt, sizeof(NiLinColorKey))) {
		return false;
	}

	// EXE-INLINED none: NiNew NiLinColorKey[uiNewNumKeys] (the overflow-clamped size, the count cookie,
	// MemoryManager::Allocate and the vector constructor iterator). The block is raw memory to the exe: a uint32_t
	// count cookie, then the keys; the casts below and before Deallocate step over that cookie.
	uint32_t uiNewNumKeys = uiNumKeys + 1;
	uint64_t ullBytes = static_cast<uint64_t>(uiNewNumKeys) * sizeof(NiLinColorKey);
	uint32_t uiBytes = (ullBytes >> 32) != 0 ? 0xFFFFFFFFu : static_cast<uint32_t>(ullBytes);
	uint32_t uiAllocSize = uiBytes + sizeof(size_t) < uiBytes ? 0xFFFFFFFFu : uiBytes + sizeof(size_t);
	size_t* puiCookie = static_cast<size_t*>(MemoryManager_Singleton.Allocate(uiAllocSize));
	NiLinColorKey* pkNewKeys;
	if (puiCookie) {
		*puiCookie = uiNewNumKeys;
		pkNewKeys = reinterpret_cast<NiLinColorKey*>(puiCookie + 1);
		rt::eh_vector_constructor_iterator(pkNewKeys, sizeof(NiLinColorKey), uiNewNumKeys,
			&fnvd::constructElement<NiLinColorKey>);
	} else {
		pkNewKeys = nullptr;
	}

	NiLinColorKey* pkOld = static_cast<NiLinColorKey*>(pkOldKeys);
	rt::memcpy(pkNewKeys, pkOld, uiInsertAt * sizeof(NiLinColorKey));
	if (uiNumKeys > uiInsertAt) {
		rt::memcpy(pkNewKeys + uiInsertAt + 1, pkOld + uiInsertAt, (uiNumKeys - uiInsertAt) * sizeof(NiLinColorKey));
	}
	NiColorA kColor = NiColorKey::GenInterpDefault(fTime, pkOld, LINKEY, uiNumKeys, sizeof(NiLinColorKey));
	NiLinColorKey* pkNewKey = pkNewKeys + uiInsertAt;
	pkNewKey->m_fTime = fTime;
	pkNewKey->m_Color = kColor;
	uiNumKeys++;

	// EXE-INLINED none: NiDelete[] of the old array (no destructor calls, MemoryManager::Deallocate of the cookie)
	if (pkOldKeys) {
		MemoryManager_Singleton.Deallocate(reinterpret_cast<size_t*>(pkOldKeys) - 1);
	}
	pkKeys = pkNewKeys;
	return true;
}

// Clears the slot's curvature, derivative and fill-derived entries before it sets the guard, then stores its own
// functions' addresses.
int32_t NiLinColorKey::RegisterLoader() {
	static char bRegistered;
	if (bRegistered) {
		return 0;
	}
	const uint32_t uiIndex = static_cast<uint32_t>(COLORKEY) * NUMKEYTYPES + LINKEY;
	ms_curves[uiIndex] = nullptr;
	ms_interpd1s[uiIndex] = nullptr;
	ms_interpd2s[uiIndex] = nullptr;
	ms_fillderived[uiIndex] = nullptr;
	bRegistered = 1;
	ms_loaders[uiIndex] = &NiLinColorKey::CreateFromStream;
	ms_saves[uiIndex] = &NiLinColorKey::SaveToStream;
	ms_copies[uiIndex] = &NiLinColorKey::Copy;
	ms_arrays[uiIndex] = &NiLinColorKey::CreateArray;
	ms_deletes[uiIndex] = &NiStepPosKey::DeleteArray;
	ms_keysizes[uiIndex] = sizeof(NiLinColorKey);
	ms_interps[uiIndex] = &NiLinColorKey::Interpolate;
	ms_equals[uiIndex] = &NiLinColorKey::Equal;
	ms_inserts[uiIndex] = &NiLinColorKey::Insert;
	return 1;
}

void NiLinColorKey::Interpolate(float fTime, const NiAnimationKey* pkKey0, const NiAnimationKey* pkKey1,
	void* pResult) {
	const NiColorA& kColor0 = static_cast<const NiColorKey*>(pkKey0)->m_Color;
	const NiColorA& kColor1 = static_cast<const NiColorKey*>(pkKey1)->m_Color;
	NiColorA* pkResult = static_cast<NiColorA*>(pResult);
	double v4 = (double)fTime;
	float v18 = (float)(1.0 - fTime);
	float v10 = kColor0.r * v18;
	float v11 = kColor0.g * v18;
	float v12 = kColor0.b * v18;
	float v13 = v18 * kColor0.a;
	float v6 = (float)(kColor1.r * v4);
	float v7 = (float)(kColor1.g * v4);
	float v8 = (float)(kColor1.b * v4);
	float v9 = (float)(v4 * kColor1.a);
	float v14 = v6 + v10;
	pkResult->r = v14;
	float v15 = v7 + v11;
	pkResult->g = v15;
	float v16 = v8 + v12;
	pkResult->b = v16;
	float v17 = v9 + v13;
	pkResult->a = v17;
}

void NiLinColorKey::SaveToStream(NiStream& kStream, NiAnimationKey* pkKeys, uint32_t uiNumKeys) {
	NiColorKey* pkColorKey = static_cast<NiColorKey*>(pkKeys);
	for (uint32_t i = uiNumKeys; i; --i) {
		NiColorKey::SaveBinary(kStream, pkColorKey);
		++pkColorKey;
	}
}
