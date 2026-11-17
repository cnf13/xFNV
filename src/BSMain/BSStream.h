#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"

class NiFile;

// The engine's NIF stream: NiStream plus an object reference map and the nodes it keeps referenced.
class BSStream : public NiStream {
public:
	BSStream();
	~BSStream() override;

	using NiStream::Load;
	using NiStream::Save;
	bool Save(const char* apFileName) override;
	bool RegisterSaveObject(NiObject* apObject) override;
	bool LoadStream() override;
	bool SaveStream() override;
	// Opens pcSrc (through NiFile::GetFile when pkIstr is null) and loads it; true on success. It is a new virtual
	// added after NiStream's.
	virtual bool Load(const char* apFileName, NiFile* apFile);

	void FreeAllObjects();
	void KillObjectRefMap();
	void LoadShaderTextures(char* apcName);
	void ObjectRefRec(NiObjectNET* apObject, uint32_t aiLevels);

	// Frees through ourFree, as the game's deleting destructor does.
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

// The texture suffixes LoadShaderTextures tries, null-terminated: "_g", "_hh", "_hl", "_n", "_s".
using BSStream_TextureSuffixArray = const char* [6];
inline BSStream_TextureSuffixArray pTextureSuffixes{"_g", "_hh", "_hl", "_n", "_s"};
