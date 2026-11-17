#pragma once

#include "Fallout Shared/ModelLoader.h"
#include "Reconstruction.hpp"

struct TREE_BILLBOARD_DATA;

// A queued load of a tree's billboard texture; once loaded, its post-processing builds the billboard and adds it to
// the distant LOD. The PDB's source module is ModelLoader.cpp.
class QueuedTreeBillboard : public QueuedTexture {
public:
	QueuedTreeBillboard(const char* apFileName, IO_TASK_PRIORITY aePriority, TREE_BILLBOARD_DATA* apBillboardData);

	void Finish() override;
	bool GetDescription(char* apDescription, uint32_t aiBufferSize) override;
	void PostProcess() override;
	~QueuedTreeBillboard() override;

protected:
	// Builds the billboard once the texture is loaded (vtable slot 12).
	virtual void CreateBillboard();

public:
	TREE_BILLBOARD_DATA* pBillboardData;
	// ModelLoader::QueueTreeBillboard allocates the class with 0x40 bytes; the PDB names no member here.
	uint8_t padding3C[4];
};
inline constexpr fnvd::Vtable QueuedTreeBillboard_vftable{};
