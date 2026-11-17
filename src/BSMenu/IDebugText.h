#pragma once

#include <cstdarg>
#include <cstdint>

class NiColorA;

// The side of the screen a line is placed on.
enum DEBUGTEXT_LOCATION : int32_t {
    DTL_LEFT = 0,
    DTL_RIGHT = 1,
    DTL_CENTER = 2,
    DTL_COUNT = 3,
};

// How a line is justified at its x. 0 is no justification: DebugText's free text slots hold 0.
enum DEBUGTEXT_JUSTIFY : int32_t {
    DTJ_LEFT = 1,
    DTJ_CENTER = 2,
    DTJ_RIGHT = 3,
    DTJ_COUNT = 4,
};

// The debug text interface DebugText implements. It has no destructor: its functions are the print functions, all pure.
// The Print overloads are C-variadic, and in each overload pair the short overload is declared first.
class IDebugText {
public:
    virtual void Reset() = 0;
    virtual void SkipLine(DEBUGTEXT_LOCATION aeLocation, int32_t aiSkip) = 0;
    virtual int32_t PrintX(int32_t aiX, const char* apFormatString, ...) = 0;
    virtual int32_t PrintX(
        int32_t aiX,
        DEBUGTEXT_LOCATION aeLocation,
        DEBUGTEXT_JUSTIFY aeJustify,
        const char* apFormatString,
        ...
    ) = 0;
    virtual int32_t VAPrintX(
        int32_t aiX,
        DEBUGTEXT_LOCATION aeLocation,
        DEBUGTEXT_JUSTIFY aeJustify,
        const char* apFormatString,
        va_list aArgs,
        const NiColorA* apColor
    ) = 0;
    virtual int32_t
    PrintLine(DEBUGTEXT_LOCATION aeLocation, DEBUGTEXT_JUSTIFY aeJustify, const char* apFormatString, ...) = 0;
    virtual int32_t VAPrintLine(
        DEBUGTEXT_LOCATION aeLocation,
        DEBUGTEXT_JUSTIFY aeJustify,
        const char* apFormatString,
        va_list aArgs,
        const NiColorA* apColor
    ) = 0;
    virtual int32_t PrintXC(int32_t aiX, const NiColorA& aColor, const char* apFormatString, ...) = 0;
    virtual int32_t PrintXC(
        int32_t aiX,
        const NiColorA& aColor,
        DEBUGTEXT_LOCATION aeLocation,
        DEBUGTEXT_JUSTIFY aeJustify,
        const char* apFormatString,
        ...
    ) = 0;
    virtual int32_t PrintLineC(
        const NiColorA& aColor,
        DEBUGTEXT_LOCATION aeLocation,
        DEBUGTEXT_JUSTIFY aeJustify,
        const char* apFormatString,
        ...
    ) = 0;
    virtual int32_t GetCurrentLine(DEBUGTEXT_LOCATION aeLocation) = 0;
    virtual void SetCurrentLine(DEBUGTEXT_LOCATION aeLoc, int32_t aiLine) = 0;
};
