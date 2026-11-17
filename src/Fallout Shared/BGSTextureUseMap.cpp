#include "Fallout Shared/BGSTextureUseMap.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSShaderProperty.h"
#include "BSSystem/BSFile.h"
#include "Fallout Shared/BGSSceneInfo.h"
#include "Fallout Shared/GridCell.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TEX_USER_DATA.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"

BGSTextureUseMap::~BGSTextureUseMap() {
	ClearAllTexUseData();
}

void BGSTextureUseMap::CollectTexUseDataRecursive(NiAVObject* ap3D, bool abCheckActors) {
	bool bignore;
	if (ap3D && !ap3D->GetAppCulled()) {
		NiNode* pnode = ap3D->IsNode();
		if (pnode) {
			bignore = 0;
			bool bsetref = 0;
			NiNode* pnode_2 = 0;
			if (pnode->IsFadeNode()) {
				pnode_2 = pnode;
			}
			if (pnode_2) {
				TESObjectREFR* pref = static_cast<BSFadeNode*>(pnode_2)->BSFadeNode::GetLinkedObj();
				if (BGSSceneInfo::pReferenceIgnoreMap) {
					BGSSceneInfo::pReferenceIgnoreMap->GetAt(pref, bignore);
				}
				if (pref && !bignore) {
					bool bisactor = 0;
					int32_t FormType = pref->GetObjectReference()->GetFormType();
					if (FormType >= 42) {
						bisactor = FormType <= 45;
					}
					if ((bisactor && !abCheckActors) || pref->IsMarker()) {
						return;
					}
					bsetref = 1;
					pactiveref = pref;
				}
			}
			if (!bignore) {
				uint32_t icount = pnode->GetArrayCount();
				for (uint32_t i = 0; i < icount; ++i) {
					NiAVObject* pchild = pnode->GetAt(i);
					CollectTexUseDataRecursive(pchild, abCheckActors);
				}
			}
			if (bsetref) {
				pactiveref = 0;
			}
			return;
		}
		NiGeometry* pgeom = ap3D->IsGeometry();
		if (pgeom) {
			BSShaderProperty* pbssprop;
			NiShadeProperty* pprop = static_cast<NiShadeProperty*>(pgeom->GetProperty(3));
			if (pprop) {
				pbssprop = pprop->iShaderPropertyType != -1 ? static_cast<BSShaderProperty*>(pprop) : 0;
			} else {
				pbssprop = 0;
			}
			if (pbssprop) {
				TESObjectREFR* pDefaultUser;
				void* kparam[2];
				kparam[0] = (void*)this;
				if (pactiveref) {
					pDefaultUser = pactiveref;
				} else {
					pDefaultUser = static_cast<TESObjectREFR*>(this->pDefaultUser);
				}
				kparam[1] = (void*)pDefaultUser;
				pbssprop->GetTextureUse(&BGSTextureUseMap::TexUseCB, kparam);
			}
		}
	}
}

void BGSTextureUseMap::AddTexUse(NiTexture* apTex, uint32_t aulMemorySize, TESForm* apForm) {
	if (apTex) {
		if (apForm) {
			TEX_USER_DATA* pdata = 0;
			if (!UserMap.GetAt(apTex, pdata) || !pdata) {
				TEX_USER_DATA* pdata_2;
				TEX_USER_DATA* apMemory = static_cast<TEX_USER_DATA*>(ourMalloc(sizeof(TEX_USER_DATA)));
				if (apMemory) {
					pdata_2 = ::new (apMemory) TEX_USER_DATA;
				} else {
					pdata_2 = 0;
				}
				pdata = pdata_2;
				UserMap.SetAt(apTex, pdata_2);
				pdata->spTexture.operator=(apTex);
				pdata->ulTextureSize = aulMemorySize;
			}
			if (!pdata->UserList.IsInList(apForm)) {
				pdata->UserList.AddHead(apForm);
			}
		}
	}
}

void BGSTextureUseMap::CollectTexUseData(NiNode* apNode, TESForm* apDefaultUser, bool abCheckActors) {
	pDefaultUser = apDefaultUser;
	CollectTexUseDataRecursive(apNode, abCheckActors);
}

