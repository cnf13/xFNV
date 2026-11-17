#include "Fallout/Magic/LightEffect.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/TESLight.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "SDK/MSVC/rtti.h"

// Not named in the PDB or IDA; name chosen here. Whether the light archetype took Instantiate: the exe stores
// RegisterActiveEffect's whole return byte.
static char bRegistered = EffectArchetypes::RegisterActiveEffect(EffectArchetypes::LIGHT, &LightEffect::Instantiate);

LightEffect::LightEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect)
	: ActiveEffect(apCaster, apSpell, apEffect), spPointLight(nullptr) {}

// The point light's release is spPointLight's own destructor.
LightEffect::~LightEffect() {}

// Copies the effect: allocates a new one, constructs it from this effect's caster, spell and effect item, then copies
// the shared data into it. The exe calls CopyData through the vtable (slot 17, +0x44) with this effect as `this`.
ActiveEffect* LightEffect::Copy() {
	LightEffect* pCopy = static_cast<LightEffect*>(ourMalloc(sizeof(LightEffect)));
	if (pCopy) {
		pCopy = ::new (pCopy) LightEffect(pCaster, pSpell, pEffect);
	} else {
		pCopy = nullptr;
	}
	static_cast<ActiveEffect*>(this)->CopyData(pCopy);
	return pCopy;
}

void LightEffect::Update(float afSecondsElapsed) {
	NiPointLight* v2;
	NiPointLight* v3;
	ShadowSceneNode* a1;
	if (spPointLight.m_pObject) {
		a1 = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD);
		if (a1) {
			v2 = spPointLight.m_pObject;
			if (!a1->GetLight(v2)) {
				v3 = spPointLight.m_pObject;
				a1->AddLight(v3, 1);
			}
		}
	}
}

void LightEffect::FinishInitLoadGame(Actor* apActor) {
	ActiveEffect::FinishInitLoadGame(apActor);
	Start();
}

void LightEffect::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
	ActiveEffect::Revert(apLoadGameBuffer);
	DetachLight();
}

// Reverts the base effect and takes the light off the target. The exe calls the base Revert (the shared empty stub
// 0x4534F0) directly. Both Revert slots hold this function (0x80F030), registered here.
void LightEffect::Revert(Actor* apActor) {
	ActiveEffect::Revert(apActor);
	DetachLight();
}

void LightEffect::FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	ActiveEffect::FinishLoadGame(apLoadGameBuffer);
	Start();
}

void LightEffect::CopyData(ActiveEffect* apDestination) {
	ActiveEffect::CopyData(apDestination);
	LightEffect* pdestination = dynamic_cast<LightEffect*>(apDestination);
	if (pdestination) {
		pdestination->spPointLight = spPointLight;
	}
}

bool LightEffect::CheckTarget(MagicTarget* apTarget) {
	Actor* pTargetActor;
	if (apTarget) {
		pTargetActor = apTarget->GetTargetAsActor();
	} else {
		pTargetActor = nullptr;
	}
	return pTargetActor != nullptr;
}

// Attaches the best light of the target actor. The exe's second null test of the actor before the MagicTarget
// adjust (0x80E880) is dead: clang folds it away.
void LightEffect::Start() {
	Actor* pActor = GetTarget() ? GetTarget()->GetTargetAsActor() : nullptr;
	if (pActor) {
		AttachBestLightEffect(pActor);
	}
}

// Takes this light off and attaches the target actor's best remaining light.
void LightEffect::Finish() {
	DetachLight();
	Actor* pActor = GetTarget() ? GetTarget()->GetTargetAsActor() : nullptr;
	if (pActor) {
		AttachBestLightEffect(pActor);
	}
}

