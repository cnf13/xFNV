#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"

#include <stdint.h>

class NiStream;

// A tension/continuity/bias position key: the time and position of NiPosKey, the three TCB parameters, the two
// derived tangents (m_DS, m_DD) and the cubic coefficients (m_A, m_B) FillDerivedVals computes. The static functions
// are protected in the engine; public here so the replacement table can name them.
class NiTCBPosKey : public NiPosKey {
public:
	void CalculateDVals(const NiPoint3& ptSub1, const NiPoint3& ptPlus1, float fPreLen, float fNextLen);
	void LoadBinary(NiStream& stream);

	// Copies the time, position, the TCB parameters, both tangents and both coefficients (ms_copies entry).
	static void Copy(NiAnimationKey* pkNewKey, const NiAnimationKey* pkOrigKey);
	// NiPosKey::Equal, then the TCB parameters, both tangents and both coefficients.
	static bool Equal(const NiAnimationKey& key0, const NiAnimationKey& key1);
	// Writes uiNumKeys keys to kStream, one SaveBinary each.
	static void SaveToStream(NiStream& stream, NiAnimationKey* pkKeys, uint32_t uiNumKeys);
	// Writes one key: the NiPosKey part, then tension, continuity and bias.
	static void SaveBinary(NiStream& stream, NiAnimationKey* pkKey);
	// The largest |d1 x d2| / |d1|^2 over ms_iSampleSize + 1 samples of each key interval.
	static float ComputeMaxCurvature(NiAnimationKey* pkKeys, uint32_t uiNumKeys);
	// The first derivative at fTime (0..1) of the interval starting at pKey0, written to
	// pResult as an NiPoint3.
	static void InterpolateD1(float fTime, const NiAnimationKey* pKey0, const NiAnimationKey* pKey1, void* pResult);
	// The second derivative, as InterpolateD1.
	static void InterpolateD2(float fTime, const NiAnimationKey* pKey0, const NiAnimationKey* pKey1, void* pResult);
	// Inserts a key at fTime (position from GenInterpDefault, zero tension, continuity and bias) into a new array
	// of uiNumKeys + 1 keys, frees the old one and recomputes the derived values; false when OkayToInsert refuses.
	static bool Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys);
	// Allocates uiNumKeys keys (NiNew[]) and loads each from stream.
	static NiAnimationKey* CreateFromStream(NiStream& stream, uint32_t uiNumKeys);
	// Fills this key type's slot of each function table NiAnimationKey keeps.
	static void RegisterSupportedFunctions(KeyContent eContent, KeyType eType);
	// Fills each key's tangents (CalculateDVals; the end keys get mirrored neighbours and unit interval lengths),
	// then each interval's cubic coefficients m_A and m_B. ucSize is unused.
	static void FillDerivedVals(NiAnimationKey* pkKeys, uint32_t uiNumKeys, uint8_t ucSize);
	// Fills the POSKEY/TCBKEY slot of the loader, save, copy, array, delete and key-size tables once, then the rest
	// through RegisterSupportedFunctions; 1 on the first call, else 0.
	static int32_t RegisterLoader();
	// Allocates uiNumKeys keys after a count cookie, from MemoryManager.
	static NiAnimationKey* CreateArray(uint32_t uiNumKeys);
	// Evaluates pkKey0's cubic at fTime and writes it to pResult.
	static void Interpolate(float fTime, const NiAnimationKey* pkKey0, const NiAnimationKey* pkKey1, void* pResult);

	float m_fTension;
	float m_fContinuity;
	float m_fBias;
	NiPoint3 m_DS;
	NiPoint3 m_DD;
	NiPoint3 m_A;
	NiPoint3 m_B;
};

// NiRegisterAnimationStream(NiTCBPosKey): a static of every file that includes this header, as the game does.
static int32_t NiTCBPosKeyregisterloaderreturn = NiTCBPosKey::RegisterLoader();
