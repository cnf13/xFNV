#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DGeometryGroupManager.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryGroupManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinPartition.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTSet.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiDynamicGeometryGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryBufferData.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiStaticGeometryGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiUnsharedGeometryGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiVBSet.h"

NiD3DGeometryGroupManager::NiD3DGeometryGroupManager()
  : m_pkD3DDevice(nullptr), m_pkVBManager(nullptr), m_bHWSkinning(false) {}

// The groups' set frees its own buffer.
NiD3DGeometryGroupManager::~NiD3DGeometryGroupManager() {
    for (uint32_t i = 0; i < m_kGroups.m_uiUsed; ++i) {
        NiGeometryGroup* pkGroup = m_kGroups.m_pBase[i];
        if (pkGroup) {
            delete pkGroup;
        }
    }
    NiVBSet::DeleteBlocks();
    if (m_pkD3DDevice) {
        m_pkD3DDevice->Release();
    }
}

NiGeometryGroup* NiD3DGeometryGroupManager::CreateGroup(NiGeometryGroupManager::RepackingBehavior eBehavior) {
    NiGeometryGroup* pkGroup;
    NiGeometryGroup* result = nullptr;
    if (eBehavior) {
        if (eBehavior == NiGeometryGroupManager::UNSHARED) {
            pkGroup = NiUnsharedGeometryGroup::Create();
        }
        else {
            if (eBehavior != NiGeometryGroupManager::DYNAMIC) {
                return result;
            }
            pkGroup = NiDynamicGeometryGroup::Create();
        }
    }
    else {
        pkGroup = NiStaticGeometryGroup::Create();
    }
    if (pkGroup) {
        NiGeometryGroup** m_pBase;
        uint32_t m_uiUsed = m_kGroups.m_uiUsed;
        NiTPrimitiveSet<NiGeometryGroup*>* p_m_kGroups = &m_kGroups;
        int32_t v8 = 0;
        if (!m_uiUsed) {
            goto LABEL_14;
        }
        m_pBase = p_m_kGroups->m_pBase;
        while (*m_pBase != pkGroup) {
            ++v8;
            ++m_pBase;
            if (v8 >= m_uiUsed) {
                goto LABEL_14;
            }
        }
        if (v8 < 0) {
        LABEL_14:;
            uint32_t m_uiAlloced = m_kGroups.m_uiAlloced;
            if (m_uiUsed == m_uiAlloced) {
                uint32_t v11;
                if (m_uiAlloced) {
                    v11 = 2 * m_uiAlloced;
                }
                else {
                    v11 = 1;
                }
                p_m_kGroups->Realloc(v11);
            }
            p_m_kGroups->m_pBase[m_kGroups.m_uiUsed++] = pkGroup;
            IDirect3DDevice9* m_pkD3DDevice = this->m_pkD3DDevice;
            pkGroup->m_pkD3DDevice = m_pkD3DDevice;
            m_pkD3DDevice->AddRef();
        }
    }
    return pkGroup;
}

// An empty group leaves the set (the last group fills its slot) and is deleted. There is deliberately no check that the
// group is in the set, as the game does: a missing group moves the last one to index -1.
bool NiD3DGeometryGroupManager::DestroyGroup(NiGeometryGroup* pkGroup) {
    if (!pkGroup || !IsGroupEmpty(pkGroup)) {
        return false;
    }
    uint32_t uiUsed = m_kGroups.m_uiUsed;
    uint32_t uiIndex = 0;
    for (; uiIndex < uiUsed; ++uiIndex) {
        if (m_kGroups.m_pBase[uiIndex] == pkGroup) {
            break;
        }
    }
    if (uiIndex >= uiUsed) {
        uiIndex = 0xFFFFFFFF;
    }
    --m_kGroups.m_uiUsed;
    m_kGroups.m_pBase[uiIndex] = m_kGroups.m_pBase[m_kGroups.m_uiUsed];
    delete pkGroup;
    return true;
}

bool NiD3DGeometryGroupManager::IsGroupEmpty(NiGeometryGroup* pkGroup) const {
    return pkGroup->m_uiRefCount == 0;
}

bool NiD3DGeometryGroupManager::AddObjectToGroup(
    NiGeometryGroup* pkGroup,
    NiGeometryData* pkData,
    NiSkinInstance* pkSkinInstance
) {
    return AddObjectToGroup(pkGroup, pkData, pkSkinInstance, m_bHWSkinning, 0, 0);
}

// A screen texture that already has buffer data belongs to a group.
bool NiD3DGeometryGroupManager::AddObjectToGroup(NiGeometryGroup* pkGroup, NiScreenTexture* pkScreenTexture) {
    if (pkScreenTexture->m_pkBuffData) {
        return false;
    }
    pkGroup->AddObject(pkScreenTexture);
    return true;
}

