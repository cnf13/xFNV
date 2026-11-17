#include "BSAudio/BSSoundHandle.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSAudioManager.h"
#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

bool BSAudioManager_SetPosition(BSAudioManager* apManager, uint32_t aiID, float afX, float afY, float afZ) {
	NiPoint3 v7;
	v7.x = afX;
	v7.y = afY;
	v7.z = afZ;
	apManager->ComposeMessage(MSET_POS, aiID, 0, nullptr, NiPointer<NiAVObject>{}, v7);
	return 0;
}

BSSoundHandle::BSSoundHandle() {
	iSoundID = -1;
	bAssumeSuccess = false;
	eState = ASSUMED_STATE_INITIALIZED;
}

void BSSoundHandle::Clear() {
	iSoundID = -1;
	bAssumeSuccess = false;
	eState = ASSUMED_STATE_INITIALIZED;
}

// The exe copies the three fields one by one (dword, byte, dword), leaving the padding after bAssumeSuccess alone.
BSSoundHandle& BSSoundHandle::operator=(const BSSoundHandle& aRhs) {
	iSoundID = aRhs.iSoundID;
	bAssumeSuccess = aRhs.bAssumeSuccess;
	eState = aRhs.eState;
	return *this;
}

bool BSSoundHandle::operator==(const BSSoundHandle& aCmp) const {
	int32_t v2 = aCmp.iSoundID;
	return v2 == iSoundID;
}

bool BSSoundHandle::operator!=(const BSSoundHandle& aCmp) const {
	int32_t v2 = aCmp.iSoundID;
	return v2 != iSoundID;
}

bool BSSoundHandle::operator!=(uint32_t aiSoundID) const {
	return aiSoundID != GetID();
}

bool BSSoundHandle::Pause() {
	if (this->iSoundID == -1) {
		return 0;
	}
	eState = ASSUMED_STATE_PAUSED;
	uint32_t iSoundID = this->iSoundID;
	BSAudioManager* v2 = &BSAudioManager::QInstance();
	return v2->Pause(iSoundID);
}

bool BSSoundHandle::SetBeamEnd(float afX, float afY, float afZ) {
	int32_t iSoundID;
	if (this->iSoundID == ((-1))) {
		return (bool)0;
	}
	iSoundID = this->iSoundID;
	BSAudioManager* singleton = &BSAudioManager::QInstance();
	return (bool)((uint8_t)singleton->SetBeamEnd(iSoundID, afX, afY, afZ));
}

bool BSSoundHandle::SetVolume(float aiNewVolume) {
	uint32_t iSoundID;
	if (this->iSoundID == ((-1))) {
		return (bool)0;
	}
	iSoundID = (uint32_t)this->iSoundID;
	BSAudioManager* singleton = &BSAudioManager::QInstance();
	return (bool)((uint8_t)singleton->SetVolume(iSoundID, aiNewVolume));
}

bool BSSoundHandle::IsValid() {
	if (iSoundID == -1) {
		return false;
	}
	return BSAudioManager::QInstance().IsValid(iSoundID);
}

bool BSSoundHandle::Play(bool abLoopPlayback) {
	if (iSoundID == -1) {
		return false;
	}
	eState = ASSUMED_STATE_PLAYING;
	return BSAudioManager::QInstance().Play(iSoundID, abLoopPlayback);
}

bool BSSoundHandle::Stop() {
	if (iSoundID == -1) {
		return false;
	}
	eState = ASSUMED_STATE_STOPPED;
	return BSAudioManager::QInstance().Stop(iSoundID);
}

bool BSSoundHandle::IsPlaying() {
	if (iSoundID == -1) {
		return false;
	}
	if (bAssumeSuccess) {
		return eState == ASSUMED_STATE_PLAYING;
	}
	return BSAudioManager::QInstance().IsPlaying(iSoundID);
}

bool BSSoundHandle::SetPosition(float afX, float afY, float afZ) {
	if (iSoundID == -1) {
		return false;
	}
	return BSAudioManager_SetPosition(&BSAudioManager::QInstance(), iSoundID, afX, afY, afZ);
}

bool BSSoundHandle::Release() {
	if (iSoundID == -1) {
		return false;
	}
	bool bReleased = BSAudioManager::QInstance().Release(iSoundID);
	if (bReleased) {
		iSoundID = -1;
	}
	return bReleased;
}

bool BSSoundHandle::SetPosition(const NiPoint3& apPosition) {
	return SetPosition(apPosition.x, apPosition.y, apPosition.z);
}

void BSSoundHandle::SetObjectToFollow(NiAVObject* apObj) {
	if (this->iSoundID != -1) {
		if (apObj) {
			if (BSSoundHandle::pfnAddMovingSoundCallback) {
				BSSoundHandle::pfnAddMovingSoundCallback(apObj);
			}
			uint32_t iSoundID = this->iSoundID;
			BSAudioManager* v2 = &BSAudioManager::QInstance();
			v2->SetObjectToFollow(iSoundID, apObj);
		}
	}
}

