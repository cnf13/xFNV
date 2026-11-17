#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustumPlanes.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class BSOcclusionPlane;
class BSPortal;
class NiCamera;

// A view frustum combined with portal and occlusion planes through union and intersection groups, evaluated as a
// list of plane operations. Only the fields the reconstructed code reads are declared.
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
	// Resets the frustum to apCamera's view frustum. thiscall, ret 4, 0xC47980.
	void SetCamera(const NiCamera* apCamera);
	// Opens an intersection group. thiscall, ret 0, 0xC47A00.
	void StartGroupIntersection();
	// Closes the open group. thiscall, ret 0, 0xC47AE0.
	void EndGroup();
	// Adds apPortal's planes. thiscall, ret 8, 0xC47B50.
	void AddPortal(BSPortal* apPortal, bool abFinalRoom);
	// Adds apPlane's occlusion planes. thiscall, ret 4, 0xC47D20.
	void AddOcclusionPlane(BSOcclusionPlane* apPlane);
	// Appends apFrustum's operations. thiscall, ret 4, 0xC48800.
	void AddCompoundFrustum(BSCompoundFrustum* apFrustum);
	// Wraps the operations in a union group. thiscall, ret 0, 0xC48B90.
	void MakeIntoUnion();
	// Builds the frustum seen through apPortal from apExistingFrustum. thiscall, ret 8,
	// 0xC48DB0.
	void BuildForPortal(BSCompoundFrustum* apExistingFrustum, BSPortal* apPortal);
	// Whether apPlane passes the frustum. thiscall, ret 4, 0xC49560.
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
