#include "SpeedTree/LibSpline_Source/IdvSpline.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/FaceGen/FR2MatrixVTC.h"
#include "SDK/MSVC/crtdefs.h"
#include "SDK/MSVC/exception.h"
#include "SDK/MSVC/iosfwd.h"
#include "SDK/MSVC/stdexcept.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"
#include "SpeedTree/LibVector_Source/IdvVector.h"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

g_mSplineCache_t g_mSplineCache{g_mSplineCache_t::key_compare{}, std::allocator<g_mSplineCache_t::value_type>{}};

const char* NextToken(const char* pInput, char* pToken) {
	while (std::isspace(*pInput)) {
		++pInput;
	}
	std::sscanf(pInput, "%s", pToken);
	return &pInput[std::strlen(pToken)];
}

CIdvBezierSpline::CIdvBezierSpline() : m_fMin(0.0f), m_fMax(1.0f), m_fVariance(0.0f) {}

// A spline already parsed from the same text is copied from the cache; otherwise the text is parsed and a copy
// cached.
CIdvBezierSpline::CIdvBezierSpline(const std::string& strInput) {
	CIdvBezierSpline* pLookup = g_mSplineCache[strInput];
	if (pLookup) {
		*this = *pLookup;
	} else {
		Parse(strInput);
		CreateEvenlySpacedPoints(500u);
		g_mSplineCache[strInput] = new CIdvBezierSpline(this);
	}
}

// The empty vectors, then a copy of a1.
CIdvBezierSpline::CIdvBezierSpline(CIdvBezierSpline* a1) {
	*this = *a1;
}

CIdvBezierSpline::~CIdvBezierSpline() {}

// The control point count divides as signed (idiv) and is compared signed (jle, jge); each index into the vectors is
// checked unsigned (operator[]). The clamps keep fPercent unless it is above 0 (fcompp, test ah,0x41) and below 1;
// a NaN takes the bound. The segment test is fPercent >= i (test ah,1) and fPercent < i + 1 (test ah,5; jp), both
// against the exact integer (fild). The segment's four spline points are indexed last first, as MSVC pushes them.
// The result is built in a local CVec(2), copied out whole and the local destroyed. The exe's cleanup-only EH frame
// is left out.
CVec CIdvBezierSpline::EvaluateRawPoint(float fPercent) {
	CVec cPoint;
	cPoint.~CVec();
	::new (&cPoint) CVec(2);
	int32_t nNumPoints = static_cast<int32_t>(m_vControlPoints.size());
	if (nNumPoints > 1) {
		float fZero = 0.0f;
		fPercent = *(fPercent > fZero ? &fPercent : &fZero);
		float fOne = 1.0f;
		fPercent = *(fOne > fPercent ? &fPercent : &fOne);
		fPercent = static_cast<float>(static_cast<double>(nNumPoints - 1) * fPercent);
		float fT = static_cast<float>(fPercent - static_cast<double>(static_cast<int32_t>(fPercent)));
		bool bDone = false;
		for (int32_t i = 0; i < nNumPoints && !bDone; ++i) {
			if (i == nNumPoints - 1) {
				cPoint = m_vControlPoints[i];
				bDone = true;
			} else if (fPercent >= static_cast<double>(i) && fPercent < static_cast<double>(i + 1)) {
				const CVec& cVec4 = m_vSplinePoints[3 * i + 3];
				const CVec& cVec3 = m_vSplinePoints[3 * i + 2];
				const CVec& cVec2 = m_vSplinePoints[3 * i + 1];
				const CVec& cVec1 = m_vSplinePoints[3 * i];
				CVec cSegment = SplineInterpolate(cVec1, cVec2, cVec3, cVec4, fT);
				cPoint = cSegment;
				bDone = true;
			}
		}
	}
	CVec cResult = cPoint;
	return cResult;
}

// The function-local static CIdvRandom (guarded by bit 0 of 0x11F8E14, the exe's atexit destructor thunk 0xFDC700).
// Every subscript is the checked operator[] (the index compared unsigned, the size divided signed). The exe's
// cleanup-only C++ EH frame is left out.
float CIdvBezierSpline::Evaluate(float fPercent) {
	float fValue = 0.0f;
	if (m_vEvenlySpacedPoints.size() == 500) {
		int32_t nClosest = static_cast<int32_t>(fPercent * 499.0);
		if (nClosest == 499) {
			fValue = m_vEvenlySpacedPoints[static_cast<size_t>(nClosest)].m_afData[1];
		} else {
			float fInterpPercent = (fPercent - static_cast<double>(nClosest) * 0.002004008f) / 0.002004008f;
			int32_t nNext = nClosest + 1;
			const CVec& cNext = m_vEvenlySpacedPoints[static_cast<size_t>(nNext)];
			float v6 = m_vEvenlySpacedPoints[static_cast<size_t>(nClosest)].m_afData[1];
			fValue = fInterpPercent * (cNext.m_afData[1] - v6) + v6;
		}
		fValue = (m_fMax - m_fMin) * fValue + m_fMin;
		// The PDB's name (S_LDATA32 under CIdvBezierSpline::Evaluate).
		static CIdvRandom cRandom;
		fValue = cRandom.GetUniform(-m_fVariance, m_fVariance) + fValue;
	}
	return fValue;
}

