#pragma once

#include "BSHavok/bhkConstraintChain.h"
#include "Fallout/AI/Combat/MissileProjectile.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

class hkpEntity;

// The creation info of a constraint chain: hkConstraintCinfo plus the chain's entities. Its destructor frees EntitiesA.
struct hkConstraintChainCinfo : public hkConstraintCinfo {
	hkConstraintChainCinfo();
	~hkConstraintChainCinfo() override;

	void LoadBinary(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	// CreateData is pure here: each chain's cinfo creates its own data.
	uint32_t CreateData() override = 0;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;

	hkArray<hkpEntity*> EntitiesA;
};

// The creation info of a ball-and-socket chain: the chain's entities and one pivot per link.
struct hkBallSocketChainCinfo : public hkConstraintChainCinfo {
	hkBallSocketChainCinfo();
	~hkBallSocketChainCinfo() override;

	void CloneData(hkpConstraintData* apData, NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	uint32_t CreateData() override;
	// Only calls the base class's GetViewerStrings.
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
	// Factory: returns a new chain with no creation info yet, or null when the allocation fails.
	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
	// Number of live instances; the constructors count them.
	static inline uint32_t iCount;
};
