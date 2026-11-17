#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryGroup.h"

#include <stdint.h>

class NiVBSet;

// The geometry group for static geometry: vertex buffer sets keyed by vertex stride and flags.
class NiStaticGeometryGroup : public NiGeometryGroup {
public:
	enum : int32_t {
		INDEX_MASK_STRIDE = -0x7FFFFFFF - 1,
		INDEX_MASK_SOFTWAREVP = 0x40000000,
	};

	NiStaticGeometryGroup();

	void Purge() override;
	void AddObject(NiScreenTexture* pkScreenTexture) override;
	void AddObject(NiGeometryData* pkData, NiSkinInstance* pkSkinInstance,
		NiSkinPartition::Partition* pkPartition) override;
	void RemoveObject(NiScreenTexture* apScreenTexture) override;
	void RemoveObject(NiSkinPartition::Partition* apPartition) override;
	void RemoveObject(NiGeometryData* apData) override;
	NiVBChip* CreateChip(NiGeometryBufferData* pkBuffData, uint32_t uiStream) override;
	void ReleaseChip(NiGeometryBufferData* pkBuffData, uint32_t uiStream) override;
	bool IsDynamic() override;
	~NiStaticGeometryGroup() override;

	static NiGeometryGroup* Create();

	NiTPointerMap<uint32_t, NiVBSet*> m_kSets;
};
