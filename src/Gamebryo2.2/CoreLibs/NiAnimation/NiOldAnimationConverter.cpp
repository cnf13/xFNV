#include "Gamebryo2.2/CoreLibs/NiAnimation/NiOldAnimationConverter.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLookAtController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLookAtInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPathController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPathInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPoint3Interpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiRollController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTransformController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTransformInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"

// The exe inlines the NiObjectNET kind-of test as a walk up the RTTI chain.
void NiOldAnimationConverter::Convert(NiStream& kStream, NiObject* pkObject) {
	if (kStream.m_uiNifFileVersion < 0x0A010068 && pkObject) {
		for (const NiRTTI* pkRTTI = pkObject->GetRTTI(); pkRTTI; pkRTTI = pkRTTI->m_pkBaseRTTI) {
			if (pkRTTI == &NiObjectNET::ms_RTTI) {
				RecurseScene(kStream, static_cast<NiObjectNET*>(pkObject));
				return;
			}
		}
	}
}

// The exe inlines each NiIsKindOf as a walk up the RTTI chain (GetRTTI called anew for each test), reads the
// controller list without reference counting, and loops over the child array's m_usSize (+0xA), not its capacity
// as the reference source's GetArrayCount does.
void NiOldAnimationConverter::RecurseScene(NiStream& kStream, NiObjectNET* pkObject) {
	if (!pkObject)
		return;
	// EXE-INLINED none: NiIsKindOf
	const NiRTTI* pkRTTI = pkObject->GetRTTI();
	for (;; pkRTTI = pkRTTI->m_pkBaseRTTI) {
		if (!pkRTTI)
			return;
		if (pkRTTI == &NiAVObject::ms_RTTI)
			break;
	}

	NiTimeController* pkController = pkObject->m_spControllers;
	while (pkController) {
		NiTimeController* pkNextController = pkController->m_spNext.m_pObject;
		bool bDone = false;
		// EXE-INLINED none: NiIsKindOf
		for (const NiRTTI* pk = pkController->GetRTTI(); pk; pk = pk->m_pkBaseRTTI) {
			if (pk == &NiRollController::ms_RTTI) {
				ConvertRollController(static_cast<NiRollController*>(pkController));
				bDone = true;
				break;
			}
		}
		if (!bDone) {
			// EXE-INLINED none: NiIsKindOf
			for (const NiRTTI* pk = pkController->GetRTTI(); pk; pk = pk->m_pkBaseRTTI) {
				if (pk == &NiLookAtController::ms_RTTI) {
					ConvertLookAtController(static_cast<NiLookAtController*>(pkController));
					bDone = true;
					break;
				}
			}
		}
		if (!bDone) {
			// EXE-INLINED none: NiIsKindOf
			for (const NiRTTI* pk = pkController->GetRTTI(); pk; pk = pk->m_pkBaseRTTI) {
				if (pk == &NiPathController::ms_RTTI) {
					ConvertPathController(static_cast<NiPathController*>(pkController));
					break;
				}
			}
		}
		pkController = pkNextController;
	}

	NiNode* pkNode = pkObject->IsNode();
	if (pkNode) {
		for (uint32_t ui = 0; ui < pkNode->m_kChildren.m_usSize; ui++)
			RecurseScene(kStream, pkNode->m_kChildren.m_pBase[ui].m_pObject);
	}
}

