#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSSpinLock.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSNiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"

#include <stdint.h>

class bhkNiCollisionObject;
class NiCollisionObject;
class NiCullingProcess;
class NiDynamicEffectState;
class NiNode;
class NiProperty;
class NiPropertyState;
class NiQuaternion;
class NiRenderer;
struct NiUpdateData;

NiSmartPointer(NiAVObject);

// A scene graph object with a transform, a bound and properties.
class NiAVObject : public NiObjectNET {
public:
    enum NiFlags : uint32_t {
        APP_CULLED = 0x1,
        SELECTIVE_UPDATE = 0x2,
        SELECTIVE_UPDATE_TRANSFORMS = 0x4,
        SELECTIVE_UPDATE_CONTROLLER = 0x8,
        SELECTIVE_UPDATE_RIGID = 0x10,
        DISPLAY_OBJECT = 0x20,
        DISABLE_SORTING = 0x40,
        SELECTIVE_UPDATE_TRANSFORMS_OVERRIDE = 0x80,
        UNK_8 = 0x100,
        TRANS_IS_IDENTITY = 0x200,
        NO_DECALS = 0x400,
        ALWAYS_DRAW = 0x800,
        PREPROCESSED_NODE = 0x1000,
        FIXED_BOUND = 0x2000,
        FADED_IN = 0x4000,
        IGNORE_FADE = 0x8000,
        LOD_FADING_OUT = 0x10000,
        HAS_MOVING_SOUND = 0x20000,
        HAS_PROPERTY_CONTROLLER = 0x40000,
        HAS_ORPHANED_BOUND = 0x80000,
        NOT_VISIBLE = 0x100000,
        IGNORES_PICKING = 0x200000,
        WORLD_BOUND_CHANGE = 0x400000,
        NO_SHADOWS = 0x800000,
        HIGH_DETAIL = 0x1000000,
        UNK_25 = 0x2000000,
        UNK_26 = 0x4000000,
        PLAYER_BONE = 0x8000000,
        IMPOSTER_LOADED = 0x10000000,
        IS_POINTLIGHT = 0x20000000,
        DONE_INIT_LIGHTS = 0x40000000,
        IS_INSERTED = 0x80000000,
    };

