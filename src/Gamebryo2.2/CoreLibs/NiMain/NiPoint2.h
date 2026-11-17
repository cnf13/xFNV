#pragma once

#include <cstdint>

class NiStream;

class NiPoint2 {
public:
	NiPoint2() = default;
	// Constexpr so that a global built from two floats is constant data.
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
