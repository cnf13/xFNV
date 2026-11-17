#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiDefaultAVObjectPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTSet.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTStringMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Reconstruction.hpp"

class BSAnimNoteListener;
class NiInterpolator;

// Owns and blends the controller sequences of an actor.
class NiControllerManager : public NiTimeController {
public:
	NiControllerManager();
	// The constructor for a manager of pkTarget's sequences, and the default
	// constructor.
	NiControllerManager(NiAVObject* pkTarget, bool bCumulativeAnimations);
	~NiControllerManager() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void ProcessClone(NiCloningProcess& kCloning) override;
	void PostLinkObject(NiStream& kStream) override;
	NiControllerManager* IsNiControllerManager() override { return this; }
	void Start(float) override {}
	void Stop() override {}
	void Update(NiUpdateData& UpdateData) override;
	void SetTarget(NiObjectNET*) override {}
	virtual void Start() {}

protected:
	bool TargetIsRequiredType() const override;

public:
	// Finds an inactive temporary blend sequence (or makes and adds one) and fills it with apSequence's pose.
	NiControllerSequence* CreateTempBlendSequence(NiControllerSequence* pkSequence,
		NiControllerSequence* pkSequenceToSynchronize);
	// Copies the cumulative flag, the sequences (each cloned and added under the empty name) and the palette clone.
	void CopyMembers(NiControllerManager* pkDest, NiCloningProcess& kCloning);
	// The first sequence holding apInterpolator (only an active one if abActiveOnly), or null.
	NiControllerSequence* FindControllerSequence(NiInterpolator* apInterpolator, bool abActiveOnly);
	// Deactivates every active sequence in the transition-source state; always returns false. The exe's names call
	// it DeactivateTransSourceSequence.
	bool StopTransSourceSequence(float fEaseOutTime);
	// The sequence named kName, or null (thiscall, ret 4; the name goes by reference).
	NiControllerSequence* GetSequenceByName(const NiFixedString& kName);
	// Deactivates every active sequence, easing out over fEaseOutTime.
	void DeactivateAll(float fEaseOutTime);
	// Activates pkSequence through NiControllerSequence::Activate (thiscall, ret 0x18; the
	// priority and bStartOver are read as bytes).
	bool ActivateSequence(NiControllerSequence* pkSequence, int32_t iPriority, bool bStartOver, float fWeight,
		float fEaseInTime, NiControllerSequence* pkTimeSyncSeq);
	// The animation note listener (pListener); the exe's shared one-field getter 0x70EC90
	// (thiscall, ret 0).
	BSAnimNoteListener* QNoteListener();
	// The palette the sequences resolve their targets' names through (m_spObjectPalette).
	NiAVObjectPalette* GetObjectPalette();
	// The sequence count, including empty slots.
	uint32_t GetSequenceCount();
	// The sequence at uiIndex, or null for an empty slot.
	NiControllerSequence* GetSequenceAt(uint32_t uiIndex);
	// Whether cumulative animations are enabled.
	bool GetCumulativeAnimations();
	// Deactivates pkSequence over fEaseOutTime.
	bool DeactivateSequence(NiControllerSequence* pkSequence, float fEaseOutTime);
	bool Morph(NiControllerSequence* pkSourceSequence, NiControllerSequence* pkDestSequence, float fDuration,
		int32_t iPriority, float fSourceWeight, float fDestWeight);
	// Eases pkSourceSequence out and pkDestSequence in over fDuration; false unless the source is active and the
	// destination inactive.
	bool CrossFade(NiControllerSequence* pkSourceSequence, NiControllerSequence* pkDestSequence, float fDuration,
		int32_t iPriority, bool bStartOver, float fWeight, NiControllerSequence* pkTimeSyncSeq);
	// Blends into pkSequence (started at fDestFrame) from a temporary sequence holding the current pose; false unless
	// pkSequence is inactive.
	bool BlendFromPose(NiControllerSequence* pkSequence, float fDestFrame, float fDuration, int32_t iPriority,
		NiControllerSequence* pkSequenceToSynchronize);
	NiAVObject* GetAccumRoot() const;
	// Clears every sequence's targets and owner, then empties the sequence array and the name map.
	void RemoveAllSequences();
	void RemoveSequence(NiControllerSequence* apSequence);
	// The PDB's RemoveSequence(pkSequence, aspControllerSequence): finds pkSequence in
	// m_kSequenceArray, removes it there, from m_kStringMap and from the active sequences, and leaves the removed
	// sequence in aspControllerSequence (thiscall, ret 8, 0xA2E7D0; the one-argument RemoveSequence 0xA2EC50 calls it
	// with a local pointer).
	void RemoveSequence(NiControllerSequence* pkSequence, NiPointer<NiControllerSequence>& aspControllerSequence);
	// The PDB's AddSequence: adds pkSequence, optionally stores its targets, and indexes it by kName (thiscall,
	// ret 0xC, 0xA2F0C0).
	bool AddSequence(NiControllerSequence* pkSequence, const NiFixedString& kName, bool bStoreTargets);

	static NiObject* CreateObject();

	NiTObjectArray<NiPointer<NiControllerSequence>> m_kSequenceArray;
	NiTPrimitiveSet<NiControllerSequence*> m_kActiveSequences;
	NiTStringPointerMap<NiControllerSequence*> m_kStringMap;
	BSAnimNoteListener* pListener;
	bool m_bCumulative;
	NiTObjectSet<NiPointer<NiControllerSequence>> m_kTempBlendSeqs;
	NiPointer<NiDefaultAVObjectPalette> m_spObjectPalette;

	static NiRTTI ms_RTTI;
};

NiSmartPointer(NiControllerManager);

inline constexpr fnvd::Vtable NiControllerManager_vftable{};

// The controller managers whose viewer strings were shown last, newest first: the scene viewer's tree stores them
// and NiBlendInterpolator::GetViewerStrings names the sequence of each item from them.
using ControllerManagerPair = NiControllerManager* [2];
inline ControllerManagerPair pLastViewerStringsControllerManager{};
