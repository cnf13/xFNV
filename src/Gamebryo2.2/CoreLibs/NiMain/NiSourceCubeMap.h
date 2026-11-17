#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Reconstruction.hpp"

class NiDX9Renderer;
class NiFile;
class NiRenderer;
class NiSearchPath;

// A source texture with six faces.
class NiSourceCubeMap : public NiSourceTexture {
public:
	enum FaceID : uint32_t {
		FACE_POS_X = 0,
		FACE_NEG_X = 1,
		FACE_POS_Y = 2,
		FACE_NEG_Y = 3,
		FACE_POS_Z = 4,
		FACE_NEG_Z = 5,
		FACE_NUM = 6,
	};

	NiSourceCubeMap();
	~NiSourceCubeMap() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void LoadPixelDataFromFile() override;
	bool CreateRendererData() override;

protected:
	// Loads the six faces through kPath and builds one cube-map pixel data from them.
	void LoadFromFiles(const char* pcPosXFilename, const char* pcNegXFilename, const char* pcPosYFilename,
		const char* pcNegYFilename, const char* pcPosZFilename, const char* pcNegZFilename, NiSearchPath& kPath);
	// Builds the cube map's pixel data from the six faces.
	void LoadFromPixelData(NiPixelData* pkPosXRaw, NiPixelData* pkNegXRaw, NiPixelData* pkPosYRaw,
		NiPixelData* pkNegYRaw, NiPixelData* pkPosZRaw, NiPixelData* pkNegZRaw);

public:
	// A cube map whose six faces are the named files, its format taken from kPrefs; kName is unused. Null when a face
	// is missing and apkRenderer refuses the result.
	static NiSourceCubeMap* Create(const char* pcPosXFilename, const char* pcNegXFilename, const char* pcPosYFilename,
		const char* pcNegYFilename, const char* pcPosZFilename, const char* pcNegZFilename, NiRenderer* apkRenderer,
		const NiFixedString& kName, NiTexture::FormatPrefs& kPrefs);
	// A cube map of the file kFilename, its format taken from kPrefs.
	static NiSourceCubeMap* Create_(const NiFixedString* kFilename, NiDX9Renderer* pkRenderer,
		const NiTexture::FormatPrefs* kPrefs);
	static NiObject* CreateObject();
	// IDA's name for the PDB's Create(kFilename, apFile, apkRenderer, kPrefs) overload. Four
	// arguments: the body reads no fifth and its caller (QueuedTexture::Run, 0x43C2E5) pops 0x10.
	static NiSourceCubeMap* CreateTexture(NiFixedString* a1, NiFile* a2, NiDX9Renderer* pkRenderer,
		NiTexture::FormatPrefs* a4);

	FaceID m_eFace;

	static NiRTTI ms_RTTI;
};

inline constexpr fnvd::Vtable NiSourceCubeMap_vftable{};
