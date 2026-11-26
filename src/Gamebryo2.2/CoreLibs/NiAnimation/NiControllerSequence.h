#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"

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
        // Equal when both interpolators and both controllers are missing or equal, and the priorities match.
        bool IsEqual(const InterpArrayItem& kDest) const;
        // Reads the interpolator and controller links and the priority.
        void LoadBinary(NiStream& kStream);
        // Assigns kOther's smart pointers and copies its fields.
        InterpArrayItem& operator=(const InterpArrayItem& kOther);

        NiPointer<NiInterpolator> m_spInterpolator;
        NiPointer<NiInterpController> m_spInterpCtlr;
        NiBlendInterpolator* m_pkBlendInterp;
        uint8_t m_ucBlendIdx;
        uint8_t m_ucPriority;
    };

    // Names the animated object, property and controller an interpolator of the sequence binds to.
    struct IDTag {
        // The five names start at the empty handle, then each takes its argument's reference.
        IDTag(
            const NiFixedString& akAVObjectName = EmptyFS,
            const NiFixedString& akPropertyType = EmptyFS,
            const NiFixedString& akCtlrType = EmptyFS,
            const NiFixedString& akCtlrID = EmptyFS,
            const NiFixedString& akInterpolatorID = EmptyFS
        );
        ~IDTag();

        // Reads the tag's names; pvDeprecatedExtraData is the old-format state the caller carries across the tags.
        void LoadBinary(NiStream& kStream, void*& pvDeprecatedExtraData);
        // Sets all five names to the empty handle.
        void ClearValues();
        IDTag* Operator_Set(const IDTag* pkOther);
        // Turns the palette offsets an old file's names hold into the palette's strings.
        void HandleDeprecatedExtraData(NiStringPalette* pkPalette, NiStream& kStream);

        // Each setter takes the new name's reference and drops the old one's, testing both against NULL_STRING.
        void SetAVObjectName(const NiFixedString& kAVObjectName) {
            AssignName(m_kAVObjectName, kAVObjectName);
        }
        void SetPropertyType(const NiFixedString& kPropertyType) {
            AssignName(m_kPropertyType, kPropertyType);
        }
        void SetCtlrType(const NiFixedString& kCtlrType) {
            AssignName(m_kCtlrType, kCtlrType);
        }
        void SetCtlrID(const NiFixedString& kCtlrID) {
            AssignName(m_kCtlrID, kCtlrID);
        }
        void SetInterpolatorID(const NiFixedString& kInterpolatorID) {
            AssignName(m_kInterpolatorID, kInterpolatorID);
        }

        NiFixedString m_kAVObjectName;
        NiFixedString m_kPropertyType;
        NiFixedString m_kCtlrType;
        NiFixedString m_kCtlrID;
        NiFixedString m_kInterpolatorID;

    private:
        static void AssignName(NiFixedString& kName, const NiFixedString& kNewName) {
            NiGlobalStringTable::GlobalStringHandle kOld = kName.GetHandle();
            NiGlobalStringTable::GlobalStringHandle kNew = kNewName.GetHandle();
            if (kOld != kNew) {
                NiGlobalStringTable::AcquireHandle(kNew);
                NiGlobalStringTable::ReleaseHandle(kOld);
                kName.SetHandle(kNew);
            }
        }
    };

    NiControllerSequence();
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
    // Starts a morph to pkDestSequence. Passes the whole of iPriority on to Activate.
    bool StartMorph(
        NiControllerSequence* pkDestSequence,
        float fDuration,
        int32_t iPriority,
        float fSourceWeight,
        float fDestWeight
    );
    State GetState();
    // Sets the state, registering a newly active sequence with its owner.
    void SetState(State eState);
    void CopyMembers(NiControllerSequence* pkDest, NiCloningProcess& kCloning);
    void AttachInterpolators(int32_t cPriority);
    void DetachInterpolators();
    // The scaled time at afTime (looped or clamped to the key range); bStoreLastTime keeps it as the last time.
    float ComputeScaledTime(float fTime, bool bStoreLastTime);
    void GetObjectNameAt(uint32_t uiIndex, char* pcObjectName);
    void SetPhase(float afPhase, bool abUpdateControllers);
    bool ReallocateArrays();
    uint32_t
    AddInterpolator(NiInterpolator* pkInterpolator, const NiControllerSequence::IDTag& kIDTag, uint8_t ucPriority);
    // Takes the interpolator at uiIndex out of its entry into aspInterpolator and clears the entry and its tag.
    void RemoveInterpolator(uint32_t uiIndex, NiPointer<NiInterpolator>& aspInterpolator);
    // Fills pkPoseSequence with a pose interpolator for each controlled interpolator of this sequence, emptying it
    // first when abRemInterps.
    void PopulatePoseSequence(
        NiControllerSequence* pkPoseSequence,
        uint32_t uiArraySize,
        NiControllerSequence* pkSequenceToSynchronize,
        bool abRemInterps
    );
    // Adds an old-format name/controller pair: the controller takes the sequence's cycle type and frequency and
    // runs forward. False when either is missing or pkController is no NiInterpController.
    bool AddNameControllerPair(const char* pcObjectName, NiTimeController* pkController);
    // Adds pkController's interpolators under tags built from an old-format "object\ntype\ntarget" name; false when
    // AddInterpolator fails.
    bool ConvertOldNameControllerPair(const char* pcObjectName, NiInterpController* pkController);
    void SetInterpsWeightAndTime(float fWeight, float fEaseSpinner, float fTime);
    bool Activate(
        int32_t cPriority,
        bool bStartOver,
        float fWeight,
        float fEaseInTime,
        NiControllerSequence* pkTimeSyncSeq,
        bool bTransition
    );
    // Blends from this sequence to pkDestSequence over fDuration, starting the destination at fDestFrame.
    bool StartBlend(
        NiControllerSequence* pkDestSequence,
        float fDuration,
        float fDestFrame,
        int32_t iPriority,
        float fSourceWeight,
        float fDestWeight,
        NiControllerSequence* pkTimeSyncSeq
    );
    // Whether this sequence can be time-synced to pkTargetSequence.
    bool CanSyncTo(NiControllerSequence* pkTargetSequence);

    // False when this sequence is in pkSequence's partner chain.
    bool VerifyDependencies(NiControllerSequence* pkSequence);
    bool VerifyMatchingMorphKeys(NiControllerSequence* pkTimeSyncSeq);
    void Update(float fTime, bool bUpdateInterpolators);
    // Binds the interpolators to their targets under pkRoot through the owner's object palette; always true.
    bool StoreTargets(NiAVObject* pkRoot);
    // Gathers the transform interpolators into the root's multi-target transform and tread controllers, making
    // the controllers that are missing; true when it added a controller.
    bool ResolveTransformInterpolators(
        NiAVObject* pkRoot,
        NiAVObjectPalette* pkObjectPalette,
        uint32_t aiTransformControllerCount
    );
    float GetOffset();
    // The time of the sequence's last key.
    float GetEndKeyTime();
    float GetLastScaledTime();
    void SetOffset(float fOffset);
    void SetName(const NiFixedString& kName);
    void SetAccumRootName(const NiFixedString& kAccumRootName);
    const NiFixedString& GetAccumRootName() const;
    void SetPriority(uint32_t auiIndex, uint8_t aucPriority);
    uint8_t GetPriority(uint32_t auiIndex);
    NiInterpolator* GetInterpolatorAt(uint32_t uiIndex);
    // The target of the controller at uiIndex, or null when the entry has no controller.
    NiObjectNET* GetTargetAt(uint32_t uiIndex);
    NiTextKeyExtraData* GetTextKeys();
    NiControllerSequence* GetPartnerSequence();

    float GetLastTime() const {
        return m_fLastTime;
    }

    NiControllerManager* GetOwner() const {
        return m_pkOwner;
    }

    // m_kName, through the shared getter.
    const NiFixedString& GetName() const {
        return m_kName;
    }

    float GetBeginKeyTime() const;
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
    static void CreateSequenceFromFile(
        const NiStream& kStream,
        uint32_t uiIndex,
        NiPointer<NiControllerSequence>& aspControllerSequence
    );
    // Builds a controller sequence from a pre-5.0 NiSequence into aspControllerSequence.
    static void ConvertOldSequence(NiSequence* pkOldSequence, NiPointer<NiControllerSequence>& aspControllerSequence);
    // Creates a sequence on the engine's heap.
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

    // An old file's palette offset for no name.
    static constexpr uint32_t INVALID_INDEX = 0xFFFFFFFFu;
    // The class's RTTI record; filled in when the engine starts up.
    static NiRTTI ms_RTTI;
    // The text key prefix that marks morph keys, and its length.
    static inline const char* pMorphTag = "m:";
    static inline uint32_t iMorphTagLen = 2;
};

