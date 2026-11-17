#pragma once

#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"

#include <new>
#include <stdint.h>

class BGSReconstructCellBlocksMap;
class BGSReconstructCellsMap;
class BGSReconstructCellSubBlocksMap;
class BGSReconstructedForms;
class BGSReconstructFormsInFileMap;
class BGSReconstructFormsMap;
class BGSReconstructWorldSpacesMap;
class TESObjectCELL;
class TESWorldSpace;

class BGSReconstructFormsInFileMap : public NiTMap<uint32_t, BGSReconstructFormsMap*> {
public:
    BGSReconstructFormsInFileMap();
    ~BGSReconstructFormsInFileMap() override;

    // Files a cell's or a world space's form in the world spaces map, creating it on first use. aiFormID is passed on
    // to BGSReconstructFormsMap::AddForm as the form ID of a form not loaded (apForm null).
    void AddCellForm(
        TESWorldSpace* apWorldSpace,
        int32_t aiCellX,
        int32_t aiCellY,
        TESForm* apForm,
        uint32_t aiFormID,
        uint32_t aiFlags
    );
    // Files apForm of apCell in the interior cells map, or in the world spaces map when the cell is an exterior one.
    // aiFormID is the form ID passed on to BGSReconstructFormsMap::AddForm for a form not loaded.
    void AddReference(TESObjectCELL* apCell, TESForm* apForm, uint32_t aiFormID, uint32_t aiFlags);
    // Files apForm where its saved type says: a reference by its editor location, a navmesh by its cell, a cell as a
    // reference to itself, a world space nowhere, and any other form in the map of its type.
    void AddForm(TESForm* apForm, uint32_t aiFlags);
    void ReconstructForms(TESFile* apFile, BGSReconstructedForms* apReconstructedForms);

    static void operator delete(void* apMemory);

    BGSReconstructCellBlocksMap* pInteriorCells;
    BGSReconstructWorldSpacesMap* pWorldSpaces;
    uint32_t iCount;
};

// The cells of one sub-block to reconstruct, by cell id. Adds nothing to the map but its destructor.
class BGSReconstructCellSubBlocksMap : public NiTMap<uint32_t, BGSReconstructCellsMap*> {
public:
    BGSReconstructCellSubBlocksMap();
    ~BGSReconstructCellSubBlocksMap() override;

    // aiFormID and aiFlags are passed on to BGSReconstructCellsMap::AddReference.
    void AddReference(TESObjectCELL* arg0, TESForm* apCellOrRef, uint32_t aiFormID, uint32_t aiFlags);
    void AddCellForm(
        TESWorldSpace* apWorldSpace,
        int32_t aiCellX,
        int32_t aiCellY,
        TESForm* apForm,
        uint32_t aiFormID,
        uint32_t aiFlags
    );
    bool GetFormMapFlag(uint32_t aiFlags);

    static void operator delete(void* apMemory);
};

// The flags of a forms map and of a form to reconstruct.
enum RECONSTRUCT_FLAGS : uint32_t {
    RECONSTRUCT_CELL = 0x1,
    RECONSTRUCT_PERSISTENT_CHILDREN = 0x2,
    RECONSTRUCT_TEMP_CHILDREN = 0x4,
    RECONSTRUCT_REVERT_ONLY = 0x8,
    RECONSTRUCT_FLAG_CELL_FOUND = 0x10000,
};

// One form to reconstruct and its flags.
struct ReconstructFormData {
    ReconstructFormData();
    ReconstructFormData(uint32_t aiFlags);
    ReconstructFormData(TESForm* apForm, uint32_t aiFlags);

    // Whether iFlags has any of aiFlag's bits.
    bool GetFlag(uint32_t aiFlags) const;

    TESForm* pForm;
    uint32_t iFlags;
};

// BSSimpleArray<ReconstructFormData, 1024>::_ConstructItems:

// The forms a save rebuilds, in two lists of form data.
class BGSReconstructedForms {
public:
    BGSReconstructedForms();
    ~BGSReconstructedForms();

    // The number of forms in both lists.
    uint32_t GetSize();
    // The aiIndex-th form to reconstruct: the entries of Forms1, then those of Forms2.
    ReconstructFormData* GetFormData(uint32_t aiIndex);
    // The aiIndex-th form to reconstruct itself (its form data's pForm): the entries of Forms1, then those of
    // Forms2.
    TESForm* GetForm(uint32_t aiIndex);
    // Empties both lists and frees their buffers.
    void Clear();
    // Reserves aiSize items in both lists.
    void SetReservedSize(uint32_t aiSize);
    void AddFormData(ReconstructFormData& arFormData);
    // Removes the aiIndex-th entry: of Forms1, else of Forms2; returns 1.
    char RemoveFormAt(uint32_t aiIndex);

    BSSimpleArray<ReconstructFormData, 1024> Forms1;
    BSSimpleArray<ReconstructFormData, 1024> Forms2;
};

// The forms to reconstruct per file: one forms-in-file map for each loaded file, and the total count.
class BGSReconstructFormsInAllFilesMap : public NiTMap<TESFile*, BGSReconstructFormsInFileMap*> {
public:
    BGSReconstructFormsInAllFilesMap();
    ~BGSReconstructFormsInAllFilesMap() override;