// The roll controller becomes the roll interpolator of the first look-at interpolator a transform controller of the
// target holds, then leaves the target's list.
void NiOldAnimationConverter::ConvertRollController(NiRollController* pkController) {
	NiObjectNET* pkTarget = pkController->m_pkTarget;
	for (NiTimeController* pkCtrl = pkTarget->m_spControllers; pkCtrl; pkCtrl = pkCtrl->m_spNext.m_pObject) {
		// EXE-INLINED none: NiIsKindOf
		const NiRTTI* pkRTTI = pkCtrl->GetRTTI();
		while (pkRTTI && pkRTTI != &NiTransformController::ms_RTTI)
			pkRTTI = pkRTTI->m_pkBaseRTTI;
		if (!pkRTTI)
			continue;

		NiTransformController* pkTransformController = static_cast<NiTransformController*>(pkCtrl);
		NiInterpolator* pkInterp = pkTransformController->GetInterpolator(0);
		if (!pkInterp)
			continue;

		// EXE-INLINED none: NiIsKindOf
		pkRTTI = pkInterp->GetRTTI();
		while (pkRTTI && pkRTTI != &NiLookAtInterpolator::ms_RTTI)
			pkRTTI = pkRTTI->m_pkBaseRTTI;
		if (!pkRTTI)
			continue;

		NiLookAtInterpolator* pkLookAtInterp = static_cast<NiLookAtInterpolator*>(pkInterp);
		NiFloatData* pkData = pkController->m_spFloatData.m_pObject;
		NiFloatInterpolator* pkRollInterp =
			static_cast<NiFloatInterpolator*>(NiMemObject::operator new(sizeof(NiFloatInterpolator)));
		if (pkRollInterp)
			pkRollInterp = ::new (pkRollInterp) NiFloatInterpolator(pkData);
		pkLookAtInterp->SetRollInterpolator(pkRollInterp);
		SynchronizeTimeControllers(pkController, pkTransformController);
		float fStart;
		float fEnd;
		pkLookAtInterp->GetActiveTimeRange(fStart, fEnd);
		pkLookAtInterp->GuaranteeTimeRange(fStart, fEnd);
		pkTransformController->ResetTimeExtrema();
		pkLookAtInterp->Collapse();
		break;
	}
	pkTarget->RemoveController(pkController);
}

// The look-at controller becomes a look-at interpolator on the target's transform controller (a new one when the
// target has none holding a transform interpolator); the old transform interpolator's position and scale keys move
// to the look-at interpolator's translation and scale interpolators. The exe counts the old interpolator's
// references through the Interlocked imports (the source's NiPointer).
void NiOldAnimationConverter::ConvertLookAtController(NiLookAtController* pkController) {
	NiObjectNET* pkTarget = pkController->m_pkTarget;
	bool bNewController = false;
	NiTransformController* pkTransformController =
		static_cast<NiTransformController*>(pkTarget->GetController(&NiTransformController::ms_RTTI));
	bool bHasTransformInterp = false;
	if (pkTransformController) {
		NiInterpolator* pkInterp = pkTransformController->GetInterpolator(0);
		if (pkInterp) {
			// EXE-INLINED none: NiIsKindOf
			const NiRTTI* pkRTTI = pkInterp->GetRTTI();
			while (pkRTTI && pkRTTI != &NiTransformInterpolator::ms_RTTI)
				pkRTTI = pkRTTI->m_pkBaseRTTI;
			bHasTransformInterp = pkRTTI != nullptr;
		}
	}
	if (!bHasTransformInterp) {
		bNewController = true;
		pkTransformController =
			static_cast<NiTransformController*>(NiMemObject::operator new(sizeof(NiTransformController)));
		if (pkTransformController)
			pkTransformController = ::new (pkTransformController) NiTransformController;
		SynchronizeTimeControllers(pkController, pkTransformController);
	}

	NiLookAtInterpolator* pkLookAtInterp =
		static_cast<NiLookAtInterpolator*>(NiMemObject::operator new(sizeof(NiLookAtInterpolator)));
	if (pkLookAtInterp)
		pkLookAtInterp = ::new (pkLookAtInterp) NiLookAtInterpolator(nullptr, NiLookAtInterpolator::X, false);
	pkLookAtInterp->m_pkLookAt = pkController->m_pkLookAt;
	pkLookAtInterp->SetLookAtName(EmptyFS);
	if (pkController->m_uFlags & NiLookAtInterpolator::FLIP_MASK) {
		pkLookAtInterp->m_uFlags |= NiLookAtInterpolator::FLIP_MASK;
	} else {
		pkLookAtInterp->m_uFlags &= ~NiLookAtInterpolator::FLIP_MASK;
	}
	pkLookAtInterp->m_uFlags ^= (pkLookAtInterp->m_uFlags ^ pkController->m_uFlags) & NiLookAtInterpolator::AXIS_MASK;

	NiInterpolator* pkOldInterp = pkTransformController->GetInterpolator(0);
	if (pkOldInterp) {
		pkOldInterp->IncRefCount();
		// EXE-INLINED none: NiIsKindOf
		const NiRTTI* pkRTTI = pkOldInterp->GetRTTI();
		while (pkRTTI && pkRTTI != &NiTransformInterpolator::ms_RTTI)
			pkRTTI = pkRTTI->m_pkBaseRTTI;
		if (pkRTTI) {
			NiTransformInterpolator* pkOldTransformInterp = static_cast<NiTransformInterpolator*>(pkOldInterp);
			NiTransformData* pkData = pkOldTransformInterp->m_spData.m_pObject;
			if (pkData && pkData->m_usNumPosKeys) {
				NiPoint3Interpolator* pkTranslateInterp =
					static_cast<NiPoint3Interpolator*>(NiMemObject::operator new(sizeof(NiPoint3Interpolator)));
				if (pkTranslateInterp)
					pkTranslateInterp =
						::new (pkTranslateInterp) NiPoint3Interpolator(static_cast<NiPosData*>(nullptr));
				uint32_t uiNumKeys;
				NiAnimationKey::KeyType eType;
				uint8_t ucSize;
				NiPosKey* pkKeys = pkOldTransformInterp->GetPosData(uiNumKeys, eType, ucSize);
				pkTranslateInterp->SetKeys(pkKeys, uiNumKeys, eType);
				pkOldTransformInterp->m_spData.m_pObject->SetPosAnim(nullptr, 0, NiAnimationKey::NOINTERP);
				pkLookAtInterp->SetTranslateInterpolator(pkTranslateInterp);
			}
			pkData = pkOldTransformInterp->m_spData.m_pObject;
			if (pkData && pkData->m_usNumScaleKeys) {
				NiFloatInterpolator* pkScaleInterp =
					static_cast<NiFloatInterpolator*>(NiMemObject::operator new(sizeof(NiFloatInterpolator)));
				if (pkScaleInterp)
					pkScaleInterp = ::new (pkScaleInterp) NiFloatInterpolator(static_cast<NiFloatData*>(nullptr));
				uint32_t uiNumKeys;
				NiAnimationKey::KeyType eType;
				uint8_t ucSize;
				NiFloatKey* pkKeys = pkOldTransformInterp->GetScaleData(uiNumKeys, eType, ucSize);
				pkScaleInterp->SetKeys(pkKeys, uiNumKeys, eType);
				pkOldTransformInterp->m_spData.m_pObject->SetScaleAnim(nullptr, 0, NiAnimationKey::NOINTERP);
				pkLookAtInterp->SetScaleInterpolator(pkScaleInterp);
			}
		}
	}

	pkTransformController->SetInterpolator(pkLookAtInterp, 0);
	pkLookAtInterp->Collapse();
	if (bNewController)
		pkTransformController->SetTarget(pkTarget);
	if (pkTarget)
		pkTarget->RemoveController(pkController);
	if (pkOldInterp) {
		pkOldInterp->DecRefCount();
	}
}

