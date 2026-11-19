#include "Fallout Shared/BGSIdleMarker.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSShader/BSFadeNode.h"
#include "Fallout Shared/BGSIdleCollection.h"
#include "Fallout Shared/PackageTarget.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "xfnv/Compiler.h"

// Reads the marker's chunks: the idle collection's flags, count, idle list and timer, and a package target built from
// the PTDT chunk and destroyed again without being kept, as the game does.
bool BGSIdleMarker::Load(TESFile* apFile) {
    TESForm::LoadForm(apFile);
    TESForm::SetInitialized(false);
    do {
        uint32_t eChunk = apFile->GetTESChunk();
        if (eChunk == NO_CHUNK) {
            break;
        }
        switch (eChunk) {
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                apFile->GetChunkData(pBuffer, 0x200);
                static_cast<TESForm*>(this)->SetFormEditorID(pBuffer);
                break;
            }
            case OBND_ID: static_cast<TESForm*>(this)->LoadObjectBound(apFile); break;
            case IDLF_ID: apFile->GetChunkData(&cIdleFlags, 1); break;
            case IDLC_ID:
                if (apFile->GetChunkSize() == 1) {
                    apFile->GetChunkData(&cIdleCount, 1);
                }
                else {
                    uint32_t uiCount = 0;
                    apFile->GetChunkData(uiCount);
                    cIdleCount = static_cast<uint8_t>(uiCount);
                }
                if (cIdleCount) {
                    if (pIdleArray) {
                        ourFree(pIdleArray);
                    }
                    pIdleArray = static_cast<TESIdleForm**>(ourMalloc(sizeof(TESIdleForm*) * cIdleCount));
                    ourMemset(pIdleArray, 0, sizeof(TESIdleForm*) * cIdleCount);
                }
                break;
            case IDLA_ID:
                if (pIdleArray) {
                    uint32_t uiSize = 4 * cIdleCount;
                    if (apFile->GetChunkSize() == uiSize) {
                        apFile->GetChunkData(pIdleArray, uiSize);
#if XFNV_64BIT
                        // The file keeps a 4-byte form ID for each idle, resolved by InitItem.
                        TESFile::WidenFormIDRecords(pIdleArray, cIdleCount, 4, sizeof(TESIdleForm*), {{0, 0}});
#endif
                        if (apFile->QEndian()) {
                            for (int32_t i = 0; i < cIdleCount; ++i) {
                                // Until InitItem resolves it, each slot holds an untyped form ID, which is swapped as a
                                // 32-bit value.
                                ENDIAN(reinterpret_cast<uint32_t&>(pIdleArray[i]), false);
                            }
                        }
                    }
                }
                break;
            case IDLT_ID:
                // Reads the float timer as raw 32-bit data through GetChunkData<uint32_t>.
                apFile->GetChunkData(reinterpret_cast<uint32_t&>(fTimerCheckForIdle));
                break;
            case PTDT_ID: {
                PACK_TARG_FILE PTData;
                ourMemset(&PTData, 0, sizeof(PACK_TARG_FILE));
                apFile->GetChunkData(&PTData, sizeof(PACK_TARG_FILE));
                if (apFile->QEndian()) {
                    PTData.Endian();
                }
                // Constructs a stack target from the file data and destroys it at once, as the game does.
                alignas(PackageTarget) uint8_t packTargetStorage[sizeof(PackageTarget)];
                PackageTarget* pPackTarget = ::new (packTargetStorage) PackageTarget(&PTData);
                pPackTarget->~PackageTarget();
                break;
            }
            default: break;
        }
    }
    while (apFile->NextChunk());
    return true;
}

BGSIdleMarker::BGSIdleMarker() {
    SetFormType(0x30u);
}

BGSIdleMarker::~BGSIdleMarker() {}

void BGSIdleMarker::Save() {
    StartForm();
    BGSIdleCollection::Save();
    CloseForm();
}

void BGSIdleMarker::InitItem() {
    if (!TESForm::GetInitialized()) {
        static_cast<BGSIdleCollection*>(this)->InitItem(this);
        TESForm::SetInitialized(true);
    }
}

void BGSIdleMarker::Copy(TESForm* apCopy) {
    BGSIdleMarker* v3 = dynamic_cast<BGSIdleMarker*>(apCopy);
    if (v3) {
        TESForm::CopyAllComponents(v3);
    }
}

bool BGSIdleMarker::Compare(TESForm* apCompare) {
    BGSIdleMarker* v4 = dynamic_cast<BGSIdleMarker*>(apCompare);
    return ((!v4) || (((int8_t)TESForm::CompareAllComponents(v4)) != 0));
}

NiAVObject* BGSIdleMarker::Clone3D(TESObjectREFR* apRequester) {
    NiFixedString kName;
    BSFadeNode* pNode = new BSFadeNode;
    const char* pEditorID = GetFormEditorID();
    NiFixedString* pName = ::new (static_cast<void*>(&kName)) NiFixedString(const_cast<char*>(pEditorID));
    pNode->SetName(*pName);
    return pNode;
}
