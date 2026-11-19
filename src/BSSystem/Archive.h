#pragma once

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSSystem/BSArchive.h"
#include "BSSystem/BSFile.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"

#include <stdint.h>

class ArchiveFile;
struct z_stream_s;

// The offset of the texture name inside a texture path.
uint32_t FindTexturePathOffset(const char* apPath, bool abStripTexture);
// Writes the standard form of the texture path apInPath (no leading texture path, lower case) to apOutPath, a buffer of
// aiOutSize characters; returns true.
bool StandardizeTexturePath(const char* apInPath, char* apOutPath, uint32_t aiOutSize);
// Sorts apHashArray[aiLeft..aiRight] by hash.
void QuickSortHashArray(BSHash** apHashArray, int32_t aiLeft, int32_t aiRight);
// Writes the standardized meshes path of apFileName into apStandardizedFileName.
void StandardizeModelPath(const char* apFileName, char* apStandardizedFileName, uint32_t aiOutSize);
void StandardizeDataPath(const char* apFileName, char* apStandardizedFileName, uint32_t aiOutSize);

// The file groups an archive can hold, as bits; ARCHIVE_TYPE_ALL asks for the group of the file's extension.
enum ARCHIVE_TYPE : int32_t {
    ARCHIVE_TYPE_ALL = 0xFFFF,
    ARCHIVE_TYPE_MESHES = 0x1,
    ARCHIVE_TYPE_TEXTURES = 0x2,
    ARCHIVE_TYPE_MENUS = 0x4,
    ARCHIVE_TYPE_SOUNDS = 0x8,
    ARCHIVE_TYPE_VOICES = 0x10,
    ARCHIVE_TYPE_SHADERS = 0x20,
    ARCHIVE_TYPE_TREES = 0x40,
    ARCHIVE_TYPE_FONTS = 0x80,
    ARCHIVE_TYPE_MISC = 0x100,
    ARCHIVE_TYPE_COUNT = 9,
};

// The archive groups a file can be looked up in.
enum ARCHIVE_TYPE_INDEX : int32_t {
    ARCHIVE_TYPE_INDEX_MESHES = 0,
    ARCHIVE_TYPE_INDEX_TEXTURES = 1,
    ARCHIVE_TYPE_INDEX_MENUS = 2,
    ARCHIVE_TYPE_INDEX_SOUNDS = 3,
    ARCHIVE_TYPE_INDEX_VOICES = 4,
    ARCHIVE_TYPE_INDEX_SHADERS = 5,
    ARCHIVE_TYPE_INDEX_TREES = 6,
    ARCHIVE_TYPE_INDEX_FONTS = 7,
    ARCHIVE_TYPE_INDEX_MISC = 8,
    ARCHIVE_TYPE_INDEX_COUNT = 9,
};

// An open BSA archive: the file stream over the BSA, its reference count and its directory table. Only part of the
// members are declared.
class Archive : public BSArchive, public BSFile, public NiRefObject {
public:
    // The bits of cArchiveFlags.
    enum ARCHIVE_FLAGS : int32_t {
        ARCHIVE_FLAG_DISABLED = 0x1,
        ARCHIVE_FLAG_PRIMARY = 0x4,
        ARCHIVE_FLAG_SECONDARY = 0x8,
        ARCHIVE_FLAG_HAS_DIRECTORY_STRINGS = 0x10,
        ARCHIVE_FLAG_HAS_FILE_STRINGS = 0x20,
    };

    // Opens the archive apArchiveName and reads its directory table.
    Archive(const char* apArchiveName, uint32_t aiBufferSize, bool abSecondaryArchive, bool abInvalidateOtherArchives);
    ~Archive() override;

