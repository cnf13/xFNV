#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPlane.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Reconstruction.hpp"

// A bounding sphere.
class NiBound : public NiMemObject {
public:
	// The default constructor (thiscall, ret 0, 0x6240D0). Gamebryo's is an empty inline one; defaulted so that a
	// zero global (NiAVObject::NullBoundS) is constant data, as the exe has it.
	NiBound() = default;

	// Grows this sphere to enclose the other one.
	void Merge(const NiBound* pBound);
	void SetCenterAndRadius(const NiPoint3& kCenter, float fRadius);
	// The PDB's operator=, out of line (thiscall, ret 4): SetCenterAndRadius with the other bound's centre and
	// radius, returning this. Named Copy, not operator=: src's other NiBound assignments are the exe's inline copies.
	NiBound* Copy(const NiBound* apBound);
	// 0 = the sphere crosses the plane, 1 = fully on the positive side, 2 = fully on the negative side.
	int32_t WhichSide(const NiPlane& kPlane) const;
	// Sets this bound to arBound moved by arTransform.
	void Update(const NiBound& arBound, const NiTransform& arTransform);
	// Moves this bound by arTransform.
	void Update(const NiTransform& arTransform);
	// Fits the sphere around iQuantity points.
	void ComputeFromData(int32_t iQuantity, const NiPoint3* pkData);
	// The center (m_kCenter); the exe's shared identity function 0x6815C0, thiscall.
	const NiPoint3& GetCenter() const;
	// Copies akCenter into m_kCenter.
	void SetCenter(const NiPoint3& akCenter);
	// Stores afRadius into m_fRadius through x87.
	void SetRadius(float fRadius);
	// Exact compare of center and radius (unordered, i.e. NaN, is unequal).
	bool operator==(const NiBound& bound) const;
	bool operator!=(const NiBound& kBound) const;

	// The center as three 4-byte transfers, then the radius.
	void LoadBinary(NiStream& kStream);
	void SaveBinary(NiStream& kStream);

	// "<prefix> = (x, y, z) , r" with "%g", in a fresh _NiMalloc block the caller owns.
	char* GetViewerString(const char* pPrefix) const;
	void SetCenter(float x, float y, float z);

	NiPoint3 m_kCenter;

	// the radius' bits as an integer
	union {
		float m_fRadius;
		int32_t m_iRadiusAsInt;
	};
};

inline float f0_000001 = 1e-06f;
