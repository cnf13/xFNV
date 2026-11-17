#pragma once

#include "BSHavok/bhkConvexShape.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

#include <stdint.h>

struct hkConvexVerticesShapeCinfo;

// The game wrapper of a convex shape given by its vertices. Its members are not declared yet.
class bhkConvexVerticesShape : public bhkConvexShape {
public:
	// The default constructor, run on the memory the caller allocated.
	bhkConvexVerticesShape();
	~bhkConvexVerticesShape() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	void KillCinfo(bool abCreated) override;
	uint32_t QCinfoSize() const override;
	// This override reads the serialized form instead of reading the cinfo raw through bhkSerializable::LoadCInfo.
	int32_t LoadCInfo(NiStream& arStream) override;
	void Init(const hkSerializableCinfo& arInfo) override;
	hkSerializableCinfo& CreateCinfo(bool& arCreated) override;
	void CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) override;
	void BuildDisplayGeometry(NiNode* apNode, const NiColor& arDefColor) override;

	// Fills arInfo from the wrapped Havok shape.
	void CopyCinfo(hkConvexVerticesShapeCinfo& arInfo);

	// The stream's object factory: a new, unbuilt wrapper.
	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
};

// The creation info of a convex vertices shape: the rounded surface's radius, the vertices and the plane equations.
struct hkConvexVerticesShapeCinfo : public hkConvexShapeCinfo {
	// The serialized form (bhkSerialized.h): the 32-bit layout, 0x20 bytes. Both arrays are empty in the files;
	// LoadBinary reads their elements after the info.
	struct Serialized : hkConvexShapeCinfo::Serialized {
		bhkSerializedArray m_vertices;
		bhkSerializedArray m_planeEquations;
	};

	// The destructor: frees the plane equations, then the vertices.
	~hkConvexVerticesShapeCinfo();

	// Scales the vertices and the planes' distances by afScale.
	void Scale(float afScale);

	// Fills the info from its serialized form.
	void ReadSerialized(const Serialized& arForm) {
		hkConvexShapeCinfo::ReadSerialized(arForm);
		arForm.m_vertices.ToNative(m_vertices);
		arForm.m_planeEquations.ToNative(m_planeEquations);
	}

	hkArray<hkVector4> m_vertices;
	hkArray<hkVector4> m_planeEquations;
};

inline int32_t g_iNumBhkConvexVerticesShape;

static_assert(offsetof(hkConvexVerticesShapeCinfo::Serialized, m_vertices) == 0x8);
static_assert(offsetof(hkConvexVerticesShapeCinfo::Serialized, m_planeEquations) == 0x14);
static_assert(sizeof(hkConvexVerticesShapeCinfo::Serialized) == 0x20);
