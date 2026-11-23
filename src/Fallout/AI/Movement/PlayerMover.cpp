#include "Fallout/AI/Movement/PlayerMover.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCharacterListener.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/AiFormulas.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESLand.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

#include <cmath>

PlayerMover::PlayerMover(Actor* apActor)
  : ActorMover(apActor), SideVec(NiPoint3::ZERO), MoveMode(0), LastTurnDir(0), fMinTurnTime(-1.0f) {}

PlayerMover::~PlayerMover() {}

void PlayerMover::ForceMoveMode(uint32_t aiMoveMode) {
    uint8_t ucChanged = (MoveMode & 1024) != (aiMoveMode & 1024);
    if ((aiMoveMode & 48) != 0) {
        fMinTurnTime = fPCTurnAnimMinTime.Float();
        LastTurnDir = aiMoveMode & 48;
    }
    MoveMode = aiMoveMode;
    if (ucChanged) {
        Animation* pAnimation = pPlayer->GetAnimation(false);
        uint16_t usGroup = pAnimation->GetGroup(AGS_WEAPON);
        if (TESAnimGroup::IsAimAction(static_cast<ANIM_GROUP_ENUM>(usGroup))) {
            int32_t iAimGroup = pPlayer->Actor::GetIronSights() ? 20 : 17;
            pAnimation = pPlayer->GetAnimation(false);
            BSAnimGroupSequence* pSequence = pAnimation->GetCurrentSequence(AGS_WEAPON);
            uint16_t usAnimGroup = pPlayer->Actor::GetAnimGroup(
                static_cast<ANIM_GROUP_ENUM>(iAimGroup),
                nullptr,
                false,
                pAnimation
            );
            if (!pSequence || usAnimGroup != pSequence->GetAnimGroup()->sType) {
                pAnimation->BlendOut(AGS_WEAPON, false);
            }
            pAnimation = pPlayer->GetAnimation(true);
            pSequence = pAnimation->GetCurrentSequence(AGS_WEAPON);
            usAnimGroup = pPlayer->Actor::GetAnimGroup(
                static_cast<ANIM_GROUP_ENUM>(iAimGroup),
                nullptr,
                false,
                pAnimation
            );
            if (!pSequence || usAnimGroup != pSequence->GetAnimGroup()->sType) {
                pAnimation->BlendOut(AGS_WEAPON, false);
            }
        }
    }
}

// The mover's data, then the side vector, move mode, last turn direction and minimum turn time.
void PlayerMover::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    ActorMover::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&SideVec), sizeof(SideVec), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&MoveMode), sizeof(MoveMode), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&LastTurnDir), sizeof(LastTurnDir), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fMinTurnTime), sizeof(fMinTurnTime), 0);
}

void PlayerMover::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    ActorMover::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&SideVec), sizeof(SideVec));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&MoveMode), sizeof(MoveMode));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&LastTurnDir), sizeof(LastTurnDir));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fMinTurnTime), sizeof(fMinTurnTime));
}

void PlayerMover::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    ActorMover::InitLoadGame(apLoadGameBuffer);
}

// Counts the minimum turn time down; while it runs and the player is not sitting or sleeping, the last turn
// direction stays in the move mode.
void PlayerMover::UpdateTurnTimer(float fdeltaT) {
    if (fMinTurnTime > 0.0) {
        fMinTurnTime = fMinTurnTime - fdeltaT;
        if (pActor->GetSitSleepState() == SIT_SLEEP_NORMAL) {
            MoveMode |= LastTurnDir;
        }
    }
}

