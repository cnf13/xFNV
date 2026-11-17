#include "Gamebryo2.2/CoreLibs/NiSystem/NiFilename.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

#include <cstring>

char* NiStrcat(char* pcDest, size_t stDestSize, const char* pcSrc) {
	strcat_s(pcDest, stDestSize, pcSrc);
	return pcDest;
}

NiFilename::NiFilename(const char* pcFullPath) {
	m_acSubDir[0] = 0;
	_splitpath_s(pcFullPath, m_acDrive, 3u, m_acDir, 0x100u, m_acFname, 0x100u, m_acExt, 0x100u);
}

// Measures the result first (drive "X:", directory and sub-directory each with a trailing backslash unless they end
// in a separator, file name, extension with a '.' unless it starts with one, and the terminator), then appends the
// parts.
bool NiFilename::Makepath(char* pcStr, size_t stStrLen) {
	if (!pcStr) {
		return false;
	}
	size_t stLen = 1;
	if (m_acDrive[0]) {
		stLen = 3;
	}
	if (m_acDir[0]) {
		size_t stDirLen = strlen(m_acDir);
		stLen += stDirLen;
		char c = m_acDir[stDirLen - 1];
		if (c != '\\' && c != '/') {
			++stLen;
		}
	}
	char* pcSubDir = m_acSubDir;
	if (m_acSubDir[0]) {
		size_t stSubDirLen = strlen(m_acSubDir);
		stLen += stSubDirLen;
		char c = m_acSubDir[stSubDirLen - 1];
		if (c != '\\' && c != '/') {
			++stLen;
		}
	}
	char* pcFname = m_acFname;
	if (m_acFname[0]) {
		stLen += strlen(m_acFname);
	}
	char* pcExt = m_acExt;
	char cExt = m_acExt[0];
	if (cExt) {
		if (cExt != '.') {
			++stLen;
		}
		stLen += strlen(m_acExt);
	}
	if (stLen > stStrLen) {
		return false;
	}
	pcStr[0] = '\0';
	char cDrive = m_acDrive[0];
	if (cDrive) {
		pcStr[0] = cDrive;
		pcStr[1] = ':';
		pcStr[2] = '\0';
	}
	if (m_acDir[0]) {
		strcat_s(pcStr, stStrLen, m_acDir);
		char c = pcStr[strlen(pcStr) - 1];
		if (c != '\\' && c != '/') {
			NiStrcat(pcStr, stStrLen, "\\");
		}
	}
	if (*pcSubDir) {
		strcat_s(pcStr, stStrLen, pcSubDir);
		char c = pcStr[strlen(pcStr) - 1];
		if (c != '\\' && c != '/') {
			NiStrcat(pcStr, stStrLen, "\\");
		}
	}
	if (*pcFname) {
		strcat_s(pcStr, stStrLen, pcFname);
	}
	if (*pcExt) {
		if (*pcExt != '.') {
			strcat_s(pcStr, stStrLen, ".");
		}
		strcat_s(pcStr, stStrLen, pcExt);
	}
	return true;
}

bool NiFilename::GetFullPath(char* pcFullPath, size_t uiStrLen) {
	return Makepath(pcFullPath, uiStrLen);
}
