#pragma once

#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

class NiStream;

class NiColor {
public:
	// The exe builds NiColor temporaries with a three-float constructor out of line; src keeps NiColor an aggregate.
	// The hybrid's implicit default constructor (no zero-parameter Construct).
	NiColor() = default;
	// A two-channel brace list leaves b at 0, as the hybrid's aggregate initialisation does (bhkShape's colour table).
	NiColor(float fR, float fG, float fB = 0.0f);

	// Limits each channel to at most 1.0.
	void Clamp();

	// Weighted channel sum used as perceived brightness.
	float Luminance() const;

	// Exact channel compares (unordered, i.e. NaN, counts as unequal).
	bool operator==(const NiColor& c) const;
	bool operator!=(const NiColor& c) const;

	// Reads / writes r, g, b as three 4-byte transfers.
	void LoadBinary(NiStream& stream);
	void SaveBinary(NiStream& stream);

	// "<prefix> = (r,g,b)" with "%5.3f" channels, in a fresh _NiMalloc block the caller owns.
	char* GetViewerString(const char* pPrefix) const;
	// Channel-wise add.
	NiColor& operator+=(const NiColor& c);

	// Each channel times fScalar (Gamebryo's NiColor.inl; the exe's out-of-line copy is 0x633B40, thiscall, ret 8
	// with the result pointer).
	NiColor operator*(float fScalar) const {
		NiColor result = *this;
		result.r *= fScalar;
		result.g *= fScalar;
		result.b *= fScalar;
		return result;
	}

	// Each channel negated, through the hidden result pointer.
	NiColor operator-() const {
		NiColor result;
		result.r = -r;
		result.g = -g;
		result.b = -b;
		return result;
	}

	NiColor* Add(NiColor* a2, NiColor* a3);
	NiColor& operator/=(float fScalar);
	NiColor& operator*=(float fScalar);

	float r;
	float g;
	float b;
};

class NiColorA {
public:
	// Inline field init: the sites where the exe builds the colour inline. Sites where the exe calls its
	// constructor (0x414430) use Construct.
	constexpr NiColorA(float fR = 0.0f, float fG = 0.0f, float fB = 0.0f, float fA = 0.0f)
		: r(fR), g(fG), b(fB), a(fA) {}

	// Exact channel compare (unordered, i.e. NaN, is unequal).
	bool operator==(const NiColorA& c) const;
	bool operator!=(const NiColorA& c) const;

	// Reads / writes r, g, b, a as four 4-byte transfers.
	void LoadBinary(NiStream& stream);
	void SaveBinary(NiStream& stream);

	// "<prefix> = (r,g,b,a)" with "%5.3f" channels, in a fresh _NiMalloc block the caller owns.
	char* GetViewerString(const char* pPrefix) const;

	// Multiplies all four channels by afScale; returns this colour.
	NiColorA& Scale(float afScale);

	// Channel-wise sum and difference (Gamebryo's NiColor.inl; the exe's out-of-line copies are 0x532E40 and
	// 0x532EC0, thiscall, ret 8 with the result pointer).
	NiColorA operator+(const NiColorA& c) const {
		NiColorA result = *this;
		result.r += c.r;
		result.g += c.g;
		result.b += c.b;
		result.a += c.a;
		return result;
	}

	NiColorA operator-(const NiColorA& c) const {
		NiColorA result = *this;
		result.r -= c.r;
		result.g -= c.g;
		result.b -= c.b;
		result.a -= c.a;
		return result;
	}

	// Multiplies all four channels by fScalar and returns the new colour (thiscall, ret 8, 0x532F60; the class result
	// arrives through the hidden pointer).
	NiColorA operator*(float fScalar) const;
	// Inferred type: a3: float: used as one at 0x507506
	NiColorA* Divide(NiColorA* a2, float a3);

	// The same operator in the exe's cdecl form (0x532F40): the result pointer, the scale and the colour are all on
	// the stack. The PDB gives it the same decorated name as the member above.
	static NiColorA* Multiply(NiColorA* apResult, float afScalar, const NiColorA* apColor);

	float r;
	float g;
	float b;
	float a;

	static NiColorA Red;
	static NiColorA Green;
	static NiColorA Blue;
	static NiColorA Yellow;
};

// NiColor::BLACK in the PDB; a global here because the class is incomplete inside its own body.
inline NiColor NiColor_BLACK{};

// NiColor::WHITE in the PDB; a global here because the class is incomplete inside its own body.
extern NiColor NiColor_WHITE;

// NiColor::RED, GREEN, BLUE, YELLOW, PURPLE and CYAN in the PDB, after WHITE in the exe's .data (no code of the exe
// reads them); globals here because the class is incomplete inside its own body.
extern NiColor NiColor_RED;
extern NiColor NiColor_GREEN;
extern NiColor NiColor_BLUE;
extern NiColor NiColor_YELLOW;
extern NiColor NiColor_PURPLE;
extern NiColor NiColor_CYAN;
inline NiColorA NiColorA::Red{1.0f, 0.0f, 0.0f, 1.0f};
inline NiColorA NiColorA::Green{0.0f, 1.0f, 0.0f, 1.0f};
inline NiColorA NiColorA::Blue{0.0f, 0.0f, 1.0f, 1.0f};
inline NiColorA NiColorA::Yellow{1.0f, 1.0f, 0.0f, 1.0f};

// NiColorA::WHITE in the PDB.
inline NiColorA NiColorA_WHITE{1.0f, 1.0f, 1.0f, 1.0f};

// NiColorA::BLACK in the PDB (0, 0, 0, 1).
inline NiColorA NiColorA_BLACK{0.0f, 0.0f, 0.0f, 1.0f};

// IDA name NiColorA::Magenta; not in the PDB. The colour of the console's debug path lines.
inline NiColorA NiColorA_Magenta{1.0f, 0.0f, 1.0f, 1.0f};

// NiColorA::CYAN in the PDB, after NiColorA_Magenta (the PDB's PURPLE); no code of the exe reads it.
inline NiColorA NiColorA_CYAN{0.0f, 1.0f, 1.0f, 1.0f};
