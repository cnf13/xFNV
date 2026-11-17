#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustumPlanes.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

#include <stdint.h>

class BSOcclusionPlane;
class BSPortal;
class NiCamera;

// A view frustum combined with portal and occlusion planes through union and intersection groups, evaluated as a list
// of plane operations. Only some of its fields are declared.
class BSCompoundFrustum {
public:
	// One plane operation of the frustum's list: the operation and the indexes it jumps to when it passes or fails.
	struct FunctionOp {
		int32_t op;
		int32_t jp;
		int32_t jf;
	};

	BSCompoundFrustum();
	~BSCompoundFrustum();

	// Shifts the plane operations by aiShift, filling the gap with no-ops when abFillNoop.
	void ShiftOps(int32_t aiShift, bool abFillNoop);
	// Prepares the operation list for testing on worker threads.
	void PrethreadOpList();
	// Resets the frustum to apCamera's view frustum.
	void SetCamera(const NiCamera* apCamera);
	// Opens an intersection group.
	void StartGroupIntersection();
	// Closes the open group.
	void EndGroup();
	// Adds apPortal's planes.
	void AddPortal(BSPortal* apPortal, bool abFinalRoom);
	// Adds apPlane's occlusion planes.
	void AddOcclusionPlane(BSOcclusionPlane* apPlane);
	// Appends apFrustum's operations.
	void AddCompoundFrustum(BSCompoundFrustum* apFrustum);
	// Wraps the operations in a union group.
	void MakeIntoUnion();
	// Builds the frustum seen through apPortal from apExistingFrustum.
	void BuildForPortal(BSCompoundFrustum* apExistingFrustum, BSPortal* apPortal);
	// Whether apPlane passes the frustum.
	bool ProcessPlane(BSOcclusionPlane* apPlane);
	int32_t QFuncOpCount();
	void SetActivePlaneState(int32_t* apPlaneState);
	void StartGroupUnion();
	bool TestNotContained(NiBound* apBound, NiFrustumPlanes& apPlanes);
	void VerifyGroups();
	void RemoveEmptyGroups();
	void CopyMembers(BSCompoundFrustum* apDest);
	void MakeIntoIntersection();
	int32_t* GetActivePlaneState();
	bool Process(NiAVObject* apObject);
	bool ProcessMultiBound(BSMultiBound* apBound);
	bool TestIntersection(NiBound* apBound, NiFrustumPlanes& apPlanes);

	static void operator delete(void* apMemory);

	bool bPrethreaded;
	uint8_t padding01[3];
	BSSimpleArray<NiFrustumPlanes> kPlanes;
	BSSimpleArray<FunctionOp> kFunctionOperators;
	int32_t iFreePlane;
	// The number of operations in use (zero: the frustum is empty).
	int32_t iFreeOp;
	int32_t iFirstOp;
	NiFrustumPlanes kViewFrustum;
	NiPoint3 kViewPosition;
	bool bSkipViewFrustum;
	uint8_t paddingA1[3];
};
