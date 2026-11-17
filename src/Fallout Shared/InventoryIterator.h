#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/InventoryChanges.h"

class ContainerObject;

// Walks the items of a reference: first the changes made to its inventory, then the objects of its base container that
// no change covers.
class InventoryIterator {
public:
	InventoryIterator();
	~InventoryIterator();

	void Reset();

	ItemChange TempItemChange;
	BSSimpleList<ContainerObject*>* pContainerObjectList;
	BSSimpleList<ItemChange*>* pInventoryChangeList;
	bool bFinished;
};
