#include "Havok/Source/Common/Base/Types/Color/hkColor.h"

#include "Havok/Source/Common/Base/Algorithm/PseudoRandom/hkPseudoRandomGenerator.h"
#include "Havok/Source/Common/Base/Math/hkMath.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"

#include <cmath>

static void cleanupLoadedObjecthkColor(void* p);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkColorTypeInfo =
    {"hkColor", nullptr, cleanupLoadedObjecthkColor, nullptr, &typeid(hkColor), sizeof(hkColor)};

// getRandomColor()'s guarded static generator, seeded with 0, and its guard.
static hkPseudoRandomGenerator s_randomColorGenerator{};
static uint32_t s_randomColorGenerator_guard;

static void cleanupLoadedObjecthkColor(void* p) {
    ;
}

// hkColorClass's enum ExtendedColors and its items, the colours as ARGB values.
using hkColorExtendedColorsEnumItemArray = const hkClassEnum::Item[143];
static hkColorExtendedColorsEnumItemArray hkColorExtendedColorsEnumItems{
    {-8388608, "MAROON"},
    {-7667712, "DARKRED"},
    {-65536, "RED"},
    {-18751, "LIGHTPINK"},
    {-2354116, "CRIMSON"},
    {-2396013, "PALEVIOLETRED"},
    {-38476, "HOTPINK"},
    {-60269, "DEEPPINK"},
    {-3730043, "MEDIUMVIOLETRED"},
    {-8388480, "PURPLE"},
    {-7667573, "DARKMAGENTA"},
    {-2461482, "ORCHID"},
    {-2572328, "THISTLE"},
    {-2252579, "PLUM"},
    {-1146130, "VIOLET"},
    {-65281, "FUCHSIA"},
    {-65281, "MAGENTA"},
    {-4565549, "MEDIUMORCHID"},
    {-7077677, "DARKVIOLET"},
    {-6737204, "DARKORCHID"},
    {-7722014, "BLUEVIOLET"},
    {-11861886, "INDIGO"},
    {-7114533, "MEDIUMPURPLE"},
    {-9807155, "SLATEBLUE"},
    {-8689426, "MEDIUMSLATEBLUE"},
    {-16777077, "DARKBLUE"},
    {-16777011, "MEDIUMBLUE"},
    {-16776961, "BLUE"},
    {-16777088, "NAVY"},
    {-15132304, "MIDNIGHTBLUE"},
    {-12042869, "DARKSLATEBLUE"},
    {-12490271, "ROYALBLUE"},
    {-10185235, "CORNFLOWERBLUE"},
    {-5192482, "LIGHTSTEELBLUE"},
    {-984833, "ALICEBLUE"},
    {-460545, "GHOSTWHITE"},
    {-1644806, "LAVENDER"},
    {-14774017, "DODGERBLUE"},
    {-12156236, "STEELBLUE"},
    {-16728065, "DEEPSKYBLUE"},
    {-9404272, "SLATEGRAY"},
    {-8943463, "LIGHTSLATEGRAY"},
    {-7876870, "LIGHTSKYBLUE"},
    {-7876885, "SKYBLUE"},
    {-5383962, "LIGHTBLUE"},
    {-16744320, "TEAL"},
    {-16741493, "DARKCYAN"},
    {-16724271, "DARKTURQUOISE"},
    {-16711681, "CYAN"},
    {-12004916, "MEDIUMTURQUOISE"},
    {-10510688, "CADETBLUE"},
    {-5247250, "PALETURQUOISE"},
    {-2031617, "LIGHTCYAN"},
    {-983041, "AZURE"},
    {-14634326, "LIGHTSEAGREEN"},
    {-12525360, "TURQUOISE"},
    {-5185306, "POWDERBLUE"},
    {-13676721, "DARKSLATEGRAY"},
    {-8388652, "AQUAMARINE"},
    {-16713062, "MEDIUMSPRINGGREEN"},
    {-10039894, "MEDIUMAQUAMARINE"},
    {-16711809, "SPRINGGREEN"},
    {-12799119, "MEDIUMSEAGREEN"},
    {-13726889, "SEAGREEN"},
    {-13447886, "LIMEGREEN"},
    {-16751616, "DARKGREEN"},
    {-16744448, "GREEN"},
    {-16711936, "LIME"},
    {-14513374, "FORESTGREEN"},
    {-7357297, "DARKSEAGREEN"},
    {-7278960, "LIGHTGREEN"},
    {-6751336, "PALEGREEN"},
    {-655366, "MINTCREAM"},
    {-983056, "HONEYDEW"},
    {-8388864, "CHARTREUSE"},
    {-8586240, "LAWNGREEN"},
    {-9728477, "OLIVEDRAB"},
    {-11179217, "DARKOLIVEGREEN"},
    {-6632142, "YELLOWGREEN"},
    {-5374161, "GREENYELLOW"},
    {-657956, "BEIGE"},
    {-331546, "LINEN"},
    {-329006, "LIGHTGOLDENRODYELLOW"},
    {-8355840, "OLIVE"},
    {-256, "YELLOW"},
    {-32, "LIGHTYELLOW"},
    {-16, "IVORY"},
    {-4343957, "DARKKHAKI"},
    {-989556, "KHAKI"},
    {-1120086, "PALEGOLDENROD"},
    {-663885, "WHEAT"},
    {-10496, "GOLD"},
    {-1331, "LEMONCHIFFON"},
    {-4139, "PAPAYAWHIP"},
    {-4684277, "DARKGOLDENROD"},
    {-2448096, "GOLDENROD"},
    {-332841, "ANTIQUEWHITE"},
    {-1828, "CORNSILK"},
    {-133658, "OLDLACE"},
    {-6987, "MOCCASIN"},
    {-8531, "NAVAJOWHITE"},
    {-23296, "ORANGE"},
    {-6972, "BISQUE"},
    {-2968436, "TAN"},
    {-29696, "DARKORANGE"},
    {-2180985, "BURLYWOOD"},
    {-7650029, "SADDLEBROWN"},
    {-744352, "SANDYBROWN"},
    {-5171, "BLANCHEDALMOND"},
    {-3851, "LAVENDERBLUSH"},
    {-2578, "SEASHELL"},
    {-1296, "FLORALWHITE"},
    {-1286, "SNOW"},
    {-3308225, "PERU"},
    {-9543, "PEACHPUFF"},
    {-2987746, "CHOCOLATE"},
    {-6270419, "SIENNA"},
    {-24454, "LIGHTSALMON"},
    {-32944, "CORAL"},
    {-1468806, "DARKSALMON"},
    {-6943, "MISTYROSE"},
    {-47872, "ORANGERED"},
    {-360334, "SALMON"},
    {-40121, "TOMATO"},
    {-4419697, "ROSYBROWN"},
    {-16181, "PINK"},
    {-3318692, "INDIANRED"},
    {-1015680, "LIGHTCORAL"},
    {-5952982, "BROWN"},
    {-5103070, "FIREBRICK"},
    {-16777216, "BLACK"},
    {-9868951, "DIMGRAY"},
    {-8355712, "GRAY"},
    {-5658199, "DARKGRAY"},
    {-4144960, "SILVER"},
    {-2894893, "LIGHTGREY"},
    {-2302756, "GAINSBORO"},
    {-657931, "WHITESMOKE"},
    {-1, "WHITE"},
    {-7829368, "GREY"},
    {-12566464, "GREY25"},
    {-8355712, "GREY50"},
    {-4144960, "GREY75"}
};
using hkColorEnumArray = const hkClassEnum[1];
static hkColorEnumArray hkColorEnums{{"ExtendedColors", hkColorExtendedColorsEnumItems, 143, nullptr, 0}};

