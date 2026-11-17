#pragma once

#include <cstdint>

// Archive file-name hash: last, second-last and first characters, length and a CRC of the rest.
class BSHash {
public:
    // How the string constructor hashes its string.
    enum HASHPROCESS : int32_t {
        HASH_FILENAME = 0,
        HASH_DIRECTORY = 1,
        HASH_NORMAL = 2,
    };

    // Leaves the fields unset, as the engine's empty constructor does.
    BSHash() = default;
    BSHash(uint8_t aucLast, uint8_t aucLast2, uint8_t aucLength, uint8_t aucFirst, uint32_t aiCRC);
    // Hashes apString as aeProcess says.
    BSHash(const char* apString, HASHPROCESS aeProcess);

    // The four character fields packed little-endian into one word, cLast lowest.
    uint32_t GetAll() const;

    // Selects the extension class from the high bits of the last, second-last and first characters.
    uint8_t ExtensionIndex() const;

    // Orders by CRC, then by the packed character word: -1, 0 or 1.
    int32_t Compare(const BSHash& arHash) const;

    // Compares the CRC; a '*' first character matches on the extension class alone, equal hashes also need equal
    // length.
    int32_t WildCardCompare(const BSHash& arHash) const;
    // Writes the hash as text (first, length, second-last and last characters, then the CRC) into the 32 bytes at
    // apString, if any.
    void MakeHashString(char* apString);
    void Endian();
    void sub_AFD960();

    // Splits apString and hashes its file and directory components.
    static void MakeDirAndFile(const char* apString, BSHash& arDirHash, BSHash& arFileHash);

    // Hashes apString with apExtension into arHash.
    static void Create(const char* apString, const char* apExtension, BSHash& arHash);

    char cLast;
    char cLast2;
    char cLength;
    char cFirst;
    uint32_t iCRC;
};

// One 24-byte record of a model's texture chunk (the stride TESTextureList::Load divides the chunk by): three hashes in
// BSHash's layout, with the members flat: the file name's, a second one that is never read, and the directory name's.
struct BSHashData {
    uint8_t cLast;
    uint8_t cLast2;
    uint8_t cLength;
    uint8_t cFirst;
    uint32_t iCRC;
    uint8_t cXLast;
    uint8_t cXLast2;
    uint8_t cXLength;
    uint8_t cXFirst;
    uint32_t iXCRC;
    uint8_t cDirLast;
    uint8_t cDirLast2;
    uint8_t cDirLength;
    uint8_t cDirFirst;
    uint32_t iDirCRC;
};

using HashFileExtensionStrings_t = char[6][10];
inline HashFileExtensionStrings_t HashFileExtensionStrings{{}, ".nif", ".kf", ".dds", ".wav", ".adp"};
