#include "Gamebryo2.2/CoreLibs/NiAnimation/NiEulerRotKey.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiStepPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

NiEulerRotKey::NiEulerRotKey() {
	m_quat.SetValues(0.0f, 0.0f, 0.0f, 0.0f);
	m_uiNumKeys[0] = 0;
	m_uiNumKeys[1] = 0;
	m_uiNumKeys[2] = 0;
	m_apkKeys[0] = 0;
	m_apkKeys[1] = 0;
	m_apkKeys[2] = 0;
	m_eType[0] = NiAnimationKey::NOINTERP;
	m_eType[1] = NiAnimationKey::NOINTERP;
	m_eType[2] = NiAnimationKey::NOINTERP;
	m_uiLastIdx[0] = 0;
	m_uiLastIdx[1] = 0;
	m_uiLastIdx[2] = 0;
	m_ucSizes[0] = 0;
	m_ucSizes[1] = 0;
	m_ucSizes[2] = 0;
}

// Deletes each angle's float keys through the key-type delete table and clears the angle's count and type.
void NiEulerRotKey::CleanUp() {
	for (uint32_t ui = 0; ui < 3; ui++) {
		if (m_apkKeys[ui]) {
			NiAnimationKey::GetDeleteFunction(NiAnimationKey::FLOATKEY, m_eType[ui])(m_apkKeys[ui]);
		}
		m_apkKeys[ui] = nullptr;
		m_uiNumKeys[ui] = 0;
		m_eType[ui] = NiAnimationKey::NOINTERP;
	}
}

// Files before 10.1.0.104 store a rotation order first, which is read and dropped.
void NiEulerRotKey::LoadBinary(NiStream& stream) {
	if (stream.GetFileVersion() < NiStream::GetVersion(10, 1, 0, 104)) {
		int32_t iOrder;
		NiStreamLoadBinary(stream, iOrder);
	}

	for (uint32_t uiI = 0; uiI < 3; uiI++) {
		NiStreamLoadBinary(stream, m_uiNumKeys[uiI]);

		if (m_uiNumKeys[uiI] > 0) {
			NiAnimationKey::KeyType eType;
			NiStreamLoadBinary(stream, eType);
			m_eType[uiI] = eType;
			m_ucSizes[uiI] = NiFloatKey::GetKeySize(eType);
			m_apkKeys[uiI] = static_cast<NiFloatKey*>(GetCreateFunction(FLOATKEY, eType)(stream, m_uiNumKeys[uiI]));
		}
	}
}

// Each half angle is the float angle times the double 0.5, stored as a float; NiSinCos stores the cosine, then the
// sine, as floats. The quaternion's products and sums are double until each component's float store.
void NiEulerRotKey::Interpolate(float fTime, const NiAnimationKey* pkKey0, const NiAnimationKey* pkKey1,
	void* pkResult) {
	NiEulerRotKey* pkEuler = const_cast<NiEulerRotKey*>(static_cast<const NiEulerRotKey*>(pkKey0));
	float afAngles[3];
	for (uint32_t i = 0; i < 3; i++) {
		if (pkEuler->m_uiNumKeys[i] == 0) {
			afAngles[i] = 0.0f;
		} else {
			afAngles[i] = NiFloatKey::GenInterp(fTime, pkEuler->m_apkKeys[i], pkEuler->m_eType[i],
				pkEuler->m_uiNumKeys[i], pkEuler->m_uiLastIdx[i], pkEuler->m_ucSizes[i]);
		}
	}

	float fSin0;
	float fCos0;
	float fSin1;
	float fCos1;
	float fSin2;
	float fCos2;
	float fHalf0 = afAngles[0] * 0.5;
	// EXE-INLINED 0x4169A0
	fSin0 = std::sin(fHalf0);
	fCos0 = std::cos(fHalf0);
	float fHalf1 = afAngles[1] * 0.5;
	// EXE-INLINED 0x4169A0
	fSin1 = std::sin(fHalf1);
	fCos1 = std::cos(fHalf1);
	float fHalf2 = afAngles[2] * 0.5;
	// EXE-INLINED 0x4169A0
	fSin2 = std::sin(fHalf2);
	fCos2 = std::cos(fHalf2);

	NiQuaternion* pkQuat = static_cast<NiQuaternion*>(pkResult);
	double dSin2Sin1 = static_cast<double>(fSin2) * fSin1;
	double dCos2Cos1 = static_cast<double>(fCos2) * fCos1;
	double dSin1Cos2 = static_cast<double>(fSin1) * fCos2;
	double dSin2Cos1 = static_cast<double>(fSin2) * fCos1;
	// EXE-INLINED 0xA6DB50
	pkQuat->m_fW = static_cast<float>(fCos0 * dCos2Cos1 + fSin0 * dSin2Sin1);
	pkQuat->m_fX = static_cast<float>(dCos2Cos1 * fSin0 - dSin2Sin1 * fCos0);
	pkQuat->m_fY = static_cast<float>(fCos0 * dSin1Cos2 + fSin0 * dSin2Cos1);
	pkQuat->m_fZ = static_cast<float>(dSin2Cos1 * fCos0 - dSin1Cos2 * fSin0);
}

