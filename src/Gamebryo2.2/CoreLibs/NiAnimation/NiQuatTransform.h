#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

#include <cfloat>

class NiStream;

// Translation, rotation and scale of an animated transform. A part an animation channel has no key for stays
// invalid (-FLT_MAX).
class NiQuatTransform : public NiMemObject {
public:
	NiQuatTransform();

	// Sets the rotation from kRotate and marks it valid.
	void SetRotate(const NiQuaternion& kRotate);
	bool IsTranslateValid() const { return m_kTranslate.x != -FLT_MAX; }
	bool IsRotateValid() const { return m_kRotate.m_fX != -FLT_MAX; }
	bool IsScaleValid() const { return m_fScale != -FLT_MAX; }
	bool IsTransformInvalid() const { return !(IsScaleValid() || IsRotateValid() || IsTranslateValid()); }
	void SetScaleValid(bool bValid);
	// An invalid rotation holds -FLT_MAX in its x component.
	void SetRotateValid(bool bValid);

	// Exact compare of every part; invalid parts compare like any other value.
	bool operator==(const NiQuatTransform& kTransform) const;
	// This transform applied on top of arTransform (parent this, child arTransform).
	NiQuatTransform HierApply(const NiQuatTransform& kTransform) const;
	// The part-wise product: scale times scale, rotation times rotation normalized, translation; a part invalid on
	// either side stays invalid.
	NiQuatTransform operator*(const NiQuatTransform& kTransform) const;

	// Files before 10.1.0.110 follow the values with one validity bool per part.
	void LoadBinary(NiStream& kStream);
	void SaveBinary(NiStream& kStream);
	// A title line, then one line per valid part.
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) const;
	void SetScale(float fScale);
	void SetTranslateValid(bool bValid);
	void SetTranslate(const NiPoint3& kTranslate);
	// The inverse into kDest part by part: 1 / scale, the unit inverse of the rotation, the
	// negated translation; an invalid part stays invalid.
	void Invert(NiQuatTransform& kDest) const;
	void HierInvert(NiQuatTransform& kDest);

	NiPoint3 m_kTranslate;
	NiQuaternion m_kRotate;
	float m_fScale;

	// The values a part without keys holds (-FLT_MAX components).
	// Gamebryo's (-NI_INFINITY, -NI_INFINITY, -NI_INFINITY): -FLT_MAX in every component.
	static inline NiPoint3 INVALID_TRANSLATE{-FLT_MAX, -FLT_MAX, -FLT_MAX};
	static inline NiQuaternion INVALID_ROTATE = {-3.40282347e+38f, -3.40282347e+38f, -3.40282347e+38f,
		-3.40282347e+38f};
};
