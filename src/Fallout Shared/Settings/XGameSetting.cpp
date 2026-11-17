#include "BSCore/BSString.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSMain/SettingCollectionMap.h"
#include "Fallout Shared/TESFile.h"

void SettingCollection::SetFileName(const char* apFileName) {
	if (apFileName) {
		BSstrcpy(pSettingFile, 260, apFileName);
	} else {
		pSettingFile[0] = 0;
	}
}

// Reads rSetting's value from the DATA chunk of the next chunk of the open file: an int or float directly, a string
// through a buffer of the chunk's size.
bool GameSettingCollection::ReadSetting(Setting& rSetting) {
	bool bResult = false;
	if (pHandle) {
		TESFile* pFile = static_cast<TESFile*>(pHandle);
		pFile->NextChunk();
		if (pFile->GetTESChunk() == DATA_ID) {
			switch (rSetting.GetType()) {
			case Setting::ST_INT:
				bResult = pFile->GetChunkData<uint32_t>(rSetting.uValue.uint);
				break;
			case Setting::ST_FLOAT:
				bResult = pFile->GetChunkData<uint32_t>(rSetting.uValue.uint);
				break;
			case Setting::ST_STRING: {
				// A 16-aligned stack buffer of the chunk's size.
				char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(pFile->GetChunkSize(), 128));
				bResult = pFile->GetChunkData(pBuffer, 0);
				rSetting = pBuffer;
				break;
			}
			default:
				break;
			}
		}
	}
	return bResult;
}
