#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceCubeMap.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiImageConverter.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiPath.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSearchPath.h"
#include "SDK/MSVC/string.h"

#include <cstring>

NiRTTI NiSourceCubeMap::ms_RTTI("NiSourceCubeMap", &NiSourceTexture::ms_RTTI);

NiSourceCubeMap::NiSourceCubeMap() : m_eFace(FACE_POS_X) {}

NiSourceCubeMap::~NiSourceCubeMap() {
	m_spSrcPixelData = nullptr;
}

// KEEP: niread: the exe reads the pixel layout into a stack slot nothing initialised and the mipmap and alpha
// formats into slots holding freed filename pointers, then stores them into the format preferences, so a short read
// (malformed data) leaves those values there, which src cannot express; porting.md's unwritten-slot exception covers
// GetChunkData only. Files from 10.3.0.6 on are plain source textures; older ones name six face files, loaded when
// the flag is set, and link six face pixel data (LinkObject). The base call is a direct call to NiTexture's
// LoadBinary slot body (0xA47350, a jump to NiObjectNET's, registered as NiSequenceStreamHelper::LoadBinary).
void NiSourceCubeMap::LoadBinary(NiStream& kStream) {
	if (kStream.GetFileVersion() >= NiStream::GetVersion(10, 3, 0, 6)) {
		NiSourceTexture::LoadBinary(kStream);
		return;
	}
	NiTexture::LoadBinary(kStream);
	NiIncRefCount(this);
	uint8_t bLoadFromFiles = 0;
	NiStreamLoadBinary(kStream, bLoadFromFiles);
	char* apcFilename[FACE_NUM] = {};
	for (uint32_t ui = 0; ui < FACE_NUM; ++ui) {
		kStream.LoadCString(apcFilename[ui]);
	}
	kStream.SetNumberOfLinkIDs(FACE_NUM);
	for (uint32_t ui = 0; ui < FACE_NUM; ++ui) {
		kStream.ReadLinkID();
	}
	if (bLoadFromFiles) {
		LoadFromFiles(apcFilename[FACE_POS_X], apcFilename[FACE_NEG_X], apcFilename[FACE_POS_Y],
			apcFilename[FACE_NEG_Y], apcFilename[FACE_POS_Z], apcFilename[FACE_NEG_Z], *kStream.m_pkSearchPath);
	}
	for (uint32_t ui = 0; ui < FACE_NUM; ++ui) {
		_NiFree(apcFilename[ui]);
	}
	uint32_t uiPixelLayout;
	NiStreamLoadBinary(kStream, uiPixelLayout);
	m_kFormatPrefs.m_ePixelLayout = static_cast<NiTexture::FormatPrefs::PixelLayout>(uiPixelLayout);
	uint32_t uiMipMapped;
	NiStreamLoadBinary(kStream, uiMipMapped);
	m_kFormatPrefs.m_eMipMapped = static_cast<NiTexture::FormatPrefs::MipFlag>(uiMipMapped);
	uint32_t uiAlphaFmt;
	NiStreamLoadBinary(kStream, uiAlphaFmt);
	m_kFormatPrefs.m_eAlphaFmt = static_cast<NiTexture::FormatPrefs::AlphaFormat>(uiAlphaFmt);
	uint8_t bStatic = m_bStatic;
	NiStreamLoadBinary(kStream, bStatic);
	m_bStatic = bStatic != 0;
	NiDecRefCount(this);
}

// Files before 10.3.0.6 link six face pixel data, from which the cube map's own is built when it has none yet.
void NiSourceCubeMap::LinkObject(NiStream& kStream) {
	if (kStream.GetFileVersion() >= NiStream::GetVersion(10, 3, 0, 6)) {
		NiSourceTexture::LinkObject(kStream);
		return;
	}
	NiTexture::LinkObject(kStream);
	NiPointer<NiPixelData> aspRaw[FACE_NUM];
	if (kStream.GetNumberOfLinkIDs() == FACE_NUM) {
		for (uint32_t ui = 0; ui < FACE_NUM; ++ui) {
			aspRaw[ui] = static_cast<NiPixelData*>(kStream.GetObjectFromLinkID());
		}
	}
	if (!m_spSrcPixelData.m_pObject) {
		LoadFromPixelData(aspRaw[FACE_POS_X].m_pObject, aspRaw[FACE_NEG_X].m_pObject, aspRaw[FACE_POS_Y].m_pObject,
			aspRaw[FACE_NEG_Y].m_pObject, aspRaw[FACE_POS_Z].m_pObject, aspRaw[FACE_NEG_Z].m_pObject);
	}
}

