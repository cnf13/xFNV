#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryGroup.h"

class NiVBDynamicSet;

// The geometry group whose vertex buffers are refilled every frame. Only some of its members are declared.
class NiDynamicGeometryGroup : public NiGeometryGroup {
public:
	NiDynamicGeometryGroup();
	~NiDynamicGeometryGroup() override;

	void AddObject(NiScreenTexture* pkScreenTexture) override;
	// The remaining slots.
	void Purge() override;
	void AddObject(NiGeometryData* apData, NiSkinInstance* apSkinInstance,
		NiSkinPartition::Partition* apPartition) override;
	void RemoveObject(NiScreenTexture* apScreenTexture) override;
	void RemoveObject(NiSkinPartition::Partition* apPartition) override;
	void RemoveObject(NiGeometryData* apData) override;
	NiVBChip* CreateChip(NiGeometryBufferData* pkBuffData, uint32_t uiStream) override;
	void ReleaseChip(NiGeometryBufferData* pkBuffData, uint32_t uiStream) override;
	bool IsDynamic() override;

	// The shared group, made on first use.
	static NiGeometryGroup* Create();

	NiTPointerMap<uint32_t, NiVBDynamicSet*> m_kSets;
	NiTPointerMap<uint32_t, NiVBChip*> m_kChips;
	NiTPrimitiveArray<NiVBDynamicSet*> m_kMultiStreamSets;
	NiTPrimitiveArray<NiVBChip*> m_kMultiStreamChips;
	uint32_t m_uiNextIndex;

	static inline NiDynamicGeometryGroup* ms_pkGroup;
};
