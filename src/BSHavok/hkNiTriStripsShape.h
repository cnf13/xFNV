#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTLargeArray.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"

#include <stdint.h>

class hkNiTriStripsShapeCinfo;
class NiTriStripsData;

// One sub part of an hkNiTriStripsShape: the triangle-strip geometry and its collision filter info.
class hkNiTriStripsData {
public:
	// Member-wise copy (the NiPointer assignment, then iFilter), which NiTLargeArray<hkNiTriStripsData>::SetSize calls.
	hkNiTriStripsData& operator=(const hkNiTriStripsData& arOther) = default;

	bool operator==(const hkNiTriStripsData& arSource) const {
		return spGeom == arSource.spGeom && iFilter == arSource.iFilter;
	}

	bool operator!=(const hkNiTriStripsData& arSource) const { return !(*this == arSource); }

	NiPointer<NiTriStripsData> spGeom;
	uint32_t iFilter;
};

using NiTLargeArrayhkNiTriStripsData = NiTLargeArray<hkNiTriStripsData, NiTMallocInterface<hkNiTriStripsData>>;

// A Havok shape collection over Gamebryo triangle strips: the sub parts with their scale and radius, and the
// world box last computed for them (CalcWorldAabb).
class hkNiTriStripsShape : public hkpShapeCollection {
public:
	// Builds the shape from its creation info and returns it.
	hkNiTriStripsShape(const hkNiTriStripsShapeCinfo& arInfo);
	~hkNiTriStripsShape() override;

	void getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const override;
	virtual hkpShapeType getType() const;
	// hkpShapeContainer's vtable at +0x10.
	uint32_t getFirstKey() const override;
	// hkpShapeContainer's vtable at +0x10.
	uint32_t getNextKey(uint32_t aOldKey) const override;
	// hkpShapeContainer's vtable at +0x10.
	uint32_t getCollisionFilterInfo(uint32_t aKey) const override;
	// hkpShapeContainer's vtable at +0x10.
	const hkpShape* getChildShape(uint32_t aKey, char (&arBuffer)[512]) const override;

	// Copies the shape's sub parts and scale into arInfo.
	void CopyCinfo(hkNiTriStripsShapeCinfo& arInfo);
	void CalcWorldAabb(const hkTransform& localToWorld);
	// Whether the triangle aKey names is degenerate (two of its
	// strip indices equal); getNextKey calls it on its const this.
	bool isChildShapeDegenerate(uint32_t a1) const;

	// Frees the object's aligned block (the padding byte before it) through MemoryManager.
	static void operator delete(void* apMemory) {
		uint8_t* pMemory = static_cast<uint8_t*>(apMemory);
		MemoryManager_Singleton.Deallocate(pMemory - pMemory[-1]);
	}

	uint8_t padding18[8];
	// CalcWorldAabb stores the transform at +0x20 and the box at +0x60.
	hkTransform AabbTransform;
	hkAabb WorldAabb;
	hkVector4 Scale;
	float m_radius;
	NiTLargePrimitiveArray<hkNiTriStripsData> pShapesA;
	uint8_t paddingAC[4];
};