const hkClassEnum* hkColorExtendedColorsEnum = &hkColorEnums[0];

hkClass hkColorClass("hkColor", nullptr, 1, nullptr, 0, hkColorEnums, 1, nullptr, 0, nullptr, nullptr, 0, 0);

// The HSV to RGB conversion behind rgbFromHSV: with no saturation all three channels are the value; else the hue times
// 6 picks one of six sectors (5 for a hue of exactly 1), and f, its fraction, gives the channels from p = (1 - s) v, q
// = (1 - f s) v and t = (1 - s (1 - f)) v.
void HSVToRGB(float* apGreen, float* apBlue, float* apRed, float afHue, float afSaturation, float afValue) {
    if (afSaturation == 0.0f) {
        *apBlue = afValue;
        *apGreen = afValue;
        *apRed = afValue;
        return;
    }
    int32_t iSector;
    float fHue6;
    if (afHue == 1.0f) {
        iSector = 5;
        fHue6 = 6.0f;
    }
    else {
        fHue6 = static_cast<float>(afHue * 6.0);
        const float fFloor = static_cast<float>(std::floor(static_cast<double>(fHue6)));
        iSector = static_cast<int32_t>(fFloor);
    }
    const float fFraction = static_cast<float>(fHue6 - static_cast<double>(iSector));
    const double dSaturation = afSaturation;
    const float fP = static_cast<float>((1.0 - dSaturation) * afValue);
    const float fQ = static_cast<float>((1.0 - fFraction * dSaturation) * afValue);
    const float fT = static_cast<float>((1.0 - dSaturation * (1.0 - fFraction)) * afValue);
    switch (iSector) {
        case 0:
            *apRed = afValue;
            *apGreen = fT;
            *apBlue = fP;
            break;
        case 1:
            *apRed = fQ;
            *apGreen = afValue;
            *apBlue = fP;
            break;
        case 2:
            *apRed = fP;
            *apGreen = afValue;
            *apBlue = fT;
            break;
        case 3:
            *apRed = fP;
            *apGreen = fQ;
            *apBlue = afValue;
            break;
        case 4:
            *apRed = fT;
            *apGreen = fP;
            *apBlue = afValue;
            break;
        default:
            *apRed = afValue;
            *apGreen = fP;
            *apBlue = fQ;
            break;
    }
}