// The path controller becomes a path interpolator on the target's transform controller: the existing one when it
// has no interpolator yet, else a new one put in the old controller's place in the target's list. The exe counts
// the controller's references through the Interlocked imports (the source's NiPointer).
void NiOldAnimationConverter::ConvertPathController(NiPathController* pkController) {
	NiObjectNET* pkTarget = pkController->m_pkTarget;
	NiTransformController* pkTransformController =
		static_cast<NiTransformController*>(pkTarget->GetController(&NiTransformController::ms_RTTI));
	if (pkTransformController)
		pkTransformController->IncRefCount();

	NiPathInterpolator* pkInterp =
		static_cast<NiPathInterpolator*>(NiMemObject::operator new(sizeof(NiPathInterpolator)));
	if (pkInterp) {
		pkInterp = ::new (pkInterp)
			NiPathInterpolator(pkController->m_spPathData.m_pObject, pkController->m_spPctData.m_pObject);
	}
	if (pkController->m_uFlags & NiPathController::ALLOW_FLIP_MASK) {
		pkInterp->m_uFlags |= NiPathInterpolator::ALLOW_FLIP_MASK;
	} else {
		pkInterp->m_uFlags &= ~NiPathInterpolator::ALLOW_FLIP_MASK;
	}
	if (pkController->m_uFlags & NiPathController::BANK_MASK) {
		pkInterp->m_uFlags |= NiPathInterpolator::BANK_MASK;
	} else {
		pkInterp->m_uFlags &= ~NiPathInterpolator::BANK_MASK;
	}
	// The two classes' BankDir enums have the same values; the source converts one to the other.
	pkInterp->m_eBankDir = static_cast<NiPathInterpolator::BankDir>(pkController->m_eBankDir);
	if (pkController->m_uFlags & NiPathController::CONST_VELOCITY_MASK) {
		pkInterp->m_uFlags |= NiPathInterpolator::CONST_VELOCITY_MASK;
		pkInterp->SetConstantVelocityData();
	} else {
		pkInterp->m_uFlags &= ~NiPathInterpolator::CONST_VELOCITY_MASK;
	}
	if (pkController->m_uFlags & NiPathController::FOLLOW_MASK) {
		pkInterp->m_uFlags |= NiPathInterpolator::FOLLOW_MASK;
	} else {
		pkInterp->m_uFlags &= ~NiPathInterpolator::FOLLOW_MASK;
	}
	pkInterp->m_fMaxBankAngle = pkController->m_fMaxBankAngle;
	pkInterp->m_fSmoothing = pkController->m_fSmoothing;
	pkInterp->m_sFollowAxis = pkController->m_sFollowAxis;
	if (pkController->m_uFlags & NiPathController::FLIP_MASK) {
		pkInterp->m_uFlags |= NiPathInterpolator::FLIP_MASK;
	} else {
		pkInterp->m_uFlags &= ~NiPathInterpolator::FLIP_MASK;
	}
	if (pkController->m_uFlags & NiPathController::CURVE_TYPE_OPEN_MASK) {
		pkInterp->m_uFlags |= NiPathInterpolator::CURVE_TYPE_OPEN_MASK;
	} else {
		pkInterp->m_uFlags &= ~NiPathInterpolator::CURVE_TYPE_OPEN_MASK;
	}

	if (pkTransformController && !pkTransformController->GetInterpolator(0)) {
		pkTransformController->SetInterpolator(pkInterp, 0);
		SynchronizeTimeControllers(pkController, pkTransformController);
	} else {
		NiTransformController* pkNew =
			static_cast<NiTransformController*>(NiMemObject::operator new(sizeof(NiTransformController)));
		if (pkNew)
			pkNew = ::new (pkNew) NiTransformController;
		if (pkTransformController != pkNew) {
			if (pkTransformController) {
				pkTransformController->DecRefCount();
			}
			pkTransformController = pkNew;
			if (pkNew)
				pkNew->IncRefCount();
		}
		pkTransformController->SetInterpolator(pkInterp, 0);
		pkTransformController->SetTarget(pkTarget);
		SynchronizeTimeControllers(pkController, pkTransformController);
		pkTarget->SetControllers(pkTransformController->m_spNext.m_pObject);
		// The source's two NiPointer assignments, the new next pointer read once before the release.
		NiTimeController* pkNext = pkController->m_spNext.m_pObject;
		NiTimeController* pkOldNext = pkTransformController->m_spNext.m_pObject;
		if (pkOldNext != pkNext) {
			if (pkOldNext && InterlockedDecrementUInt32(&pkOldNext->m_uiRefCount) == 0)
				pkOldNext->DeleteThis();
			pkTransformController->m_spNext.m_pObject = pkNext;
			if (pkNext)
				InterlockedIncrementUInt32(&pkNext->m_uiRefCount);
		}
		pkOldNext = pkController->m_spNext.m_pObject;
		if (pkOldNext != pkTransformController) {
			if (pkOldNext) {
				pkOldNext->DecRefCount();
			}
			pkController->m_spNext.m_pObject = pkTransformController;
			pkTransformController->IncRefCount();
		}
	}
	pkInterp->Collapse();
	if (pkTarget)
		pkTarget->RemoveController(pkController);
	pkTransformController->DecRefCount();
}

