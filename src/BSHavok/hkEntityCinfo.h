#pragma once

#include "BSHavok/bhkWorldObject.h"
#include "Havok/Source/Physics/Dynamics/Common/hkpMaterial.h"

// The creation info of an entity: a world object's, plus how contacts are answered.
class hkEntityCinfo : public hkWorldObjectCinfo {
public:
    // The serialized form (bhkSerialized.h): the 32-bit layout, 0x1C bytes.
    struct Serialized : hkWorldObjectCinfo::Serialized {
        int8_t m_collisionResponse;
        uint8_t padding19;
        uint16_t m_processContactCallbackDelay;
    };

    ~hkEntityCinfo();

    // Fills this part from its serialized form.
    void ReadSerialized(const Serialized& arForm) {
        hkWorldObjectCinfo::ReadSerialized(arForm);
        m_collisionResponse.m_storage = arForm.m_collisionResponse;
        m_processContactCallbackDelay = arForm.m_processContactCallbackDelay;
    }

    hkEnum<hkpMaterial::ResponseType, int8_t> m_collisionResponse;
    uint16_t m_processContactCallbackDelay;
};

static_assert(offsetof(hkEntityCinfo::Serialized, m_collisionResponse) == 0x18);
static_assert(offsetof(hkEntityCinfo::Serialized, m_processContactCallbackDelay) == 0x1A);
static_assert(sizeof(hkEntityCinfo::Serialized) == 0x1C);
