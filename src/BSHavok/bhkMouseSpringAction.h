#pragma once

#include "BSHavok/bhkUnaryAction.h"
#include "Fallout Shared/GlobalFunc.h"

class bhkWorldObject;
class hkpEntity;
class hkpMouseSpringAction;

// The mouse spring's creation info: the base's user data and entity, the attachment point in the body's space, the
// target in world space and the spring's damping and force limits.
struct hkMouseSpringActionCinfo : public hkUnaryActionCinfo {
    hkMouseSpringActionCinfo();

    bhkVector4 m_positionInRbLocal;
    bhkVector4 m_mousePositionInWorld;
    float m_springDamping;
    float m_springElasticity;
    float m_maxRelativeForce;
    float m_objectDamping;
};

// A Havok action that pulls a body toward the mouse position with a spring (the debug picking of bodies); adds no
// data to bhkUnaryAction.
class bhkMouseSpringAction : public bhkUnaryAction {
public:
    bhkMouseSpringAction();
    bhkMouseSpringAction(const hkMouseSpringActionCinfo& arInfo);
    ~bhkMouseSpringAction() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    void KillCinfo(bool abCreated) override;
    uint32_t QCinfoSize() const override;
    void Init(const hkSerializableCinfo& arInfo) override;
    hkSerializableCinfo& CreateCinfo(bool& arCreated) override;

    // The entity the spring pulls.
    hkpEntity* GetEntity();
    // Copies the spring's parameters into the creation info.
    void CopyCinfo(hkMouseSpringActionCinfo& arInfo);
    bhkWorldObject* GetWorldObject();
    // The Havok action (null-safe).
    hkpMouseSpringAction* operator*() const;
    // Copies the spring's target in world space to arPos.
    void GetMousePositionInWorld(hkVector4& arPos);
    // Game-unit overload: writes the spring's target in world space, in game units, to arPos.
    void GetMousePositionInWorld(NiPoint3& arPos);
    // Copies the spring's attachment point in the body's space to arPos.
    void GetMousePositionInRBLocal(hkVector4& arPos);
    // Moves the spring's target to the game-unit point arPos, in Havok units.
    void SetMousePosition(const NiPoint3& arPos);
    // Moves the spring's target to arPos, when there is a wrapped action.
    void SetMousePosition(const hkVector4& arPos);

    // The engine's object factory for the class.
    static NiObject* CreateObject();

    static NiRTTI ms_RTTI;
    static inline uint32_t iCount;
};