// A skinned geometry goes through its partition (the callee's result is returned); a geometry with buffer data is
// removed from its group, which is purged once no object holds it. Returns false for a geometry with no buffer data.
bool NiD3DGeometryGroupManager::RemoveObjectFromGroup(NiGeometryData* pkData, NiSkinInstance* pkSkinInstance) {
    NiSkinPartition* pkSkinPartition = nullptr;
    if (pkSkinInstance) {
        pkSkinPartition = pkSkinInstance->m_spSkinPartition.m_pObject;
    }
    if (pkSkinPartition) {
        return RemoveSkinPartitionFromGroup(pkSkinPartition);
    }
    NiGeometryBufferData* pkBuffData = pkData->m_pkBuffData;
    if (!pkBuffData) {
        return false;
    }
    NiGeometryGroup* pkGroup = pkBuffData->m_pkGeometryGroup;
    pkGroup->RemoveObject(pkData);
    if (!pkGroup->m_uiRefCount) {
        pkGroup->Purge();
    }
    return true;
}

// The group is purged once no object holds it.
bool NiD3DGeometryGroupManager::RemoveObjectFromGroup(NiScreenTexture* pkScreenTexture) {
    NiGeometryBufferData* pkBuffData = pkScreenTexture->m_pkBuffData;
    if (!pkBuffData) {
        return false;
    }
    NiGeometryGroup* pkGroup = pkBuffData->m_pkGeometryGroup;
    pkGroup->RemoveObject(pkScreenTexture);
    if (!pkGroup->m_uiRefCount) {
        pkGroup->Purge();
    }
    return true;
}

bool NiD3DGeometryGroupManager::AddObjectToGroup(
    NiGeometryGroup* apGroup,
    NiGeometryData* apData,
    NiSkinInstance* apSkinInstance,
    bool abHWSkinning,
    uint32_t auiBonesPerPartition,
    uint32_t auiBonesPerVertex
) {
    NiSkinPartition* m_pObject;
    if (apSkinInstance && (abHWSkinning && (m_pObject = apSkinInstance->m_spSkinPartition.m_pObject) != nullptr)) {
        NiSkinPartition::Partition* m_pkPartitions = m_pObject->m_pkPartitions;
        if (m_pkPartitions->m_pkBuffData) {
            return false;
        }

        uint32_t m_uiPartitions = m_pObject->m_uiPartitions;
        if (m_uiPartitions) {
            uint32_t v11 = m_uiPartitions;
            do {
                apGroup->AddObject(apData, apSkinInstance, m_pkPartitions++);
                --v11;
            }
            while (v11);
        }
        return true;
    }

    if (apData->m_pkBuffData) {
        return false;
    }

    apGroup->AddObject(apData, apSkinInstance, static_cast<NiSkinPartition::Partition*>(nullptr));
    return true;
}

// Reads the group from the first partition before the loop and ignores this.
bool NiD3DGeometryGroupManager::RemoveSkinPartitionFromGroup(NiSkinPartition* pkSkinPartition) {
    NiSkinPartition::Partition* pkPartition = pkSkinPartition->m_pkPartitions;
    NiGeometryGroup* pkGroup = pkPartition->m_pkBuffData->m_pkGeometryGroup;
    bool bRemoved = false;
    for (uint32_t i = pkSkinPartition->m_uiPartitions; i; --i, ++pkPartition) {
        if (pkPartition->m_pkBuffData) {
            pkPartition->m_pkBuffData->m_pkGeometryGroup->RemoveObject(pkPartition);
            bRemoved = true;
        }
    }
    if (bRemoved && !pkGroup->m_uiRefCount) {
        pkGroup->Purge();
    }
    return bRemoved;
}

void NiD3DGeometryGroupManager::SetHWSkinning(bool bHWSkinning) {
    m_bHWSkinning = (bool)((uint8_t)bHWSkinning);
}

NiD3DGeometryGroupManager*
NiD3DGeometryGroupManager::Create(IDirect3DDevice9* pkD3DDevice, NiDX9VertexBufferManager* pkVBManager) {
    NiD3DGeometryGroupManager* pkManager =
        static_cast<NiD3DGeometryGroupManager*>(NiMemObject::operator new(sizeof(NiD3DGeometryGroupManager)));
    if (pkManager) {
        ::new (pkManager) NiD3DGeometryGroupManager;
    }
    else {
        pkManager = nullptr;
    }
    pkManager->m_pkD3DDevice = pkD3DDevice;
    pkD3DDevice->AddRef();
    pkManager->m_pkVBManager = pkVBManager;
    return pkManager;
}
