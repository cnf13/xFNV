#pragma once

#include "BSMain/IOManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Reconstruction.hpp"

class TESObjectREFR;

// The IO task that tests whether a reference's 3D lies within a multibound, then attaches it under the multibound's
// node. Only the members the reconstructed code calls are declared.
class CheckWithinMultiBoundTask : public IOTask {
public:
	// The references the task attaches (IDA's checkWithinMultiBoundRefs; IDA types key and value as DWORD).
	using AttachMapType = NiTMap<TESObjectREFR*, TESObjectREFR*>;

	CheckWithinMultiBoundTask(TESObjectREFR* apBoundRef, NiAVObject* ap3D);
	// Declared so the NiPointer member of the incomplete BSMultiBound is not destroyed here.
	~CheckWithinMultiBoundTask() override;

	void Run() override;
	void Finish() override;
	void PostProcess() override;

	// The exe's deleting destructor (0x54AB30) frees through ourFree.
	static void operator delete(void* apMemory);
	// Drops apMoribundRef from AttachMap, so the task does not attach it.
	static void CancelAttachment(TESObjectREFR* apMoribundRef);
	// Moves each reference in AttachMap whose 3D is not already under its cell's dynamic node to its bound
	// reference's multibound node, then empties AttachMap.
	static void DoAttachments();

	TESObjectREFR* pBoundRef;
	NiPointer<NiAVObject> sp3D;
	NiPointer<BSMultiBound> spBound;
	bool bWithin;

	static AttachMapType AttachMap;
};
inline constexpr fnvd::Vtable CheckWithinMultiBoundTask_vftable{};