void NiEulerRotKey::SaveToStream(NiStream& stream, NiAnimationKey* pkKeys, uint32_t uiNumKeys) {
	NiEulerRotKey* pkEulerKeys = static_cast<NiEulerRotKey*>(pkKeys);

	for (uint32_t ui = 0; ui < uiNumKeys; ui++)
		SaveBinary(stream, &pkEulerKeys[ui]);
}

// KEEP: niread: the exe reads the old rotation order and each key type into one stack slot ([esp+0x10]) and uses the
// key type to index the loader table and the key size table, so a short read (malformed data) leaves stack garbage
// or the previous read there, which src cannot express; porting.md's unwritten-slot exception covers GetChunkData
// only.
void NiEulerRotKey::SaveBinary(NiStream& stream, NiAnimationKey* pkKey) {
	NiEulerRotKey* pkEuler = static_cast<NiEulerRotKey*>(pkKey);

	for (uint32_t uiI = 0; uiI < 3; uiI++) {
		NiStreamSaveBinary(stream, pkEuler->m_uiNumKeys[uiI]);
		if (pkEuler->m_uiNumKeys[uiI] > 0) {
			NiStreamSaveBinary(stream, pkEuler->m_eType[uiI]);
			GetSaveFunction(FLOATKEY, pkEuler->m_eType[uiI])(stream, pkEuler->m_apkKeys[uiI],
				pkEuler->m_uiNumKeys[uiI]);
		}
	}
}

// The keys are equal when each angle's count, type and key size match and the angle's FLOATKEY equal function
// accepts every key pair.
bool NiEulerRotKey::Equal(const NiAnimationKey& key0, const NiAnimationKey& key1) {
	const NiEulerRotKey& kEuler0 = static_cast<const NiEulerRotKey&>(key0);
	const NiEulerRotKey& kEuler1 = static_cast<const NiEulerRotKey&>(key1);

	for (uint32_t uiI = 0; uiI < 3; uiI++) {
		if (kEuler0.m_uiNumKeys[uiI] != kEuler1.m_uiNumKeys[uiI] || kEuler0.m_eType[uiI] != kEuler1.m_eType[uiI] ||
			kEuler0.m_ucSizes[uiI] != kEuler1.m_ucSizes[uiI]) {
			return false;
		}

		if (kEuler0.m_uiNumKeys[uiI] > 0) {
			EqualFunction pfnEqual = GetEqualFunction(FLOATKEY, kEuler0.m_eType[uiI]);
			for (uint32_t uiJ = 0; uiJ < kEuler0.m_uiNumKeys[uiI]; uiJ++) {
				if (!pfnEqual(*kEuler0.m_apkKeys[uiI]->GetKeyAt(uiJ, kEuler0.m_ucSizes[uiI]),
						*kEuler1.m_apkKeys[uiI]->GetKeyAt(uiJ, kEuler1.m_ucSizes[uiI]))) {
					return false;
				}
			}
		}
	}

	return true;
}

