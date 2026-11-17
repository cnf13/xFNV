#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class BSAnimNote;
class NiAVObject;
class NiAVObjectPalette;
class NiBlendInterpolator;
class NiControllerManager;
class NiInterpController;
class NiInterpolator;
class NiObjectNET;
class NiSequence;
class NiStream;
class NiStringPalette;
class NiTextKeyExtraData;

NiSmartPointer(NiControllerSequence);

// A named animation sequence: an array of interpolators plus the timing state its controller manager drives.
class NiControllerSequence : public NiObject {
public:
	// The phase the sequence is in; the manager moves it between these while playing it.
	enum State : int32_t {
		kInactive = 0x0,
		kAnimating = 0x1,
		kEaseIn = 0x2,
		kEaseOut = 0x3,
		kTransSource = 0x4,
		kTransDest = 0x5,
		kMorphSource = 0x6,
	};

	// How scaled time wraps around the key range.
	enum CycleType : int32_t {
		CYCLE_LOOP = 0x0,
		CYCLE_REVERSE = 0x1,
		CYCLE_CLAMP = 0x2,
	};

	// One entry of the interpolator array and the slot it occupies inside its blend interpolator.
	struct InterpArrayItem {
		InterpArrayItem();

		// Clears the entry's interpolator and controller and its blend slot.
		void ClearValues();
		// Frees the array of items this is the first of (vector deleting destructor; thiscall, ret 4, 0xA31500).
		// Returns the block, which starts at the count before the items, not an item.
		void* Free(int32_t aiFlags);
		// Equal when both interpolators and both controllers are missing or equal, and the priorities match.
		bool IsEqual(const InterpArrayItem& kDest) const;
		// Reads the interpolator and controller links and the priority.
		void LoadBinary(NiStream& kStream);
		// Assigns kOther's smart pointers and copies its fields.
		InterpArrayItem& operator=(const InterpArrayItem& kOther);
		// Not named in the PDB or IDA; name chosen here. Void: its one caller (Free, 0xA31594)
		// never reads eax.
		void ReleaseRefs();

		NiPointer<NiInterpolator> m_spInterpolator;
		NiPointer<NiInterpController> m_spInterpCtlr;
		NiBlendInterpolator* m_pkBlendInterp;
		uint8_t m_ucBlendIdx;
		uint8_t m_ucPriority;
	};

	// Names the animated object, property and controller an interpolator of the sequence binds to.
	struct IDTag {
		// The PDB's IDTag(kAVObjectName, kPropertyType, kCtlrType, kCtlrID, kInterpolatorID) constructor: the five
		// names start at the empty handle, then each takes its argument's reference (thiscall, ret 0x14, 0xA312D0).
		IDTag(const NiFixedString& akAVObjectName = EmptyFS, const NiFixedString& akPropertyType = EmptyFS,
			const NiFixedString& akCtlrType = EmptyFS, const NiFixedString& akCtlrID = EmptyFS,
			const NiFixedString& akInterpolatorID = EmptyFS);
		~IDTag();

		// Reads the tag's names; pvDeprecatedExtraData is the old-format state the caller carries across the tags.
		void LoadBinary(NiStream& kStream, void*& pvDeprecatedExtraData);
		// Sets all five names to the empty handle.
		void ClearValues();
		// The assignment operator: reads pkOther only.
		// Inferred type: pkOther: NiControllerSequence::IDTag* from 0xa32c52 (parameter 1 of
		// NiControllerSequence::AddInterpolator), its members used at 0xa31400
		IDTag* Operator_Set(const IDTag* pkOther);
		// The vector deleting destructor. Returns the block, which
		// starts at the count before the tags, not a tag.
		void* Free(char a1);
		// Turns the palette offsets an old file's names hold into the palette's strings.
		void HandleDeprecatedExtraData(NiStringPalette* pkPalette, NiStream& kStream);

		NiFixedString m_kAVObjectName;
		NiFixedString m_kPropertyType;
		NiFixedString m_kCtlrType;
		NiFixedString m_kCtlrID;
		NiFixedString m_kInterpolatorID;
	};

