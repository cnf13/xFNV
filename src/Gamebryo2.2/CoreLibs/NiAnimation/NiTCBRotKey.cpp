#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTCBRotKey.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBezFloatKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBezRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/string.h"

#include <stdint.h>

void NiTCBRotKey::CalculateDVals(const NiTCBRotKey* pPrev, const NiTCBRotKey* pNext) {
	NiQuaternion v32;
	NiQuaternion result;
	NiQuaternion a1;
	NiQuaternion v37;
	NiQuaternion* p_m_quat = &m_quat;
	NiQuaternion* v17 = &m_quat;
	NiQuaternion* v5 = ::new (&result) NiQuaternion(NiQuaternion::UnitInverse(pPrev->m_quat));
	NiQuaternion* v6 = ::new (&v32) NiQuaternion(*v5 * *v17);
	NiQuaternion logPrev = NiQuaternion::Log(*v6);
	NiQuaternion* v7 = ::new (&v32) NiQuaternion(NiQuaternion::UnitInverse(*p_m_quat));
	NiQuaternion* v8 = ::new (&result) NiQuaternion(*v7 * pNext->m_quat);
	NiQuaternion logNext = NiQuaternion::Log(*v8);
	float fInv = 1.0 / (pNext->GetTime() - pPrev->GetTime());
	float mt = 1.0 - m_fTension;
	float v28 = 1.0 - m_fContinuity;
	float v29 = m_fContinuity + 1.0;
	float v24 = 1.0 - m_fBias;
	float v26 = m_fBias + 1.0;
	float v20 = (this->GetTime() - pPrev->GetTime()) * fInv;
	// This product is kept as a double and read back for the second factor.
	double dScaled = v20 * mt;
	float arg4 = dScaled * v29 * v26;
	NiQuaternion* v18 = ::new (&result) NiQuaternion(arg4 * logPrev);
	float v22 = v28 * dScaled * v24;
	NiQuaternion* v9 = ::new (&a1) NiQuaternion(v22 * logNext);
	NiQuaternion Tout = *v9 + *v18;
	NiQuaternion* v10 = ::new (&a1) NiQuaternion(Tout - logNext);
	NiQuaternion* v11 = ::new (&result) NiQuaternion(0.5f * *v10);
	NiQuaternion* v12 = ::new (&v32) NiQuaternion(NiQuaternion::Exp(*v11));
	m_A = *::new (&v37) NiQuaternion(*p_m_quat * *v12);
	float v23 = (pNext->GetTime() - this->GetTime()) * fInv;
	dScaled = v23 * mt;
	float v27 = dScaled * v28 * v26;
	NiQuaternion* v19 = ::new (&v37) NiQuaternion(v27 * logPrev);
	float v25 = v29 * dScaled * v24;
	NiQuaternion* v13 = ::new (&a1) NiQuaternion(v25 * logNext);
	NiQuaternion Tin = *v13 + *v19;
	NiQuaternion* v14 = ::new (&v37) NiQuaternion(logPrev - Tin);
	NiQuaternion* v15 = ::new (&a1) NiQuaternion(0.5f * *v14);
	NiQuaternion* v16 = ::new (&result) NiQuaternion(NiQuaternion::Exp(*v15));
	m_B = *::new (&v32) NiQuaternion(*p_m_quat * *v16);
}

