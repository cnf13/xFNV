#pragma once

#include "BSCore/BSSpinLock.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

class BSEntryMap;
class BSFileEntry;
class BSTexMap;
class NiTexture;

// One loaded texture of the palette: the texture, its archive entry and the next item of the hash chain.
struct BSTexMapItem {
	NiPointer<NiTexture> spTexture;
	BSFileEntry* pFileEntry;
	BSTexMapItem* pNext;
	uint16_t sPriority;
	int16_t sNameBaseOffset;
};

// A walk over the palette's hash table: the item reached, its bucket index and its priority. The PDB names it
// BSTexMapIterator (IDA: pItem/uiIndex/uiPriority); BSTexturePalette::GetFirstPos (0xA616E0) and BSTexMap::GetNext
// (0xA619B0) fill it, and PurgeUnusedTextures (0xA61CD0) reads its three members.
struct BSTexMapIterator {
	BSTexMapItem* pItem;
	uint32_t iIndex;
	uint32_t iPriority;
};

// Bethesda's global table of loaded textures; all of it is static.
class BSTexturePalette {
public:
	// Seven texture groups and their count (defined after BSTexMap).
	struct PathMap;

	// Points arTexture at the loaded texture of apPath (released first); false when there is none.
	static bool GetTexture(const char* apPath, NiPointer<NiTexture>& arTexture);
	// The texture group number of the texture path apPath (cdecl, ret 0, 0xA61530; IDA name,
	// not in the PDB).
	static uint32_t GetContainerNumber(const char* apPath);
	// The palette item that holds apTexture, or null.
	static BSTexMapItem* GetItemFor(const NiTexture* apTexture);
	// Points arTexture at the loaded texture of the archive entry apEntry.
	static bool GetTexture(const BSFileEntry* apEntry, NiPointer<NiTexture>& arTexture);
	// Adds apTex to the palette, with its archive entry (looked up when null).
	static void SetTexture(NiTexture* apTex, const BSFileEntry* apEntry);
	// Drops apTexture from the palette; true when it was in it.
	static bool RemoveTexture(const NiTexture* apTexture);
	// Releases the textures nothing references any more.
	static void PurgeUnusedTextures();
	// The archive entry of apTexture in the palette (cdecl, ret 0, 0xC5D560; IDA's
	// GetTextureHandle).
	static BSFileEntry* FindEntryForTexture(const NiTexture* apTex);
	static void Lock();
	static void Unlock();
	static void SetCheckPurgedList(bool abCheck);
	static void SetMaxTexturesToPurge(int32_t aNumTextures);
	static int32_t GetMaxTexturesToPurge();
	static bool TryLock();
	static bool IsPartiallyPurged();
	// Inferred type: a1: char* from 0xa61fdb (passed to BSTexturePalette::GetTexture 0)
	static bool RemoveTexture_0(const char* a1);
	static void RemoveAll();

protected:
	static uint32_t DeterminePriority(NiTexture* apTex);

public:
	// IDA name; not in the PDB. The exe (0xA614D0) only reads both paths.
	static bool ArePathsEqual(const char* apRequestedPath, const char* apPath);
	static void Init();
	static void Kill();
	static bool GetFirstPos(BSTexMapIterator& arIterator);

	// The lock around adding textures to the palette.
	static BSSpinLock AddCrit;
	static inline int32_t iMaxTexturesToPurgeS = 25;
	static inline bool bPartiallyPurgedS;
	// The seven texture groups the palette keeps (the array BSTexturePalette::Init allocates).
	static inline BSTexMap* pTexMap;
	// The archive entries of the textures the palette keeps; null until Init.
	static inline BSEntryMap* pEntryMap;
};

// The palette's hash table: 713 chains of loaded textures and their count.
class BSTexMap {
public:
	// The item holding apTexture's path, added to the chain when it has none.
	BSTexMapItem* SetAt(NiTexture* apValue);
	// Reads the texture of auiContainer's group at apPath into aspTexture; true when found
	// (thiscall, ret 0xC, 0xA61920; IDA name, not in the PDB).
	// Inferred type: aspTexture: NiPointer<NiTexture>* from 0xa61c1b (parameter 1 of BSTexturePalette::GetTexture), its
	// members used at 0xa61967
	bool GetTexture(uint32_t auiContainer, const char* apPath, NiPointer<NiTexture>* aspTexture);
	// Drops apValue from the chain of its bucket.
	void Remove(BSTexMapItem* apValue);
	// Releases every texture of the table (the PDB's
	// BSTexturePalette::PathMap::TextureGroup::FreeTextures; thiscall, ret 0, 0xA61EC0).
	void FreeTextures();
	void* Create();
	bool GetNext(BSTexMapIterator& arIterator, NiPointer<NiTexture>& arValue);

	BSTexMapItem* pItemA[713];
	uint32_t iCount;
};

// The exe's new BSTexMap[7] block: Init (0xA61AD0) stores the count 7 at +0 and builds the groups from +4, and Kill
// (0xA62090) reads the count at pTexMap - 4.
struct BSTexturePalette::PathMap {
	uint32_t uiSize;
	BSTexMap kGroups[7];
};

// The palette's purged-list flag; a free global in the PDB, not a member.
inline bool bCheckPurgedTextureList;