	NiControllerSequence();
	// The PDB's NiControllerSequence(kName, uiArraySize, uiArrayGrowBy) constructor (thiscall, ret 0xC, 0xA32A10).
	NiControllerSequence(const NiFixedString& kName, uint32_t uiArraySize, uint32_t uiArrayGrowBy);
	~NiControllerSequence() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	void ProcessClone(NiCloningProcess& kCloning) override;
	void PostLinkObject(NiStream& kStream) override;
	virtual bool Deactivate(float fEaseOutTime, bool bTransition);

	// Deactivates the sequence and drops every interpolator's controller and blend interpolator.
	void ClearTargets();
	// Starts a morph to pkDestSequence (thiscall, ret 0x14). The exe passes iPriority's whole dword on to Activate.
	bool StartMorph(NiControllerSequence* pkDestSequence, float fDuration, int32_t iPriority, float fSourceWeight,
		float fDestWeight);
	State GetState();
	void CopyMembers(NiControllerSequence* pkDest, NiCloningProcess& kCloning);
	void AttachInterpolators(int32_t cPriority);
	void DetachInterpolators();
	// The scaled time at afTime (looped or clamped to the key range); bStoreLastTime keeps it as the last time.
	float ComputeScaledTime(float fTime, bool bStoreLastTime);
	void GetObjectNameAt(uint32_t uiIndex, char* pcObjectName);
	void SetPhase(float afPhase, bool abUpdateControllers);
	bool ReallocateArrays();
	uint32_t AddInterpolator(NiInterpolator* pkInterpolator, const NiControllerSequence::IDTag& kIDTag,
		uint8_t ucPriority);
	// Takes the interpolator at uiIndex out of its entry into aspInterpolator and clears the entry and its tag.
	void RemoveInterpolator(uint32_t uiIndex, NiPointer<NiInterpolator>& aspInterpolator);
	// Fills pkPoseSequence with a pose interpolator for each controlled interpolator of this sequence, emptying it
	// first when abRemInterps.
	void PopulatePoseSequence(NiControllerSequence* pkPoseSequence, uint32_t uiArraySize,
		NiControllerSequence* pkSequenceToSynchronize, bool abRemInterps);
	// Adds an old-format name/controller pair: the controller takes the sequence's cycle type and frequency and
	// runs forward. False when either is missing or pkController is no NiInterpController.
	bool AddNameControllerPair(const char* pcObjectName, NiTimeController* pkController);
	// Adds pkController's interpolators under tags built from an old-format "object\ntype\ntarget" name; false when
	// AddInterpolator fails.
	bool ConvertOldNameControllerPair(const char* pcObjectName, NiInterpController* pkController);
	void SetInterpsWeightAndTime(float fWeight, float fEaseSpinner, float fTime);
	bool Activate(int32_t cPriority, bool bStartOver, float fWeight, float fEaseInTime,
		NiControllerSequence* pkTimeSyncSeq, bool bTransition);
	// Blends from this sequence to pkDestSequence over fDuration, starting the destination at fDestFrame.
	bool StartBlend(NiControllerSequence* pkDestSequence, float fDuration, float fDestFrame, int32_t iPriority,
		float fSourceWeight, float fDestWeight, NiControllerSequence* pkTimeSyncSeq);
	// Whether this sequence can be time-synced to pkTargetSequence.
	bool CanSyncTo(NiControllerSequence* pkTargetSequence);

	// False when this sequence is in pkSequence's partner chain (KEEP: see its body).
	bool VerifyDependencies(NiControllerSequence* pkSequence);
	// Not reconstructed; forward to the original code.
	bool VerifyMatchingMorphKeys(NiControllerSequence* pkTimeSyncSeq);
	void Update(float fTime, bool bUpdateInterpolators);
	// Binds the interpolators to their targets under pkRoot through the owner's object palette; always true.
	bool StoreTargets(NiAVObject* pkRoot);
	// Gathers the transform interpolators into the root's multi-target transform and tread controllers, making
	// the controllers that are missing; true when it added a controller.
	bool ResolveTransformInterpolators(NiAVObject* pkRoot, NiAVObjectPalette* pkObjectPalette,
		uint32_t aiTransformControllerCount);
	// The sequence's time offset (thiscall, ret 0, ST(0), 0x639AA0).
	float GetOffset();
	// The time of the sequence's last key (thiscall, ret 0, ST(0), 0x508100).
	float GetEndKeyTime();
	// m_fLastScaledTime (+0x3C): the exe's shared field getter (thiscall, ret 0, ST(0),
	// 0x598040; IDA: BGSLightingTemplate::GetDataFogPower).
	float GetLastScaledTime();
	void SetOffset(float fOffset);
	void SetName(const NiFixedString& kName);
	void SetAccumRootName(const NiFixedString& kAccumRootName);
	NiInterpolator* GetInterpolatorAt(uint32_t uiIndex);
	// The target of the controller at uiIndex, or null when the entry has no controller.
	NiObjectNET* GetTargetAt(uint32_t uiIndex);
	NiTextKeyExtraData* GetTextKeys();
	// m_pkPartnerSequence; the exe calls the shared one-field getter 0x6286D0.
	NiControllerSequence* GetPartnerSequence();

