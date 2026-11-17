#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/BaseFormComponent.h"

class ContainerItemExtra;
class InventoryChanges;
class TESActorBase;
class TESBoundObject;
class TESForm;
class TESObject;
class TESObjectARMO;
class TESObjectREFR;
class TESObjectWEAP;

// Defined in Fallout Shared/TESBipedModelForm.h, which includes this header.
enum BIPED_OBJECT : int32_t;

// One container entry as the plugin file stores it (CNTO): the object's form ID and the count.
struct CONTAINER_OBJECT_FILE {
    // Byte-swaps the object ID and the count.
    void Endian();

    uint32_t iObjectID;
    int32_t iCount;
};

// One entry of a base container: the count and the object, plus optional per-item extra data.
class ContainerObject {
public:
    ContainerObject();
    ~ContainerObject();

    // Resolves the form ID loaded into pObj for apOwner, warning when no bound object has it.
    void InitItem(TESForm* apOwner);
    void Copy(ContainerObject* apCopy);
    bool Compare(ContainerObject* apCompare);
    void Save();

    // The deleting destructor frees through ourFree.
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    int32_t i_count;
    TESBoundObject* pObj;
    ContainerItemExtra* pItemExtra;
};

// The items a form carries.
class TESContainer : public BaseFormComponent {
public:
    TESContainer();
    ~TESContainer();

    void InitializeDataComponent() override {}
    void ClearDataComponent() override;
    void CopyComponent(BaseFormComponent* apCopyFC) override;
    bool CompareComponent(BaseFormComponent* apCompareFC) override;

    // Saves the container.
    void Save();
    // The weight of the container's items; hardcore mode weighs them differently.
    float GetContainerWeight(bool abIsHardcore);
    // Resolves the objects loaded for apOwner.
    void InitItem(TESForm* apOwner);
    // Whether an entry holds the object.
    bool HasObject(TESObject* pObj);
    // The summed count of the object's entries.
    int32_t GetObjectCount(TESObject* apObject);
    BSSimpleList<ContainerObject*>* GetObjectList();
    // Removes apCObj from the list and frees it.
    void RemoveContainerObject(ContainerObject* apCObj);
    // Gives every entry the health fraction.
    void ApplyHealthPercentToContainer(float afHealthPerc);
    // Adds the entries to the changes as instances of leveled item aiLevItemIndex.
    void AddLevItemContainerToChanges(int32_t aiLevItemIndex, InventoryChanges* apChanges);
    // Adds a new entry for the file entry apCOFile at the head of the list (the object stays a form ID until
    // InitItem); null when apCOFile is null.
    ContainerObject* AddObject(CONTAINER_OBJECT_FILE* apCOFile);
    // Adds aiCount of apObj with apExtra.
    ContainerObject* AddObject(TESObject* apObj, int32_t aiCount, ContainerItemExtra* apExtra);
    // Gives the entries to apRef, without the pick-up messages with abSilent.
    void AddContainerToReference(TESObjectREFR* apRef, bool abSilent);
    // Multiplies every entry's count by aiMult.
    void MultiplyContainerCounts(int32_t aiMult);
    // Adds apAddCont's entries to this container.
    void AddContainerContents(TESContainer* apAddCont);
    // Frees every entry and empties the list.
    void FreeObjectList();
    void AddDuplicateObject(ContainerObject* pCObj);
    TESObjectWEAP* GetBestWeapon(TESActorBase* apActorBase);
    // The armor entry filling slot aeBodySlot that apActorBase desires most; null when none
    // does or aeBodySlot is BO_NONE.
    TESObjectARMO* GetBestArmor(TESActorBase* apActorBase, BIPED_OBJECT aeBodySlot);
    BSSimpleList<ContainerObject*>* GetFilteredList();

    // Whether a container can hold forms of the given form type.
    static bool ContainerCanHoldType(uint8_t aType);
    static bool IsGold(TESObject* pObj);

    BSSimpleList<ContainerObject*> objectList;
};
