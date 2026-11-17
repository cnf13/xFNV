#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"

#include <cstdint>

class NiStream;

// Base of one animation key: the time it sits at. Keys of one sequence sit back to back in an array whose stride
// depends on the key content and interpolation type.
class NiAnimationKey : public NiMemObject {
public:
	enum KeyContent {
		FLOATKEY,
		POSKEY,
		ROTKEY,
		COLORKEY,
		TEXTKEY,
		BOOLKEY,
		NUMKEYCONTENTS,
	};

	enum KeyType {
		NOINTERP,
		LINKEY,
		BEZKEY,
		TCBKEY,
		EULERKEY,
		STEPKEY,
		NUMKEYTYPES,
	};

	typedef void (*InterpFunction)(float fTime, const NiAnimationKey* pkKey0, const NiAnimationKey* pkKey1,
		void* pvResult);
	typedef NiAnimationKey* (*CreateFunction)(NiStream& kStream, uint32_t uiNumKeys);
	typedef void (*SaveFunction)(NiStream& kStream, NiAnimationKey* pkKeys, uint32_t uiNumKeys);
	typedef float (*CurvatureFunction)(NiAnimationKey* pkKeys, uint32_t uiNumKeys);
	typedef bool (*EqualFunction)(const NiAnimationKey& kKey0, const NiAnimationKey& kKey1);
	typedef void (*FillDerivedValsFunction)(NiAnimationKey* pkKeys, uint32_t uiNumKeys, uint8_t ucSize);
	typedef void (*DeleteFunction)(NiAnimationKey* pkKeyArray);
	// Insert takes the key array and its count by reference and returns false when the time is already taken.
	typedef bool (*InsertFunction)(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys);
	typedef NiAnimationKey* (*ArrayFunction)(uint32_t uiNumKeys);
	typedef void (*CopyFunction)(NiAnimationKey* pkNewKey, const NiAnimationKey* pkOrigKey);

	// Each table is indexed by content * NUMKEYTYPES + type and filled at startup.
	static constexpr uint32_t NUMKEYSLOTS = static_cast<uint32_t>(NUMKEYCONTENTS) * NUMKEYTYPES;
	typedef CurvatureFunction CurvatureFunctionTable[NUMKEYSLOTS];
	typedef uint8_t KeySizeTable[NUMKEYSLOTS];
	typedef FillDerivedValsFunction FillDerivedValsFunctionTable[NUMKEYSLOTS];
	typedef DeleteFunction DeleteFunctionTable[NUMKEYSLOTS];
	typedef SaveFunction SaveFunctionTable[NUMKEYSLOTS];
	typedef EqualFunction EqualFunctionTable[NUMKEYSLOTS];
	typedef InsertFunction InsertFunctionTable[NUMKEYSLOTS];
	typedef ArrayFunction ArrayFunctionTable[NUMKEYSLOTS];
	typedef CopyFunction CopyFunctionTable[NUMKEYSLOTS];
	typedef InterpFunction InterpFunctionTable[NUMKEYSLOTS];
	typedef CreateFunction CreateFunctionTable[NUMKEYSLOTS];

	void SetTime(float fTime) { m_fTime = fTime; }
	float GetTime() const { return m_fTime; }

	// The key uiIndex strides of ucKeySize bytes past this one.
	NiAnimationKey* GetKeyAt(uint32_t uiIndex, uint8_t ucKeySize) const {
		return reinterpret_cast<NiAnimationKey*>(reinterpret_cast<uintptr_t>(this) + uiIndex * ucKeySize);
	}

	void LoadBinary(NiStream& stream);

	static uint8_t GetKeySize(KeyContent eContent, KeyType eType) {
		return ms_keysizes[static_cast<uint32_t>(eContent) * NUMKEYTYPES + eType];
	}

	static CreateFunction GetCreateFunction(KeyContent eContent, KeyType eType) {
		return ms_loaders[static_cast<uint32_t>(eContent) * NUMKEYTYPES + eType];
	}

	static SaveFunction GetSaveFunction(KeyContent eContent, KeyType eType) {
		return ms_saves[static_cast<uint32_t>(eContent) * NUMKEYTYPES + eType];
	}

	static EqualFunction GetEqualFunction(KeyContent eContent, KeyType eType) {
		return ms_equals[static_cast<uint32_t>(eContent) * NUMKEYTYPES + eType];
	}

	static FillDerivedValsFunction GetFillDerivedFunction(KeyContent eContent, KeyType eType) {
		return ms_fillderived[static_cast<uint32_t>(eContent) * NUMKEYTYPES + eType];
	}

