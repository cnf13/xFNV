#pragma once

#include <cstdint>

class CVec3;

// SpeedTree's vector of up to five floats and a size. CVec3 and CTransform (this file's other mirrors) live in
// SpeedTree/SpeedTreeRT.h, CRotTransform in SpeedTree/CRotTransform.h. CTransform::EInversionCodeType {OK,
// SINGULAR} is not declared here: it is nested in CTransform, which SpeedTreeRT.h owns.
class CVec {
public:
	CVec();
	CVec(int32_t nSize);
	// A two-component vector.
	CVec(float fX, float fY);
	CVec(float fX, float fY, float fZ, float fW, float fV);
	~CVec();

	CVec operator+(const CVec& cVec) const;
	CVec operator-(const CVec& cVec) const;
	CVec operator*(float fValue) const;
	// The square root of the sum of the first m_nSize squared components.
	float Magnitude() const;
	// Divides the first m_nSize components by Magnitude() when it is not zero.
	void Normalize();

	float m_afData[5];
	int32_t m_nSize;
};

// An axis-aligned box: the minimum and maximum corners. The constructor constructs both CVecs and then initialises the
// box to the unset state.
class CRegion {
public:
	CRegion();

	CRegion operator+(const CRegion& cRegion) const;
	CRegion operator^(const CVec3& cVec) const;
	float* Initialize();

	CVec m_cMin;
	CVec m_cMax;
};

// SpeedTree's angle constants. The three computed ones are statics in every file that includes the header: one copy per
// file, each the double product stored as a float.
const float c_fPi = 3.1415927f;
static const float c_fHalfPi = static_cast<float>(c_fPi * 0.5);
static const float c_fQuarterPi = static_cast<float>(c_fPi * 0.25);
static const float c_fTwoPi = static_cast<float>(c_fPi * 2.0);
