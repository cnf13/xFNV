#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <cstring>

NiColor NiColor::WHITE{1.0f, 1.0f, 1.0f};
NiColor NiColor::RED{1.0f, 0.0f, 0.0f};
NiColor NiColor::GREEN{0.0f, 1.0f, 0.0f};
NiColor NiColor::BLUE{0.0f, 0.0f, 1.0f};
NiColor NiColor::YELLOW{1.0f, 1.0f, 0.0f};
NiColor NiColor::PURPLE{1.0f, 0.0f, 1.0f};
NiColor NiColor::CYAN{0.0f, 1.0f, 1.0f};

NiColor::NiColor(float fR, float fG, float fB) : r(fR), g(fG), b(fB) {}

void NiColor::Clamp() {
    if (r > 1.0f) {
        r = 1.0f;
    }
    if (g > 1.0f) {
        g = 1.0f;
    }
    if (b > 1.0f) {
        b = 1.0f;
    }
}

// The weighted sum is computed in double and rounded to float once (x87, the float weights widened
// to double).
float NiColor::Luminance() const {
    return static_cast<float>(
        static_cast<double>(g) * 0.36f + static_cast<double>(r) * 0.31f + static_cast<double>(b) * 0.32f
    );
}

bool NiColor::operator==(const NiColor& c) const {
    return r == c.r && g == c.g && b == c.b;
}

bool NiColor::operator!=(const NiColor& c) const {
    return !(r == c.r && g == c.g && b == c.b);
}

void NiColor::LoadBinary(NiStream& stream) {
    NiStreamLoadBinary(stream, r);
    NiStreamLoadBinary(stream, g);
    NiStreamLoadBinary(stream, b);
}

void NiColor::SaveBinary(NiStream& stream) {
    NiStreamSaveBinary(stream, r);
    NiStreamSaveBinary(stream, g);
    NiStreamSaveBinary(stream, b);
}

char* NiColor::GetViewerString(const char* pPrefix) const {
    size_t stLength = strlen(pPrefix) + 34;
    char* pcString = static_cast<char*>(_NiMalloc(stLength));
    NiSprintf(
        pcString,
        stLength,
        "%s = (%5.3f,%5.3f,%5.3f)",
        pPrefix,
        static_cast<double>(r),
        static_cast<double>(g),
        static_cast<double>(b)
    );
    return pcString;
}

NiColor& NiColor::operator+=(const NiColor& c) {
    r += c.r;
    g += c.g;
    b += c.b;
    return *this;
}

NiColor* NiColor::Add(NiColor* pkResult, NiColor* pkColor) {
    NiColor v4;
    v4.r = (r + pkColor->r);
    v4.g = (g + pkColor->g);
    v4.b = (b + pkColor->b);
    *pkResult = v4;
    return pkResult;
}

bool NiColorA::operator==(const NiColorA& c) const {
    return r == c.r && g == c.g && b == c.b && a == c.a;
}

bool NiColorA::operator!=(const NiColorA& c) const {
    return !(*this == c);
}

void NiColorA::LoadBinary(NiStream& stream) {
    NiStreamLoadBinary(stream, r);
    NiStreamLoadBinary(stream, g);
    NiStreamLoadBinary(stream, b);
    NiStreamLoadBinary(stream, a);
}

void NiColorA::SaveBinary(NiStream& stream) {
    NiStreamSaveBinary(stream, r);
    NiStreamSaveBinary(stream, g);
    NiStreamSaveBinary(stream, b);
    NiStreamSaveBinary(stream, a);
}

char* NiColorA::GetViewerString(const char* pPrefix) const {
    size_t stLength = strlen(pPrefix) + 36;
    char* pcString = static_cast<char*>(_NiMalloc(stLength));
    NiSprintf(
        pcString,
        stLength,
        "%s = (%5.3f,%5.3f,%5.3f,%5.3f)",
        pPrefix,
        static_cast<double>(r),
        static_cast<double>(g),
        static_cast<double>(b),
        static_cast<double>(a)
    );
    return pcString;
}

NiColorA& NiColorA::Scale(float afScale) {
    r = r * afScale;
    g = g * afScale;
    b = b * afScale;
    a = a * afScale;
    return *this;
}

NiColorA NiColorA::operator/(float fScalar) const {
    NiColorA v4;
    v4.r = (r / fScalar);
    v4.g = (g / fScalar);
    v4.b = (b / fScalar);
    v4.a = (a / fScalar);
    return v4;
}
