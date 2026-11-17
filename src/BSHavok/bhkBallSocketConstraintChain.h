#pragma once

#include "Fallout/AI/Combat/MissileProjectile.h"
#include "BSHavok/bhkConstraintChain.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Reconstruction.hpp"

class hkpEntity;

// The creation info of a constraint chain: hkConstraintCinfo plus the chain's entities. The PDB defines it in
// bhkBallSocketConstraintChain.cpp. The exe has no vtable for it (RTTI only): its destructor 0xCC9410 frees
// EntitiesA and stores hkConstraintCinfo's vtable without storing one of its own.
struct hkConstraintChainCinfo : public hkConstraintCinfo {
	hkConstraintChainCinfo();
	~hkConstraintChainCinfo() override;

	void LoadBinary(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	// The exe has no vtable of this class and no body for it: each chain's cinfo creates its own data.
	uint32_t CreateData() override = 0;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;

	hkArray<hkpEntity*> EntitiesA;
};

// The creation info of a ball-and-socket chain: the chain's entities and one pivot per link. CreateCinfo 0xCC9560
// allocates 0x2C bytes and inlines the constructor. The exe's vtable 0x10CA22C overrides all six slots;
// GetViewerStrings 0xCC8CD0 jumps to hkBallAndSocketConstraintCinfo's.
struct hkBallSocketChainCinfo : public hkConstraintChainCinfo {
	hkBallSocketChainCinfo();
	~hkBallSocketChainCinfo() override;

	void CloneData(hkpConstraintData* apData, NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	uint32_t CreateData() override;
	// The exe's 0xCC8CD0 (thiscall, ret 4) only jumps to the base's (0xCC5080); hkFixedConstraintCinfo's vtable
	// holds the same address.
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;

	static void operator delete(void* apMemory) { MemoryManager_Singleton.Deallocate(apMemory); }

	hkArray<hkVector4> PivotsA;
};

// A chain of ball-and-socket constraints. It adds no fields to bhkConstraintChain.
class bhkBallSocketConstraintChain : public bhkConstraintChain {
public:
	bhkBallSocketConstraintChain();
	~bhkBallSocketConstraintChain() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	uint32_t QCinfoSize() const override;
	// A new hkBallSocketChainCinfo, filled from the wrapped chain when there is one.
	hkSerializableCinfo& CreateCinfo(bool& arCreated) override;

protected:
	// The chain's members, then two pivots per link of the chain data. Not virtual; it
	// takes the cinfo object's own address, as bhkConstraintChain::CopyCinfo does.
	void CopyCinfo(hkBallSocketChainCinfo* arInfo);

public:
	// The exe's factory: a new chain with no creation info yet, or null when the allocation fails (cdecl, ret 0,
	// 0xCC8DC0).
	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
	// Number of live instances; the constructors count them.
	static inline uint32_t iCount;
};

inline constexpr fnvd::Vtable hkBallSocketChainCinfo_vftable{};
inline constexpr fnvd::Vtable bhkBallSocketConstraintChain_vftable{};
