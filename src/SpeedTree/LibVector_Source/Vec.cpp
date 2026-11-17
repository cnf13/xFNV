#include "SpeedTree/LibVector_Source/IdvVector.h"

#include <cmath>

CVec::CVec() : m_afData{0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, m_nSize(3) {}

// The size is capped at the five components.
CVec::CVec(int32_t nSize) : m_afData{0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, m_nSize(nSize > 5 ? 5 : nSize) {}

CVec::CVec(float fX, float fY) : m_afData{fX, fY, 0.0f, 0.0f, 0.0f}, m_nSize(2) {}

CVec::CVec(float fX, float fY, float fZ, float fW, float fV) : m_afData{fX, fY, fZ, fW, fV}, m_nSize(5) {}

CVec::~CVec() {}

// The result is built in a local CVec of the smaller of the two sizes and returned by copy. The loop runs to
// this->m_nSize with a signed compare, reloaded each pass.
CVec CVec::operator+(const CVec& cVec) const {
	CVec cSum(m_nSize < cVec.m_nSize ? m_nSize : cVec.m_nSize);
	for (int32_t i = 0; i < m_nSize; ++i)
		cSum.m_afData[i] = m_afData[i] + cVec.m_afData[i];
	CVec cResult = cSum;
	return cResult;
}

// Same shape as operator+, with a subtraction.
CVec CVec::operator-(const CVec& cVec) const {
	CVec cDiff(m_nSize < cVec.m_nSize ? m_nSize : cVec.m_nSize);
	for (int32_t i = 0; i < m_nSize; ++i)
		cDiff.m_afData[i] = m_afData[i] - cVec.m_afData[i];
	CVec cResult = cDiff;
	return cResult;
}

CVec CVec::operator*(float fValue) const {
	CVec cScaled(m_nSize);
	for (int32_t i = 0; i < m_nSize; ++i)
		cScaled.m_afData[i] = m_afData[i] * fValue;
	CVec cResult = cScaled;
	return cResult;
}

float CVec::Magnitude() const {
	float v5 = 0.0f;
	for (int32_t i = 0; i < m_nSize; ++i) {
		v5 = m_afData[i] * m_afData[i] + v5;
	}
	return std::sqrt(static_cast<double>(v5));
}

void CVec::Normalize() {
	float v3 = Magnitude();
	if (v3 != 0.0) {
		for (int32_t i = 0; i < m_nSize; ++i) {
			m_afData[i] = m_afData[i] / v3;
		}
	}
}