void NiOldAnimationConverter::SynchronizeTimeControllers(const NiTimeController* pkSrc, NiTimeController* pkDest) {
	uint16_t v4;
	uint16_t m_usFlags;
	uint16_t v6;
	int32_t v2 = pkSrc->m_usFlags & 1;
	uint16_t v3 = (uint16_t)(v2 | (pkDest->m_usFlags & 65534));
	pkDest->m_usFlags = v3;
	if (v2) {
		v4 = (uint16_t)(v3 & 65527);
	} else {
		v4 = (uint16_t)(v3 | 8);
	}
	pkDest->m_usFlags = v4;
	pkDest->m_usFlags ^= ((pkSrc->m_usFlags ^ v4) & 6);
	m_usFlags = pkDest->m_usFlags;
	if ((pkSrc->m_usFlags & 16) != 0) {
		v6 = (uint16_t)(m_usFlags | 16);
	} else {
		v6 = (uint16_t)(m_usFlags & 65519);
	}
	pkDest->m_usFlags = v6;
	pkDest->m_fFrequency = pkSrc->m_fFrequency;
	pkDest->m_fLoKeyTime = pkSrc->m_fLoKeyTime;
	pkDest->m_fHiKeyTime = pkSrc->m_fHiKeyTime;
	if ((pkSrc->m_usFlags & 8) != 0) {
		pkDest->m_usFlags |= 8u;
	} else {
		pkDest->m_usFlags &= (~8u);
	}
}