void BSSoundHandle::SetCompletionCallback(void (*apfnCallback)(void*, bool), void* apContext) {
	if (!BSAudio::pAudioInstance->bAudioEnabled) {
		if (apfnCallback) {
			apfnCallback(apContext, 0);
		}
	}
	if (this->iSoundID != -1) {
		uint32_t iSoundID = this->iSoundID;
		BSAudioManager* v3 = &BSAudioManager::QInstance();
		v3->SetCompletionCallback(iSoundID, apfnCallback, apContext);
	}
}

void BSSoundHandle::SetAssumeSuccess(bool abAssumeSuccess) {
	bAssumeSuccess = abAssumeSuccess;
}

bool BSSoundHandle::FadeOutAndRelease(uint32_t aiMilliseconds) {
	if (iSoundID == -1) {
		return false;
	}
	BSAudioManager::QInstance().FadeOut(iSoundID, aiMilliseconds, MDELETE_SOUND);
	iSoundID = -1;
	return true;
}

float BSSoundHandle::GetVolume() {
	if (iSoundID == ((-1))) {
		return 0.0;
	}
	int32_t uiSoundID = iSoundID;
	BSAudioManager* singleton = &BSAudioManager::QInstance();
	return singleton->GetVolume(uiSoundID);
}

int32_t BSSoundHandle::GetDuration() {
	if (iSoundID == ((-1))) {
		return 0;
	}
	int32_t uiSoundID = iSoundID;
	BSAudioManager* singleton = &BSAudioManager::QInstance();
	return (int32_t)singleton->GetDuration(uiSoundID);
}

bool BSSoundHandle::SetStaticAttenuation(uint16_t aiStaticAttenutation) {
	uint32_t iSoundID;
	if (this->iSoundID == ((-1))) {
		return (bool)0;
	}
	iSoundID = (uint32_t)this->iSoundID;
	BSAudioManager* singleton = &BSAudioManager::QInstance();
	return (bool)((uint8_t)singleton->SetStaticAttenuation(iSoundID, aiStaticAttenutation));
}

void BSSoundHandle::SetAttenuationCurve(uint16_t aiPt1, uint16_t aiPt2, uint16_t aiPt3, uint16_t aiPt4,
	uint16_t aiPt5) {
	BSAudioManager* singleton;
	uint32_t iSoundID;
	if (this->iSoundID != ((-1))) {
		iSoundID = (uint32_t)this->iSoundID;
		singleton = &BSAudioManager::QInstance();
		singleton->SetAttenuationCurve(iSoundID, aiPt1, aiPt2, aiPt3, aiPt4, aiPt5);
	}
}

bool BSSoundHandle::FadeInPlay(uint32_t aiMilliseconds) {
	uint32_t iSoundID;
	if (this->iSoundID == ((-1))) {
		return (bool)0;
	}
	eState = ASSUMED_STATE_PLAYING;
	iSoundID = (uint32_t)this->iSoundID;
	BSAudioManager* singleton = &BSAudioManager::QInstance();
	singleton->FadeInPlay(iSoundID, aiMilliseconds);
	return (bool)1;
}

void BSSoundHandle::SetPriority(uint32_t aiPriority) {
	BSAudioManager* singleton;
	uint32_t iSoundID;
	if (this->iSoundID != ((-1))) {
		iSoundID = (uint32_t)this->iSoundID;
		singleton = &BSAudioManager::QInstance();
		singleton->SetPriority(iSoundID, aiPriority);
	}
}

bool BSSoundHandle::FadeOut(uint32_t aiMilliseconds) {
	if (iSoundID == ((-1))) {
		return (bool)0;
	}
	eState = ASSUMED_STATE_STOPPED;
	int32_t uiSoundID = iSoundID;
	BSAudioManager* singleton = &BSAudioManager::QInstance();
	singleton->FadeOut(uiSoundID, aiMilliseconds, MINVALID_MSG);
	return (bool)1;
}

int32_t BSSoundHandle::GetSoundType() {
	if (iSoundID == ((-1))) {
		return 0;
	}
	int32_t v3 = iSoundID;
	BSAudioManager* singleton = &BSAudioManager::QInstance();
	return singleton->GetSoundType(v3);
}

bool BSSoundHandle::SetOrientation(float afX, float afY, float afZ) {
	if (((uint32_t)iSoundID) == ((-1))) {
		return (bool)0;
	}
	uint32_t v6 = (uint32_t)iSoundID;
	BSAudioManager* singleton = &BSAudioManager::QInstance();
	return (bool)((uint8_t)singleton->SetOrientation(v6, afX, afY, afZ));
}

void BSSoundHandle::ClearSynchStartTime() {
	BSAudioManager* singleton;
	uint32_t iSoundID;
	if (this->iSoundID != ((-1))) {
		iSoundID = (uint32_t)this->iSoundID;
		singleton = &BSAudioManager::QInstance();
		singleton->ClearSynchStartTime(iSoundID);
	}
}

