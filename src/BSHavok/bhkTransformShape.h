#pragma once

#include "BSHavok/bhkTransform.h"
#include "BSHavok/bhkShape.h"
#include "Reconstruction.hpp"

struct hkTransformShapeCinfo;
class hkpTransformShape;

// The Gamebryo side of a shape that places another shape with a transform. Its members are the Havok shape's,
// reached through phkObject; IDA has no fields for it.
class bhkTransformShape : public bhkShape {
public:
	bhkTransformShape();
	bhkTransformShape(const hkTransformShapeCinfo& arInfo);
	~bhkTransformShape() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void LinkObject(NiStream& arStream) override;
	bool RegisterStreamables(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	bool IsEqual(NiObject* apObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	void Link(bool abAttach) override;
	void KillCinfo(bool abCreated) override;
	uint32_t QCinfoSize() const override;
	// The exe has none here (it reads the cinfo raw through bhkSerializable::LoadCInfo); this one reads the
	// serialized form.
	int32_t LoadCInfo(NiStream& arStream) override;
	void Init(const hkSerializableCinfo& arInfo) override;
	hkSerializableCinfo& CreateCinfo(bool& arCreated) override;
	void CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) override;
	bool CanShare(NiCloningProcess& arCloneData) override;
	void BuildDisplayGeometry(NiNode* apNode, const NiColor& arDefColor) override;

	// The Havok shape, or null for a null this (the exe's shared null-safe getter, thiscall,
	// ret 0, 0x4AE750).
	hkpTransformShape* operator*() const;
	// The Gamebryo wrapper of the placed shape.
	bhkShape* GetChildShape();
	// Drops the reference the shape holds on its child shape's wrapper (thiscall, ret 0, 0xC8C770). Protected in the
	// PDB.
	void RembhkShapeRef();
	// bhkShape::CopyCinfo, then the child shape and the transform into arInfo (thiscall, ret 4, 0xC8C410; CreateCinfo's
	// callee on its new creation info). Protected in the PDB.
	void CopyCinfo(hkTransformShapeCinfo& arInfo);

	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
	static inline uint32_t iCount;
};

// The creation info of a transform shape: the placed shape and the transform (bhkTransform, so the struct sits on
// Havok's 16-byte alignment). The PDB defines it in this file's .cpp.
struct hkTransformShapeCinfo : public hkShapeCinfo {
	// The serialized form (bhkSerialized.h): the 32-bit layout, 0x50 bytes.
	struct Serialized : hkShapeCinfo::Serialized {
		bhkSerializedPointer m_childShape;
		uint8_t padding08[8];
		bhkTransform m_transform;
	};

	hkTransformShapeCinfo();

	// Fills the info from its serialized form; the child is left null (LinkObject binds it).
	void ReadSerialized(const Serialized& arForm) {
		hkShapeCinfo::ReadSerialized(arForm);
		m_childShape = nullptr;
		m_transform = arForm.m_transform;
	}

	const hkpShape* m_childShape;
	bhkTransform m_transform;
};

inline constexpr fnvd::Vtable bhkTransformShape_vftable{};

static_assert(offsetof(hkTransformShapeCinfo::Serialized, m_transform) == 0x10);
static_assert(sizeof(hkTransformShapeCinfo::Serialized) == 0x50);
