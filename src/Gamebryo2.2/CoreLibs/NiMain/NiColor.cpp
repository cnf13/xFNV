#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <cstring>

NiColor NiColor_WHITE{1.0f, 1.0f, 1.0f};
NiColor NiColor_RED{1.0f, 0.0f, 0.0f};
NiColor NiColor_GREEN{0.0f, 1.0f, 0.0f};
NiColor NiColor_BLUE{0.0f, 0.0f, 1.0f};
NiColor NiColor_YELLOW{1.0f, 1.0f, 0.0f};
NiColor NiColor_PURPLE{1.0f, 0.0f, 1.0f};
NiColor NiColor_CYAN{0.0f, 1.0f, 1.0f};

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

float NiColor::Luminance() const {
    return g * 0.36f + r * 0.31f + b * 0.32f;
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
        pcString, stLength, "%s = (%5.3f,%5.3f,%5.3f)", pPrefix, static_cast<double>(r), static_cast<double>(g),
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

NiColor* NiColor::Add(NiColor* a2, NiColor* a3) {
    NiColor v4;
    v4.r = (r + a3->r);
    v4.g = (g + a3->g);
    v4.b = (b + a3->b);
    *a2 = v4;
    return a2;
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
        pcString, stLength, "%s = (%5.3f,%5.3f,%5.3f,%5.3f)", pPrefix, static_cast<double>(r), static_cast<double>(g),
        static_cast<double>(b), static_cast<double>(a)
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

NiColorA NiColorA::Divide(float a3) {
    NiColorA v4;
    v4.r = (r / a3);
    v4.g = (g / a3);
    v4.b = (b / a3);
    v4.a = (a / a3);
    return v4;
}
