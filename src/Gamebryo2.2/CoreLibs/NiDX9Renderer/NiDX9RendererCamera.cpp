#include "Fallout Shared/ScreenSplatter.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderTargetGroup.h"

// Builds the view, inverse view and projection matrices from the camera frame and frustum and hands them, the
// viewport and the depth range to the device. With NiRenderer::bPosAdjust set the camera location moves out of the
// view matrix into NiRenderer::PosAdjust (unless the screen splatter resets it), and the projection gets an oblique
// near plane along the plane z = ms_fPosAdjustClipHeight.
void NiDX9Renderer::Do_SetCameraData(const NiPoint3& arWorldLocation, const NiPoint3& arWorldDirection,
	const NiPoint3& arWorldUp, const NiPoint3& arWorldRight, const NiFrustum& arFrustum, const NiRect<float>& arPort) {
	if (m_bDeviceLost)
		return;

	NiPoint3 kLoc;
	if (NiRenderer::bPosAdjust && ScreenSplatter::bResetCameraPos) {
		NiRenderer::PosAdjust = NiPoint3_ZERO;
		kLoc = arWorldLocation;
	} else {
		NiRenderer::PosAdjust = arWorldLocation;
		kLoc = NiPoint3_ZERO;
	}

	m_kD3DView._11 = arWorldRight.x;
	m_kD3DView._12 = arWorldUp.x;
	m_kD3DView._13 = arWorldDirection.x;
	m_kD3DView._14 = 0.0f;
	m_kD3DView._21 = arWorldRight.y;
	m_kD3DView._22 = arWorldUp.y;
	m_kD3DView._23 = arWorldDirection.y;
	m_kD3DView._24 = 0.0f;
	m_kD3DView._31 = arWorldRight.z;
	m_kD3DView._32 = arWorldUp.z;
	m_kD3DView._33 = arWorldDirection.z;
	m_kD3DView._34 = 0.0f;
	float fDot = arWorldRight.x * kLoc.x + arWorldRight.y * kLoc.y + arWorldRight.z * kLoc.z;
	m_kD3DView._41 = -fDot;
	fDot = arWorldUp.x * kLoc.x + arWorldUp.y * kLoc.y + arWorldUp.z * kLoc.z;
	m_kD3DView._42 = -fDot;
	fDot = arWorldDirection.x * kLoc.x + arWorldDirection.y * kLoc.y + arWorldDirection.z * kLoc.z;
	m_kD3DView._43 = -fDot;
	m_kD3DView._44 = 1.0f;

	m_kInvView._11 = arWorldRight.x;
	m_kInvView._12 = arWorldRight.y;
	m_kInvView._13 = arWorldRight.z;
	m_kInvView._14 = 0.0f;
	m_kInvView._21 = arWorldUp.x;
	m_kInvView._22 = arWorldUp.y;
	m_kInvView._23 = arWorldUp.z;
	m_kInvView._24 = 0.0f;
	m_kInvView._31 = arWorldDirection.x;
	m_kInvView._32 = arWorldDirection.y;
	m_kInvView._33 = arWorldDirection.z;
	m_kInvView._34 = 0.0f;
	m_kInvView._41 = kLoc.x;
	m_kInvView._42 = kLoc.y;
	m_kInvView._43 = kLoc.z;
	m_kInvView._44 = 1.0f;
	m_pkD3DDevice9->SetTransform(D3DTS_VIEW, &m_kD3DView);

	m_kModelCamRight = arWorldRight;
	m_kCamRight = arWorldRight;
	m_kModelCamUp = arWorldUp;
	m_kCamUp = arWorldUp;

	m_fNearDepth = arFrustum.m_fNear;
	NiDX9RenderState* pkRenderState = m_pkRenderState;
	const float fDepthRange = arFrustum.m_fFar - arFrustum.m_fNear;
	m_fDepthRange = fDepthRange;
	const float fRmL = arFrustum.m_fRight - arFrustum.m_fLeft;
	const float fRpL = arFrustum.m_fLeft + arFrustum.m_fRight;
	const float fTmB = arFrustum.m_fTop - arFrustum.m_fBottom;
	const float fTpB = arFrustum.m_fBottom + arFrustum.m_fTop;
	const float fInvRange = 1.0f / fDepthRange;
	D3DMATRIX* pkProj = &m_kD3DProj;
	if (arFrustum.m_bOrtho) {
		if (pkRenderState->GetLeftHanded()) {
			pkProj->_11 = -2.0 / fRmL;
			m_kD3DProj._21 = 0.0f;
			m_kD3DProj._31 = 0.0f;
			m_kD3DProj._41 = fRpL / fRmL;
		} else {
			pkProj->_11 = 2.0 / fRmL;
			m_kD3DProj._21 = 0.0f;
			m_kD3DProj._31 = 0.0f;
			m_kD3DProj._41 = -fRpL / fRmL;
		}
		m_kD3DProj._12 = 0.0f;
		m_kD3DProj._22 = 2.0 / fTmB;
		m_kD3DProj._32 = 0.0f;
		m_kD3DProj._42 = -fTpB / fTmB;
		m_kD3DProj._13 = 0.0f;
		m_kD3DProj._23 = 0.0f;
		m_kD3DProj._33 = fInvRange;
		m_kD3DProj._43 = -(fInvRange * arFrustum.m_fNear);
		m_kD3DProj._14 = 0.0f;
		m_kD3DProj._24 = 0.0f;
		m_kD3DProj._34 = 0.0f;
		m_kD3DProj._44 = 1.0f;
	} else {
		if (pkRenderState->GetLeftHanded()) {
			pkProj->_11 = -2.0 / fRmL;
			m_kD3DProj._21 = 0.0f;
			m_kD3DProj._31 = fRpL / fRmL;
		} else {
			pkProj->_11 = 2.0 / fRmL;
			m_kD3DProj._21 = 0.0f;
			m_kD3DProj._31 = -fRpL / fRmL;
		}
		m_kD3DProj._41 = 0.0f;
		m_kD3DProj._12 = 0.0f;
		m_kD3DProj._22 = 2.0 / fTmB;
		m_kD3DProj._32 = -fTpB / fTmB;
		m_kD3DProj._42 = 0.0f;
		m_kD3DProj._13 = 0.0f;
		m_kD3DProj._23 = 0.0f;
		m_kD3DProj._33 = fInvRange * arFrustum.m_fFar;
		m_kD3DProj._43 = -(fInvRange * (arFrustum.m_fNear * arFrustum.m_fFar));
		m_kD3DProj._14 = 0.0f;
		m_kD3DProj._24 = 0.0f;
		m_kD3DProj._34 = 1.0f;
		m_kD3DProj._44 = 0.0f;
	}

	if (NiRenderer::bPosAdjust) {
		// The clip plane in view space: the transformed up axis through the transformed point (0, 0, height).
		D3DXVECTOR4 kPlane;
		kPlane.x = 0.0f;
		kPlane.y = 0.0f;
		kPlane.w = 0.0f;
		D3DXVECTOR3 kPoint;
		kPoint.x = 0.0f;
		kPoint.y = 0.0f;
		kPlane.z = 1.0f;
		kPoint.z = ms_fPosAdjustClipHeight;
		D3DXVECTOR4 kNormal;
		D3DXVec4Transform_0(&kNormal, &kPlane, &m_kD3DView);
		D3DXVec3Normalize_Thunk(reinterpret_cast<D3DXVECTOR3*>(&kNormal), reinterpret_cast<D3DXVECTOR3*>(&kNormal));
		D3DXVECTOR4 kClip;
		D3DXVec3TransformCoord_Thunk(reinterpret_cast<D3DXVECTOR3*>(&kClip), &kPoint, &m_kD3DView);
		fDot = kClip.x * kNormal.x + kClip.y * kNormal.y + kClip.z * kNormal.z;
		kPlane.w = -fDot;

		// Lengyel's oblique near plane: the far corner of the clip volume opposite the plane, scaled so that the
		// plane becomes the projection's third column.
		const float fNx = kNormal.x;
		const float fNy = kNormal.y;
		const float fNz = kNormal.z;
		float fSign;
		if (fNx > 0.0f)
			fSign = 1.0f;
		else if (fNx < 0.0f)
			fSign = -1.0f;
		else
			fSign = 0.0f;
		kNormal.x = fSign / pkProj->_11;
		if (fNy > 0.0f)
			fSign = 1.0f;
		else if (fNy < 0.0f)
			fSign = -1.0f;
		else
			fSign = 0.0f;
		kNormal.y = fSign / m_kD3DProj._22;
		kNormal.w = (1.0f - m_kD3DProj._33) / m_kD3DProj._43;
		fDot = kNormal.x * fNx + kNormal.y * fNy + fNz + kNormal.w * kPlane.w;
		const float fScale = 1.0f / fDot;
		kClip.x = fNx * fScale;
		kClip.y = fNy * fScale;
		kClip.z = fNz * fScale;
		kClip.w = fScale * kPlane.w;
		m_kD3DProj._13 = kClip.x;
		m_kD3DProj._23 = kClip.y;
		m_kD3DProj._33 = kClip.z;
		m_kD3DProj._43 = kClip.w;
	}
	m_pkD3DDevice9->SetTransform(D3DTS_PROJECTION, pkProj);

	// Converts each size as a signed integer and adds 2^32 when negative.
	const int32_t iWidth = static_cast<int32_t>(m_pkCurrRenderTargetGroup->GetWidth(0));
	double dWidth = iWidth;
	if (iWidth < 0) {
		dWidth += 4294967296.0f;
	}
	float fWidthSlot = dWidth;
	const int32_t iHeight = static_cast<int32_t>(m_pkCurrRenderTargetGroup->GetHeight(0));
	double dHeight = iHeight;
	if (iHeight < 0) {
		dHeight += 4294967296.0f;
	}
	float fHeightSlot = dHeight;
	const float fWidth = fWidthSlot;
	const float fHeight = fHeightSlot;
	D3DVIEWPORT9 kViewport;
	kViewport.X = static_cast<uint32_t>(static_cast<int64_t>(fWidth * arPort.m_left));
	kViewport.Y = static_cast<uint32_t>(static_cast<int64_t>((1.0f - arPort.m_top) * fHeight));
	kViewport.Width = static_cast<uint32_t>(static_cast<int64_t>(fWidth * (arPort.m_right - arPort.m_left)));
	kViewport.Height = static_cast<uint32_t>(static_cast<int64_t>(fHeight * (arPort.m_top - arPort.m_bottom)));
	kViewport.MinZ = 0.0f;
	kViewport.MaxZ = 1.0f;
	m_pkD3DDevice9->SetViewport(&kViewport);

	D3DXMatrixMultiply_Thunk(&m_kViewProj, &m_kD3DView, pkProj);
	m_pkRenderState->SetCameraNearAndFar(arFrustum.m_fNear, arFrustum.m_fFar);
}

