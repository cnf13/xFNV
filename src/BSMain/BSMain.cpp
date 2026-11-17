#include "BSMain/BSMain.h"

#include "BSMain/BSAttachPoints.h"
#include "BSMain/BSBound.h"
#include "BSMain/BSFurnitureMarker.h"
#include "BSMain/BSPSysArrayEmitter.h"
#include "BSMain/BSTextureData.h"
#include "BSMain/BSWArray.h"
#include "BSMain/BSXFlags.h"
#include "BSSystem/Archive.h"
#include "BSSystem/BSArchive.h"
#include "BSSystem/BSHash.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSTexturePalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"

char bMainInitialized;

void BSMain::InitSDM() {
	if (!bMainInitialized) {
		BSAttachPoints::InitSDM();
		BSBound::InitSDM();
		BSFurnitureMarker::InitSDM();
		PArrayPoint::InitSDM();
		BSTextureData::InitSDM();
		BSWArray::InitSDM();
		BSXFlags::InitSDM();
		bMainInitialized = 1;
	}
}

void BSMain::KillSDM() {
	if (bMainInitialized) {
		BSAttachPoints::KillSDM();
		BSBound::KillSDM();
		BSFurnitureMarker::KillSDM();
		PArrayPoint::KillSDM();
		BSTextureData::KillSDM();
		BSWArray::KillSDM();
		BSXFlags::KillSDM();
		bMainInitialized = 0;
	}
}

BSFileEntry* BSTexturePalette::FindEntryForTexture(const NiTexture* apTex) {
	char apFileName[264];
	BSHash arDirectoryHash;
	BSHash arFileNameHash;
	const char* apPath = (&apTex->GetFilename())->GetHandle();
	StandardizeTexturePath(apPath, apFileName, 260u);
	BSHash::MakeDirAndFile(apFileName, arDirectoryHash, arFileNameHash);
	return ArchiveManager::GetFileEntryForFileFromAllArchives(ARCHIVE_TYPE_INDEX_TEXTURES, arDirectoryHash,
		arFileNameHash, apFileName);
}
