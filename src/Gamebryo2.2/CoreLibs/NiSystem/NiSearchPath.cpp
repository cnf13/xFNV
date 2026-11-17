#include "Gamebryo2.2/CoreLibs/NiSystem/NiSearchPath.h"

#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFilename.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiPath.h"
#include "SDK/MSVC/string.h"

static uint8_t data_11F6228;

NiSearchPath::NiSearchPath() : m_uiNextPath(0) {
	m_acFilePath[0] = 0;
	m_acReferencePath[0] = 0;
}

NiSearchPath::~NiSearchPath() {}

void NiSearchPath::Reset() {
	m_uiNextPath = 0;
}

bool NiSearchPath::GetNextSearchPath(char* pcPath, uint32_t uiStringLen) {
	bool result;
	char* m_acFilePath = this->m_acFilePath;
	NiFilename Dst(this->m_acFilePath);
	switch (m_uiNextPath) {
	case 0u: {
		goto LABEL_7;
	}
	case 1u: {
		strcpy_s(Dst.m_acDrive, 3, "");
		strcpy_s(Dst.m_acDir, 256, "");
		goto LABEL_7;
	}
	case 2u: {
		strcpy_s(Dst.m_acDrive, 3, "");
		strcpy_s(Dst.m_acDir, 256, m_acReferencePath);
		NiFilename Src(m_acFilePath);
		strcpy_s(Dst.m_acSubDir, 256, Src.m_acDir);
		goto LABEL_7;
	}
	case 3u: {
		strcpy_s(Dst.m_acDrive, 3, "");
		strcpy_s(Dst.m_acDir, 256, m_acReferencePath);
		strcpy_s(Dst.m_acSubDir, 256, "");
		goto LABEL_7;
	}
	case 4u: {
		if (!NiSearchPath::ms_acDefPath[0]) {
			goto LABEL_8;
		}
		strcpy_s(Dst.m_acDrive, 3, "");
		strcpy_s(Dst.m_acDir, 256, NiSearchPath::ms_acDefPath);
	LABEL_7:;
		Dst.GetFullPath(pcPath, uiStringLen);
		NiPath::Standardize(pcPath);
		++m_uiNextPath;
		result = 1;
		break;
	}
	default: {
	LABEL_8:;
		result = 0;
		break;
	}
	}
	return result;
}

void NiSearchPath::SetFilePath(const char* pcFilePath) {
	if (pcFilePath && *pcFilePath) {
		strncpy_s(m_acFilePath, sizeof(m_acFilePath), pcFilePath, sizeof(m_acFilePath) - 1);
		NiPath::Standardize(m_acFilePath);
	} else {
		m_acFilePath[0] = '\0';
	}
}

void NiSearchPath::operator delete(void* apMemory) {
	MemoryManager_Singleton.Deallocate(apMemory);
}

void NiSearchPath::SetSearchPathCreateCallback(SEARCHPATHCREATEFUNC pFunc) {
	spSearchPathCreateCallback = pFunc;
}
