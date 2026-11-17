#pragma once

#include "Reconstruction.hpp"

class Actor;
class BGSSaveLoadFile;
class NiMatrix3;
class TESForm;
class TESObjectREFR;

// The memory buffer a save game is read from. GetForm, GetReference and GetActor are the form the data belongs to;
// the plain buffer has none. IDA names the slots GetSaveFormVersion, getForm and getREFR.
class BGSLoadGameBuffer {
public:
	// Reads the next block of the save file apFile into the buffer and returns the bytes read.
	uint32_t Load(BGSSaveLoadFile* apFile);
	// Copies aiSize bytes from the buffer into apData and steps over the '|' spacer after them.
	void LoadDataFromBuffer(void* apData, size_t aiSize);
	// Reads a 16-bit length and that many characters into apString, or into a new string when apString is null;
	// returns the string.
	char* LoadString(char* apString);
	virtual uint8_t GetVersion();
	virtual TESForm* GetForm();
	virtual TESObjectREFR* GetReference();
	virtual Actor* GetActor();

	// Reads aiSize bytes into apData.
	void LoadData(void* apData, uint32_t aiSize);
	// Reads aiSize bytes from the save file apFile (a BGSSaveLoadFile; src has no such class)
	// into the buffer and returns the bytes read.
	uint32_t Load(BGSSaveLoadFile* apFile, uint32_t aiSize);
	void AdvanceBuffer(uint32_t aiNumBytes);
	void SetBuffer(char* apBuffer, uint32_t aiSize, uint32_t aiPosition);
	BGSLoadGameBuffer();
	~BGSLoadGameBuffer();
	// Reads a form ID into *apFormID; returns true when the ID is set but names no form.
	bool LoadFormID(uint32_t& apFormID);
	// Reads a form ID into a pointer field that holds it until the forms are resolved, filling the whole field. The
	// exe treats the field as an untyped dword slot for the ID (the 32-bit form); the 64-bit program widens the ID.
	template <class T> bool LoadFormID(T*& arpForm) {
#if FNVD_64BIT
		uint32_t uiFormID;
		bool bResult = LoadFormID(uiFormID);
		arpForm = reinterpret_cast<T*>(static_cast<uintptr_t>(uiFormID));
		return bResult;
#else
		return LoadFormID(reinterpret_cast<uint32_t&>(arpForm));
#endif
	}
	// Reads a form ID and returns it.
	uint32_t LoadFormID();
	// Reads a value saved by SaveVariableSizedValue.
	uint32_t LoadVariableSizedValue();
	char* GetNextPos();

	char* pBuffer;
	uint32_t iBufferSize;
	uint32_t iBufferPosition;
	// Inferred type: rot: NiMatrix3* from 0x9c5487 (&Projectile+0x94 (this of Projectile::LoadGame))
	void LoadMatrixAsQuaternion(NiMatrix3* rot, int32_t a2);
};
inline constexpr fnvd::Vtable BGSLoadGameBuffer_vftable{};