    // Whether a file named after the stream's own file name exists on disk; apResult is ignored.
    bool PlatformGetLastWriteTime(_FILETIME* apResult);
    // The name of directory aiDirectoryIndex.
    char* GetDirectoryString(uint32_t aiDirectoryIndex);
    // The name of file aiFileIndex of directory aiDirectoryIndex, read into pFileNameStringArray unless the offsets
    // and strings are retained.
    char* GetFileString(uint32_t aiDirectoryIndex, uint32_t aiFileIndex);
    // Finds the directory and the file by their hashes; sets both indices.
    bool FindFile(
        BSHash& arDirectoryHash,
        BSHash& arFileNameHash,
        uint32_t& ariDirectoryIndex,
        uint32_t& ariFileIndex,
        const char* apFileName
    );
    // Finds the directory by its hash; sets its index.
    bool FindDirectory(BSHash& arDirectoryHash, uint32_t& ariDirectoryIndex, const char* apFileName);
    // Finds the file by its hash in directory aiDirectoryIndex; sets its index.
    bool FindFileInDirectory(
        uint32_t aiDirectoryIndex,
        BSHash& arFileNameHash,
        uint32_t& ariFileIndex,
        const char* apFileName,
        bool abSkipSecondaryArchiveCheck
    );
    // Marks the file of the directory and file hashes invalidated (its offset cleared, the secondary-archive bit
    // kept); false when the archive has no such file.
    bool InvalidateFile(BSHash& arDirectoryHash, BSHash& arFileNameHash, const char* apFileName);
    // Marks the file arFileNameHash of directory aiDirectoryIndex invalidated like the hash form.
    bool InvalidateFile(uint32_t aiDirectoryIndex, BSHash& arFileNameHash, const char* apFileName);
    // Opens file aiFileIndex of directory aiDirectoryIndex.
    ArchiveFile*
    GetFile(uint32_t aiDirectoryIndex, uint32_t aiFileIndex, uint32_t aiBufferSize, const char* apFileName);
    // The entry of the file with these hashes, or null.
    BSFileEntry* GetFileEntryForFile(BSHash& arDirectoryHash, BSHash& arFileNameHash, const char* apFileName);
    // Opens the file of apFileEntry.
    ArchiveFile* GetFileByFileEntry(BSFileEntry* apFileEntry, uint32_t aiBufferSize, const char* apFileName);
    // The archive's own flag when the INI setting aiINI is 1, else whether the setting is nonzero.
    bool CheckRetain(bool abArchive, int32_t aiINI);
    // Whether a loose file under Data\ overrides apFile; marks the entry checked otherwise.
    bool CheckInvalidateFile(BSFileEntry* apFile, const char* apFileName);
    // Whether the entry is not invalidated by a loose or newer file.
    bool CheckValidFile(BSFileEntry* apFileEntry, bool abSkipSecondaryArchiveCheck);
    // Whether the archive's name strings must stay loaded.
    bool QLoadStrings();
    // The archive's runtime ID.
    int32_t GetRuntimeID() const;
    // Reads the file name strings of the archive.
    void ReloadFilenamesAndIndices();
    // The index of the directory that holds apFileEntry.
    uint32_t GetDirectoryIndexForFileEntry(BSFileEntry* apFileEntry);
    // The index of apFileEntry in directory auiDirectory.
    uint32_t GetFileIndexForFileEntryFromDirectory(uint32_t aiDirectoryIndex, BSFileEntry* apFileEntry);
    char* GetFileNameForFileEntry(BSFileEntry* apFileEntry);
    void DeleteFileData();
    void DeleteDirectoryStringArray();
    const char* DoesFileCollide(BSHash& arDirectoryHash, BSHash& arFileNameHash, char* apFileName);
    uint32_t FindNewerAndInvalidate();
    uint32_t InvalidateOlderFilesByPath(const char* apBasePath, const char* apPath, _FILETIME* apArchiveTime);
    void DeleteFilenameStringArray(bool abPreserveOffsets);

    // NiNew's block: NiMemObject's delete, named here because BSFile declares its own.
    static void operator delete(void* apMemory, size_t aSize);

    int64_t ArchiveFileTime;
    uint32_t iFileNameArrayOffset;
    uint32_t iLastDirectoryIndex;
    uint32_t iLastFileIndex;
    BSCriticalSection ArchiveCriticalSection;
    uint8_t cArchiveFlags;
    char* pDirectoryStringArray;
    uint32_t* pDirectoryStringOffsets;
    char* pFileNameStringArray;
    uint32_t** pFileNameStringOffsets;
    int32_t m_iRuntimeID;

    static inline int32_t uiCount;
};

NiSmartPointer(Archive);

// A file stream over one entry of an archive. Only some of its members are declared.
class ArchiveFile : public BSFile {
public:
    // The stream over aiSize bytes at aiOffset of apArchive.
    ArchiveFile(const char* apFileName, Archive* apArchive, uint32_t aiOffset, uint32_t aiSize, uint32_t aiBufferSize);

