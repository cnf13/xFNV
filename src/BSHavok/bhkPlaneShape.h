#pragma once

#include "BSHavok/bhkHeightFieldShape.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

#include <stdint.h>

struct hkPlaneShapeCinfo;

// The game wrapper of a plane collision shape. Its members are not declared yet.
class bhkPlaneShape : public bhkHeightFieldShape {
public:
	bhkPlaneShape();
	~bhkPlaneShape() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	void KillCinfo(bool abCreated) override;
	uint32_t QCinfoSize() const override;
	void Init(const hkSerializableCinfo& arInfo) override;
	hkSerializableCinfo& CreateCinfo(bool& arCreated) override;
	void CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) override;

protected:
	// Calls bhkShape's CopyCinfo, then copies the wrapped hkpPlaneShape's plane, half extents and centre into arInfo;
	// CreateCinfo calls it. Not an override: like bhkShape's, it hides the base's CopyCinfo.
	void CopyCinfo(hkPlaneShapeCinfo& arInfo);

public:
	// Factory: a new default-constructed plane shape, or null when the allocation fails.
	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
	// Live wrappers of this class, counted by the constructors.
	static inline uint32_t iCount;
};

// The creation info of a height field shape: it has hkShapeCinfo as its base and no members of its own.
struct hkHeightFieldShapeCinfo : public hkShapeCinfo {};

// The creation info of a plane shape: the plane and the bounding box's half extents and centre.
struct hkPlaneShapeCinfo : public hkHeightFieldShapeCinfo {
	bhkVector4 m_plane;
	bhkVector4 m_aabbHalfExtents;
	bhkVector4 m_aabbCenter;
};
