#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiDynamicEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPlane.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"

// a dynamic effect that projects a texture (light, shadow, environment or fog map) onto the geometry it affects
class NiTextureEffect : public NiDynamicEffect {
public:
	enum TextureType : int32_t {
		PROJECTED_LIGHT = 0,
		PROJECTED_SHADOW = 1,
		ENVIRONMENT_MAP = 2,
		FOG_MAP = 3,
		TEXTURE_TYPE_MAX = 4,
	};

	enum CoordGenType : int32_t {
		WORLD_PARALLEL = 0,
		WORLD_PERSPECTIVE = 1,
		SPHERE_MAP = 2,
		SPECULAR_CUBE_MAP = 3,
		DIFFUSE_CUBE_MAP = 4,
		NUM_COORD_GEN = 5,
	};

	// The texture type names by type ID (4 entries).
	using TextureTypeStringArray = NiFixedString[4];

	NiTextureEffect();
	~NiTextureEffect() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;
	void UpdateWorldData(NiUpdateData& UpdateData) override;

protected:
	// Rebuilds the world projection and the world plane from the model ones and the world transform, then bumps
	// the revision.
	void UpdateProjection();
	// The world projection is not copied: the clone rebuilds it on its next update.
	void CopyMembers(NiTextureEffect* pDest, NiCloningProcess& kCloning);

public:
	// The ID of the texture type named kName (case-insensitive) in uiID; false when no type has that name.
	static bool GetTypeIDFromName(const NiFixedString& kName, uint32_t& uiID);

	// Interns the four texture type names.
	static void _SDMInit();
	// Releases the four texture type name handles.
	static void _SDMShutdown();
	// "<prefix> = <enumerator>" in a fresh _NiMalloc block the caller owns. An out-of-range texture type leaves the
	// block's contents unset; an out-of-range coordinate mode prints "UNKNOWN!!!".
	static char* GetViewerString(const char* apPrefix, TextureType aeMode);
	static char* GetViewerString(const char* apPrefix, CoordGenType aeMode);
	static NiObject* CreateObject();

	NiMatrix3 m_kModelProjMat;
	NiPoint3 m_kModelProjTrans;
	NiMatrix3 m_kWorldProjMat;
	NiPoint3 m_kWorldProjTrans;
	NiPointer<NiTexture> m_spTexture;
	NiTexturingProperty::FilterMode m_eFilter;
	NiTexturingProperty::ClampMode m_eClamp;
	TextureType m_eTextureMode;
	CoordGenType m_eCoordMode;
	bool m_bPlaneEnable;
	NiPlane m_kModelPlane;
	NiPlane m_kWorldPlane;

	static NiRTTI ms_RTTI;
	static TextureTypeStringArray ms_akTextureTypeStrings;
};
