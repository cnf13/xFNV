#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "BSHavok/bhkShape.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkPackedNiTriStripsShapeCinfo;

// One sub part of a packed triangle-strip mesh: its collision filter, vertex count and material.
struct SubPartData {
	uint32_t uiFilter;
	uint32_t uiNumVerts;
	HavokMaterialType uiMaterial;
};

// One triangle of a packed triangle-strip mesh: the three vertex indices and the word the format pads them with.
// 8 bytes, as in the PDB: LoadBinary (0xCA3F10) allocates 8 per triangle and keeps only the first 8 bytes of the
// 0x14-byte records older streams store (IDA's struct adds a NiPoint3 vNormal), and getChildShape (0xCA3C40) indexes
// paTriData by 8.
struct TriangleData {
	uint16_t us1;
	uint16_t us2;
	uint16_t us3;
	uint16_t unused;
};

// One triangle as streams before version 33 store it (the PDB's, in BSHavok/hkPackedNiTriStripsShape.cpp):
// the TriangleData and a normal, 0x14 bytes. LoadBinary (0xCA3F10) reads each into a local and keeps the first 8
// bytes.
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
	// Not registered: an old stream's triangle record is read into a local a short read leaves unwritten.
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
	// The PDB places paCompressedVertData at +0x20 and paSubPartData at +0x24, where SaveBinary (0xCA3DC0) writes them
	// from (0xCA3E37 and 0xCA3E8E read +0x20, 0xCA3ED4 reads +0x24); SaveBinary never touches +0x1C.
	uint8_t padding1C[4];
	uint16_t* paCompressedVertData;
	SubPartData* paSubPartData;
	uint8_t padding28[8];

	static NiRTTI ms_RTTI;
};

NiSmartPointer(hkPackedNiTriStripsData);

// A Havok shape collection over a packed triangle-strip mesh: the mesh, its scale and radius, and the world box last
// computed for it (CalcWorldAabb). The exe has two vtables: hkpShape's at +0 and hkpShapeContainer's at +0x10.
class hkPackedNiTriStripsShape : public hkpShapeCollection {
public:
	// Builds the shape from the creation info of the bhk shape it is made for (thiscall, ret 0x4, 0xCA39F0). Returns
	// the constructed object, as the exe's constructors do.
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

	// The exe's deleting destructor returns the block to the Havok heap, of the size in m_memSizeAndFlags.
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
	// The exe pads the class to the 16-byte alignment of its vectors.
	uint8_t paddingA4[12];
};

// IDA's name: the PDB's aswapSize2, a static local of hkPackedNiTriStripsData::LoadBinary (0xCA3F10) (SaveBinary's
// own copy holds that name in the .cpp). The byte-swap size list (one entry, 4) for the vertex and sub part reads.
inline uint32_t aswapSize2_0 = 4;

// IDA's name: the PDB's aswapSize1, a static local of hkPackedNiTriStripsData::LoadBinary (0xCA3F10).
using aswapSize1_0_t = uint32_t[7];
inline aswapSize1_0_t aswapSize1_0{2, 2, 2, 2, 4, 4, 4};

// The vtable hkPackedNiTriStripsData's constructor stores.
inline constexpr fnvd::Vtable hkPackedNiTriStripsData_vftable{};
inline constexpr fnvd::Vtable hkPackedNiTriStripsShape_vftable{};

// The hkpShapeContainer base's vtable, at +0x10.
inline constexpr fnvd::Vtable hkPackedNiTriStripsShape_vftable_hkpShapeContainer{};
