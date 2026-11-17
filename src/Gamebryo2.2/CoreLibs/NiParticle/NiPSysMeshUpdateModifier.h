#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"
#include "Reconstruction.hpp"

class NiAVObject;
class NiCloningProcess;
class NiPSysData;
class NiStream;

// updates the meshes of a mesh particle system.
class NiPSysMeshUpdateModifier : public NiPSysModifier {
public:
	NiPSysMeshUpdateModifier();
	~NiPSysMeshUpdateModifier() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void ProcessClone(NiCloningProcess& kCloning) override;
	void PostLinkObject(NiStream& kStream) override;
	void Update(float fTime, NiPSysData* apData) override;
	void Initialize(NiPSysData* apData, uint16_t ausNewParticle) override;

	// The mesh at uiIndex, or null past the end.
	NiAVObject* GetMeshAt(uint32_t uiIndex);
	// Stores pkMesh at uiIndex, growing the array past its end, and hands the former mesh back through aspAVObject.
	void SetMeshAt(uint32_t uiIndex, NiAVObject* pkMesh, NiPointer<NiAVObject>& aspAVObject);
	// The PDB's CopyMembers: the base's members, then a clone of each mesh into pkDest's
	// m_kMeshes (thiscall, ret 8, 0xC21770).
	void CopyMembers(NiPSysMeshUpdateModifier* pkDest, NiCloningProcess& kCloning);

	static NiObject* CreateObject();

	NiTObjectArray<NiPointer<NiAVObject>> m_kMeshes;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiPSysMeshUpdateModifier_vftable{};