int32_t getPaletteColor(char i, int32_t alpha) {
    return alpha << 24 | (hkColor::s_colorTable[i & 31] & 0xFFFFFF);
}

// Packs the channels the way the engine's debug colour tables store them.
int32_t hkColor::rgbFromChars(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha) {
    const uint32_t uiColor = blue | (static_cast<uint32_t>(green) << 8) | (static_cast<uint32_t>(red) << 16) |
        (static_cast<uint32_t>(alpha) << 24);
    return static_cast<int32_t>(uiColor);
}

// Packs four float channels, each scaled by 255 and truncated to a byte, the same way rgbFromChars packs bytes. Scales
// and converts them in the order alpha, red, green, blue, each product rounded to a float before the conversion, and
// packs them in the order red, green, blue, alpha.
int32_t hkColor::rgbFromFloats(float red, float green, float blue, float alpha) {
    const float fAlpha = alpha * 255.0f;
    const int32_t iAlpha = hkMath::hkFloatToInt(fAlpha);
    const float fRed = red * 255.0f;
    const int32_t iRed = hkMath::hkFloatToInt(fRed);
    const float fGreen = green * 255.0f;
    const int32_t iGreen = hkMath::hkFloatToInt(fGreen);
    const float fBlue = blue * 255.0f;
    const int32_t iBlue = hkMath::hkFloatToInt(fBlue);
    return rgbFromChars(
        static_cast<uint8_t>(iRed),
        static_cast<uint8_t>(iGreen),
        static_cast<uint8_t>(iBlue),
        static_cast<uint8_t>(iAlpha)
    );
}

// Converts four HSV channels to a packed colour: the helper writes the three colour channels, which then go through
// rgbFromFloats with the alpha channel.
int32_t hkColor::rgbFromHSV(float h, float s, float v, float alpha) {
    float fRed;
    float fGreen;
    float fBlue;
    HSVToRGB(&fGreen, &fBlue, &fRed, h, s, v);
    return rgbFromFloats(fRed, fGreen, fBlue, alpha);
}

int32_t hkColor::getRandomColor(hkPseudoRandomGenerator& rand) {
    uint32_t v2 = 0x19660D * rand.m_current + 0x3C6EF35F;
    uint32_t current = 0x19660D * (0x19660D * v2 + 0x3C6EF35F) + 0x3C6EF35F;
    rand.m_current = current;
    float blueUnit = static_cast<double>(current) * 2.3283064365386963e-10;
    float blue = blueUnit * (1.0 - 0.0) + 0.0;
    float greenUnit = static_cast<double>(0x19660D * v2 + 0x3C6EF35F) * 2.3283064365386963e-10;
    float green = greenUnit * (1.0 - 0.0) + 0.0;
    float redUnit = 2.3283064365386963e-10 * static_cast<double>(v2);
    float red = (1.0 - 0.0) * redUnit + 0.0;
    return hkColor::rgbFromFloats(red, green, blue, 1.0f);
}

int32_t hkColor::getRandomColor() {
    if ((s_randomColorGenerator_guard & 1) == 0) {
        s_randomColorGenerator_guard |= 1u;
        s_randomColorGenerator.m_seed = 0;
        s_randomColorGenerator.m_current = 0;
    }
    return hkColor::getRandomColor(s_randomColorGenerator);
}

const hkClass& hkColor::staticClass() {
    return hkColorClass;
}
