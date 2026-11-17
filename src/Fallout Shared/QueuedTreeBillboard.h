#pragma once

#include "Fallout Shared/ModelLoader.h"

struct TREE_BILLBOARD_DATA;

// A queued load of a tree's billboard texture; once loaded, its post-processing builds the billboard and adds it to the
// distant LOD.
class QueuedTreeBillboard : public QueuedTexture {
public:
    QueuedTreeBillboard(const char* apFileName, IO_TASK_PRIORITY aePriority, TREE_BILLBOARD_DATA* apBillboardData);

    void Finish() override;
    bool GetDescription(char* apDescription, uint32_t aiBufferSize) override;
    void PostProcess() override;
    ~QueuedTreeBillboard() override;

protected:
    // Builds the billboard once the texture is loaded.
    virtual void CreateBillboard();

public:
    TREE_BILLBOARD_DATA* pBillboardData;
    uint8_t padding3C[4];
};
