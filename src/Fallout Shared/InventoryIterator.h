#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Reconstruction.hpp"

class ContainerObject;

// Walks the items of a reference: first the changes made to its inventory, then the objects of its base container
// that no change covers. The PDB's source module is BGSLoadedReferenceCollection.cpp; IDA calls the type
// InventoryChangesIterator and lays TempItemChange out as a base class.
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