    NiAVObject();
    ~NiAVObject() override;

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
    void SetBound(BSNiBound* apBound) override;
    virtual void UpdateControllers(NiUpdateData& UpdateData);
    virtual void ApplyTransform(const NiMatrix3&, const NiPoint3&, bool) {}
    virtual void SetMaterialNeedsUpdate(bool) {}
    virtual void SetDefaultMaterialNeedsUpdateFlag(bool) {}
    virtual NiAVObject* GetObjectByName(const NiFixedString& kName);
    virtual void SetSelectiveUpdateFlags(bool& bSelectiveUpdate, int32_t bSelectiveUpdateTransforms, bool& abRigid);
    virtual void UpdateDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags);
    virtual void UpdateSelectedDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags);
    virtual void UpdateRigidDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags);
    virtual void UpdatePropertiesDownward(NiPropertyState* pkParentState);
    virtual void UpdateEffectsDownward(NiDynamicEffectState*) {}
    virtual void UpdateWorldData(NiUpdateData& UpdateData);
    virtual void UpdateWorldBound() {}
    virtual void UpdateTransformAndBounds(NiUpdateData& UpdateData);
    virtual void PreAttachUpdate(NiNode* pEventualParent, NiUpdateData& UpdateData);
    virtual void PreAttachUpdateProperties(NiNode* pEventualParent);
    virtual void PreAttachUpdateEffects(NiNode* pEventualParent);
    virtual void PostAttachUpdate();
    virtual void OnVisible(NiCullingProcess&) {}
    virtual void PurgeRendererData(NiRenderer* apkRenderer);

    // The parent node, or null.
    NiNode* GetParent() const;
    const NiPoint3& GetTranslate();
    const NiPoint3& GetWorldTranslate() const;
    const NiTransform& GetLocalTransform();
    void SetTranslate(const NiPoint3& akTrn);
    void SetTranslate(float afX, float afY, float afZ);
    void SetWorldTranslate(const NiPoint3& kTrn);
    void SetRotate(const NiMatrix3& akRot);
    // Sets the local rotation to fAngle about the axis (x, y, z).
    void SetRotate(float fAngle, float x, float y, float z);
    void SetWorldRotate(const NiMatrix3& kRot);
    void SetScale(float fScale);
    // The local scale.
    float GetScale();
    // Returns the world scale (m_kWorld.m_fScale).
    float GetWorldScale();

    uint32_t GetFlag(NiFlags auiMask) const;
    void SetBit(bool bVal, uint32_t uMask);
    bool GetAppCulled() const;
    void SetAppCulled(bool bAppCulled);
    uint32_t GetSelectiveUpdate() const;
    void SetSelectiveUpdate(bool bUpdate);
    // SetBit of SELECTIVE_UPDATE_TRANSFORMS and SELECTIVE_UPDATE_RIGID.
    void SetSelectiveUpdateTransforms(bool bUpdate);
    void SetSelectiveUpdateRigid(bool bRigid);
    // GetFlag/SetBit of WORLD_BOUND_CHANGE.
    uint32_t GetWorldBoundChange() const;
    void SetWorldBoundChange(bool abBit);
    uint32_t QHasPropController() const;
    uint32_t GetNotVisible() const;
    uint32_t GetHighDetail() const;
    void SetDisplayObject(bool bDisplay);
    void SetFadedIn(bool abFaded);
    void SetFixedBound(bool abTF);
    void SetPickOff(bool abVal);
    void SetPlayerBone(bool abVal);
    void SetShadowReceiver(bool abShadow);

    bool HasPropertyController() const;
    bool HasTransformController();
    NiProperty* GetProperty(int32_t iType) const;
    // Whether the object counts as visible geometry. True with a world bound whose radius is not 0.0; a NaN radius
    // counts.
    int32_t IsVisualObject();
    NiTPointerList<NiPointer<NiProperty>>& GetPropertyList();
    void AttachProperty(NiProperty* pkProperty);
    // Releases the removed property.
    void RemoveProperty(int32_t aiType);
    void RemoveProperty(int32_t aiType, NiPointer<NiProperty>& aspProperty);
    void DetachProperty(NiProperty* pkProperty);
    // Releases every property.
    void DetachAllProperties();
    // Set or clear the garbage collector's 0x40000000 mark in m_uiRefCount atomically; return the previous count.
    uint32_t AddGarbageMark();
    uint32_t RemoveGarbageMark();
    // Rebuilds the property state from the parents down.
    void UpdateProperties();

    const NiBound& GetWorldBound();
    float GetWorldBoundRadius();
    NiCollisionObject* GetCollisionObject() const;
    // Replaces the collision object and links it to this object.
    void SetCollisionObject(NiCollisionObject* pkCollisionObject);

    void Cull(NiCullingProcess& kCuller);
    void AttachParent(NiNode* pkParent);
    // Clears the parent pointer.
    void DetachParent();
    void Update(NiUpdateData& UpdateData);
    void UpdateSelected(NiUpdateData& UpdateData);
    // Marks the object as always drawn: gives it a bound and, when set, a unit radius.
    void SetAlwaysDraw(bool abVal);

    void UpdateObjectControllers(NiUpdateData& UpdateData, bool bProperties);
    void UpdateWorldLoc();
    void CreateBoundIfNone();
    void CopyMembers(NiAVObject* pkDest, NiCloningProcess& kCloning);
    void ProcessPropertyClones(NiTPointerList<NiPointer<NiProperty>>* pkList, NiCloningProcess& kCloning);
    void PushLocalProperties(NiPropertyState* pkParentState, bool bCopyOnChange, NiPropertyState*& aspPropertyState);

    void SetSelectiveUpdateTransformsOverride(bool bOverride) {
        SetBit(bOverride, NiAVObject::SELECTIVE_UPDATE_TRANSFORMS_OVERRIDE);
    }

    const NiTransform& GetWorldTransform() const {
        return m_kWorld;
    }
    void SetIgnoreFade(bool abIgnore) {
        SetBit(abIgnore, NiAVObject::IGNORE_FADE);
    }
    void SetHasMovingSound(bool abSound) {
        SetBit(abSound, NiAVObject::HAS_MOVING_SOUND);
    }
    void SetRelevantImposter(bool abRelevant) {
        SetBit(abRelevant, NiAVObject::IMPOSTER_LOADED);
    }
    uint32_t QHasMovingSound();
    uint32_t GetIgnoreFade();
    uint32_t QFadedIn();
    uint32_t GetAlwaysDraw();
    void SetWorldScale(float fScale);
    void GetRotate(NiQuaternion& kQuat) const;
    uint32_t GetNoDecals();
    uint32_t GetDisplayObject();
    void SetRotate_0(const NiQuaternion& kQuat);
    void SetNotVisible(bool abTF);

    static void
    CloneProperties(NiAVObject* pkDest, NiTPointerList<NiPointer<NiProperty>>* pkList, NiCloningProcess& kCloning);

    NiNode* m_pkParent;
    NiPointer<bhkNiCollisionObject> m_spCollisionObject;
    // Allocated and installed by SetBound, which takes a BSNiBound.
    BSNiBound* m_kWorldBound;
    NiTPointerList<NiPointer<NiProperty>> m_kPropertyList;
    uint32_t m_uFlags;
    NiTransform m_kLocal;
    NiTransform m_kWorld;

    static inline NiBound NullBoundS{};
    static NiRTTI ms_RTTI;
    static BSSpinLock kPropertyStateLock;
};
