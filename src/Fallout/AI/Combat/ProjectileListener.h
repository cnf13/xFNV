#pragma once

#include "BSHavok/bhkCharacterController.h"

class Projectile;

// The character controller a projectile moves with: its listener callbacks report what the projectile touches to it.
// Projectile::InitHavok allocates it 16-aligned.
class ProjectileListener : public bhkCharacterController {
public:
    ProjectileListener(bhkCharacterControllerCinfo* a1, Projectile* a2);
    ~ProjectileListener() override;

    void contactPointAddedCallback(const hkpCharacterProxy* apProxy, const hkpRootCdPoint& point) override;
    void contactPointRemovedCallback(const hkpCharacterProxy* apProxy, const hkpRootCdPoint& point) override {}

    void characterInteractionCallback(
        hkpCharacterProxy* apProxy,
        hkpCharacterProxy* apOtherProxy,
        const hkContactPoint& arContact
    ) override {}

    void objectInteractionCallback(
        hkpCharacterProxy* apProxy,
        const hkpCharacterObjectInteractionEvent& arInput,
        hkpCharacterObjectInteractionResult& arOutput
    ) override {}

    Projectile* pProj;
};
