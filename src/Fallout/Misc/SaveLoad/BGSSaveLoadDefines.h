#pragma once

#include "Reconstruction.hpp"

class BGSSaveLoadFile;
enum SIZE_TYPE : int32_t;

// Reports a save game problem through BGSSaveLoadGame's warning function when a save/load game is running; cdecl and
// variadic, a printf-style message (0x845CC0).
void SaveGameWarning(const char* apMessage, ...);
// Writes aiSize bytes of apData to the save file apFile (a BGSSaveLoadFile; src has no such
// class); cdecl, 0x845D00.
void SaveGameDataToFileFunction(BGSSaveLoadFile* apFile, void* apData, uint32_t aiSize);
// Reads aiSize bytes of the save file apFile (a BGSSaveLoadFile; src has no such class) into
// apData and returns the bytes read.
void LoadGameDataFromFileFunction(BGSSaveLoadFile* apFile, void* apData, uint32_t aiSize);
// The size type for a size of aiSize.
SIZE_TYPE GetSizeTypeForSize(uint32_t aiSize);
// The size of a size type.
uint32_t GetSizeTypeSize(SIZE_TYPE aeSizeType);
// Writes aiSize (a size of the type aeSizeType) to the save file apFile.
void SaveGameSizeToFile(BGSSaveLoadFile* apFile, uint32_t aiSize, SIZE_TYPE aeSizeType);
// Writes aiSize (a size of the type aeSizeType) to the buffer apBuffer.
void SaveGameSizeToBuffer(char* apBuffer, uint32_t aiSize, SIZE_TYPE aeSizeType);
// Reads a size of the type aeSizeType from apBuffer into ariSize.
uint32_t LoadGameSizeFromBuffer(const char* apBuffer, uint32_t& ariSize, SIZE_TYPE aeSizeType);
// Reads a value saved with a variable size prefix from apBuffer into ariValue.
uint32_t LoadVariableSizedValueFromBuffer(const char* apBuffer, uint32_t& ariVariableValue);
uint32_t GetVariableSizedValueSize(uint32_t aiVariableValue);
void SaveVariableSizedValueToBuffer(char* apBuffer, uint32_t aiVariableValue);
uint32_t LoadGameSizeFromFile(BGSSaveLoadFile* apFile, uint32_t& ariSize, SIZE_TYPE aeSizeType);

using iSizeTypeSize_t = int32_t[3];
inline iSizeTypeSize_t iSizeTypeSize{1, 2, 4};
