#pragma once

#include "BSMain/Setting.h"

class NiPixelData;

// The 54 bytes a .bmp screenshot starts with: the file header (the "BM" tag, the file's size, two reserved words, the
// offset of the pixel data), then the 40-byte BITMAPINFOHEADER, packed, built in one object and written with one
// 0x36-byte write.
#pragma pack(push, 1)
struct BMPHeader {
    uint16_t bhType;
    uint32_t bhSize;
    uint16_t bhReserved1;
    uint16_t bhReserved2;
    uint32_t bhOffset;
    uint32_t biSize;
    int32_t biWidth;
    int32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t biXPelsPerMeter;
    int32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
};
#pragma pack(pop)

class ScreenShot {
public:
    static NiPixelData* TakeSaveGameScreenShot(int32_t& aiWidth, int32_t& aiHeight, const char* apScreenshotFileName);
    static void TakeScreenShot(char* apName);
};

extern SettingT<INISettingCollection> g_sScreenShotBaseName_Display;

extern SettingT<INIPrefSettingCollection> g_iScreenShotIndex_Display;
