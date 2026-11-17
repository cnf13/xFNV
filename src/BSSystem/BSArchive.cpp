#include "BSSystem/BSArchive.h"

#include "BSCore/BSString.h"
#include "BSSystem/BSHash.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/stdlib.h"

void ArchiveFlag::Endian() {
	iFlag = std::byteswap<uint32_t>(iFlag);
}

uint32_t ArchiveFlag::CheckFlag(uint32_t aiFlag) {
	return iFlag & aiFlag;
}

BSArchiveHeader::BSArchiveHeader()
	: iTag(4281154), iVersion(104), iHeaderSize(36), oFlags{0}, iDirectories(0), iFiles(0), iDirectoryNamesLength(0),
	  iFileNamesLength(0), iArchiveType(0) {}

void BSArchiveHeader::Endian() {
	iTag = std::byteswap<uint32_t>(iTag);
	iVersion = std::byteswap<uint32_t>(iVersion);
	iHeaderSize = std::byteswap<uint32_t>(iHeaderSize);
	oFlags.Endian();
	iDirectories = std::byteswap<uint32_t>(iDirectories);
	iFiles = std::byteswap<uint32_t>(iFiles);
	iDirectoryNamesLength = std::byteswap<uint32_t>(iDirectoryNamesLength);
	iFileNamesLength = std::byteswap<uint32_t>(iFileNamesLength);
	iArchiveType = std::byteswap<uint16_t>(iArchiveType);
}

uint32_t BSFileEntry::GetEntryOffset() {
	return iOffset & 2147483647;
}

void BSFileEntry::Endian() {
	iSize = std::byteswap<uint32_t>(iSize);
	iOffset = std::byteswap<uint32_t>(iOffset);
}

bool BSDirectoryEntry::IsFileEntryInDirectory(BSFileEntry* apFileEntry) {
	BSFileEntry* pFiles = this->pFiles;
	return apFileEntry >= pFiles && apFileEntry < &pFiles[iFiles];
}

void BSDirectoryEntry::Endian() {
	iFiles = std::byteswap<uint32_t>(iFiles);
}

bool BSArchive::IsFileEntryInArchive(BSFileEntry* apFileEntry) {
	uint32_t i;
	for (i = 0; i < iDirectories; ++i) {
		if ((&pDirectories[i])->IsFileEntryInDirectory(apFileEntry)) {
			return (bool)1;
		}
	}
	return (bool)0;
}

int32_t BSArchive::GetFlag(int32_t a2) {
	return (int32_t)(a2 & iTag);
}

int32_t BSArchive::GetFlag_(BSArchive::ARCHIVE_FLAGS aeFlag) {
	return static_cast<int32_t>(oFlags.CheckFlag(static_cast<uint32_t>(aeFlag)));
}

int32_t BSArchive::HasEmbeddedFileNames() {
	return (int32_t)((uint8_t)(GetFlag_(BSArchive::ARCHIVE_EMBEDDED_FILE_NAMES) != 0));
}

void BSHash::MakeHashString(char* apString) {
	if (apString) {
		// The exe zero-extends the characters.
		BSsprintf(apString, 0x20, "%c%02X%c%c%08X", static_cast<uint8_t>(cFirst), static_cast<uint8_t>(cLength),
			static_cast<uint8_t>(cLast2), static_cast<uint8_t>(cLast), iCRC);
	}
}

void BSHash::Endian() {
	BSHash v2 = *this;
	// The hash's 8 bytes in reverse order.
	for (uint32_t i = 0; i < sizeof(BSHash); ++i) {
		reinterpret_cast<char*>(this)[i] = reinterpret_cast<const char*>(&v2)[sizeof(BSHash) - 1 - i];
	}
}

// Splits apString, hashes its file component, then hashes its directory component.
void BSHash::MakeDirAndFile(const char* apString, BSHash& arDirHash, BSHash& arFileHash) {
	char cDrive[0x10];
	char cDir[0x104];
	char cFile[0x104];
	char cExt[0x10];
	cDrive[0] = 0;
	cDir[0] = 0;
	cFile[0] = 0;
	cExt[0] = 0;
	_splitpath_s(apString, cDrive, 0x10, cDir, 0x104, cFile, 0x104, cExt, 0x10);
	BSHash::Create(cFile, cExt, arFileHash);
	strcpy_s(cFile, 0x104, cDrive);
	strcat_s(cFile, 0x104, cDir);
	if (!cFile[0]) {
		cFile[0] = '.';
		cFile[1] = 0;
	}
	size_t v6 = std::strlen(cFile);
	if (cFile[v6 - 1] == '\\') {
		cFile[v6 - 1] = 0;
	}
	BSHash::Create(cFile, nullptr, arDirHash);
}
