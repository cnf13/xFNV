#pragma once

#include "BSCore/BSSpinLock.h"
#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCollisionObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"

#include <stdint.h>

class bhkRigidBody;
class bhkWorldObject;
class hkpWorldObject;
class NiAVObject;
class NiColor;
class NiGeometry;
class NiNode;
class NiPoint3;
class NiTransform;

// Joins a scene graph node to the Havok body that moves it.
class bhkNiCollisionObject : public NiCollisionObject {
public:
    // Called with the object and its flags when the Havok side's transform changed.
    using TransChangeFunc = void (*)(bhkNiCollisionObject*, uint32_t);
    // Called with the debug geometry to display and its color.
    using DebugDisplayFunc = void (*)(NiGeometry*, const NiColor&);

    // What Synchronize copies: both ways, only to Havok, only from Havok, or nothing.
    enum SYNCMODE : int32_t {
        SYNC_BOTH = 0,
        SYNC_TOHAVOK = 1,
        SYNC_FROMHAVOK = 2,
        SYNC_NONE = 3,
    };

    enum bhkNiCollisionObjectFLAGS : int32_t {
        ACTIVE_POS = 1,
        RESETTRANS_POS = 2,
        NOTIFY_POS = 4,
        SETLOCAL_POS = 8,
        DEBUGDISPLAY_POS = 16,
        USEVEL_POS = 32,
        RESET_POS = 64,
        // bhkBlendCollisionObject's.
        BLENDPOS_POS = 256,
        ALWAYSBLEND_POS = 512,
    };

    bhkNiCollisionObject();
    bhkNiCollisionObject(NiAVObject* apSceneObject);
    ~bhkNiCollisionObject() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    bhkNiCollisionObject* IsBhkNiCollisionObject() override {
        return this;
    }
    void LoadBinary(NiStream& arStream) override;
    void LinkObject(NiStream& arStream) override;
    bool RegisterStreamables(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    bool IsEqual(NiObject* apObject) override;
    void AddViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    void SetSceneGraphObject(NiAVObject* apSceneObject) override;
    void UpdateWorldData(NiUpdateData& arUpdateData) override;
    void RecreateWorldData() override {}

    // The body's velocity in game units (zero here).
    virtual NiPoint3 GetVelocity();

    // Copies the Havok body's transform to the scene graph object, and the other way round.
    virtual void hktoNiTransform() = 0;
    virtual void NitohkTransform() = 0;

    virtual void ClearVelocities() = 0;

    virtual void SetMotionType(hkpMotion::MotionType aeType, bhkWorldObject* apObject, bool abAllowActivate) = 0;

    // True when the body is keyframed (moved by the scene graph rather than simulated).
    virtual bool QKeyframe();

    virtual void SetTransform(NiTransform& arTrans);
    virtual bool QIncludedInSkeleton() const {
        return true;
    }
    virtual void SetDebugDisplay(bool abTF);

    // The flag bits that are in usFlags (true when any of them is set).
    bool GetFlag(uint16_t ausFlag);
    // Sets or clears the notify flag, and the set-local flag when setting. Defined in TESObjectREFR.cpp.
    void SetNotify(bool abTF);

    // True when the object has been flagged for a reset (flag 0x40).
    bool IsReset();

    // The world object (the rigid body) the collision object wraps.
    bhkWorldObject* GetWorldObject();
    // Holds the world object as this object's Havok body, dropping the old one.
    void SetWorldObject(bhkWorldObject* apWorldObject);
    bool QNotify();
    bool HasVelocity();
    bool QSetLocal();
    void SetReset(bool abTF);
    void SetSetLocal(bool abTF);
    // Whether the velocity flag (0x20) is set and the object is not flagged for a reset.
    bool GetUseVelocity();

protected:
    void SetBit(bool bVal, uint16_t uMask);

public:
    static void SetTransChangeCallback(TransChangeFunc apFunc);
    static void SetDebugDisplayCallback(void (*apCallbackFunc)(NiGeometry*, const NiColor&));
    // The collision object apBody carries as a property.
    static bhkNiCollisionObject* Getbhk(const hkpWorldObject* apBody);
    static bhkNiCollisionObject* GetbhkNiCollisionObject(NiAVObject* apObject);
    // Whether the object below apRoot named apName (apRoot itself without one) faces up.
    static bool GetFaceUp(NiAVObject* apRoot, const char* apName);
    // Flags the node's collision objects for a reset, recursively when abRecurse.
    static void ResetSim(NiAVObject* apObject, bool abRecurse);
    // Synchronizes the Havok transforms of apObject and the objects below it, forcing when abForce.
    static void SynchronizeRec(NiAVObject* apObject, bool abUpdate);
    // Copies transforms between the scene graph and Havok for apObject with the calling thread's sync mode set to
    // aeMode, restoring the previous mode afterwards.
    static void Synchronize(NiAVObject* apObject, SYNCMODE aeMode);

    // The calling thread's sync mode: SetSyncMode stores it, the transform updates read it.
    static inline thread_local SYNCMODE eHavokSyncMode;

    static void SetSyncMode(bhkNiCollisionObject::SYNCMODE aeMode);
    static void EnableCollision(NiAVObject* apObject, bool abEnable);
    static void SetRigidBodyName(NiAVObject* apObject);

    uint16_t usFlags;
    bhkRigidBody* spWorldObject;

    static NiRTTI ms_RTTI;
    static inline TransChangeFunc pTransChangeFunc{};
    static inline DebugDisplayFunc pDebugDispFunc{};
    // Held across a Havok motion type change.
    static BSSpinLock kKeyframeLock;
    // The bWONameSync:Havok ini setting.
    static SettingT<INISettingCollection> bWONameSync;
    static inline bool bAlwaysReportTransform;
    static inline int32_t iCount;
};

// Zero: a plain collision object does not move.
inline NiPoint3 bhkNiCollisionObject::GetVelocity() {
    return NiPoint3::ZERO;
}

inline void bhkNiCollisionObject::SetSyncMode(bhkNiCollisionObject::SYNCMODE aeMode) {
    eHavokSyncMode = aeMode;
}

inline void bhkNiCollisionObject::SetNotify(bool abTF) {
    SetBit(abTF, 4);
    if (abTF) {
        SetSetLocal(true);
    }
}
