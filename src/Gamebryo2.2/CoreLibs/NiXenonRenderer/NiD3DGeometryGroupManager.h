#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryGroupManager.h"
#include "Reconstruction.hpp"

class NiDX9VertexBufferManager;
class NiGeometryData;
class NiGeometryGroup;
class NiScreenTexture;
class NiSkinInstance;
class NiSkinPartition;

#include "Gamebryo2.2/CoreLibs/NiMain/NiTSet.h"

// The Direct3D geometry group manager: places geometry data (or its skin partitions) in vertex buffer groups.
class NiD3DGeometryGroupManager : public NiGeometryGroupManager {
public:
	NiD3DGeometryGroupManager();
	~NiD3DGeometryGroupManager() override;

	NiGeometryGroup* CreateGroup(NiGeometryGroupManager::RepackingBehavior eBehavior) override;
	bool DestroyGroup(NiGeometryGroup* pkGroup) override;
	// Whether no object holds the group.
	bool IsGroupEmpty(NiGeometryGroup* pkGroup) const override;
	// Adds pkData with the manager's hardware skinning setting.
	bool AddObjectToGroup(NiGeometryGroup* pkGroup, NiGeometryData* pkData, NiSkinInstance* pkSkinInstance) override;
	bool AddObjectToGroup(NiGeometryGroup* pkGroup, NiScreenTexture* pkScreenTexture) override;
	// Takes pkData (skinned by pkSkinInstance) out of its group.
	bool RemoveObjectFromGroup(NiGeometryData* pkData, NiSkinInstance* pkSkinInstance) override;
	// Takes the screen texture's buffer data out of its group.
	bool RemoveObjectFromGroup(NiScreenTexture* pkScreenTexture) override;

	bool AddObjectToGroup(NiGeometryGroup* apGroup, NiGeometryData* apData, NiSkinInstance* apSkinInstance,
		bool abHWSkinning, uint32_t auiBonesPerPartition, uint32_t auiBonesPerVertex);

	// Takes every partition of apSkinPartition out of the first partition's group; whether any partition had buffer
	// data. The group is purged when no object holds it any more.
	bool RemoveSkinPartitionFromGroup(NiSkinPartition* pkSkinPartition);
	void SetHWSkinning(bool bHWSkinning);

	// Gamebryo's Create: a new manager (its constructor inlined) holding a reference to
	// pkD3DDevice and pkVBManager (cdecl, ret 0, 0xE88EB0; ecx is not read). The PDB names the address after the
	// constructor, which the exe has no out-of-line copy of.
	static NiD3DGeometryGroupManager* Create(IDirect3DDevice9* pkD3DDevice, NiDX9VertexBufferManager* pkVBManager);

	// The exe keeps the groups in a packed set (base, allocated, used).
	NiTPrimitiveSet<NiGeometryGroup*> m_kGroups;
	IDirect3DDevice9* m_pkD3DDevice;
	NiDX9VertexBufferManager* m_pkVBManager;
	bool m_bHWSkinning;
};
inline constexpr fnvd::Vtable NiD3DGeometryGroupManager_vftable{};
