#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class NiTextureTransform;
class NiTexturingProperty;

// The map names _SDMInit gives ms_akMapIDtoNames: the exe's own strings (text and NUL), whose addresses it compares
// with the names' handles. IDA names; not in the PDB.
using MapNameText5 = const char[5];
using MapNameText6 = const char[6];
using MapNameText7 = const char[7];

NiSmartPointer(NiTexturingProperty);

// The fixed-function texture stages of a geometry object: m_kMaps holds the base, dark, detail, gloss, glow, bump,
// normal, parallax and decal maps by MapEnum index, m_pkShaderMaps the maps handed to shaders.
class NiTexturingProperty : public NiProperty {
public:
	enum ApplyMode : int32_t {
		APPLY_REPLACE = 0x0,
		APPLY_DECAL = 0x1,
		APPLY_MODULATE = 0x2,
		APPLY_DEPRECATED = 0x3,
		APPLY_DEPRECATED2 = 0x4,
		APPLY_MAX_MODES = 0x5,
	};

	enum FilterMode : int32_t {
		FILTER_NEAREST = 0x0,
		FILTER_BILERP = 0x1,
		FILTER_TRILERP = 0x2,
		FILTER_NEAREST_MIPNEAREST = 0x3,
		FILTER_NEAREST_MIPLERP = 0x4,
		FILTER_BILERP_MIPNEAREST = 0x5,
		FILTER_ANISO = 0x6,
		FILTER_MAGLERP_MINANISO = 0x7,
		FILTER_MAX_MODES = 0x8,
	};

	enum ClampMode : int32_t {
		CLAMP_S_CLAMP_T = 0x0,
		CLAMP_S_WRAP_T = 0x1,
		WRAP_S_CLAMP_T = 0x2,
		WRAP_S_WRAP_T = 0x3,
		CLAMP_MAX_MODES = 0x4,
	};

	enum MapClassID : int32_t {
		MAP_CLASS_BASE = 0x0,
		MAP_CLASS_BUMP = 0x1,
		MAP_CLASS_PARALLAX = 0x2,
		MAP_CLASS_SHADER = 0x3,
		MAP_CLASS_MAX = 0x4,
	};

	// the PDB also lists SHADER_BASE and INDEX_MAX after DECAL_BASE; their values are not settled from the exe
	enum MapEnum : int32_t {
		BASE_INDEX = 0x0,
		DARK_INDEX = 0x1,
		DETAIL_INDEX = 0x2,
		GLOSS_INDEX = 0x3,
		GLOW_INDEX = 0x4,
		BUMP_INDEX = 0x5,
		NORMAL_INDEX = 0x6,
		PARALLAX_INDEX = 0x7,
		DECAL_BASE = 0x8,
		// PDB name; the exe's NiDX9FragmentShader::AppendTextureSampler takes 9 as the shader map.
		SHADER_BASE = 0x9,
	};

	enum Flags : uint16_t {
		MULTITEXTURE_MASK = 0x1,
		APPLYMODE_MASK = 0xE,
		APPLYMODE_POS = 0x1,
		DECALCOUNT_MASK = 0xFF0,
		DECALCOUNT_POS = 0x4,
	};

	// One texture stage; m_uFlags packs the texture coordinate set, the filter mode and the clamp mode.
	class Map : public NiMemObject {
	public:
		enum Flags : uint16_t {
			TEXCOORD_MASK = 0xFF,
			TEXCOORD_POS = 0x0,
			FILTERMODE_MASK = 0xF00,
			FILTERMODE_POS = 0x8,
			CLAMPMODE_MASK = 0x3000,
			CLAMPMODE_POS = 0xC,
		};

		Map();
		Map(const Map& kObj);
		// a2: the texture coordinate set, a3: the clamp mode, a4: the filter mode.
		Map(NiTexture* a1, int16_t a2, int16_t a3, uint16_t a4, NiTextureTransform* a5);
		virtual ~Map();

		virtual void LoadBinary(NiStream& kStream);
		virtual void SaveBinary(NiStream& kStream);
		virtual NiTexturingProperty::MapClassID GetClassID() const { return MAP_CLASS_BASE; }

		// Same texture (by IsEqual), flags and texture transform.
		bool operator==(const Map& kObj) const;

		// The exe takes the whole FilterMode dword and passes its low word to SetField (movzx).
		void SetFilterMode(NiTexturingProperty::FilterMode eFilterMode) {
			SetField(static_cast<uint16_t>(eFilterMode), 0xF00, 8);
		}

		void SetTexture(NiTexture* pkTexture);
		// The texture, m_spTexture's object (thiscall, ret 0, 0x5585E0; the exe's shared +8
		// NiPointer getter, folded with Animation::GetSceneRoot).
		NiTexture* GetTexture() const;
		void SetClampMode(NiTexturingProperty::ClampMode eClampMode);

	protected:
		void SetField(uint16_t uVal, uint16_t uMask, uint16_t uPos);

	public:
		// The exe frees maps through MemoryManager, not NiMemObject's operators.
		static void operator delete(void* apMemory);

		uint16_t m_uFlags;
		NiPointer<NiTexture> m_spTexture;
		NiTextureTransform* m_pkTextureTransform;
	};

	// A bump map: the luminance scale and offset and the 2x2 bump matrix.
	class BumpMap : public Map {
	public:
		BumpMap();
		BumpMap(const BumpMap& kObj);
		~BumpMap() override;

		void LoadBinary(NiStream& kStream) override;
		void SaveBinary(NiStream& kStream) override;
		// folded with NiCullingProperty::Type at 0xB6F470
		NiTexturingProperty::MapClassID GetClassID() const override { return MAP_CLASS_BUMP; }

		bool operator==(const BumpMap& kObj) const;