    uint32_t GetSize() override;
    // Picks the swapping or plain callbacks: the swapping branch is BSFile's, the plain one this class's own read
    // function and BSFile's write function.
    void SetEndianSwap(bool bDoSwap) override;
    // Waits for the archive and opens the entry.
    bool Open(bool abWaitForOpen, bool abTextMode) override;
    // Line reads from the archive entry.
    uint32_t Read(BSStringT<char>& arString, uint32_t aiMaxLength) override;
    uint32_t Read(BSStringT<char16_t>& arString, uint32_t aiMaxLength) override;
    // Whether the entry still has its archive.
    bool Exist() override;
    // Reads aiBytes of the entry through the buffer, clamped to the end of the entry.
    uint32_t ReadF(void* apReadBuffer, uint32_t aiByteCount) override;
    // An archive entry is read-only: writes nothing and returns 0.
    uint32_t WriteF(const void* apBuffer, uint32_t aiBytes) override;
    ~ArchiveFile() override;
    void Seek(int32_t aiByteCount, int32_t aiSeekFlags) override;

    // Reads up to auiBytes from the stream into apBuffer; returns the bytes read.
    uint32_t ReadFromBuffer(void* apReadBuffer, uint32_t aiByteCount);
    uint32_t ReadFromArchive(void* apReadBuffer, uint32_t aiByteCount, int32_t aiOffset);

    // The unswapped read function the stream installs: ReadF on apThis. The last two arguments are unused.
    static uint32_t
    RdNoSwap(NiBinaryStream* apThis, void* apBuffer, uint32_t aiBytes, uint32_t* apNothing1, uint32_t aiNothing2);

    NiPointer<Archive> spArchive;
    uint32_t iArchiveOffset;
};

// An archive file stream that inflates its zlib-compressed entry. Only some of its members are declared.
class CompressedArchiveFile : public ArchiveFile {
public:
    // The stream over the aiSize compressed bytes at aiOffset of apArchive.
    CompressedArchiveFile(
        const char* apFileName,
        Archive* apArchive,
        uint32_t aiOffset,
        uint32_t aiSize,
        uint32_t aiBufferSize
    );

    uint32_t GetSize() override;
    uint32_t GetFileSize() const override;
    // Picks the swapping or plain callbacks: the swapping branch is BSFile's, the plain one this class's own read
    // function and BSFile's write function.
    void SetEndianSwap(bool bDoSwap) override;
    bool Open(bool abWaitForOpen, bool abTextMode) override;
    // A compressed entry has no line reads: both return 0.
    uint32_t Read(BSStringT<char>& arString, uint32_t aiMaxLength) override;
    uint32_t Read(BSStringT<char16_t>& arString, uint32_t aiMaxLength) override;
    // Whether the entry still has its archive.
    bool Exist() override;
    void Seek(int32_t aiByteCount, int32_t aiSeekFlags) override;
    // Inflates aiBytes of the entry into apBuffer.
    uint32_t ReadF(void* apReadBuffer, uint32_t aiByteCount) override;
    // A compressed entry is read-only: writes nothing and returns 0.
    uint32_t WriteF(const void* apBuffer, uint32_t aiBytes) override;
    ~CompressedArchiveFile() override;

    // The unswapped read function SetEndianSwap installs: ReadF on apThis. The last two arguments
    // are unused.
    static uint32_t
    RdNoSwap(NiBinaryStream* apThis, void* apBuffer, uint32_t aiBytes, uint32_t* apNothing1, uint32_t aiNothing2);

    z_stream_s* pZStream;
    char* pUncompressedBuffer;
    uint32_t iUncompressedFileSize;
    uint32_t iUncompressedBufferSize;
    uint32_t iUncompressedBufferPos;
    uint32_t iUncompressedBufferReadSize;
};

// An entry of the extension table: the extension without its dot, and its file group.
struct ArchiveTypeExtension {
    char Extension[4];
    ARCHIVE_TYPE eArchiveType;
};

// The registry of the open BSA archives.
class ArchiveManager {
public:
    using MasterFilePathBuffer = char[0x104];
    using MasterArchiveArray = NiPointer<Archive>[2][ARCHIVE_TYPE_INDEX_COUNT];
    // The extension table, sorted for the binary search: 26 entries.
    using ArchiveTypeExtensionArray = ArchiveTypeExtension[26];

