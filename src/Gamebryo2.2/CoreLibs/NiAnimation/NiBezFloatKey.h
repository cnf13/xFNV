#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatKey.h"

class NiStream;

// Fills NiAnimationKey's tables for the FLOATKEY/BEZKEY slot once; returns 1 the first time, else 0.
int32_t NiBezFloatKey_RegisterLoader();

// A Bezier float key: the time and value of NiFloatKey and the incoming and outgoing tangents. The static functions
// are protected in the engine; public here so the replacement table can name them.
class NiBezFloatKey : public NiFloatKey {
public:
	// The key loader (NiAnimationKey::ms_loaders[2]): uiNumKeys keys in a new[] block, each read from stream.
	static NiAnimationKey* CreateFromStream(NiStream& stream, uint32_t uiNumKeys);
	// Nothing to derive.
	static void FillDerivedVals(NiAnimationKey*, uint32_t, uint8_t) {}
	// The table's InterpFunction: the Bezier value between pkKey0 and pkKey1 at fTime, stored at pResult.
	static void Interpolate(float fTime, const NiAnimationKey* pkKey0, const NiAnimationKey* pkKey1, void* pResult);
	// CreateArray is the table's ArrayFunction: it allocates uiNumKeys 16-byte keys after a count cookie, from
	// MemoryManager. NiLinPosKey's table entry holds it too (same size).
	static NiAnimationKey* CreateArray(uint32_t uiNumKeys);
	// The table's InsertFunction: inserts a key at fTime between its neighbours, false when OkayToInsert refuses.
	static bool Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys);
	// The table's SaveFunction: writes each key's NiFloatKey part, then its two tangents.
	static void SaveToStream(NiStream& kStream, NiAnimationKey* pkKeys, uint32_t uiNumKeys);
	// NiFloatKey::Equal, then both tangents.
	static bool Equal(const NiAnimationKey& key0, const NiAnimationKey& key1);
	// The key's time, value and tangents.
	static void Copy(NiAnimationKey* pkNewKey, const NiAnimationKey* pkOrigKey);

	float m_fInTan;
	float m_fOutTan;
};

inline char NiBezFloatKey_RegisterLoader_bRegistered;

// NiRegisterAnimationStream(NiBezFloatKey): a static of every file that includes this header, as the game does.
static int32_t NiBezFloatKeyregisterloaderreturn = NiBezFloatKey_RegisterLoader();
