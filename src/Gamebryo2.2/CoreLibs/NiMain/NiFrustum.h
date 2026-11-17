#pragma once

#include "Reconstruction.hpp"

// The view volume of a camera, as distances on the near plane and the near and far plane distances.
class NiFrustum {
public:
	// The constructor NiFrustum(bool bOrtho), thiscall, ret 4, 0xA71B70: it stores only
	// m_bOrtho (IDA names it SetOrtho).
	// The hybrid's implicit default constructor (no zero-parameter Construct).
	NiFrustum() = default;
	NiFrustum(bool bOrtho);
	NiFrustum(float bOrtho, float a2, float a3, float a4, float a5, float a6, bool a7);

	// A new viewer string describing the frustum.
	char* GetViewerString(const char* pcPrefix) const;

	float m_fLeft;
	float m_fRight;
	float m_fTop;
	float m_fBottom;
	float m_fNear;
	float m_fFar;
	bool m_bOrtho;
	uint8_t padding19[3];
};