void LightEffect::DetachLight() {
	Actor* TargetAsActor;
	if (GetTarget()) {
		MagicTarget* Unk24 = GetTarget();
		TargetAsActor = Unk24->GetTargetAsActor();
	} else {
		TargetAsActor = 0;
	}
	if (spPointLight.m_pObject) {
		ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(0);
		if (ShadowSceneNode_) {
			NiLight* v2 = spPointLight.m_pObject;
			ShadowSceneNode_->RemoveLight(v2);
		}
		NiPointLight* v3 = spPointLight.m_pObject;
		NiNode* Parent = v3->GetParent();
		if (Parent) {
			NiAVObject* v4 = spPointLight.m_pObject;
			Parent->DetachChild(v4);
		}
		if (TargetAsActor) {
			TargetAsActor->KillLight(1);
		}
		spPointLight.operator=(static_cast<NiPointLight*>(0));
		if (--LightEffect::iLightCount < 0) {
			LightEffect::iLightCount = 0;
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out. The colors are NiColors built with NiColor's three-float
// constructor (the folded NiPoint3 one, 0x416870).
void LightEffect::AttachLight() {
	Actor* ptargetActor;
	NiPoint3 v19;
	FNVD_INLINED_LOCAL(NiPointer<NiNode>, pparent, std::nullopt, std::nullopt);
	if (pTarget) {
		ptargetActor = pTarget->GetTargetAsActor();
	} else {
		ptargetActor = 0;
	}
	if (!spPointLight.m_pObject) {
		if (ptargetActor) {
			if (!ptargetActor->GetSpellEffectLight()) {
				int32_t* v1 = &iMagicLightMaxCount.Int();
				if (LightEffect::iLightCount <= *v1) {
					TESObjectLIGH* plight = GetEffectSetting()->data.pLight;
					if (plight) {
						NiNode* v12;
						NiPointLight* v16;
						NiPointLight* apMemory =
							static_cast<NiPointLight*>(NiMemObject::operator new(sizeof(NiPointLight)));
						if (apMemory) {
							v16 = ::new (apMemory) NiPointLight;
						} else {
							v16 = 0;
						}
						spPointLight.operator=(v16);
						uint32_t ucolor = plight->GetColor();
						float b = static_cast<float>(static_cast<uint8_t>(ucolor >> 16));
						float g = static_cast<float>(static_cast<uint8_t>(ucolor >> 8));
						FNVD_CONSTRUCTED_LOCAL(NiColor, color, nullptr,
							static_cast<float>(static_cast<uint8_t>(ucolor)), g, b);
						color.operator/=(255.0f);
						spPointLight.m_pObject->SetDiffuseColor(color);
						float* v2 = &fMagicLightRadiusBase.Float();
						double v15 = fMagnitude + *v2;
						float fradius = fMagicUnitsPerFoot.Float() * v15;
						FNVD_CONSTRUCTED_LOCAL(NiColor, fakeSpecular, nullptr, fradius, 0.0f, 0.0f);
						spPointLight.m_pObject->SetSpecularColor(fakeSpecular);
						float ba = plight->fFade;
						spPointLight.m_pObject->SetDimmer(ba);
						ShadowSceneNode* pnode = BSShaderManager::GetShadowSceneNode(0);
						if (pnode) {
							pnode->AddLight(spPointLight.m_pObject, 1);
						}
						float* v3 = &fMagicLightSideOffset.Float();
						float fX = ptargetActor->GetScale() * *v3;
						NiPoint3* v4 = ::new (&v19) NiPoint3(ptargetActor->GetBoundMax());
						double v14 = v4->y + fMagicLightForwardOffset.Float();
						float fY = ptargetActor->GetScale() * v14;
						double Height = ptargetActor->GetHeight();
						float* v5 = &fMagicLightHeightOffset.Float();
						float fZ = ptargetActor->GetScale() * *v5 + Height;
						spPointLight.m_pObject->SetTranslate(fX, fY, fZ);
						if (ptargetActor == pPlayer) {
							v12 = static_cast<NiNode*>(pPlayer->Get3D(0));
						} else {
							v12 = ptargetActor->Get3D();
						}
						// The exe's NiPointer constructor here is the folded copy at 0x633C90.
						::new (&pparent) NiPointer<NiNode>(v12);
						NiNode* v11 = pparent.m_pObject;
						v11->AttachChild(spPointLight.m_pObject, 1);
						NiPointLight* bb = spPointLight.m_pObject;
						ptargetActor->SetSpellEffectLight(bb);
						++LightEffect::iLightCount;
						pparent.~NiPointer<NiNode>();
					} else {
						Warning("EDITOR: Light Effect has no Light object associated with it. Use the editor to select "
								"a light in the Magic Effects window for the Light effect.");
					}
				}
			}
		}
	}
}

void LightEffect::AttachBestLightEffect(MagicTarget* apTarget) {
	if (apTarget) {
		LightEffect* v6 = 0;
		for (BSSimpleList<ActiveEffect*>* ecx0 = apTarget->GetActiveEffectList(); ecx0 && !ecx0->IsEmpty();
			ecx0 = ecx0->m_pkNext) {
			ActiveEffect* v4 = ecx0->m_item;
			if (v4) {
				EffectSetting* EffectSetting_ = v4->GetEffectSetting();
				if (EffectSetting_->GetEffectArchetype() == 13 && !v4->bDone) {
					double SequenceWeight;
					LightEffect* v3 = dynamic_cast<LightEffect*>(v4);
					if (v6 && (SequenceWeight = v6->GetMagnitude(), !(v3->GetMagnitude() > SequenceWeight))) {
						v3->DetachLight();
					} else {
						if (v6) {
							v6->DetachLight();
						}
						v6 = v3;
					}
				}
			}
		}
		if (v6) {
			v6->AttachLight();
		}
	}
}

ActiveEffect* LightEffect::Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
	void* v5 = ourMalloc(sizeof(LightEffect));
	if (v5) {
		return ::new (v5) LightEffect(apCaster, apSpell, apEffect);
	} else {
		return nullptr;
	}
}

NiColor& NiColor::operator/=(float fScalar) {
	r = r / fScalar;
	g = g / fScalar;
	b = b / fScalar;
	return *this;
}
