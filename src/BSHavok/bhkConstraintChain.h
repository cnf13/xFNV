#pragma once

#include "BSHavok/bhkConstraint.h"
#include "Reconstruction.hpp"

struct hkConstraintChainCinfo;

// The Gamebryo side of a chain of constraints. Only its virtuals are declared; CreateCinfo stays pure, as in the exe.
class bhkConstraintChain : public bhkConstraint {
public:
	bhkConstraintChain();
	~bhkConstraintChain() override;

	const NiRTTI* GetRTTI() const override;
	// The exe shares the body with bhkBallAndSocketConstraint's (thiscall, ret 4, 0xCA98D0).
	void LoadBinary(NiStream& arStream) override;
	// Links the chain cinfo's entities from the stream's link IDs, then bhkConstraint's.
	void LinkObject(NiStream& arStream) override;
	// Registers each chained entity's wrapper, then bhkConstraint's part.
	bool RegisterStreamables(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	// bhkConstraint's test, then the same chained entities in the same order.
	bool IsEqual(NiObject* apObject) override;
	bool Add(bhkWorld* apWorld) override;
	// Takes the chain out of its bhkWorld, then its entities.
	bool Remove() override;
	// The exe shares the body with bhkHingeConstraint's (thiscall, ret 4, 0xCC9AF0).
	void KillCinfo(bool abCreated) override;
	uint32_t QCinfoSize() const override;
	// Builds the chain instance from the creation info and wraps it.
	void Init(const hkSerializableCinfo& arInfo) override;
	// Adds the chain's entities to apWorld or removes them.
	virtual bool ProcessEntities(bhkWorld* apWorld, bool abAdd);

	// Copies the chain's own members into apDestination.
	void CopyMembers(bhkSerializable* pDest, NiCloningProcess& arCloneData);

protected:
	// Copies the constraint's members into the creation info, then the chain instance's entities (thiscall, ret 4,
	// 0xD53900). Not virtual; it takes the cinfo object's own address, so the PDB's reference is a pointer here.
	void CopyCinfo(hkConstraintChainCinfo* arInfo);

public:
	static NiRTTI ms_RTTI;
	// Number of live instances; the constructors count them.
	static inline uint32_t iCount;
};

inline constexpr fnvd::Vtable bhkConstraintChain_vftable{};