bool NiSourceCubeMap::RegisterStreamables(NiStream& kStream) {
	return (((int8_t)NiSourceTexture::RegisterStreamables(kStream)) != 0);
}

void NiSourceCubeMap::SaveBinary(NiStream& kStream) {
	NiSourceTexture::SaveBinary(kStream);
}

// Equal when the source textures are and the format preferences and the static flag match.
bool NiSourceCubeMap::IsEqual(NiObject* pkObject) {
	if (!NiSourceTexture::IsEqual(pkObject)) {
		return false;
	}
	NiSourceCubeMap* pkCubeMap = static_cast<NiSourceCubeMap*>(pkObject);
	return m_kFormatPrefs.m_ePixelLayout == pkCubeMap->m_kFormatPrefs.m_ePixelLayout &&
		m_kFormatPrefs.m_eMipMapped == pkCubeMap->m_kFormatPrefs.m_eMipMapped &&
		m_kFormatPrefs.m_eAlphaFmt == pkCubeMap->m_kFormatPrefs.m_eAlphaFmt && m_bStatic == pkCubeMap->m_bStatic;
}

void NiSourceCubeMap::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiSourceTexture::GetViewerStrings(pkStrings);
	pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// Reads the platform file's pixel data through the image converter (from the open file, if any) when none is held.
void NiSourceCubeMap::LoadPixelDataFromFile() {
	if (m_spSrcPixelData.m_pObject || m_kPlatformFilename.GetHandle() == NiGlobalStringTable::NULL_STRING) {
		return;
	}
	NiImageConverter* pkConverter = NiImageConverter::GetImageConverter();
	m_spSrcPixelData.SetNew(pkConverter->ReadImageFile(m_kPlatformFilename, nullptr, m_pFile));
	m_pFile = nullptr;
}

// Has the renderer (declared on the platform renderer) build the cube map's data, then drops the open file either way;
// false only when a renderer exists and fails.
bool NiSourceCubeMap::CreateRendererData() {
	NiRenderer* pkRenderer = NiRenderer::ms_pkRenderer;
	if (pkRenderer && !static_cast<NiDX9Renderer*>(pkRenderer)->CreateSourceCubeMapRendererData(this)) {
		if (m_pFile) {
			delete m_pFile;
			m_pFile = nullptr;
		}
		return false;
	}
	if (m_pFile) {
		delete m_pFile;
		m_pFile = nullptr;
	}
	return true;
}

// Reads the six faces through kPath into one cube-map pixel data. A face that cannot be read leaves its slot empty
// and stops the loop, and then nothing is loaded; the local pointers release their references on the way out.
void NiSourceCubeMap::LoadFromFiles(const char* pcPosXFilename, const char* pcNegXFilename, const char* pcPosYFilename,
	const char* pcNegYFilename, const char* pcPosZFilename, const char* pcNegZFilename, NiSearchPath& kPath) {
	const char* apcNames[FACE_NUM] = {pcPosXFilename, pcNegXFilename, pcPosYFilename, pcNegYFilename, pcPosZFilename,
		pcNegZFilename};
	NiPointer<NiPixelData> aspFaceSrcPixelDatas[FACE_NUM];
	NiImageConverter* pkConverter = NiImageConverter::GetImageConverter();
	char acFile[260];
	uint32_t i = 0;
	for (; i < FACE_NUM; ++i) {
		const char* pcName = apcNames[i];
		size_t stLength = strlen(pcName);
		char* pcCopy = static_cast<char*>(_NiMalloc(stLength + 1));
		strcpy_s(pcCopy, stLength + 1, pcName);
		NiPath::Standardize(pcCopy);
		char* pcPlatformName = NiImageConverter::ConvertFilenameToPlatformSpecific(pcCopy);
		kPath.SetFilePath(pcPlatformName);
		kPath.Reset();
		if (kPath.GetNextSearchPath(acFile, sizeof(acFile))) {
			while (!NiFile::Access(acFile, NiFile::READ_ONLY)) {
				if (!kPath.GetNextSearchPath(acFile, sizeof(acFile))) {
					goto nextFace;
				}
			}
			aspFaceSrcPixelDatas[i] = pkConverter->ReadImageFile(acFile, nullptr, nullptr);
			_NiFree(pcPlatformName);
			_NiFree(pcCopy);
		}
	nextFace:
		if (!aspFaceSrcPixelDatas[i]) {
			break;
		}
	}
	if (i == FACE_NUM) {
		LoadFromPixelData(aspFaceSrcPixelDatas[0], aspFaceSrcPixelDatas[1], aspFaceSrcPixelDatas[2],
			aspFaceSrcPixelDatas[3], aspFaceSrcPixelDatas[4], aspFaceSrcPixelDatas[5]);
	}
}

