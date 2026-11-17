#pragma once

#include "BSCore/BSStringT.h"
#include "BSShader/BSShaderLightingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTStringMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVector4.h"

#include <stdint.h>

class GrassLoadTask;
class NiStencilProperty;
class NiTriBasedGeomData;
class TallGrassGroup;

// The shader property of the instanced tall grass.
class TallGrassShaderProperty : public BSShaderLightingProperty {
public:
	class CachedGrass;

	// The parameters of one grass type.
	class GrassParam {
	public:
		GrassParam();

		const char* pcGeomFilename;
		uint32_t iGrassID;
		float fPositionRange;
		float fHeightRange;
		float fColorRange;
		float fWaveOffsetRange;
		float fWavePeriod;
		bool bVertexLighting;
		bool bUniformScaing;
		bool bFitToSlope;
		bool bWind;
	};

	// The shared geometry, texture and properties of one grass model.
	class CachedGeometry : public NiRefObject {
	public:
		CachedGeometry();
		~CachedGeometry() override;

		NiPointer<NiTriBasedGeomData> spTriGeomData;
		float* pInstanceIndex;
		int32_t iVertices;
		NiPointer<NiTexture> spBaseTexture;
		NiPointer<NiAlphaProperty> spAlphaBlend;
		NiPointer<NiAlphaProperty> spAlphaNoBlend;
		NiPointer<NiStencilProperty> spStencil;
		NiPointer<GrassLoadTask> spGrassLoadTask;
		float fBoundRadius;
		int32_t iMaxInstanceCount;
		bool bTristrip;
		bool bBillboard;
		bool bQueued;
		uint8_t padding33;
		uint16_t usInstanceTriCount;
		uint8_t padding36[2];
	};

	// The cached low detail grass of one grass type: its parameters, groups, geometry and terrain chunks.
	class CachedGrass {
	public:
		CachedGrass();

		void Free();

		GrassParam kGrassParam;
		NiTPointerList<TallGrassGroup*> pLowGrassGroupList;
		NiPointer<CachedGeometry> spCachedLowGeometry;
		NiTMap<int32_t, int32_t> pLowTerrainChunkMap;
	};

	// The types of GetCachedGrassMap's function-local static pmGrassMap (grass ID to CachedGrass) and of
	// GetCachedGeometryMap's pmCachedGeometryMap.
	using pmGrassMap_t = NiTPointerMap<uint32_t, CachedGrass*>;
	using pmCachedGeometryMap_t = NiTStringPointerMap<NiPointer<CachedGeometry>>;

	TallGrassShaderProperty();
	~TallGrassShaderProperty() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject*) override { return false; }
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	void PostLinkObject(NiStream& kStream) override;
	BSShaderProperty::RenderPassArray* GetRenderPasses(NiGeometry* apGeom, int32_t aiEnabledPasses, uint16_t& asCount,
		uint32_t auiRenderMode, BSShaderAccumulator* apAccumulator, int32_t abUpdateList) override;
	NiTexture* GetBaseTexture() override { return spBaseTexture; }
	// The D3DTEXTUREADDRESS of the texture.
	virtual D3DTEXTUREADDRESS GetClampD3D() const { return D3DTADDRESS_WRAP; }
	virtual void SetClampD3D(D3DTEXTUREADDRESS aClampMode);
	virtual NiTexturingProperty::ClampMode GetClampNI() const { return TextureClampMode; }
	virtual void SetClampNI(NiTexturingProperty::ClampMode aClampMode) { TextureClampMode = aClampMode; }

	// Copies this property's members into pDest while cloning.
	void CopyMembers(TallGrassShaderProperty* pDest, NiCloningProcess& arCloneData);

	// The cached grass map, grass ID to CachedGrass.
	static NiTPointerMap<uint32_t, CachedGrass*>* GetCachedGrassMap();
	// Whether the cached grass of apGrassParam has terrain chunk alTerrainChunkSeed; false when the grass is not cached.
	static bool ContainsChunk(GrassParam* apGrassParam, int32_t alTerrainChunkSeed);
	static void SetUseBackgroundLoading(bool abUseBackgroundLoading);
	// Releases the grass's shared static maps.
	static void ClearStaticMaps();
	static int32_t Hash_2DPosition(NiPoint3& arPos);
	static void SetFadeParameters(float afLowMinFade, float afLowMin, float afLowMax, float afLowMaxFade);
	// The stream's factory for a new property.
	static NiObject* CreateObject();
	static NiTStringPointerMap<NiPointer<CachedGeometry>>* GetCachedGeometryMap();
	static CachedGeometry* GetCachedGeometry(GrassParam* apGrassParam);
	static void RemoveFromTerrainChunk(int32_t alTerrainChunkSeed);
	static void RemoveFromCell(int32_t aiCellX, int32_t aiCellY);
	// Whether the cached grass aiGrassID has terrain chunk alTerrainChunkSeed; false when the grass is not cached.
	// aiGrassID is the cached grass map's key (NiTPointerMap<unsigned long, CachedGrass*>), passed to its GetAt as is.
	static bool ContainsChunk_00B617B0(uint32_t aiGrassID, int32_t alTerrainChunkSeed);
	// Frees every cached grass and empties the cached grass map.
	static void RemoveAllGrass();
	// The cached grass group of apGrassParam's grass at the cell with room left, made when there is none; returns
	// its free instance count.
	static uint16_t GetGrassGroup(TallGrassShaderProperty::GrassParam* apGrassParam, int32_t aiCellX, int32_t aiCellY,
		TallGrassGroup** apGrassGroup, NiNode* apGrassNode);
	// Adds aiNumPositions grass instances to the cell's groups and the terrain chunk to the cached grass.
	static void AddGrass(int32_t alTerrainChunkSeed, int32_t aiCellX, int32_t aiCellY, NiNode* apGrassNode,
		TallGrassShaderProperty::GrassParam* apGrassParam, NiPoint3* apPositions, float* afColors,
		uint16_t aiNumPositions);

	TallGrassGroup* pGrassGroup;
	NiPointer<NiObject> spTerrainChunk;
	NiPointer<NiTexture> spBaseTexture;
	BSStringT<char> TextureFilename;
	NiTexturingProperty::ClampMode TextureClampMode;
	GrassParam* pGrassParam;

	static NiRTTI ms_RTTI;
	// Whether the grass geometry loads on a background task.
	static inline bool bUseBackgroundLoading = true;
	// The fade distances the grass shader reads (start, end - start of the two fades; four floats).
	static inline NiVector4 kFadeParams{};
	// Whether the grass shader lights the grass with the non-shadow point lights.
	static inline bool bPointLighting;
	static inline int32_t (*pGeometryLoadCallback)(const char*){};
};

// The string map of cached grass geometry by model name. Its SetValue copies the key with strcpy_s and releases the
// by-value smart pointer it receives; defined in TallGrassShaderProperty.cpp.
using NiTStringTemplateMapCachedGeometry =
	NiTStringTemplateMap<NiTPointerMap<const char*, NiPointer<TallGrassShaderProperty::CachedGeometry>>,
		NiPointer<TallGrassShaderProperty::CachedGeometry>>;

inline int32_t uiGrassGeometryCount;
inline int32_t uiGrassInstanceCount;