void BGSTextureUseMap::CollectSceneTexUseData(bool abCheckActors) {
	NiNode* v2;
	NiNode* v3;
	TESObjectCELL* interior;
	TESObjectCELL* pCell;
	GridCell* gridCell;
	uint32_t j;
	uint32_t i;
	uint32_t v10;
	uint32_t v11;
	if (TES::pInstance->TES::GetInterior()) {
		interior = TES::pInstance->TES::GetInterior();
		v2 = interior->Get3D();
		CollectTexUseData(v2, interior, abCheckActors);
	} else {
		v10 = ((&uGridsToLoad)->UInt()) >> 1;
		v11 = v10 + 2;
		for (i = v10 - 2; i <= v11; ++i) {
			for (j = v10 - 2; j <= v11; ++j) {
				gridCell = TES::pInstance->GetGridCell(i, j);
				if (gridCell->pCell) {
					pCell = gridCell->pCell;
					v3 = gridCell->pCell->Get3D();
					CollectTexUseData(v3, pCell, abCheckActors);
				}
			}
		}
	}
}

uint32_t BGSTextureUseMap::GetTotalTextureSize() {
	NiTMapItem<NiTexture*, TEX_USER_DATA*>* iter;
	uint32_t ulretsize = 0;
	iter = UserMap.GetFirstPos();
	while (iter) {
		NiTexture* pignore = 0;
		TEX_USER_DATA* pdata = 0;
		UserMap.GetNext(iter, pignore, pdata);
		if (pdata) {
			ulretsize += pdata->ulTextureSize;
		}
	}
	return ulretsize;
}

void BGSTextureUseMap::ClearAllTexUseData() {
	NiTMapItem<NiTexture*, TEX_USER_DATA*>* iter;
	iter = UserMap.GetFirstPos();
	while (iter) {
		NiTexture* pignore = 0;
		TEX_USER_DATA* pdata = 0;
		UserMap.GetNext(iter, pignore, pdata);
		if (pdata) {
			delete pdata;
		}
	}
	UserMap.RemoveAll();
}

bool BGSTextureUseMap::WriteToFile(const char* apFilename) {
	NiTMapItem<NiTexture*, TEX_USER_DATA*>* iter;
	bool bretval = 0;
	BSFile mapfile(apFilename, NiFile::WRITE_ONLY, 1024u, 1);
	if (mapfile.BSFile::Open(0, 0)) {
		bretval = 1;
		iter = UserMap.GetFirstPos();
		while (iter) {
			NiTexture* pbasetex = 0;
			TEX_USER_DATA* pdata = 0;
			UserMap.GetNext(iter, pbasetex, pdata);
			NiSourceTexture* ptex =
				static_cast<NiSourceTexture*>(NiObject::DynamicCastStatic(&NiSourceTexture::ms_RTTI, pbasetex));
			if (ptex) {
				if (pdata) {
					BSStringT<char> outstring;
					int32_t v6 = pdata->UserList.ItemsInList();
					uint32_t ulTextureSize = pdata->ulTextureSize;
					const NiFixedString* v2 = &ptex->GetFilename();
					outstring.SPrintF("%s\t%d bytes\t%i users\r\n", v2->GetHandleString(), ulTextureSize, v6);
					mapfile.BSFile::Write(outstring, 1);
					BSSimpleList<TESForm*>* plist = &pdata->UserList;
					while (plist && !plist->IsEmpty()) {
						TESForm* puser = plist->m_item;
						plist = plist->m_pkNext;
						int32_t CastingType = puser->iFormID;
						const char* v5 = puser->GetFormEditorID();
						outstring.SPrintF("%s\t%s\t%08X\r\n", puser->GetFormTypeString(), v5, CastingType);
						mapfile.BSFile::Write(outstring, 1);
					}
					outstring.SPrintF("\r\n");
					mapfile.BSFile::Write(outstring, 1);
				}
			}
		}
		mapfile.Close();
	}
	return bretval;
}

// apParam is CollectTexUseData's two-pointer array: the map, then the default user form.
void BGSTextureUseMap::TexUseCB(NiTexture* apTexture, uint32_t aulMemorySize, void* apParam) {
	void** ppParam = static_cast<void**>(apParam);
	if (ppParam[0]) {
		static_cast<BGSTextureUseMap*>(ppParam[0])
			->AddTexUse(apTexture, aulMemorySize, static_cast<TESForm*>(ppParam[1]));
	}
}

TEX_USER_DATA::TEX_USER_DATA() : spTexture(nullptr) {}

TEX_USER_DATA::~TEX_USER_DATA() {
	UserList.RemoveAll();
}
