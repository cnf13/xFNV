#pragma once

#include "SDK/MSVC/ehvec.h"

class NiStream;

class NiColor {
public:
    NiColor() = default;
    // A two-channel brace list leaves b at 0 (bhkShape's colour table).
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

    NiColor operator*(float fScalar) const {
        NiColor result = *this;
        result.r *= fScalar;
        result.g *= fScalar;
        result.b *= fScalar;
        return result;
    }

    NiColor operator-() const {
        NiColor result;
        result.r = -r;
        result.g = -g;
        result.b = -b;
        return result;
    }

    NiColor* Add(NiColor* pkResult, NiColor* pkColor);
    NiColor& operator/=(float fScalar);
    NiColor& operator*=(float fScalar);

    float r;
    float g;
    float b;

    static NiColor WHITE;
    static NiColor BLACK;
    static NiColor RED;
    static NiColor GREEN;
    static NiColor BLUE;
    static NiColor YELLOW;
    static NiColor PURPLE;
    static NiColor CYAN;
};

class NiColorA {
public:
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

    // Multiplies all four channels by fScalar and returns the new colour.
    NiColorA operator*(float fScalar) const;
    // Divides all four channels by fScalar and returns the new colour.
    NiColorA operator/(float fScalar) const;

    float r;
    float g;
    float b;
    float a;

    static NiColorA Red;
    static NiColorA Green;
    static NiColorA Blue;
    static NiColorA Yellow;

    static NiColorA WHITE;
    static NiColorA BLACK;
    static NiColorA Magenta;
    static NiColorA CYAN;
};

// The colour times afScalar.
NiColorA operator*(float afScalar, const NiColorA& arColor);

inline NiColor NiColor::BLACK{};

inline NiColorA NiColorA::Red{1.0f, 0.0f, 0.0f, 1.0f};
inline NiColorA NiColorA::Green{0.0f, 1.0f, 0.0f, 1.0f};
inline NiColorA NiColorA::Blue{0.0f, 0.0f, 1.0f, 1.0f};
inline NiColorA NiColorA::Yellow{1.0f, 1.0f, 0.0f, 1.0f};

inline NiColorA NiColorA::WHITE{1.0f, 1.0f, 1.0f, 1.0f};

inline NiColorA NiColorA::BLACK{0.0f, 0.0f, 0.0f, 1.0f};

// The colour of the console's debug path lines.
inline NiColorA NiColorA::Magenta{1.0f, 0.0f, 1.0f, 1.0f};

inline NiColorA NiColorA::CYAN{0.0f, 1.0f, 1.0f, 1.0f};