void NiTCBRotKey::LoadBinary(NiStream& stream) {
	uint32_t v10;
	NiStream* v2 = &stream;
	NiRotKey::LoadBinary(stream);
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

void NiTCBRotKey::Interpolate(float fTime, const NiAnimationKey* pKey0, const NiAnimationKey* pKey1, void* pResult) {
	const NiTCBRotKey* pTCB0 = static_cast<const NiTCBRotKey*>(pKey0);
	const NiTCBRotKey* pTCB1 = static_cast<const NiTCBRotKey*>(pKey1);
	static_cast<NiQuaternion*>(pResult)->Squad(fTime, pTCB0->m_quat, pTCB0->m_A, pTCB1->m_B, pTCB1->m_quat);
}

// NiRotKey::Equal, then the TCB parameters and the two tangent quaternions.
bool NiTCBRotKey::Equal(const NiAnimationKey& key0, const NiAnimationKey& key1) {
	if (!NiRotKey::Equal(key0, key1)) {
		return false;
	}
	const NiTCBRotKey& tcb0 = static_cast<const NiTCBRotKey&>(key0);
	const NiTCBRotKey& tcb1 = static_cast<const NiTCBRotKey&>(key1);
	return tcb0.m_fTension == tcb1.m_fTension && tcb0.m_fContinuity == tcb1.m_fContinuity &&
		tcb0.m_fBias == tcb1.m_fBias && tcb0.m_A == tcb1.m_A && tcb0.m_B == tcb1.m_B;
}

// The ms_saves entry for ROTKEY/TCBKEY reaches this: NiRotKey::SaveBinary, then the three TCB floats.
void NiTCBRotKey::SaveBinary(NiStream& stream, NiAnimationKey* pkKey) {
	uint32_t v9;
	NiTCBRotKey* pkTCBKey = static_cast<NiTCBRotKey*>(pkKey);
	NiRotKey::SaveBinary(stream, pkKey);
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

void NiTCBRotKey::SaveToStream(NiStream& stream, NiAnimationKey* pkKeys, uint32_t uiNumKeys) {
	NiTCBRotKey* pkTCBRotKeys = static_cast<NiTCBRotKey*>(pkKeys);

	for (uint32_t ui = 0; ui < uiNumKeys; ui++)
		SaveBinary(stream, &pkTCBRotKeys[ui]);
}

void NiTCBRotKey::Copy(NiAnimationKey* pkNewKey, const NiAnimationKey* pkOrigKey) {
	NiTCBRotKey* pkNewTCB = static_cast<NiTCBRotKey*>(pkNewKey);
	const NiTCBRotKey* pkOrigTCB = static_cast<const NiTCBRotKey*>(pkOrigKey);

	pkNewTCB->m_fTime = pkOrigTCB->m_fTime;
	pkNewTCB->m_quat = pkOrigTCB->m_quat;
	pkNewTCB->m_fTension = pkOrigTCB->m_fTension;
	pkNewTCB->m_fContinuity = pkOrigTCB->m_fContinuity;
	pkNewTCB->m_fBias = pkOrigTCB->m_fBias;
	pkNewTCB->m_A = pkOrigTCB->m_A;
	pkNewTCB->m_B = pkOrigTCB->m_B;
}

// Stores this class's functions into the ROTKEY/TCBKEY slot of each table, as the engine's RegisterSupportedFunctions
// does; the curvature and derivative entries are cleared.
int32_t NiTCBRotKey::RegisterLoader() {
	static char bRegistered;
	if (bRegistered) {
		return 0;
	}
	ms_curves[static_cast<uint32_t>(ROTKEY) * NUMKEYTYPES + TCBKEY] = nullptr;
	ms_interpd1s[static_cast<uint32_t>(ROTKEY) * NUMKEYTYPES + TCBKEY] = nullptr;
	ms_interpd2s[static_cast<uint32_t>(ROTKEY) * NUMKEYTYPES + TCBKEY] = nullptr;
	bRegistered = 1;
	ms_loaders[static_cast<uint32_t>(ROTKEY) * NUMKEYTYPES + TCBKEY] = &NiTCBRotKey::CreateFromStream;
	ms_saves[static_cast<uint32_t>(ROTKEY) * NUMKEYTYPES + TCBKEY] = &NiTCBRotKey::SaveToStream;
	ms_copies[static_cast<uint32_t>(ROTKEY) * NUMKEYTYPES + TCBKEY] = &NiTCBRotKey::Copy;
	ms_arrays[static_cast<uint32_t>(ROTKEY) * NUMKEYTYPES + TCBKEY] = &NiTCBRotKey::CreateArray;
	ms_deletes[static_cast<uint32_t>(ROTKEY) * NUMKEYTYPES + TCBKEY] = &NiStepPosKey::DeleteArray;
	ms_keysizes[static_cast<uint32_t>(ROTKEY) * NUMKEYTYPES + TCBKEY] = sizeof(NiTCBRotKey);
	ms_interps[static_cast<uint32_t>(ROTKEY) * NUMKEYTYPES + TCBKEY] = &NiTCBRotKey::Interpolate;
	ms_equals[static_cast<uint32_t>(ROTKEY) * NUMKEYTYPES + TCBKEY] = &NiTCBRotKey::Equal;
	ms_fillderived[static_cast<uint32_t>(ROTKEY) * NUMKEYTYPES + TCBKEY] = &NiTCBRotKey::FillDerivedVals;
	ms_inserts[static_cast<uint32_t>(ROTKEY) * NUMKEYTYPES + TCBKEY] = &NiTCBRotKey::Insert;
	return 1;
}

// NiNew NiTCBRotKey[uiNumKeys]: allocates through MemoryManager with an untyped count cookie before the keys (the cast
// below steps over it), and each key is built through the vector constructor iterator.
NiAnimationKey* NiTCBRotKey::CreateArray(uint32_t uiNumKeys) {
	size_t uiSize = uiNumKeys <= SIZE_MAX / sizeof(NiTCBRotKey) ? sizeof(NiTCBRotKey) * uiNumKeys : SIZE_MAX;
	size_t* puiCookie =
		static_cast<size_t*>(MemoryManager_Singleton.Allocate(uiSize + static_cast<uint32_t>(sizeof(size_t)) < uiSize
				? -1
				: uiSize + sizeof(size_t)));
	if (!puiCookie) {
		return nullptr;
	}
	*puiCookie = uiNumKeys;
	NiTCBRotKey* pkKeys = reinterpret_cast<NiTCBRotKey*>(puiCookie + 1);
	rt::eh_vector_constructor_iterator(pkKeys, sizeof(NiTCBRotKey), static_cast<int32_t>(uiNumKeys),
		&fnvd::constructElement<NiBezRotKey>);
	return pkKeys;
}

// Allocates NiNew NiTCBRotKey[uiNumKeys + 1] and releases the old array with NiDelete[] by hand: through MemoryManager,
// with an untyped count cookie before the keys (the casts below step over it).
bool NiTCBRotKey::Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys) {
	NiTCBRotKey* pkOldKeys = static_cast<NiTCBRotKey*>(pkKeys);
	uint32_t uiInsertAt;
	if (!NiAnimationKey::OkayToInsert(fTime, pkOldKeys, uiNumKeys, uiInsertAt, sizeof(NiTCBRotKey))) {
		return false;
	}
	uint32_t uiNewNumKeys = uiNumKeys + 1;
	size_t uiSize = uiNewNumKeys <= SIZE_MAX / sizeof(NiTCBRotKey) ? sizeof(NiTCBRotKey) * uiNewNumKeys : SIZE_MAX;
	size_t* puiCookie =
		static_cast<size_t*>(MemoryManager_Singleton.Allocate(uiSize + static_cast<uint32_t>(sizeof(size_t)) < uiSize
				? -1
				: uiSize + sizeof(size_t)));
	NiTCBRotKey* pkNewKeys;
	if (puiCookie) {
		*puiCookie = uiNewNumKeys;
		pkNewKeys = reinterpret_cast<NiTCBRotKey*>(puiCookie + 1);
		rt::eh_vector_constructor_iterator(pkNewKeys, sizeof(NiTCBRotKey), static_cast<int32_t>(uiNewNumKeys),
			&fnvd::constructElement<NiBezRotKey>);
	} else {
		pkNewKeys = nullptr;
	}
	rt::memcpy(pkNewKeys, pkOldKeys, uiInsertAt * sizeof(NiTCBRotKey));
	if (uiNumKeys > uiInsertAt) {
		rt::memcpy(&pkNewKeys[uiInsertAt + 1], &pkOldKeys[uiInsertAt], (uiNumKeys - uiInsertAt) * sizeof(NiTCBRotKey));
	}
	NiQuaternion kQuat =
		NiRotKey::GenInterpDefault(fTime, pkOldKeys, NiAnimationKey::TCBKEY, uiNumKeys, sizeof(NiTCBRotKey));
	NiTCBRotKey* pkNewKey = &pkNewKeys[uiInsertAt];
	pkNewKey->m_fTime = fTime;
	pkNewKey->m_quat = kQuat;
	pkNewKey->m_fTension = 0.0f;
	pkNewKey->m_fContinuity = 0.0f;
	pkNewKey->m_fBias = 0.0f;
	++uiNumKeys;
	if (pkOldKeys) {
		MemoryManager_Singleton.Deallocate(reinterpret_cast<size_t*>(pkOldKeys) - 1);
	}
	pkKeys = pkNewKeys;
	FillDerivedVals(pkNewKeys, uiNumKeys, sizeof(NiTCBRotKey));
	return true;
}

// Allocates the keys as NiTCBRotKey::CreateArray does for NiNew NiTCBRotKey[uiNumKeys], with an untyped count cookie
// before the keys; each key is then read by its LoadBinary.
NiAnimationKey* NiTCBRotKey::CreateFromStream(NiStream& kStream, uint32_t uiNumKeys) {
	size_t uiSize = uiNumKeys <= SIZE_MAX / sizeof(NiTCBRotKey) ? sizeof(NiTCBRotKey) * uiNumKeys : SIZE_MAX;
	size_t* puiCookie =
		static_cast<size_t*>(MemoryManager_Singleton.Allocate(uiSize + static_cast<uint32_t>(sizeof(size_t)) < uiSize
				? -1
				: uiSize + sizeof(size_t)));
	NiTCBRotKey* pkKeys;
	if (puiCookie) {
		*puiCookie = uiNumKeys;
		pkKeys = reinterpret_cast<NiTCBRotKey*>(puiCookie + 1);
		rt::eh_vector_constructor_iterator(pkKeys, sizeof(NiTCBRotKey), static_cast<int32_t>(uiNumKeys),
			&fnvd::constructElement<NiBezRotKey>);
	} else {
		pkKeys = nullptr;
	}
	for (uint32_t ui = 0; ui < uiNumKeys; ++ui) {
		pkKeys[ui].LoadBinary(kStream);
	}
	return pkKeys;
}

void NiTCBRotKey::FillDerivedVals(NiAnimationKey* pkKeys, uint32_t uiNumKeys, uint8_t ucSize) {
	uint32_t v3 = uiNumKeys;
	if (uiNumKeys >= 2) {
		NiRotKey::FillDerivedVals(pkKeys, uiNumKeys, ucSize);
		static_cast<NiTCBRotKey*>(pkKeys)->NiTCBRotKey::CalculateDVals(static_cast<NiTCBRotKey*>(pkKeys),
			static_cast<NiTCBRotKey*>(&pkKeys[16]));
		if (uiNumKeys - 1 > 1) {
			NiTCBRotKey* v4 = static_cast<NiTCBRotKey*>(pkKeys);
			uint32_t v5 = uiNumKeys - 2;
			do {
				NiTCBRotKey* v6 = v4 + 1;
				(v4 + 1)->CalculateDVals(v4, v4 + 2);
				--v5;
				v4 = v6;
			} while (v5);
			v3 = uiNumKeys;
		}
		static_cast<NiTCBRotKey*>(&pkKeys[16 * uiNumKeys - 16])
			->NiTCBRotKey::CalculateDVals(static_cast<NiTCBRotKey*>(&pkKeys[16 * v3 - 32]),
				static_cast<NiTCBRotKey*>(&pkKeys[16 * uiNumKeys - 16]));
	}
}

NiQuaternion operator*(float fScalar, const NiQuaternion& kQ) {
	float fZ = kQ.m_fZ * fScalar;
	float fY = kQ.m_fY * fScalar;
	float fX = kQ.m_fX * fScalar;
	float fW = fScalar * kQ.m_fW;
	NiQuaternion kResult;
	kResult.SetValues(fW, fX, fY, fZ);
	return kResult;
}
