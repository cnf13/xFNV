#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTSet.h"

#include <stdint.h>

class NiGeometry;
class NiNode;
class NiSkinInstance;

// Switches the bones and skins of a skeleton by level of detail.
class NiBoneLODController : public NiTimeController {
public:
	class SkinInfo : public NiMemObject {
	public:
		NiGeometry* m_pkShape;
		NiPointer<NiSkinInstance> m_spSkinInstance;
	};

	NiBoneLODController();
	~NiBoneLODController() override;

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
	void Update(NiUpdateData& UpdateData) override;

protected:
	bool TargetIsRequiredType() const override;

public:
	// Switches to LOD iLOD (-1: all skins culled); false when there are no LODs, iLOD is current or out of range.
	bool SetBoneLOD(int32_t iLOD);
	void CleanUp();

	static NiObject* CreateObject();

	int32_t m_iLOD;
	uint32_t m_uiNumLODs;
	NiTPrimitiveArray<NiTPrimitiveSet<NiNode*>*> m_kBoneArray;
	NiTPrimitiveArray<NiTPrimitiveSet<SkinInfo*>*> m_kSkinArray;
	NiTPrimitiveSet<NiGeometry*> m_kSkinSet;

	static NiRTTI ms_RTTI;
	static inline int32_t ms_iGlobalLOD = -1;
};
