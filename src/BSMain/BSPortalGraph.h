#pragma once

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSSimpleArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Reconstruction.hpp"

class BSMultiBoundRoom;
class BSOcclusionPlane;
class BSPortal;

// The occlusion and room/portal state of one cell's scene graph.
class BSPortalGraph : public NiRefObject {
public:
	BSPortalGraph();
	~BSPortalGraph() override;

	void SetRoomRoot(BSMultiBoundRoom* apRoom);
	void SetPortalNodeRoot(NiNode* apNode);
	NiNode* GetRootNode() const;
	NiTPointerList<NiPointer<BSMultiBoundRoom>>* GetMBRoomAccumList();
	void ResetRoomAccumList();
	const NiTPointerList<NiPointer<BSMultiBoundRoom>>& QRooms() const;
	BSMultiBoundRoom* QRoomRoot();
	NiTPointerList<BSOcclusionPlane*>& QOccluderList();
	NiTPointerList<BSPortal*>& QPortalList();
	uint32_t GetAlwaysRenderArrayCount() const;
	NiAVObject* GetAlwaysRenderAt(uint32_t i) const;
	void ProcessQueue();
	void AddRoomToAccumList(BSMultiBoundRoom* apRoom);
	// Queues pkChild to join the always-rendered children.
	void AttachAlwaysRenderChild(NiAVObject* pkChild);
	// Queues pkChild to leave the always-rendered children.
	void DetachAlwaysRenderChild(NiAVObject* pkChild);
	// Adds apOccluder to the graph's occluder list.
	void AddOccluder(BSOcclusionPlane* apOccluder);
	void AddUnboundNode(NiNode* apNode);
	// Adds apRoom and apPortal to the graph's room and portal lists.
	void AddRoom(BSMultiBoundRoom* apRoom);
	void AddPortal(BSPortal* apPortal);
	void AttachQAlwaysRenderChild(NiAVObject* pkChild);
	void DetachQAlwaysRenderChild(NiAVObject* pkChild);
	void RemoveAllOccluders();
	void RemoveOccluder(BSOcclusionPlane* apOccluder);
	void RemoveRoom(BSMultiBoundRoom* apRoom);
	void RemovePortal(BSPortal* apPortal);
	void RemoveAllAlwaysRenderChildren();
	void RemoveUnboundNode(NiNode* apNode);

	// A pointer list: the exe walks it with NiTPointerListBase::GetNext (0x57CBE0, HighActorCuller::Operate) and
	// empties it with NiTPointerList<T*>::RemoveAll (0x4ED900), as BSMultiBoundRoom's list.
	NiTPointerList<BSOcclusionPlane*> lOccluderList;
	NiTPointerList<BSPortal*> lPortalList;
	// The exe keeps NiPointer elements here: AddRoom adds through AddTail 0xA5A310 taking a NiPointer, RemoveRoom
	// removes through Remove 0xC5BDA0, the destructor empties it through RemoveAll 0x4A4650, and QRooms (0x891170,
	// this + 0x20) returns it as this type.
	NiTPointerList<NiPointer<BSMultiBoundRoom>> lMultiBoundRoomList;
	NiPointer<NiObject> spMultiBoundRoomRoot;
	NiTPointerList<NiPointer<BSMultiBoundRoom>> lMultiBoundRoomAccumList;
	// IDA: NiTArray_NiAVObjectPtr; the constructor (0xC5A9D0) installs NiTObjectArray<NiPointer<NiAVObject>>'s vtable.
	NiTObjectArray<NiPointer<NiAVObject>> m_kAlwaysRenderChildren;
	NiPointer<NiNode> spPortalNodeRoot;
	NiTPointerList<NiPointer<NiAVObject>> kAttachAlwaysRenderQueue;
	NiTPointerList<NiPointer<NiAVObject>> kDetachAlwaysRenderQueue;

	BSSimpleArray<NiPointer<NiNode>, 1024> kUnboundNodes;

	// The PDB's BSCriticalSection: the exe constructs it at start-up (0xFBC7C0) and deletes it at exit (0xFDDCD0).
	static BSCriticalSection kAlwaysQueueCrit;
};

inline constexpr fnvd::Vtable BSPortalGraph_vftable{};
