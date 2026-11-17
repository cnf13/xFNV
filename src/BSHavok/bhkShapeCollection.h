#pragma once

#include "BSHavok/bhkShape.h"
#include "Reconstruction.hpp"

class hkpShapeCollection;

// The Gamebryo side of a shape made of child shapes, each reached through a key (a list shape, a mesh shape).
class bhkShapeCollection : public bhkShape {
public:
	bhkShapeCollection() { ++iCount; }
	~bhkShapeCollection() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	bhkShapeCollection* FindShapeCollection() override { return this; }
	bool CanShare(NiCloningProcess& arCloneData) override;
	void BuildDisplayGeometry(NiNode* apNode, const NiColor& arDefColor) override;

	// The material of the child shape with key auiKey (the PDB's QSubMaterial; IDA's vtable names it GetSubMaterial).
	virtual HavokMaterialType QSubMaterial(uint32_t auiKey) = 0;

	// The Havok shape, or null for a null this (the exe's shared null-safe getter, thiscall,
	// ret 0, 0x4AE750).
	hkpShapeCollection* operator*() const;
	// The Havok collection's first child key, and the key after auiKey.
	uint32_t GetFirstKey();
	uint32_t GetNextKey(uint32_t aiOldKey);

	static bhkShape* Getbhk(hkpShape* a1);

	static NiRTTI ms_RTTI;
	static inline uint32_t iCount;
};

inline constexpr fnvd::Vtable bhkShapeCollection_vftable{};
