#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Reconstruction.hpp"

class NiFile;

// The engine's NIF stream: NiStream plus an object reference map and the nodes it keeps referenced.
class BSStream : public NiStream {
public:
	BSStream();
	~BSStream() override;

	using NiStream::Load;
	using NiStream::Save;
	// Saves to the file apFileName (slot 6; thiscall, ret 4, 0xC3A950).
	bool Save(const char* apFileName) override;
	// slot 9; thiscall, ret 4, 0xC3AFC0.
	bool RegisterSaveObject(NiObject* apObject) override;
	// slot 15; thiscall, ret 0, 0xC3AC10.
	bool LoadStream() override;
	// slot 16; thiscall, ret 0, 0xC3B270.
	bool SaveStream() override;
	// Opens pcSrc (through NiFile::GetFile when pkIstr is null) and loads it; true on
	// success. The exe's BSStream vtable has it as a new virtual after NiStream's (slot 24); callers holding the
	// stream by value call it directly (thiscall, ret 8, 0xC3A8A0).
	virtual bool Load(const char* apFileName, NiFile* apFile);

	void FreeAllObjects();
	void KillObjectRefMap();
	void LoadShaderTextures(char* apcName);
	void ObjectRefRec(NiObjectNET* apObject, uint32_t aiLevels);

	// Frees through ourFree, as the exe's deleting destructor does.
	static void operator delete(void* apMemory);
	// The create function registered for the class name apObjectName; null when there is none.
	static NiStream::CreateFunction GetCreatorFunction(const char* apObjectName);
	static void GetModifiedTextureFilename(char* apDstFileName, const char* apSrcFileName, const char* apPostfix);
	static void InitLoaders();
	static void RegisterLoadersINI(const char* apINIFilePath);

	// The streamed objects by name: ObjectRefRec stores each object under its name's handle through the string map's
	// SetAt, and BSReference looks its name up here.
	NiTStringMap<NiObjectNET*>* pObjectRefMap;
	NiPointer<NiRefObject> spNodeReferences;
};
inline constexpr fnvd::Vtable BSStream_vftable{};

// The texture suffixes LoadShaderTextures tries, null-terminated: "_g", "_hh", "_hl", "_n", "_s" (the exe's data at
// 0x11AF484 holds the five pointers and the null).
using BSStream_TextureSuffixArray = const char* [6];
inline BSStream_TextureSuffixArray pTextureSuffixes{"_g", "_hh", "_hl", "_n", "_s"};