// Copies each angle's count and type (not its key size) and, when it has keys, a new FLOATKEY array of the original's
// keys.
void NiEulerRotKey::Copy(NiAnimationKey* pkNewKey, const NiAnimationKey* pkOrigKey) {
	NiEulerRotKey* pkNewEuler = static_cast<NiEulerRotKey*>(pkNewKey);
	const NiEulerRotKey* pkOrigEuler = static_cast<const NiEulerRotKey*>(pkOrigKey);

	for (uint32_t uiI = 0; uiI < 3; uiI++) {
		pkNewEuler->m_uiNumKeys[uiI] = pkOrigEuler->m_uiNumKeys[uiI];
		pkNewEuler->m_eType[uiI] = pkOrigEuler->m_eType[uiI];

		uint32_t uiNumKeys = pkOrigEuler->m_uiNumKeys[uiI];
		if (uiNumKeys) {
			pkNewEuler->m_apkKeys[uiI] =
				static_cast<NiFloatKey*>(GetArrayFunction(FLOATKEY, pkOrigEuler->m_eType[uiI])(uiNumKeys));

			CopyFunction pfnCopy = GetCopyFunction(FLOATKEY, pkOrigEuler->m_eType[uiI]);
			for (uint32_t uiJ = 0; uiJ < uiNumKeys; uiJ++) {
				pfnCopy(pkNewEuler->m_apkKeys[uiI]->GetKeyAt(uiJ, pkNewEuler->m_ucSizes[uiI]),
					pkOrigEuler->m_apkKeys[uiI]->GetKeyAt(uiJ, pkOrigEuler->m_ucSizes[uiI]));
			}
		}
	}
}

NiAnimationKey* NiEulerRotKey::CreateArray(uint32_t uiNumKeys) {
	// EXE-INLINED none: NiNew NiEulerRotKey[uiNumKeys] (the overflow-clamped size, the count cookie,
	// MemoryManager::Allocate and the vector constructor iterator). The block is raw memory to the exe: a uint32_t
	// count cookie, then the keys.
	uint64_t ullBytes = static_cast<uint64_t>(uiNumKeys) * sizeof(NiEulerRotKey);
	uint32_t uiBytes = (ullBytes >> 32) != 0 ? 0xFFFFFFFFu : static_cast<uint32_t>(ullBytes);
	uint32_t uiAllocSize = uiBytes + sizeof(size_t) < uiBytes ? 0xFFFFFFFFu : uiBytes + sizeof(size_t);
	size_t* puiCookie = static_cast<size_t*>(MemoryManager_Singleton.Allocate(uiAllocSize));
	if (!puiCookie) {
		return nullptr;
	}
	*puiCookie = uiNumKeys;
	NiEulerRotKey* pkKeys = reinterpret_cast<NiEulerRotKey*>(puiCookie + 1);
	rt::eh_vector_constructor_iterator(pkKeys, sizeof(NiEulerRotKey), uiNumKeys,
		&fnvd::constructElement<NiEulerRotKey>);
	return pkKeys;
}

// Stores each angle's count and keys back and also clears its last interpolation index (SetKeys).
bool NiEulerRotKey::Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys) {
	if (uiNumKeys == 0) {
		// EXE-INLINED none: NiNew NiEulerRotKey[1] (the count cookie, MemoryManager::Allocate and the vector
		// constructor iterator); the exe's size is the folded constant. The block is raw memory to the exe: a
		// uint32_t count cookie, then the key.
		size_t* puiCookie =
			static_cast<size_t*>(MemoryManager_Singleton.Allocate(sizeof(NiEulerRotKey) + sizeof(size_t)));
		NiEulerRotKey* pkNewKeys;
		if (puiCookie) {
			*puiCookie = 1;
			pkNewKeys = reinterpret_cast<NiEulerRotKey*>(puiCookie + 1);
			rt::eh_vector_constructor_iterator(pkNewKeys, sizeof(NiEulerRotKey), 1,
				&fnvd::constructElement<NiEulerRotKey>);
		} else {
			pkNewKeys = nullptr;
		}
		pkKeys = pkNewKeys;
		uiNumKeys = 1;
	}

	NiEulerRotKey* pkEulerKey = static_cast<NiEulerRotKey*>(pkKeys);
	bool bInsertedKey = false;

	for (uint32_t i = 0; i < 3; i++) {
		NiAnimationKey* pkFloatKeys = pkEulerKey->m_apkKeys[i];
		uint32_t uiNumFloatKeys = pkEulerKey->m_uiNumKeys[i];
		if (uiNumFloatKeys == 0) {
			continue;
		}

		InsertFunction pfnInsert = GetInsertFunction(FLOATKEY, pkEulerKey->m_eType[i]);
		if (pfnInsert(fTime, pkFloatKeys, uiNumFloatKeys)) {
			bInsertedKey = true;
		}

		pkEulerKey->m_uiNumKeys[i] = uiNumFloatKeys;
		pkEulerKey->m_apkKeys[i] = static_cast<NiFloatKey*>(pkFloatKeys);
		pkEulerKey->m_uiLastIdx[i] = 0;
	}

	return bInsertedKey;
}