// Takes pkKeys over, creating the data object first when there is none; no keys empties the data.
void NiFloatInterpolator::SetKeys(NiFloatKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType) {
	if (!uiNumKeys) {
		if (m_spFloatData.m_pObject)
			m_spFloatData.m_pObject->SetAnim(nullptr, 0, NiAnimationKey::NOINTERP);
		return;
	}
	if (!m_spFloatData.m_pObject) {
		NiFloatData* pkData = static_cast<NiFloatData*>(NiMemObject::operator new(sizeof(NiFloatData)));
		if (pkData) {
			::new (pkData) NiFloatData;
		}
		m_spFloatData = pkData;
	}
	m_spFloatData.m_pObject->SetAnim(pkKeys, uiNumKeys, eType);
	m_uiLastIdx = 0;
}

// The exe inlines the smart pointer assignment through the Interlocked imports, testing pkInterp first.
void NiLookAtInterpolator::SetTranslateInterpolator(NiInterpolator* pkInterp) {
	NiInterpolator* pkOld = m_aspInterpolators[0].m_pObject;
	if (pkInterp) {
		if (pkOld == pkInterp) {
			return;
		}
		if (pkOld) {
			pkOld->DecRefCount();
		}
		m_aspInterpolators[0].m_pObject = pkInterp;
		pkInterp->IncRefCount();
	} else if (pkOld) {
		pkOld->DecRefCount();
		m_aspInterpolators[0].m_pObject = nullptr;
	}
}

// As SetTranslateInterpolator, for the scale slot.
void NiLookAtInterpolator::SetScaleInterpolator(NiInterpolator* pkInterp) {
	NiInterpolator* pkOld = m_aspInterpolators[2].m_pObject;
	if (pkInterp) {
		if (pkOld == pkInterp) {
			return;
		}
		if (pkOld) {
			pkOld->DecRefCount();
		}
		m_aspInterpolators[2].m_pObject = pkInterp;
		pkInterp->IncRefCount();
	} else if (pkOld) {
		pkOld->DecRefCount();
		m_aspInterpolators[2].m_pObject = nullptr;
	}
}

void NiLookAtInterpolator::SetRollInterpolator(NiInterpolator* pkInterp) {
	NiInterpolator* m_pObject = m_aspInterpolators[1].m_pObject;
	if (pkInterp) {
		if (m_pObject != pkInterp) {
			if (m_pObject) {
				NiDecRefCount(m_pObject);
			}
			m_aspInterpolators[1].m_pObject = pkInterp;
			NiIncRefCount(pkInterp);
		}
	} else {
		if (m_pObject) {
			NiDecRefCount(m_pObject);
			m_aspInterpolators[1].m_pObject = 0;
		}
	}
}

// Takes pkKeys over, creating the data object first when there is none; no keys empties the data.
void NiPoint3Interpolator::SetKeys(NiPosKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType) {
	if (!uiNumKeys) {
		if (m_spPoint3Data.m_pObject)
			m_spPoint3Data.m_pObject->SetAnim(nullptr, 0, NiAnimationKey::NOINTERP);
		return;
	}
	if (!m_spPoint3Data.m_pObject) {
		NiPosData* pkData = static_cast<NiPosData*>(NiMemObject::operator new(sizeof(NiPosData)));
		if (pkData) {
			::new (pkData) NiPosData;
		}
		m_spPoint3Data.SetNew(pkData);
	}
	m_spPoint3Data.m_pObject->SetAnim(pkKeys, uiNumKeys, eType);
	m_uiLastIdx = 0;
}
