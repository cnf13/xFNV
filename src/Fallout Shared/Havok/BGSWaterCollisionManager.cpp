#include "Fallout Shared/Havok/BGSWaterCollisionManager.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSGameSound.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESWaterForm.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout Shared/TempEffects/BSTempEffectParticle.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

// The heaviest splash sound afSplashingBodySize exceeds plays at the splash, with its particle scale; the particles
// spawn, at a random heading, only for a positive scale in the player's cell when its water is not dangerous. The
// ripple is queued as a task where 3D tasks are queued, else added at once. The sound handles' destructor is empty, so
// the temporaries and the handle need no destruct.
void BGSWaterCollisionManager::PlaySplashEffects(float afSplashingBodySize, NiPoint3 arSplashLocation) {
    float fScale = 1.0f;
    BSSoundHandle kSound;
    if (fSplashSoundHeavy.Float() < static_cast<double>(afSplashingBodySize)) {
        FNVD_INLINED_LOCAL(BSSoundHandle, kLarge, std::nullopt, std::nullopt);
        kSound = *BSAudio::QInstance().GetSoundHandleByName(
            &kLarge, "CWaterLarge", BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT
        );
        fScale = fSplashScale1.Float();
    }
    if (!kSound.IsValid() && fSplashSoundMedium.Float() < static_cast<double>(afSplashingBodySize)) {
        FNVD_INLINED_LOCAL(BSSoundHandle, kMedium, std::nullopt, std::nullopt);
        kSound = *BSAudio::QInstance().GetSoundHandleByName(
            &kMedium, "CWaterMedium", BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT
        );
        fScale = fSplashScale2.Float();
    }
    if (!kSound.IsValid() && fSplashSoundLight.Float() < static_cast<double>(afSplashingBodySize)) {
        FNVD_INLINED_LOCAL(BSSoundHandle, kSmall, std::nullopt, std::nullopt);
        kSound = *BSAudio::QInstance().GetSoundHandleByName(
            &kSmall, "CWaterSmall", BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT
        );
        fScale = fSplashScale3.Float();
    }
    if (kSound.IsValid()) {
        kSound.SetPosition(arSplashLocation.x, arSplashLocation.y, arSplashLocation.z);
        kSound.Play(0);
        if (fScale > 0.0 && pPlayer->GetParentCell() && pPlayer->GetParentCell()->GetWaterType() &&
            !pPlayer->GetParentCell()->GetWaterType()->IsDangerous()) {
            NiMatrix3 kRotation;
            kRotation.MakeZRotation(RandomFloatTwoPi());
            const char* pFilename = sSplashParticles.String();
            BSTempEffectParticle::Spawn(
                pPlayer->GetParentCell(), 1.0f, pFilename, kRotation, arSplashLocation, fScale, 1, nullptr
            );
        }
    }
    if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
        TaskQueueInterface::GetInstance()->QueueAddRipple(0.01f, arSplashLocation);
    }
    else {
        TES::pInstance->GetWaterSystem()->AddRipple(arSplashLocation, 0.01f);
    }
}