// The function-local static CIdvRandom (guarded by bit 0 of 0x11F8E1C, the exe's atexit destructor thunk 0xFDC710).
// The exe's cleanup-only C++ EH frame is left out.
float CIdvBezierSpline::ScaledVariance(float fPercent) const {
	float fValue = 0.0f;
	if (m_vEvenlySpacedPoints.size() == 500) {
		int32_t nClosest = static_cast<int32_t>(fPercent * 499.0 + 0.5);
		fValue = m_vEvenlySpacedPoints[static_cast<size_t>(nClosest)].m_afData[1];
		// The PDB's name (S_LDATA32 under CIdvBezierSpline::ScaledVariance).
		static CIdvRandom cRandom;
		float fMax = m_fVariance * fValue;
		float fMin = -m_fVariance * fValue;
		fValue = cRandom.GetUniform(fMin, fMax);
	}
	return fValue;
}

CVec CIdvBezierSpline::SplineInterpolate(const CVec& cVec1, const CVec& cVec2, const CVec& cVec3, const CVec& cVec4,
	float x) {
	float v16 = x * (cVec2.m_afData[0] - cVec1.m_afData[0]) + cVec1.m_afData[0];
	float v17 = x * (cVec2.m_afData[1] - cVec1.m_afData[1]) + cVec1.m_afData[1];
	float v10 = x * (cVec3.m_afData[0] - cVec2.m_afData[0]) + cVec2.m_afData[0];
	float v11 = x * (cVec3.m_afData[1] - cVec2.m_afData[1]) + cVec2.m_afData[1];
	float v12 = x * (cVec4.m_afData[0] - cVec3.m_afData[0]) + cVec3.m_afData[0];
	float v13 = x * (cVec4.m_afData[1] - cVec3.m_afData[1]) + cVec3.m_afData[1];
	float v18 = x * (v10 - v16) + v16;
	float v19 = x * (v11 - v17) + v17;
	float v14 = x * (v12 - v10) + v10;
	float v15 = x * (v13 - v11) + v11;
	float a2 = x * (v15 - v19) + v19;
	float a1 = x * (v14 - v18) + v18;
	return CVec(a1, a2);
}

// speedtree-2's body. Both resize(n) calls pass a default CVec built out of line (CVec() sets size 3) by value,
// copied from the pointer the constructor returns (eax), and destroy it after the call; each loop temporary is
// destroyed through ~CVec too. Each percent divides the
// zero-extended index by the zero-extended count (fild qword) and rounds to float. The segment test is
// fPercent >= x[j] (test ah,0x41; jp) and fPercent < x[j + 1] (test ah,0x41; jne); a NaN fails both. Every
// subscript is the checked operator[] (compared unsigned, the size divided signed), in the exe's order: an
// assignment's source first, VecInterpolate's arguments last first. The local vector's ~vector is inlined: _Tidy,
// then ourFree of the iterator proxy. The exe's cleanup-only EH frame is left out.
void CIdvBezierSpline::CreateEvenlySpacedPoints(uint32_t nNumPoints) {
	std::vector<CVec, std::allocator<CVec>> vRawPoints;
	vRawPoints.~vector<CVec, std::allocator<CVec>>();
	::new (&vRawPoints) std::vector<CVec, std::allocator<CVec>>;
	CVec cRawDefault;
	CVec* pRawDefault = FNVD_RECONSTRUCT(CVec)(&cRawDefault);
	vRawPoints.resize(nNumPoints, *pRawDefault);

	uint32_t i = 0;
	for (i = 0; i < nNumPoints; ++i) {
		float fRawPercent = static_cast<double>(i) / static_cast<double>(nNumPoints);
		CVec cRawPoint = EvaluateRawPoint(fRawPercent);
		vRawPoints[i] = cRawPoint;
	}

	m_vEvenlySpacedPoints.clear();
	CVec cEvenDefault;
	CVec* pEvenDefault = FNVD_RECONSTRUCT(CVec)(&cEvenDefault);
	m_vEvenlySpacedPoints.resize(nNumPoints, *pEvenDefault);
	uint32_t nIndex = 0;
	m_vEvenlySpacedPoints[0] = m_vControlPoints[0];

	for (i = 1; i < nNumPoints - 1; ++i) {
		float fPercent = static_cast<double>(i) / static_cast<double>(nNumPoints);
		for (uint32_t j = nIndex; j < nNumPoints - 1; ++j) {
			if (fPercent >= vRawPoints[j].m_afData[0] && fPercent < vRawPoints[j + 1].m_afData[0]) {
				nIndex = j;
				break;
			}
		}
		CVec cOut;
		cOut.~CVec();
		::new (&cOut) CVec(fPercent, 0.0f, 0.0f, 0.0f, 0.0f);
		const CVec& cLow = vRawPoints[nIndex];
		const CVec& cHigh = vRawPoints[nIndex + 1];
		const CVec& cLowAgain = vRawPoints[nIndex];
		fPercent = (static_cast<double>(fPercent) - cLow.m_afData[0]) /
			(static_cast<double>(cHigh.m_afData[0]) - cLowAgain.m_afData[0]);
		// VecInterpolate(float), inlined; the start through a float copy.
		const CVec& cEnd = vRawPoints[nIndex + 1];
		const CVec& cStart = vRawPoints[nIndex];
		float fStart = cStart.m_afData[1];
		float fValue = (static_cast<double>(cEnd.m_afData[1]) - fStart) * fPercent + fStart;
		cOut.m_afData[1] = fValue;
		m_vEvenlySpacedPoints[i] = cOut;
	}

	m_vEvenlySpacedPoints[nNumPoints - 1] = m_vControlPoints[m_vControlPoints.size() - 1];
}

