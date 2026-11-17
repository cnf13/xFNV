#pragma once

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/Package.h"
#include "Reconstruction.hpp"

class BGSLoadFormBuffer;
class BGSSaveFormBuffer;
class TESObjectREFR;

// The per-actor data of a running package, in PC vtable order.
class ActorPackageData {
public:
	ActorPackageData();
	virtual ~ActorPackageData();

	// slot 1
	virtual void HandleDeletedReference(TESObjectREFR* apDeletedObj) = 0;
	// slot 2; the base slot holds UnimplementedVirtFunc (0xEC60FA). PACKAGE_TYPE is src's spelling of the PDB's
	// PTYPE.
	virtual PACKAGE_TYPE GetPackageType() = 0;
	virtual void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) = 0;
	virtual void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) = 0;
	virtual void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) = 0;

	static void operator delete(void* apMemory) { ourFree(apMemory); }
	// The factory: the package data of aPackageType.
	static ActorPackageData* CreateActorPackageData(PACKAGE_TYPE aPackageType);
};
inline constexpr fnvd::Vtable ActorPackageData_vftable{};
