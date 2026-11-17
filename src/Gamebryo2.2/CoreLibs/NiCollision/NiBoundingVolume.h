#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"

#include <cstdint>

class NiBoundingVolume;
class NiPoint3;
class NiStream;
class NiTransform;

template <class T> class NiTPrimitiveArray;

// The entry types of the type-pair dispatch tables (Gamebryo's names).
using NiBVTestIntersectFunction = bool (*)(float fTime, const NiBoundingVolume& kABV0, const NiPoint3& kV0,
	const NiBoundingVolume& kABV1, const NiPoint3& kV1);
using NiBVFindIntersectFunction = bool (*)(float fTime, const NiBoundingVolume& kABV0, const NiPoint3& kV0,
	const NiBoundingVolume& kABV1, const NiPoint3& kV1, float& fIntrTime, NiPoint3& kIntrPt, int32_t bCalcNormals,
	NiPoint3& kNormal0, NiPoint3& kNormal1);
using NiBVTriTestIntersectFunction = bool (*)(float fTime, const NiBoundingVolume& kABV, const NiPoint3& kV0,
	const NiPoint3& kVert0, const NiPoint3& kVert1, const NiPoint3& kVert2, const NiPoint3& kV1);
using NiBVTriFindIntersectFunction = bool (*)(float fTime, const NiBoundingVolume& kABV, const NiPoint3& kV0,
	const NiPoint3& kVert0, const NiPoint3& kVert1, const NiPoint3& kVert2, const NiPoint3& kV1, float& fIntrTime,
	NiPoint3& kIntrPt, int32_t bCalcNormals, NiPoint3& kNormal0, NiPoint3& kNormal1);

// a collision bounding volume; the concrete shapes override the pure slots
class NiBoundingVolume : public NiMemObject {
public:
	enum BoundType : int32_t {
		BASE_BV = -1,
		SPHERE_BV = 0,
		BOX_BV = 1,
		CAPSULE_BV = 2,
		LOZENGE_BV = 3,
		UNION_BV = 4,
		HALFSPACE_BV = 5,
		MAXTYPE_BV = 6,
	};

	// The loaders by type (each class's CreateFromStream).
	using CreateFunction = NiBoundingVolume* (*)(NiStream& arStream);
	using CreateFunctionTable = CreateFunction[MAXTYPE_BV];
	// The entry types of the sphere and box conversions; Gamebryo declares them inline with no names. Names chosen
	// here.
	using ConvertToSphereFunction = NiBoundingVolume* (*)(float fRadius, const NiPoint3& kCenter);
	using ConvertToBoxFunction = NiBoundingVolume* (*)(int32_t aiQuantity, const NiPoint3* pkData);

	// The stream functions are deliberately declared ahead of the destructor, as the engine declares them.
	virtual void LoadBinary(NiStream&) {}
	virtual void SaveBinary(NiStream& kStream);
	virtual ~NiBoundingVolume();
	virtual int32_t Type() const { return BASE_BV; }
	// shared "return 0" stub
	virtual uint32_t WhichObjectIntersect() const { return 0; }
	virtual void UpdateWorldData(const NiBoundingVolume& arModelABV, const NiTransform& arWorld) = 0;
	virtual NiBoundingVolume* Create() const = 0;
	virtual NiBoundingVolume* Clone() const = 0;
	virtual void Copy(const NiBoundingVolume& arABV) = 0;
	virtual bool operator==(const NiBoundingVolume& arABV) const = 0;
	virtual bool operator!=(const NiBoundingVolume& arABV) const = 0;
	virtual void AddViewerStrings(const char* apcPrefix, NiTPrimitiveArray<char*>* apStrings) const = 0;

	// Whether the volumes, moving by apV0 and apV1 over afTime, intersect.
	static bool TestIntersect(float fTime, const NiBoundingVolume& kBV0, const NiPoint3& kVel0,
		const NiBoundingVolume& kBV1, const NiPoint3& kVel1);
	// The first intersection time and point of the moving volumes, with their normals when
	// bCalcNormals.
	static bool FindIntersect(float fTime, const NiBoundingVolume& kBV0, const NiPoint3& kVel0,
		const NiBoundingVolume& kBV1, const NiPoint3& kVel1, float& fIntrTime, NiPoint3& kIntrPt, int32_t bCalcNormals,
		NiPoint3& kNormal0, NiPoint3& kNormal1);
	// Whether kBV0, moving by kVel0, and the triangle, moving by kVel1, intersect: the triangle test of kBV0's type.
	static bool TestTriIntersect(float fTime, const NiBoundingVolume& kBV0, const NiPoint3& kVel0,
		const NiPoint3& kVert0, const NiPoint3& kVert1, const NiPoint3& kVert2, const NiPoint3& kVel1);
	// The first intersection time and point of kBV0, moving by kVel0, and the triangle, moving by kVel1, with their
	// normals when bCalcNormals: the triangle search of kBV0's type. bCalcNormals is an int32_t rather than a bool
	// because the caller's whole value is passed on; a bool would be zero-extended.
	static bool FindTriIntersect(float fTime, const NiBoundingVolume& kBV0, const NiPoint3& kVel0,
		const NiPoint3& kVert0, const NiPoint3& kVert1, const NiPoint3& kVert2, const NiPoint3& kVel1, float& fIntrTime,
		NiPoint3& kIntrPt, int32_t bCalcNormals, NiPoint3& kNormal0, NiPoint3& kNormal1);
	// Reads a volume's type tag and hands the stream to that type's loader.
	static NiBoundingVolume* CreateFromStream(NiStream& kStream);
	// Bounding volumes are freed through MemoryManager::Deallocate, not NiMemObject's delete.
	static void operator delete(void* apMemory);

	static inline CreateFunctionTable ms_apfnLoaders{};
	// The type-pair dispatch tables (MAXTYPE_BV * MAXTYPE_BV entries, the tri tables MAXTYPE_BV), allocated and filled
	// by NiCollisionSDM::Init. TestIntersect reads the test table, FindIntersect the find table.
	static inline NiBVTestIntersectFunction* ms_pfnTestFuncTable;
	static inline NiBVFindIntersectFunction* ms_pfnFindFuncTable;
	static inline NiBVTriTestIntersectFunction* ms_pfnTriTestFuncTable;
	static inline NiBVTriFindIntersectFunction* ms_pfnTriFindFuncTable;
	static inline ConvertToSphereFunction ms_pfnConvertToSphereBV_Fast{};
	static inline ConvertToSphereFunction ms_pfnConvertToSphereBV_Safe{};
	static inline ConvertToBoxFunction ms_pfnConvertToBoxBV{};
};