inline void NiControllerSequence::SetAccumRootName(const NiFixedString& kAccumRootName) {
    m_kAccumRootName.Assign(kAccumRootName);
}

inline const NiFixedString& NiControllerSequence::GetAccumRootName() const {
    return m_kAccumRootName;
}

inline void NiControllerSequence::SetPriority(uint32_t auiIndex, uint8_t aucPriority) {
    if (auiIndex < m_uiArraySize) {
        m_pkInterpArray[auiIndex].m_ucPriority = aucPriority;
    }
}

inline uint8_t NiControllerSequence::GetPriority(uint32_t auiIndex) {
    if (auiIndex >= m_uiArraySize) {
        return 0xFF;
    }

    return m_pkInterpArray[auiIndex].m_ucPriority;
}

inline void NiControllerSequence::SetName(const NiFixedString& kName) {
    m_kName.Assign(kName);
}

inline float NiControllerSequence::GetLastScaledTime() {
    return m_fLastScaledTime;
}

inline NiInterpolator* NiControllerSequence::GetInterpolatorAt(uint32_t uiIndex) {
    return m_pkInterpArray[uiIndex].m_spInterpolator;
}

// The index is re-read and the controller fetched again for the GetTarget call.
inline NiObjectNET* NiControllerSequence::GetTargetAt(uint32_t uiIndex) {
    if (m_pkInterpArray[uiIndex].m_spInterpCtlr) {
        return m_pkInterpArray[uiIndex].m_spInterpCtlr->GetTarget();
    }
    return nullptr;
}

inline NiTextKeyExtraData* NiControllerSequence::GetTextKeys() {
    return m_spTextKeys;
}

inline NiControllerSequence* NiControllerSequence::GetPartnerSequence() {
    return m_pkPartnerSequence;
}

inline NiControllerSequence::CycleType NiControllerSequence::GetCycleType() const {
    return m_eCycleType;
}

inline float NiControllerSequence::GetPhase() {
    double dOffset = GetOffset();
    return dOffset - GetBeginKeyTime();
}

inline void NiControllerSequence::SetOffset(float fOffset) {
    m_fOffset = fOffset;
}
