#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "Fallout/Magic/MagicProjectile.h"

class EffectSetting;
class MagicCaster;
class MagicItem;
class MagicLight;

class MagicBallProjectile : public MagicProjectile {
public:
    // The ball's stage.
    enum State : int32_t {
        BALL_STATE_CREATION = 0,
        BALL_STATE_FLYING = 1,
        BALL_STATE_EXPLODING = 2,
    };

    MagicBallProjectile();
    MagicBallProjectile(
        MagicCaster* apCaster,
        MagicItem* apSpell,
        EffectItem* apDominantEffect,
        EffectSetting* apAVEffect,
        NiPoint3 aOrigin,
        NiPoint3 aRotation,
        NiAVObject* apData3D,
        MagicLight* apLight
    );
    ~MagicBallProjectile() override;

    uint16_t GetSaveSize(uint32_t aiFlags) override;
    void SaveGame(uint32_t aiFlags) override;
    void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) override;
    void InitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) override;
    void FinishInitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) override;
    void InitHavok() override;
    void Update(float afDelta) override;
    void Collide(NiPoint3 aContactPoint, bhkRigidBody* apTargetBody, TESObjectREFR* apCollidee, bool abFizzle) override;
    bool IsActive() override;
    void BuildAreaEffectGeometry() override;

    void Init3D();

    float fExplodingAge;
    State eState;
    float fScale;
    BSSoundHandle Sound;
    MagicLight* pLight;
};