void CIdvBezierSpline::Parse(const std::basic_string<char, std::char_traits<char>, std::allocator<char>>& strInput) {
	char szToken[256];
	const char* pInput = strInput.c_str();
	const char* pInput_2 = pInput;
	pInput_2 = NextToken(pInput, szToken);
	if (!std::strcmp(szToken, "BezierSpline")) {
		pInput_2 = NextToken(pInput_2, szToken);
		m_fMin = std::atof(szToken);
		pInput_2 = NextToken(pInput_2, szToken);
		m_fMax = std::atof(szToken);
		pInput_2 = NextToken(pInput_2, szToken);
		m_fVariance = std::atof(szToken);
		pInput_2 = NextToken(pInput_2, szToken);
		if (szToken[0] == 123) {
			pInput_2 = NextToken(pInput_2, szToken);
			int32_t nNumControlPoints = std::atoi(szToken);
			for (int32_t i = 0; i < nNumControlPoints; ++i) {
				float afControlPoint[2];
				float afTangent[2];
				pInput_2 = NextToken(pInput_2, szToken);
				afControlPoint[0] = std::atof(szToken);
				pInput_2 = NextToken(pInput_2, szToken);
				afControlPoint[1] = std::atof(szToken);
				pInput_2 = NextToken(pInput_2, szToken);
				afTangent[0] = std::atof(szToken);
				pInput_2 = NextToken(pInput_2, szToken);
				afTangent[1] = std::atof(szToken);
				pInput_2 = NextToken(pInput_2, szToken);
				float fTangentLength = std::atof(szToken);
				AddControlPoint(afControlPoint, afTangent, fTangentLength);
			}
		}
	}
}

// The point and its normalised tangent are two-component CVecs (0xB3C500). After the first point, the previous
// point's outgoing handle (point + tangent * length) and this point's incoming handle (point - tangent * length) go
// into the spline points first. Every subscript is the checked operator[] (the index compared unsigned, the size
// divided signed). Each CVec temporary is destroyed through ~CVec (0xAA7830) where the exe destroys it. The exe's
// cleanup-only C++ EH frame is left out.
void CIdvBezierSpline::AddControlPoint(float* afPoint, float* afTangent, float fTangentLength) {
	CVec cPoint(afPoint[0], afPoint[1]);
	CVec cTangent(afTangent[0], afTangent[1]);
	cTangent.Normalize();
	if (m_vControlPoints.size() != 0) {
		uint32_t nPrev = static_cast<uint32_t>(m_vControlPoints.size() - 1);
		float* pPrevLength = &m_vControlPointTangentLengths[nPrev];
		const CVec& cPrevTangent = m_vControlPointTangents[nPrev];
		CVec cOutOffset = cPrevTangent * *pPrevLength;
		CVec cOutHandle = m_vControlPoints[nPrev] + cOutOffset;
		m_vSplinePoints.push_back(cOutHandle);
		CVec cInOffset = cTangent * fTangentLength;
		CVec cInHandle = cPoint - cInOffset;
		m_vSplinePoints.push_back(cInHandle);
	}
	m_vControlPoints.push_back(cPoint);
	m_vSplinePoints.push_back(cPoint);
	m_vControlPointTangents.push_back(cTangent);
	m_vControlPointTangentLengths.push_back(fTangentLength);
}

// The exe calls begin() const and the const_iterator's operator* (0xB3B090) and stores through its result: VC9's
// iterator::operator*, inlined, is that call with the const cast away (`(reference)**(const_iterator *)this`). end()
// is the const one, inlined.
void CIdvBezierSpline::ClearCache() {
	g_mSplineCache_t::const_iterator hProcess = g_mSplineCache.begin();
	while (hProcess != std::as_const(g_mSplineCache).end()) {
		CIdvBezierSpline* Block = hProcess->second;
		if (Block) {
			Block->~CIdvBezierSpline();
			ourFree(Block);
		}
		// EXE-INLINED none: iterator::operator*
		const_cast<g_mSplineCache_t::value_type&>(*hProcess).second = 0;
		++hProcess;
	}
	g_mSplineCache.clear();
}