// Builds one six-face pixel data in the format of the +X face and copies every mipmap level of each face into it.
void NiSourceCubeMap::LoadFromPixelData(NiPixelData* pkPosXRaw, NiPixelData* pkNegXRaw, NiPixelData* pkPosYRaw,
	NiPixelData* pkNegYRaw, NiPixelData* pkPosZRaw, NiPixelData* pkNegZRaw) {
	NiPixelData* pkCubeData = static_cast<NiPixelData*>(NiMemObject::operator new(sizeof(NiPixelData)));
	if (pkCubeData) {
		pkCubeData = ::new (pkCubeData) NiPixelData(*pkPosXRaw->m_puiWidth, *pkPosXRaw->m_puiHeight,
			pkPosXRaw->m_kPixelFormat, pkPosXRaw->m_uiMipmapLevels, FACE_NUM);
	}
	m_spSrcPixelData = pkCubeData;

	NiPointer<NiPixelData> aspRaw[FACE_NUM];
	aspRaw[FACE_POS_X] = pkPosXRaw;
	aspRaw[FACE_NEG_X] = pkNegXRaw;
	aspRaw[FACE_POS_Y] = pkPosYRaw;
	aspRaw[FACE_NEG_Y] = pkNegYRaw;
	aspRaw[FACE_POS_Z] = pkPosZRaw;
	aspRaw[FACE_NEG_Z] = pkNegZRaw;

	for (uint32_t i = 0; i < FACE_NUM; ++i) {
		NiPixelData* pkRaw = aspRaw[i].m_pObject;
		for (uint32_t j = 0; j < pkRaw->m_uiMipmapLevels; ++j) {
			NiPixelData* pkDest = m_spSrcPixelData.m_pObject;
			uint32_t uiSize = pkRaw->m_puiOffsetInBytes[j + 1] - pkRaw->m_puiOffsetInBytes[j];
			rt::memcpy(pkDest->m_pucPixels + pkDest->m_puiOffsetInBytes[j] +
					i * pkDest->m_puiOffsetInBytes[pkDest->m_uiMipmapLevels],
				pkRaw->m_pucPixels + pkRaw->m_puiOffsetInBytes[j], uiSize);
		}
	}
}

// Builds a cube map from the six named files, then hands it to the renderer; a face that fails to load only makes
// the cube map empty, which the renderer may still accept. The search path is destroyed on both ways out.
NiSourceCubeMap* NiSourceCubeMap::Create(const char* pcPosXFilename, const char* pcNegXFilename,
	const char* pcPosYFilename, const char* pcNegYFilename, const char* pcPosZFilename, const char* pcNegZFilename,
	NiRenderer* apkRenderer, const NiFixedString& kName, NiTexture::FormatPrefs& kPrefs) {
	NiSourceCubeMap* pkTexture = static_cast<NiSourceCubeMap*>(NiMemObject::operator new(sizeof(NiSourceCubeMap)));
	if (pkTexture) {
		pkTexture = ::new (pkTexture) NiSourceCubeMap;
	} else {
		pkTexture = nullptr;
	}
	pkTexture->m_kFormatPrefs = kPrefs;
	alignas(4) uint8_t aucPath[sizeof(NiSearchPath)];
	NiSearchPath* pkPath = ::new (aucPath) NiSearchPath;
	pkTexture->LoadFromFiles(pcPosXFilename, pcNegXFilename, pcPosYFilename, pcNegYFilename, pcPosZFilename,
		pcNegZFilename, *pkPath);
	if (pkTexture->m_eFace) {
		pkTexture->m_eFace = FACE_POS_X;
	}
	if (!apkRenderer ||
		(pkTexture->m_spSrcPixelData.m_pObject &&
			static_cast<NiDX9Renderer*>(apkRenderer)->CreateSourceCubeMapRendererData(pkTexture))) {
		pkPath->~NiSearchPath();
		return pkTexture;
	}
	delete pkTexture;
	pkPath->~NiSearchPath();
	return nullptr;
}