NiAnimationKey* NiEulerRotKey::CreateFromStream(NiStream& stream, uint32_t uiNumKeys) {
	// EXE-INLINED none: NiNew NiEulerRotKey[uiNumKeys], as in CreateArray (raw memory to the exe: the count cookie,
	// then the keys).
	uint64_t ullBytes = static_cast<uint64_t>(uiNumKeys) * sizeof(NiEulerRotKey);
	uint32_t uiBytes = (ullBytes >> 32) != 0 ? 0xFFFFFFFFu : static_cast<uint32_t>(ullBytes);
	uint32_t uiAllocSize = uiBytes + sizeof(size_t) < uiBytes ? 0xFFFFFFFFu : uiBytes + sizeof(size_t);
	size_t* puiCookie = static_cast<size_t*>(MemoryManager_Singleton.Allocate(uiAllocSize));
	NiEulerRotKey* pkEulerRotKeys;
	if (puiCookie) {
		*puiCookie = uiNumKeys;
		pkEulerRotKeys = reinterpret_cast<NiEulerRotKey*>(puiCookie + 1);
		rt::eh_vector_constructor_iterator(pkEulerRotKeys, sizeof(NiEulerRotKey), uiNumKeys,
			&fnvd::constructElement<NiEulerRotKey>);
	} else {
		pkEulerRotKeys = nullptr;
	}

	for (uint32_t ui = 0; ui < uiNumKeys; ui++) {
		pkEulerRotKeys[ui].LoadBinary(stream);
	}
	return pkEulerRotKeys;
}

// Clears the slot's curvature and derivative entries before it sets the guard, then stores its own functions'
// addresses.
int32_t NiEulerRotKey::RegisterLoader() {
	static char bRegistered;
	if (bRegistered) {
		return 0;
	}
	const uint32_t uiIndex = static_cast<uint32_t>(ROTKEY) * NUMKEYTYPES + EULERKEY;
	ms_curves[uiIndex] = nullptr;
	ms_interpd1s[uiIndex] = nullptr;
	ms_interpd2s[uiIndex] = nullptr;
	bRegistered = 1;
	ms_loaders[uiIndex] = &NiEulerRotKey::CreateFromStream;
	ms_saves[uiIndex] = &NiEulerRotKey::SaveToStream;
	ms_copies[uiIndex] = &NiEulerRotKey::Copy;
	ms_arrays[uiIndex] = &NiEulerRotKey::CreateArray;
	ms_deletes[uiIndex] = &NiStepPosKey::DeleteArray;
	ms_keysizes[uiIndex] = sizeof(NiEulerRotKey);
	ms_interps[uiIndex] = &NiEulerRotKey::Interpolate;
	ms_equals[uiIndex] = &NiEulerRotKey::Equal;
	ms_fillderived[uiIndex] = &NiEulerRotKey::FillDerivedVals;
	ms_inserts[uiIndex] = &NiEulerRotKey::Insert;
	return 1;
}

// pkKeys is the one Euler key; uiNumKeys and ucSize are unused. Tests the angle's key array, not its count.
void NiEulerRotKey::FillDerivedVals(NiAnimationKey* pkKeys, uint32_t uiNumKeys, uint8_t ucSize) {
	NiEulerRotKey* pkEulerKey = static_cast<NiEulerRotKey*>(pkKeys);
	for (uint32_t ui = 0; ui < 3; ui++) {
		if (pkEulerKey->m_apkKeys[ui]) {
			NiAnimationKey::GetFillDerivedFunction(NiAnimationKey::FLOATKEY,
				pkEulerKey->m_eType[ui])(pkEulerKey->m_apkKeys[ui], pkEulerKey->m_uiNumKeys[ui],
				pkEulerKey->m_ucSizes[ui]);
		}
	}
}
