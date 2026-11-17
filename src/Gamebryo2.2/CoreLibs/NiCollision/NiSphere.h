#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

class NiStream;

// a sphere by centre and radius
class NiSphere {
public:
	// Exact compare of centre and radius (unordered, i.e. NaN, is unequal).
	bool operator==(const NiSphere& kSphere) const;

	// The centre as three 4-byte transfers, then the radius.
	void LoadBinary(NiStream& kStream);
	void SaveBinary(NiStream& kStream);
	NiSphere* Copy(NiSphere* apSphere);
	// NiSphere has NiBound's layout (centre, radius) and Copy calls it on this.
	void SetCenterAndRadius(const NiPoint3& kCenter, float fRadius);

	NiPoint3 m_kCenter;
	float m_fRadius;
};
