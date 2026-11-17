#pragma once

#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpAllCdPointCollector.h"

class bhkSimpleShapePhantom;
class bhkWorld;
class HitCaster;
class NiPoint3;

// Casts a shape phantom through the Havok world and collects every contact point (a small and a large instance).
class HitCaster : public hkpAllCdPointCollector {
public:
    enum HitCasterSize : int32_t {
        SIZE_SMALL = 0x0,
        SIZE_LARGE = 0x1,
    };

    HitCaster(float afRadius, uint32_t aiGroup);
    ~HitCaster() override;

    uint32_t GetShapeGroup();
    // Points the custom pick layer at the layers a hit cast collides with.
    void SetCustomPickCollisions();
    // Casts the shape phantom from apOrigin towards apOrigin + apDirection * afPickDistance and sorts the contact
    // points it found; true when it hit anything.
    bool CastShape(const NiPoint3& aOrigin, const NiPoint3& aDirection, float afPickDistance);
    void SetShapeGroup(uint32_t aiGroup);
    // Adds the caster's phantom to apWorld.
    void Add(bhkWorld* apWorld);
    hkpAllCdPointCollector* GetPointCollector();
    void Init(float afRadius, uint32_t aiGroup);

    // The small or the large caster, created on first use when abCreate.
    static HitCaster* GetInstance(HitCasterSize eSize, bool abCreate);
    // Frees with ourMemalignFree.
    static void operator delete(void* apMemory);
    static void ClearInstance();

    NiPointer<bhkSimpleShapePhantom> spShapePhantom;
    bool bInit;
    uint8_t padding3A5;
    uint8_t padding3A6;
    uint8_t padding3A7;
    uint32_t iGroup;

    static inline HitCaster* pSmallHitCaster;
    static inline HitCaster* pLargeHitCaster;
};

extern SettingT<GameSettingCollection> fHitCasterSizeSmall;
extern SettingT<GameSettingCollection> fHitCasterSizeLarge;