	// m_kName, through the shared getter.
	const NiFixedString& GetName() const { return m_kName; }

	// Out of line: the exe calls it (0x759450).
	float GetBeginKeyTime() const;
	// The cycle type, m_eCycleType; the exe calls the shared one-field +0x24 getter
	// (thiscall, ret 0, 0x59BB30; same function as Menu::GetFadeState).
	CycleType GetCycleType() const;
	float GetSequenceWeight() const;
	void SetSequenceWeight(float fWeight);
	void ResetSequence();
	bool SetCycleType(NiTimeController::CycleType eCycleType);
	NiControllerSequence::IDTag* GetIDTag(uint32_t uiTagIndex);
	float GetPhase();
	float FindCorrespondingMorphFrame(NiControllerSequence* pkSourceSequence, float fSourceSequenceTime);
	void RemoveInterpolator_0(uint32_t uiIndex);

	// A viewer line "<prefix> = <state name>" on the engine's heap.
	static char* GetViewerString(const char* apPrefix, State aeState);
	// The sequence at uiIndex of a loaded stream into aspControllerSequence (null when out of range); a file before
	// 5.0.0.0 holds an old NiSequence, converted.
	static void CreateSequenceFromFile(const NiStream& kStream, uint32_t uiIndex,
		NiPointer<NiControllerSequence>& aspControllerSequence);
	// Builds a controller sequence from a pre-5.0 NiSequence into aspControllerSequence.
	static void ConvertOldSequence(NiSequence* pkOldSequence, NiPointer<NiControllerSequence>& aspControllerSequence);
	// Creates a sequence on the engine's heap; the constructor is not reconstructed yet.
	static NiObject* CreateObject();

	NiFixedString m_kName;
	uint32_t m_uiArraySize;
	uint32_t m_uiArrayGrowBy;
	InterpArrayItem* m_pkInterpArray;
	IDTag* m_pkIDTagArray;
	float m_fSeqWeight;
	NiPointer<NiTextKeyExtraData> m_spTextKeys;
	CycleType m_eCycleType;
	float m_fFrequency;
	float m_fBeginKeyTime;
	float m_fEndKeyTime;
	float m_fLastTime;
	float m_fWeightedLastTime;
	float m_fLastScaledTime;
	NiControllerManager* m_pkOwner;
	State m_eState;
	float m_fOffset;
	float m_fStartTime;
	float m_fEndTime;
	float m_fDestFrame;
	NiControllerSequence* m_pkPartnerSequence;
	NiFixedString m_kAccumRootName;
	NiAVObject* m_pkAccumRoot;
	// Deprecated string palette of the pre-2.2 stream format.
	NiPointer<NiObject> m_spDeprecatedStringPalette;
	int16_t ssCurAnimNIdx;
	NiPointer<BSAnimNote>* spAnimNotesA;
	uint16_t usNumNotes;
	bool bRemovableObjects;

	// The PDB's NiControllerSequence::INVALID_INDEX: an old file's palette offset for no name (the exe compares
	// with -1, cmp eax, -1 at 0xA30D77).
	static constexpr uint32_t INVALID_INDEX = 0xFFFFFFFFu;
	// The class's RTTI record; filled in when the engine starts up.
	static NiRTTI ms_RTTI;
	// The text key prefix that marks morph keys, and its length.
	static inline const char* pMorphTag = "m:";
	static inline uint32_t iMorphTagLen = 2;
};
inline constexpr fnvd::Vtable NiControllerSequence_vftable{};
