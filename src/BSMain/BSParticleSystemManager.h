#pragma once

#include "BSMain/UpdateMasterPsysTaskletData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "SDK/MSVC/ehvec.h"

class BSMasterParticleSystem;
class BSTaskletGroup;
class NiCloningProcess;

// The manager node the weather and effect particle systems attach under.
class BSParticleSystemManager : public NiNode {
public:
	BSParticleSystemManager();
	~BSParticleSystemManager() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void AttachChild(NiAVObject* apChild, int32_t abFirstAvail) override;
	void DetachChild(NiAVObject* apChild, NiPointer<NiAVObject>& arspAVObject) override;
	void DetachChild(NiAVObject* apChild) override;
	void DetachChildAt(uint32_t auiIndex, NiPointer<NiAVObject>& arspAVObject) override;
	void DetachChildAt(uint32_t auiIndex) override;
	void SetAt(uint32_t auiIndex, NiAVObject* apChild, NiPointer<NiAVObject>& arspAVObject) override;
	void SetAt(uint32_t auiIndex, NiAVObject* apChild) override;

	// The master particle system auiIndex.
	BSMasterParticleSystem* GetMasterParticleSystem(uint32_t auiIndex);
	// Attaches a master particle system under the manager.
	void AddMasterParticleSystem(BSMasterParticleSystem* apMasterParticleSystem);
	// Removes the master particle system at auiIndex, warning when none is loaded there.
	void RemoveMasterParticleSystem(uint32_t auiIndex);
	// Detaches apMasterParticleSystem from the manager.
	void RemoveMasterParticleSystem(BSMasterParticleSystem* apMasterParticleSystem);
	// Updates the particle systems.
	void UpdateParallel(NiUpdateData& updateData);
	// Adds apEmitter to the master particle system at the index.
	void AddEmitter(uint32_t auiMasterParticleSystemIndex, NiAVObject* apEmitter);
	void ClearAllParticles();
	// The index of apChild among the effective children, -1 when absent.
	uint32_t GetChildIndex(NiAVObject* apChild);

protected:
	void CopyMembers(BSParticleSystemManager* pDest, NiCloningProcess& arCloneData);

public:
	static bool CheckEmitterRecurse(NiAVObject* apObject, BSMasterParticleSystem* apMasterSystem);
	static bool IsValidMasterParticleSystem(NiAVObject* apObject);
	static bool ReplaceEmitterRecurse(NiAVObject* apObject, BSMasterParticleSystem* apMasterSystem);
	static BSMasterParticleSystem* MakeMasterParticleSystem(NiNode* apBaseNode);

	// per master particle system index, the child slot holding it (-1 when none).
	uint32_t* aIndexToChildLookup;
	uint32_t uiIndexToChildLookupSize;
	BSTaskletGroup* pUpdateTaskletGroup;

	static NiRTTI ms_RTTI;
};

NiSmartPointer(BSParticleSystemManager);
