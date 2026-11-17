#pragma once

#include "BSHavok/bhkShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"

#include <stdint.h>

class hkPackedNiTriStripsShapeCinfo;

// One sub part of a packed triangle-strip mesh: its collision filter, vertex count and material.
struct SubPartData {
	uint32_t uiFilter;
	uint32_t uiNumVerts;
	HavokMaterialType uiMaterial;
};

// One triangle of a packed triangle-strip mesh: the three vertex indices and the word the format pads them with. 8
// bytes: LoadBinary allocates 8 per triangle and keeps only the first 8 bytes of the 0x14-byte records older streams
// store, and getChildShape indexes paTriData by 8.
struct TriangleData {
	uint16_t us1;
	uint16_t us2;
	uint16_t us3;
	uint16_t unused;
};

// One triangle as streams before version 33 store it: the TriangleData and a normal, 0x14 bytes. LoadBinary reads each
// into a local and keeps the first 8 bytes.
struct OldTriangleData {
	uint16_t us1;
	uint16_t us2;
	uint16_t us3;
	uint16_t unused;
	NiPoint3 vNormal;
};

// The streamed mesh of a packed triangle-strip shape: the triangles, the vertices (plain or compressed) and the
// sub parts.
class hkPackedNiTriStripsData : public NiObject {
public:
	hkPackedNiTriStripsData();
	~hkPackedNiTriStripsData() override;

	const NiRTTI* GetRTTI() const override;
	// LoadBinary reads an old stream's triangle record into a local that a short read leaves unwritten, as the game
	// does.
	void LoadBinary(NiStream& arStream) override;
	bool RegisterStreamables(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	bool IsEqual(NiObject* apObject) override;

	// An empty mesh: the base object, this class's vtable, every count and pointer cleared.
	static NiObject* CreateObject();

	uint32_t uiKeyCount;
	uint32_t uiVertCount;
	uint16_t usPartCount;
	uint8_t padding12[2];
	TriangleData* paTriData;
	NiPoint3* paVertData;
	// SaveBinary writes paCompressedVertData and paSubPartData, and never touches padding1C.
	uint8_t padding1C[4];
	uint16_t* paCompressedVertData;
	SubPartData* paSubPartData;
	uint8_t padding28[8];

	static NiRTTI ms_RTTI;
};

NiSmartPointer(hkPackedNiTriStripsData);

// A Havok shape collection over a packed triangle-strip mesh: the mesh, its scale and radius, and the world box last
// computed for it (CalcWorldAabb).
class hkPackedNiTriStripsShape : public hkpShapeCollection {
public:
	// Builds the shape from the creation info of the bhk shape it is made for.
	hkPackedNiTriStripsShape(const hkPackedNiTriStripsShapeCinfo& arInfo);
	~hkPackedNiTriStripsShape() override;

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

	// Copies the mesh, its radius and its scale into arInfo.
	void CopyCinfo(hkPackedNiTriStripsShapeCinfo& arInfo) const;
	void CalcWorldAabb(const hkTransform& localToWorld);

	// Deleting returns the block to the Havok heap, of the size in m_memSizeAndFlags.
	static void operator delete(void* apMemory) { hkReferencedObject::deleteObject(apMemory); }

	// hkpShapeCollection ends at 0x18; the transform is 16-byte aligned.
	uint8_t padding18[8];
	// CalcWorldAabb stores the transform and the box.
	hkTransform AabbTransform;
	hkAabb WorldAabb;
	float fAabbScale;
	NiPointer<hkPackedNiTriStripsData> spTriStripsData;
	uint8_t padding88[8];
	hkVector4 vScale;
	float fRadius;
	// Pads the class to the 16-byte alignment of its vectors.
	uint8_t paddingA4[12];
};

// The byte-swap size list (one entry, 4) for the vertex and sub part reads in hkPackedNiTriStripsData::LoadBinary.
inline uint32_t aswapSize2_0 = 4;

// A static local of hkPackedNiTriStripsData::LoadBinary.
using aswapSize1_0_t = uint32_t[7];
inline aswapSize1_0_t aswapSize1_0{2, 2, 2, 2, 4, 4, 4};
