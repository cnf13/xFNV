#pragma once

#include "BSHavok/bhkCollisionFilter.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/CharacterProxy/hkpCharacterProxyListener.h"

enum HavokMaterialType : int32_t;

// Watches a character controller's contacts and keeps the character's behavior flags, the support surface it stands
// on and the collision count that decides whether it sticks to what it touches.
class bhkCharacterListener : public hkpCharacterProxyListener {
public:
    // Sets up the up direction, the support surface and the heights, and the cast depth that follows from the slope and
    // the step height.
    bhkCharacterListener(const hkVector4& arUp, float afMaxSlope, float afStepHeight);
    ~bhkCharacterListener() override;

    void contactPointAddedCallback(const hkpCharacterProxy* apProxy, const hkpRootCdPoint& arPoint) override;
    void contactPointRemovedCallback(const hkpCharacterProxy* apProxy, const hkpRootCdPoint& arPoint) override;
    void characterInteractionCallback(
        hkpCharacterProxy* apProxy,
        hkpCharacterProxy* apOtherProxy,
        const hkContactPoint& arContact
    ) override;

    // The flags of the given mask that are set.
    uint32_t GetFlag(uint32_t aiFlags);
    void SetFlag(uint32_t aiFlags, bool abTF);
    void ClearFlag(uint32_t aiFlags);

    bool QQuadruped();
    bool QStuckQuadruped();

    // Takes the flag by pointer; turning it off also forgets the collisions counted so far.
    void SetStickyCollisions(const bool& bSticky);
    void SetStuckQuadruped(bool abTF);
    bool StuckToSomeone();
    // Averages afHeight into support height aiIndex, or sets it while the slot still holds FLT_MIN (empty).
    void AddSupportHeight(uint32_t aiIndex, float afHeight);
    // Averages arPos's height into the support slot of the nearer of the first two points (0/1) and of the nearer of
    // the last two (2/3), measured on x and y.
    void AddSupportHeight2(hkVector4* apFrontBack, const hkVector4& arPos);

    static void SetMoveLimitMass(float afValue);
    // Frees the aligned block the object sits in: the byte in front of the object gives the block's start.
    static void operator delete(void* apMemory);

    uint32_t iFlags;
    // hkVector4 is 16-byte aligned, so the compiler leaves eight bytes of padding after the flags.
    hkVector4 hkUp;
    float fMaxSlope;
    HavokMaterialType eSupportMaterial;
    COL_LAYER eSupportLayer;
    // hkSupportVelocity is a 16-byte-aligned hkVector4, so the four bytes after eSupportLayer are compiler padding.
    hkVector4 hkSupportVelocity;
    hkVector4 hkSupportNorm;
    float SupportHeight[4];
    float fStepHeight;
    float fCastDepth;
    bool bAllSurfacesStopped;
    bool bStickyCollisions;
    uint8_t padding6A[2];
    uint32_t iNumCollisions;

    static inline float fMoveLimitMass = 95.0f;
};
