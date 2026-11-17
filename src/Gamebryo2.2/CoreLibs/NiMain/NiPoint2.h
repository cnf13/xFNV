#pragma once

#include "Reconstruction.hpp"

class NiStream;

class NiPoint2 {
public:
	NiPoint2() = default;
	// Inline, as Gamebryo's NiPoint2.inl has it; constexpr so that a global built from two floats is constant data.
	// The exe's out-of-line copy (0x452DC0) is registered from NiPoint2.cpp.
	constexpr NiPoint2(float fX, float fY) : x(fX), y(fY) {}

	float SqrLength() const;

	bool operator!=(const NiPoint2& pt) const;
	void LoadBinary(NiStream& arStream);
	void SaveBinary(NiStream& arStream);
	// sqrt(x*x + y*y), the sum rounded to a float before NiSqrt.
	float Length() const;
	bool operator==(const NiPoint2& pt) const;
	float Dot(const NiPoint2& pt) const;
	NiPoint2 operator*(float fScalar) const;
	NiPoint2 operator+(const NiPoint2& pt) const;
	NiPoint2 operator-(const NiPoint2& pt) const;

	// Each coordinate divided by fScalar (Gamebryo's NiPoint2.inl; the exe's out-of-line copy is 0x7F2890, thiscall,
	// ret 8 with the result pointer).
	NiPoint2 operator/(float fScalar) const {
		NiPoint2 result = *this;
		result.x /= fScalar;
		result.y /= fScalar;
		return result;
	}

	float Unitize();
	NiPoint2& operator/=(float fScalar);
	NiPoint2& operator+=(const NiPoint2& pt);
	NiPoint2& operator-=(const NiPoint2& pt);
	NiPoint2& operator*=(float fScalar);

	// apSource scaled by afScale, built in apResult.
	// Inferred type: afScale: float from 0x532beb (parameter 0 of TESImageSpaceModifier::BlendApply)
	static NiPoint2* Scale_(NiPoint2* apResult, float afScale, NiPoint2* apSource);
	static void SaveBinary(NiStream& stream, NiPoint2* pkValues, uint32_t uiNumValues);
	static void LoadBinary(NiStream& stream, NiPoint2* pkValues, uint32_t uiNumValues);

	float x;
	float y;

	static NiPoint2 ZERO;
	static NiPoint2 UNIT_Y;
};
inline NiPoint2 NiPoint2::UNIT_Y{0.0f, 1.0f};
inline NiPoint2 NiPoint2::ZERO{};
