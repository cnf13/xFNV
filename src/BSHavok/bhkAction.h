#pragma once

#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkSerialized.h"

// The creation info every action's starts with: the user data its Havok action carries.
struct hkActionCinfo : public hkSerializableCinfo {
    // The serialized form (bhkSerialized.h): the 32-bit layout, 4 bytes. The file's user data is the address of the
    // writer's bhkAction (bhkAction::Link stores the wrapper there), meaningless here.
    struct Serialized {
        uint32_t m_userData;
    };

    // Fills this part from its serialized form: the user data is cleared, as the file's is an address. Init passes 0
    // to the Havok action and bhkAction::Link stores the wrapper.
    void ReadSerialized(const Serialized&) {
        m_userData = 0;
    }

    // The SDK's hkUlong: a copy of the Havok action's user data, which holds the bhkAction.
    hkUlong m_userData;
};

static_assert(sizeof(hkActionCinfo::Serialized) == 0x4);

// A Havok action (a force applied to bodies each step) wrapped for the game; it adds no data.
class bhkAction : public bhkSerializable {
public:
    bhkAction();
    ~bhkAction() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    void Link(bool abAttach) override;
    const hkpWorld* GethkWorld() const override;
    hkpWorld* GethkWorldRw() override;
    bool Add(bhkWorld* apWorld) override;
    bool Remove() override;

    // Called each step with the step's time; the base implementation does nothing.
    virtual void Update(float afTime) {}

    // Copies the Havok action's user data into arInfo, or 0 without an action.
    void CopyCinfo(hkActionCinfo& arInfo);

    static NiRTTI ms_RTTI;
    // The live object count, kept per class level by CreateObject and CreateClone.
    static inline uint32_t iCount;
};
