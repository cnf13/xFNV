#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTFixedStringMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTLargeArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTSet.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTStringMap.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiProcessorAffinity.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSemaphore.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiThread.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiThreadProcedure.h"
#include "Reconstruction.hpp"

class NiBinaryStream;
class NiObjectGroup;
class NiSearchPath;

// Reads and writes NIF files: a header, then the object blocks, linked by index; the root objects end up in
// m_kTopObjects.
class NiStream {
public:
	enum ErrorMessages : int32_t {
		STREAM_OKAY = 0,
		FILE_NOT_LOADED = 1,
		NOT_NIF_FILE = 2,
		OLDER_VERSION = 3,
		LATER_VERSION = 4,
		NO_CREATE_FUNCTION = 5,
		ENDIAN_MISMATCH = 6,
	};

	enum ThreadStatus : int32_t {
		IDLE = 0,
		LOADING = 1,
		CANCELLING = 2,
		PAUSING = 3,
		PAUSED = 4,
	};

	// Bethesda's header extension: its version and three short strings.
	struct BSStreamHeader {
		uint32_t uiVersion;
		char cAuthor[64];
		char cProcessScript[64];
		char cExportScript[64];

		// The PDB's; no code reads it (21 in the exe's .rdata, after NiStream::NULL_LINKID).
		static constexpr uint32_t uiMinVersion = 21;
	};

	// Makes an empty object of a registered class; NiStream calls it with the class name it reads.
	typedef NiObject* (*CreateFunction)();
	// Runs over each loaded top-level object once loading is done.
	typedef void (*PostProcessFunction)(NiStream&, NiObject*);
	typedef NiTPrimitiveArray<PostProcessFunction> PostProcessFunctionArray;

	// The procedure the background load thread runs (the PDB's NiStream::BackgroundLoadProcedure, nested as the
	// original Gamebryo NiStream.h has it). 8 bytes: the NiThreadProcedure vtable, then the stream pointer at +4.
	class BackgroundLoadProcedure : public NiThreadProcedure {
	public:
		~BackgroundLoadProcedure() override;

		virtual uint32_t ThreadProcedure(void* pvArg);

		NiStream* m_pkStream;
	};

	NiStream();
	virtual ~NiStream();

	// clang's MS ABI reverses each overload group, as MSVC does; declared in this order the slots match the exe:
	// 1 Load(NiBinaryStream*) 0xA63330, 2 Load(char*, int) 0xA632F0, 3 Load(const char*) 0xA63BE0, and Save the same
	// at 4-6 (0xA63380, 0xA635E0, 0xA640B0).
	virtual bool Load(const char* pcFileName);
	virtual bool Load(char* pcBuffer, int32_t iBufferSize);
	virtual bool Load(NiBinaryStream* pkIstr);
	virtual bool Save(const char* pcFileName);
	virtual bool Save(char*& pcBuffer, int32_t& iBufferSize);
	virtual bool Save(NiBinaryStream* pkOstr);
	virtual void BackgroundLoadOnExit() {}
	virtual bool RegisterFixedString(const NiFixedString& kString);
	virtual bool RegisterSaveObject(NiObject* pkObject);
	virtual void ChangeObject(NiObject* pkNewObject);
	virtual uint32_t GetLinkIDFromObject(const NiObject* pkObject);
	virtual void SaveLinkID(const NiObject* pkObject);
	virtual bool LoadHeader();
	virtual void SaveHeader();
	virtual bool LoadStream();
	virtual bool SaveStream();
	virtual void RegisterObjects();
	virtual void LoadTopLevelObjects();
	virtual void SaveTopLevelObjects();
	virtual bool LoadObject();
	virtual uint32_t PreSaveObjectSizeTable();
	virtual bool SaveObjectSizeTable(uint32_t uiStartOffset);
	virtual bool LoadObjectSizeTable();

	void FreeLoadData();
	// Reads a length-prefixed class name into pcString and terminates it.
	void LoadRTTIString(char* pcString);
	// Reads the class-name table and creates each block's object; false on an unknown, unskippable class.
	bool LoadRTTI();
	// The version of the file being loaded.
	uint32_t GetFileVersion() const { return m_uiNifFileVersion; }
	NiBinaryStream* Istr() const { return m_pkIstr; }
	NiBinaryStream* Ostr() const { return m_pkOstr; }

