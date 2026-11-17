#include "BSSystem/Archive.h"
#include "BSSystem/BSHash.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/string.h"
#include "platform/Paths.h"

#include <vector>

// Whether a loose file named after the stream's own file name exists on disk: the exe asks FindFirstFileA for it and
// closes the search handle at once. The exe's body never reads apResult, and its stack-cookie check is not an effect.
bool Archive::PlatformGetLastWriteTime(_FILETIME* apResult) {
	// The exe passes the slot-6 getter's result (BSFile::GetDevice: the stream's cFileName) as the name.
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
	// The exe's FindFirstFileA and FindNextFileA, in their order.
	const std::vector<platform::FileEntry> kFound = platform::FindFiles(FileName);
	if (kFound.empty()) {
		return 0;
	}
	strcpy_s(pcDest, 260, apPath);
	uint8_t clength = std::strlen(pcDest);
	if (clength > 1u) {
		pcDest[clength - 1] = 0;
	}
	FNVD_CONSTRUCTED_LOCAL(BSHash, directoryHash, nullptr, pcDest, BSHash::HASH_NORMAL);
	bool Directory = FindDirectory(directoryHash, idirectoryIndex, 0);
	for (const platform::FileEntry& rEntry : kFound) {
		if ((rEntry.attributes & 16) == 0 || rEntry.name[0] == 46) {
			// CompareFileTime(entry, apArchiveTime) > 0: the entry is newer.
			if (Directory &&
				rEntry.writeTime >
					((static_cast<uint64_t>(apArchiveTime->dwHighDateTime) << 32) | apArchiveTime->dwLowDateTime)) {
				FNVD_CONSTRUCTED_LOCAL(BSHash, filenameHash, nullptr, rEntry.name.c_str(), BSHash::HASH_FILENAME);
				if (InvalidateFile(idirectoryIndex, filenameHash, 0)) {
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