		float m_fLumaScale;
		float m_fLumaOffset;
		float m_fBumpMat00;
		float m_fBumpMat01;
		float m_fBumpMat10;
		float m_fBumpMat11;
	};

	// A map handed to a shader under the shader's own map id.
	class ShaderMap : public Map {
	public:
		ShaderMap();
		ShaderMap(const ShaderMap& kObj);
		~ShaderMap() override;

		void LoadBinary(NiStream& kStream) override;
		void SaveBinary(NiStream& kStream) override;
		// folded with NiShadeProperty::Type at 0xA6A000
		NiTexturingProperty::MapClassID GetClassID() const override { return MAP_CLASS_SHADER; }

		bool operator==(const ShaderMap& kObj) const;

		uint32_t m_uiID;
	};

	// A parallax map: a map with a height offset.
	class ParallaxMap : public Map {
	public:
		ParallaxMap();
		ParallaxMap(const ParallaxMap& kObj);
		~ParallaxMap() override;

		void LoadBinary(NiStream& kStream) override;
		void SaveBinary(NiStream& kStream) override;
		// folded with NiMaterialProperty::Type at 0xC1BA40
		NiTexturingProperty::MapClassID GetClassID() const override { return MAP_CLASS_PARALLAX; }

		bool operator==(const ParallaxMap& kObj) const;

		float m_fOffset;
	};

	// The map names by map ID (10 entries in the exe).
	using MapNameArray = NiFixedString[10];

	NiTexturingProperty();
	// A texturing property for the image pkPixelData, named kName, with the format preferences arPrefs.
	NiTexturingProperty(NiPixelData* pkPixelData, const NiFixedString& kName, NiTexture::FormatPrefs& arPrefs);
	~NiTexturingProperty() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	int32_t Type() const override { return TEXTURING; }

	// The shader map at uiIndex, or null past the end.
	ShaderMap* GetShaderMap(uint32_t uiIndex);
	// The shader array's element count (m_usSize), 0 without one.
	uint32_t GetShaderArrayCount() const;
	// The stage maps by MapEnum index; null when the stage is unused.
	NiTexture* GetBaseTexture();
	Map* GetDarkMap();
	Map* GetDetailMap();
	Map* GetGlossMap();
	Map* GetGlowMap();
	BumpMap* GetBumpMap();
	Map* GetDecalMap(uint32_t uiIndex);
	// The number of decal maps, packed in m_uFlags.
	uint32_t GetDecalMapCount();

	void SetBaseTexture(NiTexture* pkTexture);
	void SetBaseFilterMode(FilterMode eMode);
	// Sets the base map's clamp mode, creating the base map when there is none.
	void SetBaseClampMode(ClampMode eMode);
	// Sets the apply mode bits of the flags (SetField(eMode, 0xE, 1); thiscall, ret 4,
	// 0x533FB0).
	void SetApplyMode(ApplyMode eMode);

protected:
	uint16_t GetField(uint16_t uMask, uint16_t uPos) const;
	void SetField(uint16_t uVal, uint16_t uMask, uint16_t uPos);
	void CopyMembers(NiTexturingProperty* pkDest, NiCloningProcess& kCloning);

public:
	// Replaces the base map, deleting the old one.
	void SetBaseMap(NiTexturingProperty::Map* pkMap);
	// Sets the multitexture flag when pkMap or any map past the base map is set.
	void SetMultiTexture(NiTexturingProperty::Map* pkMap);
	void SetDecalMap(uint32_t uiIndex, NiTexturingProperty::Map* pkMap);

	// The ID of the map named kName (case-insensitive) in uiID; false when no map has that name.
	static bool GetMapIDFromName(const NiFixedString& kName, uint32_t& uiID);
	// A new viewer string naming the mode.
	static char* GetViewerString(const char* pcPrefix, FilterMode eMode);
	static char* GetViewerString(const char* pcPrefix, ClampMode eMode);
	// The property type id of a texturing property, TEXTURING (cdecl, 0x4AF340). Out of line: the exe calls it
	// (BipedAnim::GetLightingProperty).
	static int32_t GetType();
	static void _SDMShutdown();
	static NiObject* CreateObject();
	static void _SDMInit();

	uint16_t m_uFlags;
	NiTPrimitiveArray<Map*> m_kMaps;
	NiTPrimitiveArray<ShaderMap*>* m_pkShaderMaps;

	static NiRTTI ms_RTTI;
	static MapNameArray ms_akMapIDtoNames;
	static NiPointer<NiTexturingProperty> ms_spDefault;
};

using MapNameText9 = const char[9];
inline MapNameText6 pcString{"Decal"};
inline MapNameText5 aDark{"Dark"};
inline MapNameText5 aBase_0{"Base"};
inline MapNameText7 aShader_0{"Shader"};
inline MapNameText9 aParallax{"Parallax"};
inline MapNameText5 aBump{"Bump"};
inline MapNameText5 aGlow{"Glow"};
inline MapNameText6 aGloss{"Gloss"};
inline MapNameText7 aDetail{"Detail"};
inline constexpr fnvd::Vtable NiTexturingProperty_Map_vftable{};
inline constexpr fnvd::Vtable NiTexturingProperty_vftable{};

// The map subclasses' vtables, as CopyMembers writes them into the copies it allocates.
inline constexpr fnvd::Vtable NiTexturingProperty_BumpMap_vftable{};
inline constexpr fnvd::Vtable NiTexturingProperty_ShaderMap_vftable{};
inline constexpr fnvd::Vtable NiTexturingProperty_ParallaxMap_vftable{};

// The shader map array's instance, as CopyMembers writes it into the array it allocates.
inline constexpr fnvd::Vtable NiTPrimitiveArray_NiTexturingProperty_ShaderMapPtr_vftable{};
