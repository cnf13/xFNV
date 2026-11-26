#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class NiObjectNET;
struct NiUpdateData;

// Animates an object over time; the controllers of one object form a singly linked list.
class NiTimeController : public NiObject {
public:
    enum Flags : uint16_t {
        ANIMTYPE_POS = 0x0,
        ANIMTYPE_MASK = 0x1,
        CYCLETYPE_POS = 0x1,
        CYCLETYPE_MASK = 0x6,
        ACTIVE_MASK = 0x8,
        DIRECTION_MASK = 0x10,
        MANAGERCONTROLLED_MASK = 0x20,
        COMPUTESCALEDTIME_MASK = 0x40,
        FORCEUDPATE_MASK = 0x80,
    };

    // How Update reads its time: as given, or relative to the time Start was called.
    enum AnimType {
        APP_TIME,
        APP_INIT,
    };

    // What happens past the key range: wrap, run back and forth, or hold the end.
    enum CycleType {
        LOOP,
        REVERSE,
        CLAMP,
        MAX_CYCLE_TYPES,
    };

    NiTimeController();
    ~NiTimeController() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
    void ProcessClone(NiCloningProcess& kCloning) override;
    virtual void Start(float fTime);
    virtual void Stop();
    virtual void Update(NiUpdateData& arUpdateData) = 0;
    virtual void SetTarget(NiObjectNET* pkTarget);
    virtual bool IsTransformController() const {
        return false;
    }
    virtual bool IsVertexController() const {
        return false;
    }
    virtual float ComputeScaledTime(float afTime);
    virtual void OnPreDisplay() {}
    virtual bool IsStreamable() const {
        return true;
    }

protected:
    // True when the target is of the class the controller animates.
    virtual bool TargetIsRequiredType() const = 0;

public:
    bool GetBit(uint16_t uMask) const;
    void SetBit(bool bVal, uint16_t uMask);
    bool GetActive() const;
    void SetComputeScaledTime(bool bComputeScaledTime);
    void SetPhase(float fPhase);

    // Copies the timing fields to apDest, retargets it at this target's clone (when the clone has the target's
    // class) and gives it a clone of the rest of the controller list.
    void CopyMembers(NiTimeController* pkDest, NiCloningProcess& kCloning);
    // True when Update has nothing to do at afTime: inactive, or the time (and scaled time) did not move and no
    // update is forced. Otherwise caches the scaled time and clears the forced update.
    bool DontDoUpdate(float fTime);
    void SetNext(NiTimeController* pkNext);
    // The animated object.
    NiObjectNET* GetTarget() const;
    // The next controller.
    NiTimeController* GetNext();
    void SetActive(bool bActive);
    int32_t GetLast();

    // "<prefix> = <type name>" in a fresh _NiMalloc block the caller owns.
    static char* GetViewerString(const char* apcPrefix, AnimType aeType);
    static char* GetViewerString(const char* apcPrefix, CycleType aeType);
    // Starts the controllers of pkObj at fTime.
    static void StartAnimations(NiObjectNET* pkObj, float fTime);
    // Starts the controllers of pkObj.
    static void StartAnimations(NiObjectNET* pkObj);

    uint16_t m_usFlags;
    float m_fFrequency;
    float m_fPhase;
    float m_fLoKeyTime;
    float m_fHiKeyTime;
    float m_fStartTime;
    float m_fLastTime;
    float m_fWeightedLastTime;
    float m_fScaledTime;
    NiObjectNET* m_pkTarget;
    NiPointer<NiTimeController> m_spNext;

    static NiRTTI ms_RTTI;
};

inline NiTimeController* NiTimeController::GetNext() {
    return m_spNext;
}

inline NiObjectNET* NiTimeController::GetTarget() const {
    return m_pkTarget;
}

inline void NiTimeController::SetNext(NiTimeController* pkNext) {
    NiTimeController* pkOld = m_spNext.m_pObject;
    NiTimeController* pkNew = pkNext;
    if (pkOld != pkNew) {
        if (pkOld) {
            NiDecRefCount(pkOld);
        }
        m_spNext.m_pObject = pkNew;
        if (pkNew) {
            NiIncRefCount(pkNew);
        }
    }
}