	static InsertFunction GetInsertFunction(KeyContent eContent, KeyType eType) {
		return ms_inserts[static_cast<uint32_t>(eContent) * NUMKEYTYPES + eType];
	}

	static ArrayFunction GetArrayFunction(KeyContent eContent, KeyType eType) {
		return ms_arrays[static_cast<uint32_t>(eContent) * NUMKEYTYPES + eType];
	}

	static CopyFunction GetCopyFunction(KeyContent eContent, KeyType eType) {
		return ms_copies[static_cast<uint32_t>(eContent) * NUMKEYTYPES + eType];
	}

	// Adds keys at fStartTime and fEndTime when the array lacks them, reallocating pkKeys and updating uiNumKeys.
	static void GuaranteeKeyAtStartAndEnd(KeyContent eContent, KeyType eType, NiAnimationKey*& pkKeys,
		uint32_t& uiNumKeys, float fStartTime, float fEndTime);
	// Whether a key may go in at fTime, and the index it goes in at.
	static bool OkayToInsert(float fTime, const NiAnimationKey* pkKeys, uint32_t uiNumKeys, uint32_t& uiInsertAt,
		uint8_t ucSize);
	// Copies the keys of eContent and eType that lie between fStartTime and fEndTime into a
	// new key array, which it returns through pkNewKeys with its count in uiNewNumKeys.
	static void CopySequence(KeyContent eContent, KeyType eType, const NiAnimationKey* pkOrigKeys,
		uint32_t uiOrigNumKeys, float fStartTime, float fEndTime, NiAnimationKey*& pkNewKeys, uint32_t& uiNewNumKeys);

	static DeleteFunction GetDeleteFunction(KeyContent eContent, KeyType eType) {
		return ms_deletes[static_cast<uint32_t>(eContent) * NUMKEYTYPES + eType];
	}

	static InterpFunction GetInterpFunction(KeyContent eContent, KeyType eType);

	static CurvatureFunction GetCurvatureFunction(KeyContent eContent, KeyType eType) {
		return ms_curves[static_cast<uint32_t>(eContent) * NUMKEYTYPES + eType];
	}

	static InterpFunction GetInterpD1Function(KeyContent eContent, KeyType eType) {
		return ms_interpd1s[static_cast<uint32_t>(eContent) * NUMKEYTYPES + eType];
	}

	static InterpFunction GetInterpD2Function(KeyContent eContent, KeyType eType) {
		return ms_interpd2s[static_cast<uint32_t>(eContent) * NUMKEYTYPES + eType];
	}

	// NiLinFloatKey's CreateFunction: reads uiNumKeys 8-byte float keys after a count cookie, each read by
	// NiFloatKey::LoadBinary.
	static NiAnimationKey* CreateFunction_00A2A5A0(NiStream& arStream, uint32_t auiCount);
	// Writes uiNumKeys 8-byte float keys, one NiFloatKey::SaveBinary (NiFloatKey::SaveBinary) each.
	static void SaveToStream(NiStream& stream, NiAnimationKey* pkKeys, uint32_t uiNumKeys);
	// Writes pkKey's m_fTime; each key type's SaveBinary starts with
	// it.
	static void SaveBinary(NiStream& stream, NiAnimationKey* pkKey);
	static int32_t Equal(const float* a1, const float* a2);

	float m_fTime;

	static inline KeySizeTable ms_keysizes{};
	static inline FillDerivedValsFunctionTable ms_fillderived{};
	static inline DeleteFunctionTable ms_deletes{};
	static inline SaveFunctionTable ms_saves{};
	static inline EqualFunctionTable ms_equals{};
	static inline CurvatureFunctionTable ms_curves{};
	static inline CreateFunctionTable ms_loaders{};
	static inline InsertFunctionTable ms_inserts{};
	static inline ArrayFunctionTable ms_arrays{};
	static inline CopyFunctionTable ms_copies{};
	static inline InterpFunctionTable ms_interps{};
	// The derivative interpolation functions; NiPathInterpolator::PathSpeed reads the POSKEY row.
	static inline InterpFunctionTable ms_interpd1s{};
	// The second-derivative table, NUMKEYSLOTS entries (NiTCBPosKey's RegisterSupportedFunctions stores InterpolateD2
	// there).
	static inline InterpFunctionTable ms_interpd2s{};
};

inline NiAnimationKey::InterpFunction NiAnimationKey::GetInterpFunction(KeyContent eContent, KeyType eType) {
	return ms_interps[static_cast<uint32_t>(eContent) * NUMKEYTYPES + eType];
}
