#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

class NiStream;

// A rotation, translation and uniform scale.
class NiTransform {
public:
	NiTransform();

	// Identity rotation, zero translation, unit scale.
	void MakeIdentity();
	// True when the rotations differ or the translations or scales compare unequal.
	bool operator!=(const NiTransform& xform) const;
	// Rotation * scale of arPoint, plus the translation. Returned through a hidden result pointer.
	NiPoint3 operator*(const NiPoint3& arPoint) const;
	// This transform composed with arXform (arXform applied first). Returned through a hidden result pointer.
	NiTransform operator*(const NiTransform& arXform) const;
	// The inverse of this transform into arDest (transposed rotation, reciprocal scale).
	void Invert(NiTransform& kDest) const;
	// The inverse of this transform into kDest, for a scale that may differ per axis.
	void InvertNonUniform(NiTransform& kDest) const;
	// Rotation, translation, then scale.
	void LoadBinary(NiStream& stream);
	void SaveBinary(NiStream& stream);

	NiMatrix3 m_Rotate;
	NiPoint3 m_Translate;
	float m_fScale;
};