	// Reads a 4-byte link ID and queues it; the link pass takes the queued IDs back in order with
	// GetObjectFromLinkID.
	void ReadLinkID();
	// Reads a 4-byte link ID and returns its object at once: null for -1, no reference taken.
	NiObject* ResolveLinkID();
	// The object of the next queued link ID: null for -1, no reference taken.
	NiObject* GetObjectFromLinkID();
	// Reads a 4-byte count and that many link IDs, queueing the count first; GetNumberOfLinkIDs takes it back in the
	// link pass.
	uint32_t ReadMultipleLinkIDs();
	uint32_t GetNumberOfLinkIDs();
	NiObjectGroup* GetGroupFromID(uint32_t uiID) const;
	uint32_t GetIDFromGroup(NiObjectGroup* pkGroup);
	// A 4-byte index into the file's string table (-1: no string).
	void LoadFixedString(NiFixedString& kString);
	// Older files: the string's characters inline, as LoadCString reads them.
	void LoadCStringAsFixedString(NiFixedString& kString);
	void SaveFixedString(const NiFixedString& kString);
	// A 4-byte length, then that many characters: pcString gets a terminated copy from _NiMalloc, or null when the
	// length is not positive.
	void LoadCString(char*& pcString);
	// A 4-byte length (0 for null), then the characters without the terminator.
	void SaveCString(const char* pcString);
	// Queues a link ID count for GetNumberOfLinkIDs, as ReadMultipleLinkIDs does.
	void SetNumberOfLinkIDs(uint32_t uiLinks);
	// Number of root objects of the loaded file, and one of them (no reference taken).
	uint32_t GetObjectCount();
	NiObject* GetObjectAt(uint32_t i);
	// Empties the root objects, the string table and the object sizes, under the cleanup section.
	void RemoveAllObjects();
	// Hands the texture to BSTexturePalette with no file entry.
	void SetTexture(NiTexture* pkTexture);
	// Looks the texture up in BSTexturePalette.
	void GetTexture(const char* pcURL, NiPointer<NiTexture>& arTexture);
	// Offers the file's folder once (copied into pcPath, NI_MAX_PATH bytes) while bUseDefaultPath is set.
	bool GetNextSearchPath(char* pcPath, uint32_t uiStringLen);
	// The folder searched for the file's textures: copied and standardized, or empty.
	void SetFilePath(const char* pcFilePath);
	void InsertObject(NiObject* pkObject);
	void SetLastError(uint32_t error);

protected:
	// The index of kString in the save's string table, or -1.
	uint32_t GetStringID(const NiFixedString& kString);
	// Files before 4.1.0.12 carry no selective update flags: computes them for every root scene graph object.
	void SetSelectiveUpdateFlagsForOldVersions();
	// Marks the subtree for selective update of transforms and property controllers, not rigid.
	void SetSelectiveUpdateFlagsTTTFRecursive(NiAVObject* pkAVObject);
	// A 4-byte count (plus the null group 0), then per group a 4-byte size; old files only.
	void LoadObjectGroups();
	// KEEP: niread: a short read leaves uiNumGroups (the array size and the loop bound) or a uiBytes (the group's
	// size) unwritten, and the exe uses them.
	void SaveObjectGroups();
	void UpdateObjectGroups();
	// The fixed-string table: the count, the longest length, then each string.
	void SaveFixedStringTable();
	// Writes the class-name table and each object's index into it.
	void SaveRTTI();

public:
	// Reads the fixed-string table into m_kFixedStrings.
	bool LoadFixedStringTable();

	static void UnregisterLoader(const char* pcClassName);
	// Maps the class name pcName to pfnFunc; always 0.
	static int32_t RegisterLoader(const char* pcName, CreateFunction pfnFunc);
	// Makes an empty object of the class registered under pcRTTI, or null.
	static NiObject* CreateObjectByRTTI(const char* pcRTTI);
	// Empties the first slot holding pfnFunc.
	static void UnregisterPostProcessFunction(PostProcessFunction pfnFunc);

	// A version as the file header stores it: one byte per part, major part highest.
	static uint32_t GetVersion(uint32_t uiMajor, uint32_t uiMinor, uint32_t uiPatch, uint32_t uiInternal) {
		return (uiMajor << 24) | (uiMinor << 16) | (uiPatch << 8) | uiInternal;
	}