// A cube map named kFilename in the kPrefs format; with preloading on, its pixel data is read and the renderer builds
// its data, the map being deleted and null returned when that fails.
NiSourceCubeMap* NiSourceCubeMap::Create_(const NiFixedString* kFilename, NiDX9Renderer* pkRenderer,
	const NiTexture::FormatPrefs* kPrefs) {
	NiSourceCubeMap* pkCubeMap = static_cast<NiSourceCubeMap*>(NiMemObject::operator new(sizeof(NiSourceCubeMap)));
	if (pkCubeMap) {
		::new (pkCubeMap) NiSourceCubeMap;
	}
	pkCubeMap->m_kFormatPrefs = *kPrefs;
	const char* pcHandle = kFilename->GetHandle();
	NiFixedString* pkFilename = &pkCubeMap->m_kFilename;
	if (pkCubeMap->m_kFilename.GetHandle() != kFilename->GetHandle()) {
		NiGlobalStringTable::AcquireHandle(pcHandle);
		// EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
		NiGlobalStringTable::ReleaseHandle(pkFilename->GetHandle());
		pkFilename->m_kHandle = pcHandle;
	}
	NiStandardizeFilePath(pkCubeMap->m_kFilename);
	NiImageConverter::ConvertFilenameToPlatformSpecific(pkCubeMap->m_kFilename, pkCubeMap->m_kPlatformFilename);
	if (pkCubeMap->m_eFace) {
		pkCubeMap->m_eFace = FACE_POS_X;
	}
	if (!NiSourceTexture::ms_bPreload) {
		return pkCubeMap;
	}
	pkCubeMap->LoadPixelDataFromFile();
	if (!pkRenderer || pkRenderer->CreateSourceCubeMapRendererData(pkCubeMap)) {
		return pkCubeMap;
	}
	delete pkCubeMap;
	return nullptr;
}

NiObject* NiSourceCubeMap::CreateObject() {
	NiSourceCubeMap* pkObject = static_cast<NiSourceCubeMap*>(NiMemObject::operator new(sizeof(NiSourceCubeMap)));
	if (!pkObject) {
		return nullptr;
	}
	return ::new (pkObject) NiSourceCubeMap;
}

// As Create_, from the already open file a2, whose pixel data is always read.
NiSourceCubeMap* NiSourceCubeMap::CreateTexture(NiFixedString* a1, NiFile* a2, NiDX9Renderer* pkRenderer,
	NiTexture::FormatPrefs* a4) {
	NiSourceCubeMap* pkCubeMap = static_cast<NiSourceCubeMap*>(NiMemObject::operator new(sizeof(NiSourceCubeMap)));
	if (pkCubeMap) {
		::new (pkCubeMap) NiSourceCubeMap;
	}
	pkCubeMap->m_kFormatPrefs = *a4;
	const char* pcHandle = a1->GetHandle();
	NiFixedString* pkFilename = &pkCubeMap->m_kFilename;
	if (pkCubeMap->m_kFilename.GetHandle() != a1->GetHandle()) {
		NiGlobalStringTable::AcquireHandle(pcHandle);
		// EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
		NiGlobalStringTable::ReleaseHandle(pkFilename->GetHandle());
		pkFilename->m_kHandle = pcHandle;
	}
	NiStandardizeFilePath(pkCubeMap->m_kFilename);
	NiImageConverter::ConvertFilenameToPlatformSpecific(pkCubeMap->m_kFilename, pkCubeMap->m_kPlatformFilename);
	if (pkCubeMap->m_eFace) {
		pkCubeMap->m_eFace = FACE_POS_X;
	}
	pkCubeMap->m_pFile = a2;
	pkCubeMap->LoadPixelDataFromFile();
	if (!pkRenderer || pkRenderer->CreateSourceCubeMapRendererData(pkCubeMap)) {
		return pkCubeMap;
	}
	delete pkCubeMap;
	return nullptr;
}