    // The master archive of the group: the first of the two sets that holds one.
    static Archive* GetMasterArchive(ARCHIVE_TYPE_INDEX aeTypeIndex);
    static void
    Init(bool abUseArchives, const char* apMasterFilePath, const char* apArchiveList, uint32_t aiFileBufferSize);
    static void SetCheckRuntimeCollisions(bool abCheck);
    static void SetInvalidation(bool abInvalidate, const char* apInvalidationFile);
    static void SetStartupFinished();
    // Opens and registers apArchiveName; the archive, or null.
    static Archive* OpenArchive(const char* apArchiveName, uint16_t aiForceArchiveType, bool abInvalidateOtherArchives);
    // The open archive named apArchiveName, or null.
    static Archive* GetArchive(const char* apArchiveName);
    // Reloads the file name strings of every open archive, letting the file cache through while it does.
    static void ReloadArchiveFilenamesAndIndices();
    static ArchiveFile* GetFile(const char* apFileName, uint32_t aiBufferSize, ARCHIVE_TYPE aeArchiveType);
    static Archive* GetArchiveForFile(const char* apFileName, ARCHIVE_TYPE aeArchiveType);
    static bool FindFile(const char* apFileName, ARCHIVE_TYPE aeArchiveType);
    static char GetLastWriteTimeForFile(char* apFileName, _FILETIME* apFiletime, ARCHIVE_TYPE aeArchiveType);
    // Drops apFileName from the archives' file lookups after it was written on disk.
    static bool InvalidateFile(const char* apFileName);
    static ArchiveFile* GetFileByFileEntry(
        ARCHIVE_TYPE_INDEX aeArchiveTypeIndex,
        BSFileEntry* apFileEntry,
        uint32_t aiBufferSize,
        const char* apFileName
    );
    // The entry of the file in the first open archive of the group that has it, or null.
    static BSFileEntry* GetFileEntryForFileFromAllArchives(
        ARCHIVE_TYPE_INDEX aeArchiveTypeIndex,
        BSHash& arDirectoryHash,
        BSHash& arFileNameHash,
        const char* apFileName
    );
    static Archive* GetArchiveForFileEntry(BSFileEntry* apFileEntry, ARCHIVE_TYPE aeArchiveType);
    static void AddToFileList(
        BSSimpleList<const char*>* apFileNameList,
        const char* apSearchName,
        const char* apBaseFilename,
        ARCHIVE_TYPE aeArchiveType
    );
    static BSHash** AddArchiveToFileList(
        Archive* apArchive,
        uint32_t aiNumFiles,
        BSHash** apHashArray,
        BSHash& arDirectoryHash,
        BSHash& arSearchHash,
        BSSimpleList<const char*>* apFileNameList,
        const char* apBaseFilename
    );
    static ARCHIVE_TYPE GetArchiveTypeFromFileName(char* apFileName);
    static ARCHIVE_TYPE GetArchiveTypeFromFileExtension(char* apExtension);
    static const char* TrimFileName(const char* apFileName);
    static bool ArchivesEnabled() {
        return ArchiveManager::bUseArchives;
    }
    static bool QInvalidateOlderFiles();
    static bool OutputProfileStats(char* apFileName);
    static void SetRetainTables(int32_t aiDirString, int32_t aiFileString, int32_t aiFileOffset);
    static bool WildCardMatch(const char* apSearchName, BSHash* apHash);
    // Drops apArchive from the archive list, the master archive slots and the last-archive pointer, under the list
    // lock; false without a list or an archive.
    static bool RemoveArchive(Archive* apArchive);
    // Frees the archive list, the two invalidation hash arrays and the master archive pointers.
    static void CloseAll();
    static void AddArchive(Archive* apArchive);
    static void Shutdown();
    static bool OpenMasterArchives();
    static bool CloseArchive(const char* apArchiveName);
    static void LoadInvalidationFile(const char* apFileName);
    static BSFileEntry* GetFileEntryForFileOnlyFromMasterArchives(
        ARCHIVE_TYPE_INDEX aeArchiveTypeIndex,
        BSHash& arDirectoryHash,
        BSHash& arFileNameHash,
        const char* apFileName
    );
    static BSFileEntry* GetFileEntryForFile_1(
        uint16_t aeArchiveTypeIndex,
        BSHash* arDirectoryHash,
        BSHash* arFileNameHash,
        char* apFileName
    );
    static bool
    GetFileNameForFileEntry(ARCHIVE_TYPE_INDEX aeArchiveTypeIndex, BSFileEntry* apFileEntry, char* apFileName);
    static bool GetRandomFileNameForDirectory(const char* apDirectory, char* apFileName, ARCHIVE_TYPE aeArchiveType);
    static bool
    GetFileNameForSmallestFileInDirectory(const char* apDirectory, char* apFileName, ARCHIVE_TYPE aeArchiveType);
    static void InvalidateFileInArchives(BSHash& arDirectoryHash, BSHash& arFileHash, uint32_t aiArchiveTypes);
    static char CloseArchive(Archive* a1);
    static void CleanupArchiveStringsAfterStartup();