// Sets up a camera for screen-space drawing: a fixed view matrix, a projection that maps pixels of the current render
// target, near 1 and depth range 9999, and the viewport of apPort (or the whole target when null).
void NiDX9Renderer::Do_SetScreenSpaceCameraData(const NiRect<float>* apPort) {
	if (m_bDeviceLost)
		return;

	const uint32_t uiWidth = m_pkCurrRenderTargetGroup->GetWidth(0);
	const uint32_t uiHeight = m_pkCurrRenderTargetGroup->GetHeight(0);
	// Converts each size as a signed integer and adds 2^32 when negative.
	double dWidth = static_cast<int32_t>(uiWidth);
	if (static_cast<int32_t>(uiWidth) < 0) {
		dWidth += 4294967296.0f;
	}
	float fWidthSlot = dWidth;
	double dHeight = static_cast<int32_t>(uiHeight);
	if (static_cast<int32_t>(uiHeight) < 0) {
		dHeight += 4294967296.0f;
	}
	float fHeightSlot = dHeight;
	const float fWidth = fWidthSlot;
	const float fHeight = fHeightSlot;

	m_kD3DView._11 = 1.0f;
	m_kD3DView._12 = 0.0f;
	m_kD3DView._13 = 0.0f;
	m_kD3DView._14 = 0.0f;
	m_kD3DView._21 = 0.0f;
	m_kD3DView._22 = -1.0f;
	m_kD3DView._23 = 0.0f;
	m_kD3DView._24 = 0.0f;
	m_kD3DView._31 = 0.0f;
	m_kD3DView._32 = 0.0f;
	m_kD3DView._33 = 1.0f;
	m_kD3DView._34 = 0.0f;
	m_kD3DView._41 = -0.5f;
	m_kD3DView._42 = 0.5f;
	m_kD3DView._43 = 1.0f;
	m_kD3DView._44 = 1.0f;
	m_kInvView._11 = 1.0f;
	m_kInvView._12 = 0.0f;
	m_kInvView._13 = 0.0f;
	m_kInvView._14 = 0.0f;
	m_kInvView._21 = 0.0f;
	m_kInvView._22 = -1.0f;
	m_kInvView._23 = 0.0f;
	m_kInvView._24 = 0.0f;
	m_kInvView._31 = 0.0f;
	m_kInvView._32 = 0.0f;
	m_kInvView._33 = 1.0f;
	m_kInvView._34 = 0.0f;
	m_kInvView._41 = 0.5f;
	m_kInvView._42 = 0.5f;
	m_kInvView._43 = -1.0f;
	m_kInvView._44 = 1.0f;
	m_pkD3DDevice9->SetTransform(D3DTS_VIEW, &m_kD3DView);

	NiPoint3 kRight;
	kRight.x = 1.0f;
	kRight.y = 0.0f;
	kRight.z = 0.0f;
	m_kModelCamRight = kRight;
	m_kCamRight = kRight;
	NiPoint3 kUp;
	kUp.x = 0.0f;
	kUp.y = -1.0f;
	kUp.z = 0.0f;
	m_kModelCamUp = kUp;
	m_kCamUp = kUp;
	m_fNearDepth = 1.0f;
	m_fDepthRange = 9999.0f;

	m_kD3DProj._11 = 2.0f;
	m_kD3DProj._21 = 0.0f;
	m_kD3DProj._31 = 0.0f;
	m_kD3DProj._41 = -1.0 / fWidth;
	m_kD3DProj._12 = 0.0f;
	m_kD3DProj._32 = 0.0f;
	m_kD3DProj._22 = 2.0f;
	m_kD3DProj._42 = 1.0f / fHeight;
	m_kD3DProj._13 = 0.0f;
	m_kD3DProj._23 = 0.0f;
	m_kD3DProj._33 = 0.00010001000191550702f;
	m_kD3DProj._43 = -0.00010001000191550702f;
	m_kD3DProj._14 = 0.0f;
	m_kD3DProj._24 = 0.0f;
	m_kD3DProj._34 = 0.0f;
	m_kD3DProj._44 = 1.0f;
	m_pkD3DDevice9->SetTransform(D3DTS_PROJECTION, &m_kD3DProj);

	D3DVIEWPORT9 kViewport;
	if (apPort) {
		kViewport.X = static_cast<uint32_t>(static_cast<int64_t>(apPort->m_left * fWidth));
		kViewport.Y = static_cast<uint32_t>(static_cast<int64_t>((1.0f - apPort->m_top) * fHeight));
		kViewport.Width = static_cast<uint32_t>(static_cast<int64_t>(fWidth * (apPort->m_right - apPort->m_left)));
		kViewport.Height = static_cast<uint32_t>(static_cast<int64_t>(fHeight * (apPort->m_top - apPort->m_bottom)));
	} else {
		kViewport.X = 0;
		kViewport.Y = 0;
		kViewport.Width = uiWidth;
		kViewport.Height = uiHeight;
	}
	kViewport.MinZ = 0.0f;
	kViewport.MaxZ = 1.0f;
	m_pkD3DDevice9->SetViewport(&kViewport);

	m_pkRenderState->SetCameraNearAndFar(1.0f, 10000.0f);
}
