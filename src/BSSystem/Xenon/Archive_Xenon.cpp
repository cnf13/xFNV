#include "BSSystem/Archive.h"
#include "BSSystem/BSHash.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "platform/Paths.h"
#include "SDK/MSVC/string.h"

#include <vector>

// Returns whether a loose file named after the stream's own file name exists on disk. apResult is never read or
// written, as the game does.
bool Archive::PlatformGetLastWriteTime(_FILETIME* apResult) {
	// The name passed is the result of GetDevice, which is the stream's cFileName.
	return !platform::FindFiles(this->GetDevice()).empty();
}

uint32_t Archive::InvalidateOlderFilesByPath(const char* apBasePath, const char* apPath, _FILETIME* apArchiveTime) {
	uint32_t idirectoryIndex;
	char pcDest[264];
	char FileName[264];
	uint32_t inumInvalidated = 0;
	strcpy_s(FileName, 260, apBasePath);
	strcat_s(FileName, 260, apPath);
	strcat_s(FileName, 260, "*.*");
	const std::vector<platform::FileEntry> kFound = platform::FindFiles(FileName);
	if (kFound.empty()) {
		return 0;
	}
	strcpy_s(pcDest, 260, apPath);
	uint8_t clength = std::strlen(pcDest);
	if (clength > 1u) {
		pcDest[clength - 1] = 0;
	}
	BSHash directoryHash(pcDest, BSHash::HASH_NORMAL);
    bool Directory = FindDirectory(directoryHash, idirectoryIndex, nullptr);
    for (const platform::FileEntry& rEntry : kFound) {
		if ((rEntry.attributes & 16) == 0 || rEntry.name[0] == 46) {
			// CompareFileTime(entry, apArchiveTime) > 0: the entry is newer.
			if (Directory &&
				rEntry.writeTime >
					((static_cast<uint64_t>(apArchiveTime->dwHighDateTime) << 32) | apArchiveTime->dwLowDateTime)) {
				BSHash filenameHash(rEntry.name.c_str(), BSHash::HASH_FILENAME);
                if (InvalidateFile(idirectoryIndex, filenameHash, nullptr)) {
                    ++inumInvalidated;
                }
            }
		} else {
			strcpy_s(FileName, 260, apPath);
			strcat_s(FileName, 260, rEntry.name.c_str());
			strcat_s(FileName, 260, "\\");
			uint32_t v5 = InvalidateOlderFilesByPath(apBasePath, FileName, apArchiveTime);
			inumInvalidated += v5;
		}
	}
	return inumInvalidated;
}

uint32_t ArchiveFile::ReadFromBuffer(void* apReadBuffer, uint32_t aiByteCount) {
	int32_t aiOffset = 0;
	uint32_t uiRemainingSize = m_uiBufferReadSize - m_uiPos;
	if (aiByteCount > uiRemainingSize) {
		if (uiRemainingSize) {
			rt::memcpy(apReadBuffer, &m_pBuffer[m_uiPos], uiRemainingSize);
			apReadBuffer = static_cast<char*>(apReadBuffer) + uiRemainingSize;
			aiByteCount -= uiRemainingSize;
			aiOffset = uiRemainingSize;
		}
		m_uiBufferReadSize = 0;
		m_uiPos = 0;
		if (aiByteCount > m_uiBufferAllocSize) {
			return aiOffset + ReadFromArchive(apReadBuffer, aiByteCount, aiOffset);
		}
		m_uiBufferReadSize = ReadFromArchive(m_pBuffer, m_uiBufferAllocSize, aiOffset);
		if (m_uiBufferReadSize < aiByteCount) {
			aiByteCount = m_uiBufferReadSize;
		}
	}
	rt::memcpy(apReadBuffer, &m_pBuffer[m_uiPos], aiByteCount);
	m_uiPos += aiByteCount;
	return aiByteCount + aiOffset;
}

// Drops a leading backslash, then a leading "Data\" or "data\".
const char* ArchiveManager::TrimFileName(const char* apFileName) {
	if (*apFileName == '\\') {
		++apFileName;
	}
	if (!std::strncmp(apFileName, "Data\\", 5) || !std::strncmp(apFileName, "data\\", 5)) {
		apFileName += 5;
	}
	return apFileName;
}