void PlayerMover::Update(float fdeltaT) {
    NiPoint3 movedelta;
    movedelta = NiPoint3::ZERO;
    Animation* panim = pPlayer->GetAnimation(false);
    if (!pPlayer->Is3rdPerson()) {
        Animation* panim1st = pPlayer->GetAnimation(true);
        if (!panim) {
            panim = panim1st;
        }
        else {
            int32_t iGroup = panim1st->GetGroup(AGS_MOVEMENT);
            if (iGroup == panim->GetGroup(AGS_MOVEMENT)) {
                panim = panim1st;
            }
        }
    }
    if (!pPlayer->GetKnockedDownMovementDelta(movedelta)) {
        if (panim) {
            if (pPlayer->GetNoCollision()) {
                panim->GetDelta(&movedelta, pPlayer, true, false);
            }
            else {
                panim->GetDelta(&movedelta, pPlayer, false, true);
            }
        }
        bhkCharacterController* pController = pPlayer->GetCharController();
        bhkCharacterListener* pListener = static_cast<bhkCharacterListener*>(pController);
        if (pListener && pListener->StuckToSomeone()) {
            movedelta = NiPoint3::ZERO;
        }

        if ((this->MoveMode & 15) != 0 && !pPlayer->IsDying() && !pPlayer->GetNoCollision()) {
            pPlayer->GetAnimAction();
            float fMotionMagnitude = SideVec.Length();
            float fAnimMagnitude = movedelta.Length();
            if (fMotionMagnitude > 0.0 && fAnimMagnitude > 0.0 && (this->MoveMode & 64) == 0) {
                NiPoint2 fScaledPos;
                fScaledPos.x = SideVec.x / fMotionMagnitude;
                fScaledPos.y = SideVec.y / fMotionMagnitude;
                movedelta.x = fScaledPos.x * fAnimMagnitude;
                movedelta.y = fScaledPos.y * fAnimMagnitude;
            }
        }
        if ((this->MoveMode & 3) != 0 && (this->MoveMode & 12) != 0 && !pPlayer->IsDying()) {
            ANIMATION_ACTION eAction = pPlayer->pCurrentProcess->GetAnimAction();
            if ((eAction < ANIM_ACTION_WAIT_FOR_SPECIAL_IDLE || eAction > ANIM_ACTION_FORCE_SCRIPT_ANIM) &&
                movedelta.y != 0.0) {
                double fXVector = SideVec.x * fdeltaT;
                float fdeltax = pPlayer->GetScale() * fXVector;
                float fangle = fdeltax / movedelta.y * 0.7853981852531433;
                if ((this->MoveMode & 4) != 0) {
                    float fComplement = 1.5707963705062866 - fangle;
                    double dCos = std::cos(fComplement);
                    float fSideOffset = dCos * movedelta.y;
                    double dAbsOffset = std::fabs(fSideOffset);
                    movedelta.x = movedelta.x - dAbsOffset;
                }
                else if ((this->MoveMode & 8) != 0) {
                    float fComplement = 1.5707963705062866 - fangle;
                    double dCos = std::cos(fComplement);
                    float fSideOffset = dCos * movedelta.y;
                    double dAbsOffset = std::fabs(fSideOffset);
                    movedelta.x = dAbsOffset + movedelta.x;
                }
                double dCosAngle = std::cos(fangle);
                movedelta.y = dCosAngle * movedelta.y;
            }
        }
    }
    if (pPlayer->GetNoCollision()) {
        NiPoint3 pos = *pPlayer->QLocationOnReference() + movedelta;
        pPlayer->SetPosition(pos);
    }
    else {
        uint32_t uiMoveMode = pPlayer->GetMoveMode();
        pPlayer->Move(fdeltaT, movedelta, uiMoveMode);
        TESObjectCELL* pCell = pPlayer->GetParentCell();
        if (pCell && !pCell->GetInterior()) {
            float fHeight = 0.0f;
            NiPoint3 refpos = pPlayer->GetLocationOnReference();
            TESObjectLAND* pLand = pCell->GetLand();
            if (pLand->GetLandHeight(refpos, fHeight) && fHeight - refpos.z > 32.0) {
                refpos.z = fHeight + 32.0;
                pPlayer->SetPosition(refpos);
            }
        }
    }
}

void PlayerMover::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    ActorMover::Revert(apLoadGameBuffer);
    SideVec = NiPoint3::ZERO;
    MoveMode = 0;
    fMinTurnTime = -1.0f;
    LastTurnDir = 0;
}

void PlayerMover::SetMoveMode(uint32_t auiMoveMode, bool abValue) {
    if (abValue) {
        MoveMode |= auiMoveMode;
    }
    else {
        MoveMode &= ~auiMoveMode;
    }
}

void PlayerMover::ClearMoveMode(uint32_t auiMoveMode) {
    MoveMode &= ~auiMoveMode;
}

void PlayerMover::SetSideVector(const NiPoint3& aSideVec) {
    SideVec = aSideVec;
}
