#pragma once

#include "BSHavok/bhkConvert.h"
#include "BSHavok/bhkUnaryAction.h"
#include "Havok/Source/Physics/Utilities/Actions/MouseSpring/hkpMouseSpringAction.h"

class bhkWorldObject;
class hkpEntity;

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
    // The wrapped action's spring damping, elasticity and object damping; 0 when this or the action is null.
    float GetDamping() {
        return (this && phkObject) ? static_cast<hkpMouseSpringAction*>(phkObject)->m_springDamping : 0.0f;
    }
    float GetElasticity() {
        return (this && phkObject) ? static_cast<hkpMouseSpringAction*>(phkObject)->m_springElasticity : 0.0f;
    }
    float GetObjectDamping() {
        return (this && phkObject) ? static_cast<hkpMouseSpringAction*>(phkObject)->m_objectDamping : 0.0f;
    }
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

inline hkpMouseSpringAction* bhkMouseSpringAction::operator*() const {
    if (this) {
        return static_cast<hkpMouseSpringAction*>(GethkObject());
    }

    return nullptr;
}

// bhkVector4's constructor zeroes the vectors.
inline hkMouseSpringActionCinfo::hkMouseSpringActionCinfo()
  : hkUnaryActionCinfo(), m_springDamping(0.5f), m_springElasticity(0.3f), m_maxRelativeForce(250.0f),
    m_objectDamping(0.95f) {}

// Converts the game-unit point to Havok units in a local (NI2HK, which writes all four lanes) and moves the spring's
// target there through the hkVector4 overload.
inline void bhkMouseSpringAction::SetMousePosition(const NiPoint3& arPos) {
    hkVector4 hkp;
    SetMousePosition(NI2HK(hkp, arPos));
}

// Sets the wrapped action's target to arPos when there is a wrapped action.
inline void bhkMouseSpringAction::SetMousePosition(const hkVector4& arPos) {
    hkpMouseSpringAction* pm = **this;
    if (pm) {
        pm->setMousePosition(arPos);
    }
}

inline void bhkMouseSpringAction::GetMousePositionInWorld(hkVector4& arPos) {
    hkpMouseSpringAction* pAction = **this;
    if (pAction) {
        arPos = pAction->m_mousePositionInWorld;
    }
}

inline void bhkMouseSpringAction::GetMousePositionInRBLocal(hkVector4& arPos) {
    hkpMouseSpringAction* pAction = **this;
    if (pAction) {
        arPos = pAction->m_positionInRbLocal;
    }
}

// The game-unit overload.
inline void bhkMouseSpringAction::GetMousePositionInWorld(NiPoint3& arPos) {
    hkpMouseSpringAction* pm = **this;
    if (pm) {
        HK2NI(arPos, pm->m_mousePositionInWorld);
    }
}
