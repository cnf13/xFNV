#pragma once

#include <cstddef>
#include <cstdint>

class Actor;
class BGSSaveLoadFile;
class TESForm;
class TESObjectREFR;

// The memory buffer a save game is written into. GetForm, GetReference and GetActor are the form the data belongs to;
// the plain buffer has none.
class BGSSaveGameBuffer {
public:
	BGSSaveGameBuffer();
	~BGSSaveGameBuffer();

	virtual TESForm* GetForm();
	virtual TESObjectREFR* GetReference();
	virtual Actor* GetActor();

	// Appends aiSize bytes of apData to the buffer.
	void SaveDataToBuffer(void* apData, size_t aiSize, bool abDirectSave);
	// Writes aiSize bytes of apData; the third argument is ignored, and every caller passes it as 0.
	void SaveDataEndian(void* apData, uint32_t aiSize, uint32_t aiUnusuedMetaDataArg);
	// Writes apForm's form ID (0 for none) as a three-byte numeric ID index.
	void SaveFormID(TESForm* apForm, uint32_t aiUnusuedMetaDataArg);
	// Writes apString with its length; the second argument is ignored.
	void SaveString(const char* apString, uint32_t aiUnusuedMetaDataArg);
	// Reserves room for a variable-sized value and returns its position.
	uint32_t StartVariableSizedValue();
	// Writes aiVariableValue at aiPosition, reserved by StartVariableSizedValue.
	void SaveVariableSizedValue(uint32_t aiVariableValue, uint32_t aiPosition);
	uint32_t GetBufferPosition();
	// Writes the size of a variable-sized value.
	void SaveVariableSizedValue(uint32_t auiSize);
	// Writes aiFormID as a three-byte numeric ID index.
	void SaveFormID(uint32_t aiFormID, uint32_t aiUnusuedMetaDataArg);
	// Writes aiSize bytes of apData to aiPosition; abDirectSave writes them without the
	// separator bookkeeping.
	void SaveDataToBuffer(void* apData, size_t aiSize, uint32_t aiPosition, bool abDirectSave);
	// Writes aiSize bytes of the buffer to the save file apFile.
	void Save(BGSSaveLoadFile* apFile, uint32_t aiSize);
	// Writes the buffer position, then that many bytes of the buffer, to the save file apFile.
	void Save(BGSSaveLoadFile* apFile);
	// Rewinds the buffer: zeroes iBufferPosition and iDebugOverhead.
	void ClearPosition();
	// The bytes written without the separators: iBufferPosition less iDebugOverhead.
	uint32_t GetDataSize();
	void ExpandBuffer();
	// Moves the bytes from aiPosition to the end aiAmount bytes on and grows the position by aiAmount.
	void ShiftBuffer(uint32_t aiPosition, int32_t aiAmount);

	char* pBuffer;
	uint32_t iBufferSize;
	uint32_t iBufferPosition;
	// The bytes written as separators; the buffer position without them is the size of the data.
	uint32_t iDebugOverhead;
};
