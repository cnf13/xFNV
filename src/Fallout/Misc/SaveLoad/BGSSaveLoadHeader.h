#pragma once

#include <cstdint>

class BGSSaveLoadFile;
class BGSSaveLoadFileEntry;
class NiTexture;

// Reads and writes the header at the start of a save file: the player, location and play time texts and the
// screenshot.
class BGSSaveLoadHeader {
public:
    // Reads the header texts of apFile into apFileEntry.
    static void LoadHeaderData(BGSSaveLoadFile* apFile, BGSSaveLoadFileEntry* apFileEntry);
    // Reads the aiWidth x aiHeight screenshot at aiOffset of apFile into a new texture.
    static NiTexture*
    LoadHeaderScreenshot(BGSSaveLoadFile* apFile, uint32_t aiOffset, uint32_t aiWidth, uint32_t aiHeight);
    // Writes the header of apFile.
    static void SaveHeader(BGSSaveLoadFile* apFile);
    static int32_t LoadHeader(BGSSaveLoadFile* apFile);
};

inline const char* pSaveGameHeaderString = "FO3SAVEGAME";
