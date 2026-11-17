#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObjectPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTFixedStringMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Reconstruction.hpp"

// The palette controller managers use: a name -> object map filled from a scene root.
class NiDefaultAVObjectPalette : public NiAVObjectPalette {
public:
	NiDefaultAVObjectPalette();
	// The constructor: a palette over pkScene whose map has uiHashSize buckets.
	NiDefaultAVObjectPalette(NiAVObject* pkScene, uint32_t uiHashSize);
	~NiDefaultAVObjectPalette() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& stream) override;
	void LinkObject(NiStream& stream) override;
	// RegisterStreamables folds into a jump to the NiAVObjectPalette body
	bool RegisterStreamables(NiStream& stream) override;
	void SaveBinary(NiStream& stream) override;
	bool IsEqual(NiObject* pObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;
	void ProcessClone(NiCloningProcess& kCloning) override;
	NiAVObject* GetAVObject(const NiFixedString& arName) override;
	void SetAVObject(NiFixedString& arName, NiAVObject* pkObj) override;
	virtual void SetScene(NiAVObject* pkScene) { m_pkScene = pkScene; }
	// folded with NiD3DShaderProgram::GetCode at 0xBE0AA0
	virtual NiAVObject* GetScene() const { return m_pkScene; }

	// Empties the palette and refills it from the scene; does nothing without a scene.
	void ResetAndFillFromScenegraph();

	static NiObject* CreateObject();

	NiTFixedStringMap<NiAVObject*> m_kHash;
	NiAVObject* m_pkScene;

	static NiRTTI ms_RTTI;
};

// Adds apObj3D and its children to apPalette.
void RecurseAndAddObjectsToPalette(NiAVObject* apObj3D, NiDefaultAVObjectPalette* apPalette);
// Removes apObj3D and its children from apPalette.
void RecurseAndRemoveObjectsFromPalette(NiAVObject* apObj3D, NiDefaultAVObjectPalette* apPalette);

inline constexpr fnvd::Vtable NiDefaultAVObjectPalette_vftable{};
