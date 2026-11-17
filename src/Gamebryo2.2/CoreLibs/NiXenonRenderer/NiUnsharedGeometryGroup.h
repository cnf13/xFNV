#pragma once

#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryGroup.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// The geometry group that gives each geometry its own vertex buffers.
// Only the members reconstructed so far are declared. The destructor is declared first so that it stays the key
// function: src emits no vtable for the class.
class NiUnsharedGeometryGroup : public NiGeometryGroup {
public:
	NiUnsharedGeometryGroup();
	~NiUnsharedGeometryGroup() override;

	// Nothing is pooled, so there is nothing to purge.
	void Purge() override {}
	void AddObject(NiGeometryData* apData, NiSkinInstance* apSkinInstance,
		NiSkinPartition::Partition* apPartition) override;
	// Gives the screen texture its own buffer data and adds it to the group, once.
	void AddObject(NiScreenTexture* apScreenTexture) override;
	void RemoveObject(NiGeometryData* apData) override;
	void RemoveObject(NiSkinPartition::Partition* apPartition) override;
	void RemoveObject(NiScreenTexture* apScreenTexture) override;
	NiVBChip* CreateChip(NiGeometryBufferData* pkBuffData, uint32_t uiStream) override;
	void ReleaseChip(NiGeometryBufferData* pkBuffData, uint32_t uiStream) override;
	bool IsDynamic() override;

	static NiGeometryGroup* Create();

	static inline NiUnsharedGeometryGroup* ms_pkGroup;
};
inline constexpr fnvd::Vtable NiUnsharedGeometryGroup_vftable{};
