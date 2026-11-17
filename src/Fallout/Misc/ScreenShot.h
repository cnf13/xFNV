#pragma once

#include "BSMain/Setting.h"
#include "Reconstruction.hpp"

class NiPixelData;

// The 54 bytes a .bmp screenshot starts with: the file header (the "BM" tag, the file's size, two reserved words, the
// offset of the pixel data), then the 40-byte BITMAPINFOHEADER, packed, built in one frame object and written with one
// 0x36-byte write (TakeScreenShot 0x878DDD, TakeSaveGameScreenShot 0x879783). The PDB's unnamed local type of
// TakeSaveGameScreenShot's bmpHeader, with its field names; the struct's name is chosen here.
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
	// Inferred type: apName: char* from 0x878b0f (passed to BSstrcpy 2)
	static void TakeScreenShot(char* apName);
};

// INISettingCollection: 0xfa3b5d calls 0x45cfe0.
extern SettingT<INISettingCollection> g_sScreenShotBaseName_Display;

// INIPrefSettingCollection: 0xfa3b8a calls 0x719c00.
extern SettingT<INIPrefSettingCollection> g_iScreenShotIndex_Display;
