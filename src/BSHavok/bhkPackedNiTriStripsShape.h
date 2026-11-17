#pragma once

#include "BSHavok/bhkListShape.h"
#include "BSHavok/bhkShapeCollection.h"
#include "BSHavok/hkPackedNiTriStripsShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Reconstruction.hpp"

class hkPackedNiTriStripsShapeCinfo;

// The game wrapper of a packed triangle-strip mesh (hkPackedNiTriStripsShape). Its members are not
// declared yet.
class bhkPackedNiTriStripsShape : public bhkShapeCollection {
public:
	bhkPackedNiTriStripsShape();
	~bhkPackedNiTriStripsShape() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void LinkObject(NiStream& arStream) override;
	bool RegisterStreamables(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	bool IsEqual(NiObject* apObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	void KillCinfo(bool abCreated) override;
	uint32_t QCinfoSize() const override;
	int32_t LoadCInfo(NiStream& arStream) override;
	void Init(const hkSerializableCinfo& arInfo) override;
	hkSerializableCinfo& CreateCinfo(bool& arCreated) override;
	void CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) override;
	bool CanShare(NiCloningProcess& arCloneData) override;
	void BuildDisplayGeometry(NiNode* apNode, const NiColor& arDefColor) override;
	HavokMaterialType QSubMaterial(uint32_t aiKey) override;

	// Copies the wrapped shape's data into the creation info (thiscall, ret 4, 0xCA5360). Its
	// parameter is this shape's creation info, not the base's hkSerializableCinfo, so it does not override the base
	// virtual: the exe's slot 48 keeps bhkSerializable's empty body.
	void CopyCinfo(hkPackedNiTriStripsShapeCinfo& arInfo);
	float GetScale();

	// The engine's object factory for the class.
	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
	static inline uint32_t iCount;
};

// The creation info of a packed triangle-strip shape: the streamed mesh, its radius and its scale, at the PDB's
// offsets; bhkExtendedMeshShape::Init (0xCCC980) reads the mesh at +0x4 (0xCCC9F3), the radius at +0x8 (0xCCCE79)
// and the scale at +0x10 (0xCCCE7C).
class hkPackedNiTriStripsShapeCinfo : public hkShapeCollectionCinfo {
public:
	hkPackedNiTriStripsShapeCinfo();
	~hkPackedNiTriStripsShapeCinfo();

	NiPointer<hkPackedNiTriStripsData> spData;
	float fRadius;
	hkVector4 vScale;
};

inline constexpr fnvd::Vtable bhkPackedNiTriStripsShape_vftable{};
