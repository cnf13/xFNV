#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustum.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRect.h"
#include "Reconstruction.hpp"

// A scene graph object that defines a view: a frustum, a viewport and the world-to-camera matrix.
class NiCamera : public NiAVObject {
public:
	// Constructs a camera in apMemory.
	NiCamera();
	~NiCamera() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	// folded with NiDynamicEffect::RegisterStreamables: both only call NiAVObject::RegisterStreamables
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	// Updates the world transform, then the world-to-camera matrix from it.
	void UpdateWorldData(NiUpdateData& UpdateData) override;
	// The world bound is a zero-radius sphere at the camera's location.
	void UpdateWorldBound() override;

	// The camera's axes in world space: direction is column 0 of the world rotation, up column 1, right column 2.
	NiPoint3 GetWorldDirection() const;
	NiPoint3 GetWorldUpVector() const;
	NiPoint3 GetWorldRightVector() const;
	const NiFrustum& GetViewFrustum() const;
	// Copies the frustum; the near plane is raised to far / m_fMaxFarNearRatio and then to m_fMinNearPlaneDist.
	void SetViewFrustum(const NiFrustum& kFrustum);
	void SetMinNearPlaneDist(float fMin);
	float GetMaxFarNearRatio() const;
	void SetMaxFarNearRatio(float fMax);
	void SetViewPort(const NiRect<float>& kPort);
	// The viewport, m_kPort; the exe's body is a shared field-address getter (thiscall,
	// ret 0, 0x4A0D10, folded with TESDataHandler::GetImpactDataSetList).
	const NiRect<float>& GetViewPort() const;
	// Resizes the frustum to the aspect ratio (width / height) about its center, vertically or horizontally.
	void AdjustAspectRatio(float fTargetAspectRatio, bool bAdjustVerticalDims);
	// Projects a world point into the viewport. False when the point is behind the camera (w not above the
	// tolerance) or outside the frustum.
	bool WorldPtToScreenPt(const NiPoint3& kPt, float& fBx, float& fBy, float fZeroTolerance = 1e-5f);
	// Like WorldPtToScreenPt, without the frustum test and with the depth in fBz; w is taken as its magnitude.
	bool WorldPtToScreenPt3(const NiPoint3& kPt, float& fBx, float& fBy, float& fBz, float fZeroTolerance = 1e-5f);
	// Sets near and far to enclose the bound along the view direction; false when the bound is behind the camera.
	bool FitNearAndFarToBound(const NiBound& kBound);
	// Turns the camera to look at kWorldPt with kWorldUp as up; false when the point is the camera's location or
	// the up vector is parallel to the view direction.
	bool LookAtWorldPoint(const NiPoint3& kWorldPt, const NiPoint3& kWorldUp);
	// The ray through a window pixel; false without a renderer or when the pixel is outside the viewport.
	bool WindowPointToRay(int32_t lWx, int32_t lWy, NiPoint3& kOrigin, NiPoint3& kDir,
		const NiRenderTargetGroup* pkTarget = nullptr);
	// The ray through a point on the frustum's near plane (frustum units).
	void ViewPointToRay(float fVx, float fVy, NiPoint3& kOrigin, NiPoint3& kDir);
	// Builds m_aafWorldToCam from the world transform, the viewport and the frustum.
	void WorldToCameraMatrix();
	void CopyMembers(NiCamera* pkDest, NiCloningProcess& kCloning);
	void ScreenSpaceBoundBound(const NiBound& kSBound, NiPoint3& kMinBound, NiPoint3& kMaxBound, float fZeroTolerance);

	// Scales the camera's frustum for Eyefinity displays.
	// Inferred type: apCamera: NiCamera* from 0x740f9d (NiPointer<NiCamera>::m_pObject (&CaravanMenu::sp3DCamera));
	// afScale: float: used as one at 0x709da6
	static void HandleEyeFinityFOVScale(NiCamera* apCamera, float afScale);
	// IDA names it retn_0.75; not in the PDB. Returns 0.75f through a float local, in ST0
	// (cdecl, ret 0, 0x709AC0). Typed double so callers round ST0 into their float local as the exe does, rather
	// than assume a float return is already rounded.
	static double retn_0_75();
	static NiObject* CreateObject();

	float m_aafWorldToCam[4][4];
	NiFrustum m_kViewFrustum;
	float m_fMinNearPlaneDist;
	float m_fMaxFarNearRatio;
	NiRect<float> m_kPort;
	float m_fLODAdjust;

	static NiRTTI ms_RTTI;
};

NiSmartPointer(NiCamera);

inline constexpr fnvd::Vtable NiCamera_vftable{};
