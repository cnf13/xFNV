#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

class Actor;
class BGSLoadFormBuffer;
class BGSSaveFormBuffer;
class ExtraDataList;
class QueuedFile;
class TESBoundObject;

// An item an actor is waiting to equip or unequip until the files for it are loaded: the items of
// MiddleHighProcess::pListItemstoEquipUnequip, which AddQueuedItem fills and ResolveEquipmentQueue runs. The PDB
// places it in MiddleHighProcess.cpp; the layout comes from the constructor's stores and the PDB's member order.
class QueuedItem {
public:
	QueuedItem();
	~QueuedItem();

	// Saves the item into apSaveGameBuffer.
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);
	// Loads the item from the form buffer.
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
	// Queues the files the item needs before it can be equipped.
	bool QueueFiles(Actor* apActor);
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer);

	static void operator delete(void* apMemory);

	TESBoundObject* pObject;
	ExtraDataList* pExtra;
	int32_t iNumber;
	bool bEquip;
	bool bNow;
	bool bForceEquip;
	bool bPlayPickUpSounds;
	bool bForceUnequip;
	bool bEquipCalling;
	uint8_t padding12[2];
	NiPointer<QueuedFile> spQueuedFiles;
};
