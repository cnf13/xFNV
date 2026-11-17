#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

// The exe calls __CIsqrt (0xEC6040) and inlines NiSinCos. |sin| below 0.001 (not NaN) gives a coefficient of 1.
// The result is filled through the out-of-line four-float setter 0xA6DB50 (SetValues: `mov eax, ecx`, ret 0x10).
NiQuaternion NiQuaternion::Exp(const NiQuaternion& q) {
	float fSqrLength = q.m_fY * q.m_fY + q.m_fX * q.m_fX + q.m_fZ * q.m_fZ;
	float fAngle = std::sqrt(fSqrLength);
	// EXE-INLINED 0x4169A0
	float fCos = std::cos(fAngle);
	float fSin = std::sin(fAngle);
	float fAbsSin = __builtin_fabsf(fSin);
	float fCoeff = fAbsSin < 0.001f ? 1.0f : fSin / fAngle;
	float fZ = q.m_fZ * fCoeff;
	float fY = fCoeff * q.m_fY;
	float fX = fCoeff * q.m_fX;
	NiQuaternion kResult;
	kResult.SetValues(fCos, fX, fY, fZ);
	return kResult;
}