	// Builds the loader map and the post-process array (the static data manager's initialisation).
	static void _SDMInit();
	// Deletes both of them (the static data manager's shutdown).
	static void _SDMShutdown();
	static void operator delete(void* apMemory);
	static uint32_t GetVersionFromString(const char* pcVersionString);
	static void RegisterPostProcessFunction(void (*pfnFunc)(NiStream&, NiObject*));

	BSStreamHeader m_BSStreamHeader;
	NiTPrimitiveArray<NiObjectGroup*> m_kGroups;
	uint32_t m_uiNifFileVersion;
	uint32_t m_uiNifFileUserDefinedVersion;
	char m_acFileName[260];
	bool m_bSaveLittleEndian;
	bool m_bSourceIsLittleEndian;
	NiSearchPath* m_pkSearchPath;
	NiTLargeObjectArray<NiPointer<NiObject>> m_kObjects;
	NiTLargePrimitiveArray<uint32_t> m_kObjectSizes;
	NiTLargeObjectArray<NiPointer<NiObject>> m_kTopObjects;
	NiTLargeObjectArray<NiFixedString> m_kFixedStrings;
	NiBinaryStream* m_pkIstr;
	NiBinaryStream* m_pkOstr;
	NiTPrimitiveSet<uint32_t> m_kLinkIDs;
	uint32_t m_uiLinkIndex;
	NiTPrimitiveSet<uint32_t> m_kLinkIDBlocks;
	uint32_t m_uiLinkBlockIndex;
	NiTPointerMap<const NiObject*, uint32_t> m_kRegisterMap;
	uint16_t m_usNiAVObjectFlags;
	uint16_t m_usNiTimeControllerFlags;
	uint16_t m_usNiPropertyFlags;
	ThreadStatus m_eBackgroundLoadStatus;
	bool m_bBackgroundLoadExitStatus;
	uint32_t m_uiLoad;
	uint32_t m_uiLink;
	uint32_t m_uiPostLink;
	NiThread* m_pkThread;
	BackgroundLoadProcedure* m_pkBGLoadProc;
	// The background load thread's priority.
	NiThread::Priority m_ePriority;
	NiProcessorAffinity m_kAffinity;
	char m_acLastLoadedRTTI[260];
	ErrorMessages m_uiLastError;
	char m_acLastErrorMessage[260];
	char m_acFilePath[260];

	// The NIF file versions and user versions this build loads.
	static constexpr uint32_t ms_uiNifMinVersion = 0x4020100;
	static constexpr uint32_t ms_uiNifMaxVersion = 0x14020007;
	static constexpr uint32_t ms_uiNifMinUserDefinedVersion{};
	static constexpr uint32_t ms_uiNifMaxUserDefinedVersion = 11;
	// Held while the stream's object arrays are emptied.
	static NiCriticalSection ms_kCleanupCriticalSection;
	// The PDB's; no code reads it: the exe only creates its handle at start-up and closes it at exit.
	static NiSemaphore m_kSemaphore;
	// Set while GetNextSearchPath has the file's own folder still to offer.
	static inline bool bUseDefaultPath;
	// The link ID saved for a null object or string.
	static constexpr uint32_t NULL_LINKID = 0xFFFFFFFF;
	// The create function of each class name.
	static inline NiTStringPointerMap<CreateFunction>* ms_pkLoaders{};
	static inline PostProcessFunctionArray* ms_pkPostProcessFunctions;
};

// Not named in the PDB or IDA; name chosen here. The exe's out-of-line NiStreamSaveBinary of one byte: its callers
// pass a byte field's address (BSValueNode::m_uFlags at 0xc4398a), written with size 1.
void NiStreamSaveByte(NiStream* a1, const uint8_t* a2);

inline constexpr fnvd::Vtable NiStream_vftable{};

// The vtables _SDMInit writes: the loader map's base and derived tables, and the post-process array's.
inline constexpr fnvd::Vtable NiTMapBase_NiTPointerAllocator_uint_CreateFunction_vftable{};
inline constexpr fnvd::Vtable NiTStringPointerMap_NiStream_CreateFunction_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_NiStream_PostProcessFunction_vftable{};