void BSSoundHandle::SetMinMax(float afMin, float afMax) {
	BSAudioManager* singleton;
	uint32_t iSoundID;
	if (this->iSoundID != ((-1))) {
		iSoundID = (uint32_t)this->iSoundID;
		singleton = &BSAudioManager::QInstance();
		singleton->SetMinMax(iSoundID, afMin, afMax);
	}
}

void BSSoundHandle::SetTimeConstraints(uint8_t aiStart, uint8_t aiStop) {
	BSAudioManager* singleton;
	uint32_t iSoundID;
	if (this->iSoundID != ((-1))) {
		iSoundID = (uint32_t)this->iSoundID;
		singleton = &BSAudioManager::QInstance();
		singleton->SetTimeConstraints(iSoundID, aiStart, aiStop);
	}
}

void BSSoundHandle::SetFollowsListener(bool abFollowsListener) {
	BSAudioManager* singleton;
	uint32_t iSoundID;
	if (this->iSoundID != ((-1))) {
		iSoundID = (uint32_t)this->iSoundID;
		singleton = &BSAudioManager::QInstance();
		singleton->SetAtListener(iSoundID, abFollowsListener);
	}
}

bool BSSoundHandle::SetFrequency(float afNewFrequency) {
	uint32_t iSoundID;
	if (this->iSoundID == ((-1))) {
		return (bool)0;
	}
	iSoundID = (uint32_t)this->iSoundID;
	BSAudioManager* singleton = &BSAudioManager::QInstance();
	return (bool)((uint8_t)singleton->SetFrequency(iSoundID, afNewFrequency));
}

bool BSSoundHandle::PlayAfter(uint32_t aiDelay, int32_t aiPlayFlags) {
	if (this->iSoundID == -1) {
		return 0;
	}
	eState = ASSUMED_STATE_PLAYING;
	uint32_t iSoundID = this->iSoundID;
	BSAudioManager* v4 = &BSAudioManager::QInstance();
	return v4->PlayAfter(iSoundID, aiDelay, aiPlayFlags);
}

bool BSSoundHandle::SetModFrequency(float afNewFrequency) {
	if (this->iSoundID == -1) {
		return 0;
	}
	uint32_t iSoundID = this->iSoundID;
	BSAudioManager* v3 = &BSAudioManager::QInstance();
	return v3->SetModFrequency(iSoundID, afNewFrequency);
}

bool BSSoundHandle::SetFrequencyVariance(uint8_t aiFreqVariance) {
	if (this->iSoundID == -1) {
		return 0;
	}
	uint32_t iSoundID = this->iSoundID;
	BSAudioManager* v3 = &BSAudioManager::QInstance();
	return v3->SetFrequencyVariance(iSoundID, aiFreqVariance);
}

bool BSSoundHandle::SetBalance(int32_t aiNewBalance) {
	if (this->iSoundID == -1) {
		return 0;
	}
	uint32_t iSoundID = this->iSoundID;
	BSAudioManager* v3 = &BSAudioManager::QInstance();
	return v3->SetBalance(iSoundID, aiNewBalance);
}

void BSSoundHandle::SetReverbAttenuation(uint16_t aiReverbAtt) {
	if (this->iSoundID != -1) {
		uint32_t iSoundID = this->iSoundID;
		BSAudioManager* v2 = &BSAudioManager::QInstance();
		v2->SetReverbAttenuation(iSoundID, aiReverbAtt);
	}
}

void BSSoundHandle::Seek(uint32_t aiMilliseconds) {
	if (this->iSoundID != -1) {
		uint32_t iSoundID = this->iSoundID;
		BSAudioManager* v2 = &BSAudioManager::QInstance();
		v2->Seek(iSoundID, aiMilliseconds);
	}
}

void BSSoundHandle::SetSynchStartTime(uint32_t aiStartTime) {
	if (this->iSoundID != -1) {
		uint32_t iSoundID = this->iSoundID;
		BSAudioManager* v2 = &BSAudioManager::QInstance();
		v2->SetSynchStartTime(iSoundID, aiStartTime);
	}
}

void BSSoundHandle::SetPlayCallback(void (*apfnCallback)(void*, int32_t), void* apContext) {
	if (!BSAudio::pAudioInstance->bAudioEnabled) {
		if (apfnCallback) {
			apfnCallback(apContext, 0x1388);
		}
	}
	if (this->iSoundID != -1) {
		uint32_t iSoundID = this->iSoundID;
		BSAudioManager* v3 = &BSAudioManager::QInstance();
		v3->SetPlayCallback(iSoundID, apfnCallback, apContext);
	}
}

void BSSoundHandle::SetAddMovingSoundCallback(void (*apfnCallback)(NiAVObject*)) {
	pfnAddMovingSoundCallback = apfnCallback;
}

// The exe's deleting destructor (0x7B3FA0) frees through ourFree.
void BSSoundHandle::operator delete(void* apMemory) {
	ourFree(apMemory);
}
