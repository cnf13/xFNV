#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatKey.h"
#include "Reconstruction.hpp"

class NiStream;

// Named here after its address; it is the PDB's NiBezFloatKey::RegisterLoader, with RegisterSupportedFunctions
// inlined: fills NiAnimationKey's tables for the FLOATKEY/BEZKEY slot once; 1 the first time, else 0.
int32_t NiBezFloatKey_RegisterLoader();

// A Bezier float key: the time and value of NiFloatKey and the incoming and outgoing tangents. The static functions
// are protected in the engine; public here so the replacement table can name them.
class NiBezFloatKey : public NiFloatKey {
public:
	// The key loader (NiAnimationKey::ms_loaders[2], cdecl, 0xA27250): uiNumKeys keys in a new[] block, each read
	// from kStream.
	static NiAnimationKey* CreateFromStream(NiStream& stream, uint32_t uiNumKeys);
	// The PDB's NiBezFloatKey::FillDerivedVals: nothing to derive.
	static void FillDerivedVals(NiAnimationKey*, uint32_t, uint8_t) {}
	// Named here after its address; it is the PDB's NiBezFloatKey::Interpolate (the table's InterpFunction): the Bezier
	// value between pkKey0 and pkKey1 at fTime, stored at pResult. The exe leaves pResult in eax; the table's callers
	// ignore it.
	static void Interpolate(float fTime, const NiAnimationKey* pkKey0, const NiAnimationKey* pkKey1, void* pResult);
	// Not named in the PDB or IDA; name chosen here. The PDB's NiBezFloatKey::CreateArray (the table's ArrayFunction):
	// uiNumKeys 16-byte keys after a count cookie, from MemoryManager. NiLinPosKey's slot holds it too (same size).
	static NiAnimationKey* CreateArray(uint32_t uiNumKeys);
	// Named here after its address; it is the PDB's NiBezFloatKey::Insert (the table's InsertFunction): a key at fTime
	// between its neighbours, false when OkayToInsert refuses.
	static bool Insert(float fTime, NiAnimationKey*& pkKeys, uint32_t& uiNumKeys);
	// Named here after its address; it is the PDB's NiBezFloatKey::SaveToStream (the table's SaveFunction): each key's
	// NiFloatKey part, then its two tangents.
	static void SaveToStream(NiStream& kStream, NiAnimationKey* pkKeys, uint32_t uiNumKeys);
	// NiFloatKey::Equal, then both tangents.
	static bool Equal(const NiAnimationKey& key0, const NiAnimationKey& key1);
	// The key's time, value and tangents.
	static void Copy(NiAnimationKey* pkNewKey, const NiAnimationKey* pkOrigKey);

	float m_fInTan;
	float m_fOutTan;
};

inline char NiBezFloatKey_RegisterLoader_bRegistered;

// NiRegisterAnimationStream(NiBezFloatKey): a static of every file that includes this header, as in the original.
static int32_t NiBezFloatKeyregisterloaderreturn = NiBezFloatKey_RegisterLoader();
