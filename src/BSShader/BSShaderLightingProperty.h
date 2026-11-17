#pragma once

#include "BSShader/BSShaderProperty.h"
#include "BSShader/ShadowSceneLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiPoint4.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class NiTriBasedGeom;

// A shader property that is lit by a list of the scene's lights.
class BSShaderLightingProperty : public BSShaderProperty {
public:
	enum etLightType : int32_t {
		SLP_LT_NONE = 0,
		SLP_LT_DIR = 1,
		SLP_LT_POINT = 2,
	};

	// The type of each of the eight shader lights (the PDB's etLightType[8]; ResetLights clears the tail).
	using LightTypeTable = etLightType[8];
	using ShadowSceneLightItem = NiTListItem<ShadowSceneLight*>;

	// The lighting shaders' shared constant registers (IDA's kShaderLightConstants), by register.
	enum etConstantIndex : int32_t {
		SLP_SC_AMBIENT = 0,
		SLP_SC_LIGHTCOLOR = 1,
		SLP_SC_LIGHTPOSITION = 11,
		SLP_SC_LIGHTDIRECTION = 19,
		SLP_SC_LIGHTRADIUS = 19,
		SLP_SC_EMITTANCECOLOR = 27,
		SLP_SC_FOGPARAM = 28,
		SLP_SC_FOGCOLOR = 29,
		SLP_SC_EYEPOSITION = 30,
		SLP_SC_EYEDIRECTION = 31,
		SLP_SC_EYERIGHT = 32,
		SLP_SC_TOGGLES = 33,
		SLP_SC_HIGHDETAILRANGE = 34,
		SLP_SC_STBBCOLORCONSTANTS = 35,
		SLP_SC_LODTEXPARAMS = 36,
		SLP_SC_COUNT = 37,
	};

	using ShaderLightConstantTable = NiPoint4[SLP_SC_COUNT];

	BSShaderLightingProperty();
	~BSShaderLightingProperty() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	// BSShaderProperty::CopyTo, then CopyToMembers
	virtual void CopyTo(BSShaderLightingProperty* pDest);
	virtual void CopyToMembers(BSShaderLightingProperty*) {}
	using BSShaderProperty::CopyTo;
	using BSShaderProperty::CopyToMembers;

	// Copies the base members, then calls CopyToMembers on pDest, while cloning.
	void CopyMembers(BSShaderLightingProperty* pDest, NiCloningProcess& arCloneData);

	// Looks apLight up in the light list; on success stores its list node in arItem (the PDB's NiTListIterator, a
	// void*), otherwise stores null. The bound is unused.
	bool IsAttachedToLight(ShadowSceneLight* apSSLight, NiBound* apObjBound, void*& apInsertPosition) const;

	// The first light that is on screen and not culled; arIterator (a list node) is left at the node after the one
	// examined last.
	ShadowSceneLight* GetFirstActiveLight(void*& aitIter) const;
	ShadowSceneLight* GetNextActiveLight(void*& aitIter) const;
	// Takes the list's first light and leaves arIterator at the node after it.
	ShadowSceneLight* GetNextLight(void*& aitIter);

	// The same, additionally skipping lights that cast shadows.
	ShadowSceneLight* GetFirstActiveNonShadowLight(void*& aitIter) const;
	ShadowSceneLight* GetNextActiveNonShadowLight(void*& aitIter) const;

	// The number of lights that pass GetFirstActiveLight's test.
	uint16_t GetNumberofActiveLights() const;
	uint16_t GetNumberofActiveNonShadowLights() const;

	// Moves kEmptyLight to the head of the list and makes that node the fence.
	void FenceAllLights();
	// Removes the lights after the fence, and the fence.
	void RemoveFencedLights(NiTriBasedGeom* apGeometry);
	// Adds apSSLight to the light list.
	void AttachLight(ShadowSceneLight* apSSLight, NiBound* apObjBound);
	// Removes apSSLight from the light list.
	void RemoveLight(ShadowSceneLight* apSSLight);
	void ResortLights(NiBound* apObjBound);

	// Sets directional light auiLightIndex of the 3x shader.
	static void SetDirectionalLight3x(uint32_t auiLightIndex, ShadowSceneLight* apLight, float afForcedDarkness);
	// Sets light auiLightIndex of the 1x and 2x shaders.
	static void SetLight1x2x(uint32_t auiLightIndex, ShadowSceneLight* apLight, float afForcedDarkness);
	// Sets light auiLightIndex for the current shader level.
	static void SetLight(uint32_t auiLightIndex, ShadowSceneLight* apLight, float afForcedDarkness);
	// Sets point light auiLightIndex of the 3x shader.
	static void SetPointLight3x(uint32_t auiLightIndex, ShadowSceneLight* apLight, float afForcedDarkness);
	// Stores aValue in constant register aiConstant.
	static void SetShaderConstant(int32_t aiConstant, NiColorA aValue);
	// The stream's object factory: allocates a 0x7C-byte property and constructs it.
	static NiObject* CreateObject();
	// Clears the light types and resets the light colours from aiStartIndex on.
	static void ResetLights(int32_t aiStartIndex);

	// The PDB's NiTPointerListSingleThread<ShadowSceneLight *>: the pooled list of raw pointers (FenceAllLights stores
	// kEmptyLight with no IncRefCount; the exe calls the T* copies of Remove, AddTail and RemovePos on it).
	NiTPointerList<ShadowSceneLight*> lLightList;
	float fForcedDarkness;
	uint32_t uiReferenceID;
	bool bLightListChanged;
	uint8_t padding75[3];
	ShadowSceneLightItem* kLightListFence;

	static inline LightTypeTable eLightType{};
	static inline NiColorA kHairTint{};
	static NiRTTI ms_RTTI;
	static ShaderLightConstantTable pShaderLightConstants;
	// The LOD land shader's four floats: the geomorph parameter and the land drop.
	static inline NiPoint4 kLODLandParams{};
	static ShadowSceneLight kEmptyLight;
};

inline char bLightIsPoint;
inline NiColorA resetColor{};
inline int32_t resetColor_guard;

// IDA name; not in the PDB. Sixteen zero bytes between kHairTint and kLODLandParams that no code of the exe
// references.
using unused_0_t = uint32_t[4];
inline unused_0_t unused_0{};
inline constexpr fnvd::Vtable BSShaderLightingProperty_vftable{};