    static inline bool bUseArchives = true;
    static inline bool bInvalidateOlderFiles = true;
    // The INI settings that keep an archive's name tables loaded: 1 follows the archive's own flag.
    static inline int32_t iRetainDirectoryStringTable = 1;
    static inline int32_t iRetainFilenameStringTable = 1;
    static inline int32_t iRetainFilenameOffsetTable = 1;
    static inline ArchiveTypeExtensionArray ArchiveTypeExtensionsMap{
        {"kf", ARCHIVE_TYPE_MESHES},    {"nif", ARCHIVE_TYPE_MESHES},   {"egm", ARCHIVE_TYPE_MESHES},
        {"egt", ARCHIVE_TYPE_MESHES},   {"tri", ARCHIVE_TYPE_MESHES},   {"cmp", ARCHIVE_TYPE_MESHES},
        {"lst", ARCHIVE_TYPE_MESHES},   {"dtl", ARCHIVE_TYPE_MESHES},   {"dds", ARCHIVE_TYPE_TEXTURES},
        {"tai", ARCHIVE_TYPE_TEXTURES}, {"tga", ARCHIVE_TYPE_TEXTURES}, {"bmp", ARCHIVE_TYPE_TEXTURES},
        {"wav", ARCHIVE_TYPE_SOUNDS},   {"ogg", ARCHIVE_TYPE_VOICES},   {"lip", ARCHIVE_TYPE_VOICES},
        {"xml", ARCHIVE_TYPE_MENUS},    {"txt", ARCHIVE_TYPE_MENUS},    {"spt", ARCHIVE_TYPE_TREES},
        {"fnt", ARCHIVE_TYPE_FONTS},    {"tex", ARCHIVE_TYPE_FONTS},    {"vso", ARCHIVE_TYPE_SHADERS},
        {"pso", ARCHIVE_TYPE_SHADERS},  {"vsh", ARCHIVE_TYPE_SHADERS},  {"psh", ARCHIVE_TYPE_SHADERS},
        {"lsl", ARCHIVE_TYPE_SHADERS},  {"h", ARCHIVE_TYPE_SHADERS}
    };
    static inline MasterFilePathBuffer sMasterFilePath{};
    static inline bool bCheckRuntimeCollisions;
    static inline char* sArchiveInvalidationFile;
    static inline char* sMasterArchiveList;
    static inline BSSimpleList<NiPointer<Archive>>* pArchiveList;
    static inline uint32_t iArchiveFileBufferSize;
    static BSCriticalSection s_ArchiveListLock;
    static MasterArchiveArray spMasterArchives;
    // The names and directories of the invalidation file's hashes; every hash is freed on its own. LoadInvalidationFile
    // fills them with BSHash::BSHash results, and FindNewerAndInvalidate compares their items as hashes.
    static inline BSSimpleArray<BSHash*>* pInvalidateFileNameHashes;
    static inline BSSimpleArray<BSHash*>* pInvalidateDirectoryHashes;
    static NiPointer<Archive> spLastArchive;
};

// The buffer size each archive group's master archive is given (0x10000 or 0x4000); OpenMasterArchives grows a
// smaller buffer to it.
using MasterArchiveBufferSizeArray = uint32_t[ARCHIVE_TYPE_INDEX_COUNT];
inline MasterArchiveBufferSizeArray MasterArchiveBufferSize{0x10000, 0x10000, 16384, 0x10000, 16384,
                                                            16384,   16384,   16384, 16384};