    // Constructs every form the map holds.
    void ReconstructForms();
    // The number of constructed forms.
    uint32_t GetReconstructedCount();
    // The aiIndex-th constructed form (its form data's pForm).
    TESForm* GetReconstructedForm(uint32_t aiIndex);
    // Queues apForm for reconstruction with aiFlags.
    void AddForm(TESForm* apForm, uint32_t aiFlags);
    // Queues reference aiRefID of apParentCell for reconstruction with aiFlags.
    void AddReference(TESObjectCELL* apParentCell, uint32_t aiRefID, uint32_t aiFlags);
    // Queues reference aiRefID of cell aiCellX, aiCellY of apWorldSpace for reconstruction with aiFlags.
    void
    AddReference(TESWorldSpace* apWorldSpace, int32_t aiCellX, int32_t aiCellY, uint32_t aiRefID, uint32_t aiFlags);
    TESFile* GetFileForFormID(BSSimpleList<TESFile*>* apFileList, uint32_t aiFormID);
    // Deletes every file's map, then empties the map and the reconstructed forms.
    void Clear();
    // Readies a constructed form: a reference's scale, trees, process and default outfit, an actor base's template
    // data, a navmesh's disabled state.
    void InitReconstructedForm(ReconstructFormData* apFormData);

    static void operator delete(void* apMemory);

    BGSReconstructedForms ReconstructedForms;
    uint32_t iCount;
};

// The cell blocks of a reconstruction: the sub-block map for each block, the persistent cell's forms and whether the
// blocks are exterior.
class BGSReconstructCellBlocksMap : public NiTMap<uint32_t, BGSReconstructCellSubBlocksMap*> {
public:
    BGSReconstructCellBlocksMap(bool abExteriors);
    ~BGSReconstructCellBlocksMap() override;

    // Records apForm of apCell in the block map, or in the persistent cell's map.
    void AddReference(TESObjectCELL* apCell, TESForm* apForm, uint32_t aiFormID, uint32_t aiFlags);
    void AddCellForm(
        TESWorldSpace* apWorldSpace,
        int32_t aiCellX,
        int32_t aiCellY,
        TESForm* apForm,
        uint32_t aiFormID,
        uint32_t aiFlags
    );
    bool GetFormMapFlag(uint32_t aiFlags);

    static void operator delete(void* apMemory);

    BGSReconstructFormsMap* pPersistentCell;
    bool bExteriors;
    uint8_t padding15[3];
};

// The forms of one block to reconstruct, by form ID.
class BGSReconstructFormsMap : public NiTMap<uint32_t, ReconstructFormData> {
public:
    BGSReconstructFormsMap();
    ~BGSReconstructFormsMap() override;

    void SetFlag(uint32_t aiFlags);
    // Whether any of aiFlags is set.
    bool GetFlag(uint32_t aiFlags);
    // Files apForm under auiFormID (apForm's own ID when it has one) with its flags.
    void AddForm(TESForm* apForm, uint32_t aiFormID, uint32_t aiFlags);

    static void operator delete(void* apMemory);

    uint32_t iFlags;
};

// A reconstruct entry's file offset and the forms map read at it.
class OffsetData {
public:
    OffsetData();
    OffsetData(uint32_t aiOffset, BGSReconstructFormsMap* apFormsMap);

    void Clear();
    // Whether the entry has an offset (iOffset not 0).
    bool IsValid();

    uint32_t iOffset;
    BGSReconstructFormsMap* pFormsMap;
};

using BSSimpleArrayOffsetData = BSSimpleArray<OffsetData, 1024>;

// The file offsets of the reconstruct entries, on the scrap heap.
class BGSOffsetDataArray : public BSScrapArray<OffsetData, 1024> {
public:
    BGSOffsetDataArray();
    ~BGSOffsetDataArray();

    // Appends an entry for aiOffset and apFormsMap.
    void AddOffset(uint32_t aiOffset, BGSReconstructFormsMap* apFormsMap);

    static void operator delete(void* apMemory);
};

// The cells of a reconstruction: the forms map for each cell, by form ID.
class BGSReconstructCellsMap : public NiTMap<uint32_t, BGSReconstructFormsMap*> {
public:
    BGSReconstructCellsMap();
    ~BGSReconstructCellsMap() override;

    // Records apForm of apCell, or of its persistent cell, in the map.
    void AddReference(TESObjectCELL* apCell, TESForm* apCellOrRef, uint32_t aiFormID, uint32_t aiFlags);
    // Records apForm for the cell at aiX, aiY.
    void AddCellForm(
        TESWorldSpace* apWorldSpace,
        int32_t aiCellX,
        int32_t aiCellY,
        TESForm* apForm,
        uint32_t aiFormID,
        uint32_t aiFlags
    );
    bool GetFormMapFlag(uint32_t aiFlags);

    static void operator delete(void* apMemory);
};

// The world spaces of a reconstruction: the cell blocks map for each world space, by form ID.
class BGSReconstructWorldSpacesMap : public NiTMap<uint32_t, BGSReconstructCellBlocksMap*> {
public:
    BGSReconstructWorldSpacesMap();
    ~BGSReconstructWorldSpacesMap() override;

    // Files apRef's cell in the cell blocks map of its world space.
    // aiFormID and aiFlags are passed on to BGSReconstructCellBlocksMap::AddReference.
    void AddReference(TESObjectCELL* apCell, TESForm* apRefCell, uint32_t aiFormID, uint32_t aiFlags);
    // Files a cell's or a world space's form in the map.
    void AddCellForm(
        TESWorldSpace* apWorldSpace,
        int32_t aiCellX,
        int32_t aiCellY,
        TESForm* apForm,
        uint32_t aiFormID,
        uint32_t aiFlags
    );

    static void operator delete(void* apMemory);
};
