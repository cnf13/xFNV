#pragma once

#include <cstdint>

// A form type's id and its name strings.
struct FORM_ENUM_STRING {
    uint8_t cFormID;
    uint8_t padding01[3];
    char* pFormString;
    uint32_t iFormString;
};
