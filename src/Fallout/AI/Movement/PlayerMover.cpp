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
  : ActorMover(apActor), SideVec(NiPoint3_ZERO), MoveMode(0), LastTurnDir(0), fMinTurnTime(-1.0f) {}

PlayerMover::~PlayerMover() {}

void PlayerMover::ForceMoveMode(uint32_t aiMoveMode) {
    Animation* animation;
    uint16_t group;
    TESAnimGroup* v4;
    TESAnimGroup* v5;
    int32_t v7;
    uint16_t animGroup;
    uint16_t v9;
    Animation* v10;
    Animation* v11;
    BSAnimGroupSequence* currentSequence;
    BSAnimGroupSequence* v13;
    uint8_t v14 = (uint8_t)((MoveMode & 1024) != (aiMoveMode & 1024));
    if ((aiMoveMode & 48) != 0) {
        fMinTurnTime = (&fPCTurnAnimMinTime)->Float();
        LastTurnDir = aiMoveMode & 48;
    }
    MoveMode = aiMoveMode;
    if (v14) {
        animation = pPlayer->GetAnimation(false);
        group = animation->GetGroup(AGS_WEAPON);
        if (TESAnimGroup::IsAimAction(static_cast<ANIM_GROUP_ENUM>((int32_t)group))) {
            v7 = pPlayer->Actor::GetIronSights() ? 20 : 17;
            v10 = pPlayer->GetAnimation(false);
            currentSequence = v10->GetCurrentSequence(AGS_WEAPON);
            animGroup = pPlayer->Actor::GetAnimGroup(static_cast<ANIM_GROUP_ENUM>(v7), nullptr, false, v10);
            if ((!currentSequence) || ((v4 = currentSequence->GetAnimGroup()), (animGroup != v4->sType))) {
                v10->BlendOut(AGS_WEAPON, false);
            }
            v11 = pPlayer->GetAnimation(true);
            v13 = v11->GetCurrentSequence(AGS_WEAPON);
            v9 = pPlayer->Actor::GetAnimGroup(static_cast<ANIM_GROUP_ENUM>(v7), nullptr, false, v11);
            if ((!v13) || ((v5 = v13->GetAnimGroup()), (v9 != v5->sType))) {
                v11->BlendOut(AGS_WEAPON, false);
            }
        }
    }
}

// The mover's data, then the side vector, move mode, last turn direction and minimum turn time.
void PlayerMover::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    ActorMover::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&SideVec), 0xC, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&MoveMode), 4, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&LastTurnDir), 4, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fMinTurnTime), 4, 0);
}

void PlayerMover::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    ActorMover::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&SideVec), 0xC);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&MoveMode), 4);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&LastTurnDir), 4);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fMinTurnTime), 4);
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
    movedelta = NiPoint3_ZERO;
    Animation* panim = pPlayer->GetAnimation(false);
    if (!pPlayer->Is3rdPerson()) {
        int32_t Group;
        Animation* panim1st = pPlayer->GetAnimation(true);
        if (!panim || (Group = panim1st->GetGroup(AGS_MOVEMENT), Group == panim->GetGroup(AGS_MOVEMENT))) {
            panim = panim1st;
        }
    }
    if (!pPlayer->GetKnockedDownMovementDelta(movedelta)) {
        bhkCharacterListener* pListener;
        if (panim) {
            if (pPlayer->GetNoCollision()) {
                panim->GetDelta(&movedelta, pPlayer, true, false);
            }
            else {
                panim->GetDelta(&movedelta, pPlayer, false, true);
            }
        }
        bhkCharacterController* pController = pPlayer->GetCharController();
        if (pController) {
            pListener = static_cast<bhkCharacterListener*>(pController);
        }
        else {
            pListener = nullptr;
        }
        if (pListener) {
            if (pListener->StuckToSomeone()) {
                movedelta = NiPoint3_ZERO;
            }
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
            BaseProcess* CurrentAIProcess = pPlayer->pCurrentProcess;
            ANIMATION_ACTION eAction = CurrentAIProcess->GetAnimAction();
            if ((eAction < ANIM_ACTION_WAIT_FOR_SPECIAL_IDLE || eAction > ANIM_ACTION_FORCE_SCRIPT_ANIM) &&
                movedelta.y != 0.0) {
                double fXVector = SideVec.x * fdeltaT;
                float fdeltax = pPlayer->GetScale() * fXVector;
                float fangle = fdeltax / movedelta.y * 0.7853981852531433;
                if ((this->MoveMode & 4) != 0) {
                    float v13 = 1.5707963705062866 - fangle;
                    double v3 = std::cos(v13);
                    float v12 = v3 * movedelta.y;
                    double v4 = std::fabs(v12);
                    movedelta.x = movedelta.x - v4;
                }
                else {
                    if ((this->MoveMode & 8) != 0) {
                        float v11 = 1.5707963705062866 - fangle;
                        double v5 = std::cos(v11);
                        float v10 = v5 * movedelta.y;
                        double v6 = std::fabs(v10);
                        movedelta.x = v6 + movedelta.x;
                    }
                }
                double v7 = std::cos(fangle);
                movedelta.y = v7 * movedelta.y;
            }
        }
    }
    if (pPlayer->GetNoCollision()) {
        NiPoint3 pos = *pPlayer->QLocationOnReference() + movedelta;
        pPlayer->SetPosition(pos);
    }
    else {
        uint32_t MoveMode = pPlayer->GetMoveMode();
        pPlayer->Move(fdeltaT, movedelta, MoveMode);
        TESObjectCELL* pcell = pPlayer->GetParentCell();
        if (pcell) {
            if (!pcell->GetInterior()) {
                float fheight = 0.0f;
                NiPoint3 refpos = pPlayer->GetLocationOnReference();
                TESObjectLAND* Land = pcell->GetLand();
                if (Land->GetLandHeight(refpos, fheight) && fheight - refpos.z > 32.0) {
                    refpos.z = fheight + 32.0;
                    pPlayer->SetPosition(refpos);
                }
            }
        }
    }
}

void PlayerMover::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    ActorMover::Revert(apLoadGameBuffer);
    SideVec = NiPoint3_ZERO;
    MoveMode = 0;
    fMinTurnTime = -1.0f;
    LastTurnDir = 0;
}

void PlayerMover::SetMoveMode(uint32_t auiMoveMode, bool abValue) {
    if (abValue) {
        MoveMode |= auiMoveMode;
    }
    else {
        MoveMode &= (~auiMoveMode);
    }
}

void PlayerMover::ClearMoveMode(uint32_t auiMoveMode) {
    MoveMode &= (~auiMoveMode);
}

void PlayerMover::SetSideVector(const NiPoint3& aSideVec) {
    SideVec = aSideVec;
}
