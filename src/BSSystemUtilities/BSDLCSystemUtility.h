#pragma once

#include "BSCore/BSSimpleList.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class BSContentPackage;
// The Xbox marketplace enumeration contexts (PDB structs), held by pointer only.
struct OfferCountContext;
struct OfferListContext;
struct _XCONTENT_DATA;

// The downloadable content utility: finds, validates and mounts content packages.
class BSDLCSystemUtility {
public:
	// Called for each package MountDLC mounts (which, last, valid, package).
	using MountCallback = void (*)(uint32_t, uint32_t, bool, const BSContentPackage*);

	BSDLCSystemUtility();
	~BSDLCSystemUtility();

	// The per-frame update; the body does nothing.
	void Idle();
	// Mounts the content of player anPlayerIndex, calling cb for each package.
	uint32_t MountDLC(uint32_t anPlayerIndex, MountCallback cb);
	// Opens the Steam overlay's store page.
	void ShowMarketplace(uint32_t anPlayerIndex);
	// Adds a package for each installed DLC of the table to m_validDLC.
	uint32_t LoadDLC(uint32_t anPlayerIndex, uint32_t anDevice);
	void UnloadDLC(bool unmount);
	bool GetOfferCounts(uint32_t playerIndex, void (*callback)(uint32_t, uint32_t, void*), void* param);
	// The PC bodies do nothing; IsContentCorrupt returns false.
	void CheckForCorruptContentAndDisplayMessage();
	bool IsContentCorrupt(uint32_t anPlayerIndex, const _XCONTENT_DATA& aData);
	void doIdleOfferCounts();
	void doIdleMount();

	// The utility, created on first use.
	static BSDLCSystemUtility* GetSingleton();
	static void operator delete(void* apMemory);

	// The PDB's members. The constructor (0xAF27C0) clears the first three dwords and the lists; the destructor
	// (0xAF2850) empties m_mountedFiles, m_corruptDLC and m_validDLC.
	uint32_t m_state;
	OfferCountContext* m_pOfferCountCtx;
	OfferListContext* m_pOfferListCtx;
	BSSimpleList<BSContentPackage*> m_validDLC;
	BSSimpleList<BSContentPackage*> m_corruptDLC;
	BSSimpleList<char*> m_mountedFiles;

	// IDA name; not in the PDB (PDB: m_pInstance).
	static inline BSDLCSystemUtility* pSingleton;
};
