#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Reconstruction.hpp"

class BSMultiBound;
class NiBound;
class NiFrustumPlanes;
class NiLines;
class NiPoint3;
class NiTriShape;

// base of the volumes a BSMultiBound can have; kCullResult is the culler's per-frame verdict for the shape
class BSMultiBoundShape : public NiObject {
public:
	enum BSMBShapeType : int32_t {
		BS_SHAPE_INVALID = 0,
		BS_SHAPE_AABB = 1,
		BS_SHAPE_OBB = 2,
		BS_SHAPE_SPHERE = 3,
		BS_SHAPE_CAPSULE = 4,
	};

	enum BSMBIntersectResult : int32_t {
		BS_INTERSECT_NONE = 0,
		BS_INTERSECT_PARTIAL = 1,
		BS_INTERSECT_CONTAINSTARGET = 2,
	};

	enum BSMBCullResult : int32_t {
		BS_CULL_UNTESTED = 0,
		BS_CULL_VISIBLE = 1,
		BS_CULL_CULLED = 2,
		BS_CULL_OCCLUDED = 3,
	};

	// The debug geometry's colour per shape type (the PDB's NiColorA[4]).
	using DebugVisColorArray = NiColorA[4];

	BSMultiBoundShape();
	~BSMultiBoundShape() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void LinkObject(NiStream& arStream) override;
	bool RegisterStreamables(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	bool IsEqual(NiObject* apObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	virtual BSMultiBoundShape::BSMBShapeType QType() const = 0;
	virtual float QMultiBoundRadius() const = 0;
	// the overloads sit in the vtable in reverse declaration order: the BSMultiBound one is slot 37 (its bodies load
	// the argument's shape pointer), the NiBound one slot 38 (its bodies read the center floats)
	virtual BSMultiBoundShape::BSMBIntersectResult CheckBound(const NiBound& aTargetBound) = 0;
	virtual BSMultiBoundShape::BSMBIntersectResult CheckBound(const BSMultiBound& aTargetBound) = 0;
	virtual bool WithinFrustum(const NiFrustumPlanes& aPlanes) = 0;
	virtual bool CompletelyWithinFrustum(const NiFrustumPlanes& aPlanes) = 0;
	virtual void GetNiBound(NiBound& aBound) = 0;
	virtual void MakeDebugGeom(NiLines* apLines, NiTriShape* apGeometry, NiColorA aColor) = 0;
	virtual int32_t QDebugGeomLineSize() = 0;
	virtual int32_t QDebugGeomShapeSize() = 0;
	virtual bool QPointWithin(NiPoint3&) { return false; }
	virtual void SetCenter(const NiPoint3&) {}

	void ResetCullResult() { kCullResult = BS_CULL_UNTESTED; }

protected:
	void CopyMembers(BSMultiBoundShape* pDest, NiCloningProcess& arCloneData);

public:
	BSMBCullResult kCullResult;

	static NiRTTI ms_RTTI;
	static inline NiColorA DebugVisColors[4] = {NiColorA(0.4f, 0.4f, 0.4f, 0.1f), NiColorA(0.0f, 0.8f, 0.0f, 0.1f),
		NiColorA(0.0f, 0.0f, 0.7f, 0.1f), NiColorA(0.5f, 0.0f, 0.0f, 0.1f)};
};

inline constexpr fnvd::Vtable BSMultiBoundShape_vftable{};
