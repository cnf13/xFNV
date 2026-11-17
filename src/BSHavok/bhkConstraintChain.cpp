#include "BSHavok/bhkConstraintChain.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/ahkpWorld.h"
#include "BSHavok/bhkBallSocketConstraintChain.h"
#include "BSHavok/bhkConstraint.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/hkpConstraintChainData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/hkpConstraintChainInstance.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

NiRTTI bhkConstraintChain::ms_RTTI("bhkConstraintChain", &bhkConstraint::ms_RTTI);

void bhkConstraintChain::LoadBinary(NiStream& arStream) {
	bhkConstraint::LoadBinary(arStream);
}

// The cinfo's entity array gets one entry per link ID: the Havok entity each linked wrapper holds. The exe's
// cleanup-only C++ EH frame is left out, and so is its null test of the cinfo it converts from CreateCinfo's
// reference (a reference is never null).
void bhkConstraintChain::LinkObject(NiStream& arStream) {
	TLSData* pTLS = GetThreadTLSData();
	MEM_CONTEXT eMemContext = pTLS->eMemContext;
	pTLS->eMemContext = MEM_CONTEXT_HAVOK;
	uint32_t uiNumLinks = arStream.GetNumberOfLinkIDs();
	bool bCreated;
	hkConstraintChainCinfo* pInfo = static_cast<hkConstraintChainCinfo*>(&CreateCinfo(bCreated));
	hkArray<hkpEntity*>& rEntities = pInfo->EntitiesA;
	hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
	int32_t iCapacity = static_cast<int32_t>(rEntities.m_capacityAndFlags & hkArrayBase<hkpEntity*>::CAPACITY_MASK);
	if (iCapacity < static_cast<int32_t>(uiNumLinks)) {
		int32_t iReserve = 2 * iCapacity;
		if (static_cast<int32_t>(uiNumLinks) >= iReserve) {
			iReserve = static_cast<int32_t>(uiNumLinks);
		}
		hkArrayUtil::_reserve(pHeap, &rEntities, iReserve, sizeof(hkpEntity*));
	}
	rEntities.m_size = uiNumLinks;
	for (uint32_t i = 0; i < uiNumLinks; ++i) {
		bhkRefObject* pObject = static_cast<bhkRefObject*>(arStream.GetObjectFromLinkID());
		rEntities.m_data[i] = static_cast<hkpEntity*>(pObject->phkObject);
	}
	bhkConstraint::LinkObject(arStream);
	pTLS->eMemContext = eMemContext;
}

// The exe tests this for null before it reads the wrapped chain instance, and calls a null entity's wrapper.
bool bhkConstraintChain::RegisterStreamables(NiStream& arStream) {
	hkpConstraintChainInstance* pInstance = this ? static_cast<hkpConstraintChainInstance*>(phkObject) : nullptr;
	for (int32_t i = 0; i < pInstance->m_chainedEntities.m_size; ++i) {
		hkpEntity* pEntity = pInstance->m_chainedEntities.m_data[i];
		bhkWorldObject* pWrapper = pEntity ? pEntity->getUserData() : nullptr;
		pWrapper->RegisterStreamables(arStream);
	}
	return bhkConstraint::RegisterStreamables(arStream);
}

void bhkConstraintChain::SaveBinary(NiStream& arStream) {
	bhkConstraint::SaveBinary(arStream);
}

// The exe tests this and apObject for null before it reads their wrapped chain instances.
bool bhkConstraintChain::IsEqual(NiObject* apObject) {
	bool bres = bhkConstraint::IsEqual(apObject);
	if (bres) {
		hkpConstraintChainInstance* pInstance = this ? static_cast<hkpConstraintChainInstance*>(phkObject) : nullptr;
		hkpConstraintChainInstance* pOther = apObject
			? static_cast<hkpConstraintChainInstance*>(static_cast<bhkConstraintChain*>(apObject)->phkObject)
			: nullptr;
		int32_t iCount = pInstance->m_chainedEntities.m_size;
		bres &= iCount == pOther->m_chainedEntities.m_size;
		for (int32_t i = 0; i < iCount; ++i) {
			if (!bres) {
				break;
			}
			bres &= pInstance->m_chainedEntities.m_data[i] == pOther->m_chainedEntities.m_data[i];
		}
	}
	return bres;
}

// Adds the chain's wrapped constraint instance to apWorld, moving it out of another world first; false when the
// chain wraps nothing, apWorld is null, or apWorld is already the instance's world. The exe's EH frame (scope table
// 0xf34838) is cleanup-only and left out.
bool bhkConstraintChain::Add(bhkWorld* apWorld) {
	// The exe inlines the heap context's save and restore and reads the thread's TLS block once, before the calls.
	int32_t& iMemContext = GetThreadMemContext();
	int32_t iSavedMemContext = iMemContext;
	iMemContext = MEM_CONTEXT_HAVOK;

	// The exe null-checks this before it reads the field.
	hkpConstraintInstance* pkConstraint;
	if (this)
		pkConstraint = static_cast<hkpConstraintInstance*>(phkObject);
	else
		pkConstraint = nullptr;

	bool bAdded = false;
	if (pkConstraint) {
		if (apWorld) {
			// Moving between worlds: leave the old one, add the entities to the new one, then the constraint.
			if (GethkWorld() != apWorld->GethkWorld()) {
				Remove();
				ProcessEntities(apWorld, true);
				apWorld->Add(pkConstraint);
				bAdded = true;
			}
		} else {
			Remove();
		}
	}

	iMemContext = iSavedMemContext;
	return bAdded;
}

// The bhkWorld is the Havok world's user data. The exe's cleanup-only C++ EH frame is left out.
bool bhkConstraintChain::Remove() {
	TLSData* pTLS = GetThreadTLSData();
	MEM_CONTEXT eMemContext = pTLS->eMemContext;
	pTLS->eMemContext = MEM_CONTEXT_HAVOK;
	bhkWorld* pbw = nullptr;
	const hkpWorld* phkWorld = GethkWorld();
	if (phkWorld) {
		pbw = static_cast<bhkWorld*>(static_cast<const ahkpWorld*>(phkWorld)->m_userData);
	}
	if (pbw) {
		// RemoveConstraint returns 0 or 1, which the exe returns as it is.
		bool bres = pbw->RemoveConstraint(static_cast<hkpConstraintInstance*>(phkObject)) != 0;
		ProcessEntities(pbw, false);
		pTLS->eMemContext = eMemContext;
		return bres;
	}
	pTLS->eMemContext = eMemContext;
	return false;
}

// The exe inlines the heap-context guard: it saves the thread's context, charges this scope to Havok's and restores
// the saved one on every path. The cinfo goes through its deleting destructor, the first slot of its vtable, with the
// scalar delete flag; the exe's cleanup-only C++ EH frame is left out.
void bhkConstraintChain::KillCinfo(bool abCreated) {
	AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
	if (abCreated) {
		if (pInfo) {
			delete static_cast<hkConstraintChainCinfo*>(pInfo);
		}
		pInfo = nullptr;
	}
}

uint32_t bhkConstraintChain::QCinfoSize() const {
	return (uint32_t)32;
}

// The chain instance over the creation info's data, with each of its entities added, handed to SethkObject (which
// takes its own reference), then the creation info is dropped. The exe's cleanup-only EH frame around the
// constructor is left out. It converts the reference to the cinfo object with a null test; a reference cannot be
// null, so src leaves the test out.
void bhkConstraintChain::Init(const hkSerializableCinfo& arInfo) {
	AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
	const hkConstraintChainCinfo* pChainInfo = static_cast<const hkConstraintChainCinfo*>(&arInfo);
	hkpConstraintData* pData = pChainInfo->pData;
	void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpConstraintChainInstance));
	// EXE-INLINED none: hkReferencedObject::operator new
	static_cast<hkReferencedObject*>(pMemory)->m_memSizeAndFlags = sizeof(hkpConstraintChainInstance);
	hkpConstraintChainInstance* pInstance =
		::new (pMemory) hkpConstraintChainInstance(static_cast<hkpConstraintChainData*>(pData));
	for (int32_t i = 0; i < pChainInfo->EntitiesA.m_size; ++i) {
		pInstance->addEntity(pChainInfo->EntitiesA.m_data[i]);
	}
	SethkObject(pInstance);
	pInstance->removeReference();
	PostInit(arInfo);
}

// The exe tests this for null before it reads the wrapped chain instance. The exe's cleanup-only C++ EH frame is
// left out.
bool bhkConstraintChain::ProcessEntities(bhkWorld* apWorld, bool abAdd) {
	TLSData* pTLS = GetThreadTLSData();
	MEM_CONTEXT eMemContext = pTLS->eMemContext;
	pTLS->eMemContext = MEM_CONTEXT_HAVOK;
	bool bResult = false;
	hkpConstraintChainInstance* pInstance = this ? static_cast<hkpConstraintChainInstance*>(phkObject) : nullptr;
	for (int32_t i = 0; i < pInstance->m_chainedEntities.m_size; ++i) {
		hkpEntity* pEntity = pInstance->m_chainedEntities.m_data[i];
		if (abAdd) {
			bhkWorldObject* pe;
			if (pEntity && (pe = pEntity->getUserData()) != nullptr) {
				hkpWorldObject* pObject = static_cast<hkpWorldObject*>(pe->phkObject);
				bool bInWorld = false;
				if (pObject) {
					pe->MarkRead();
					bInWorld = pObject->isAddedToWorld();
					pe->UnMarkRead();
				}
				if (!bInWorld) {
					// Take entity A's collision group (the filter info's upper half).
					pEntity->m_collidable.m_broadPhaseHandle.m_collisionFilterInfo =
						static_cast<uint16_t>(pEntity->m_collidable.m_broadPhaseHandle.m_collisionFilterInfo) |
						(GethkEntityA()->m_collidable.m_broadPhaseHandle.m_collisionFilterInfo & 0xFFFF0000u);
				}
				if (!apWorld->iBatchAdd) {
					pe->Add(apWorld);
					if (!pe->GethkWorld() && (pe->iFlags & 0x30) == 0) {
						char acBuffer[512];
						{
							FNVD_CONSTRUCTED_LOCAL(hkOstream, kStream, &hkOstream::Destruct, acBuffer, 512, true);
							kStream << "A Constraint is being added to the world before its target bodies have been "
									   "added. Did an bad actor skeleton just switch from keyframed?\n";
							hkError::s_instance->message(hkError::MESSAGE_WARNING, 0x234F24FC, acBuffer,
								"D:\\_Fallout3\\Platforms\\Common\\Code\\BSHavok\\bhkConstraintChain.cpp", 360);
						}
						pe->ForceAdd(apWorld);
					}
				}
			} else {
				// No wrapper: attach the constraint to the world's fixed body.
				bResult = true;
				const hkpWorld* pWorld = apWorld->GethkWorld();
				hkpRigidBody* pFixed = nullptr;
				if (pWorld) {
					pFixed = pWorld->m_fixedRigidBody;
				}
				SetEntityB(pFixed);
			}
		} else {
			// The entity is the world's fixed body: unhook it.
			const hkpWorld* pWorld = apWorld->GethkWorld();
			hkpRigidBody* pFixed = nullptr;
			if (pWorld) {
				pFixed = pWorld->m_fixedRigidBody;
			}
			if (pEntity == pFixed) {
				SetEntityB(nullptr);
			}
		}
	}
	pTLS->eMemContext = eMemContext;
	return bResult;
}

// Puts the clone's chained entities in the creation info: each entity's bhkWorldObject is shared from the cloning
// process or cloned, and its Havok entity replaces the original's; then bhkConstraint's members (thiscall, ret 8,
// 0xD53A80). The exe's cleanup-only C++ EH frame is left out.
void bhkConstraintChain::CopyMembers(bhkSerializable* pDest, NiCloningProcess& arCloneData) {
	TLSData* pTLSData = GetThreadTLSData();
	MEM_CONTEXT eMemContext = pTLSData->eMemContext;
	pTLSData->eMemContext = MEM_CONTEXT_HAVOK;
	bool bCreated;
	// The exe tests the result for null before stepping back to the chain info, as a cast from a pointer does.
	hkSerializableCinfo* pBaseInfo = &CreateCinfo(bCreated);
	hkConstraintChainCinfo* pInfo = static_cast<hkConstraintChainCinfo*>(pBaseInfo);

	// EXE-INLINED none: hkArray<bhkWorldObject*>::hkArray
	FNVD_INLINED_LOCAL(hkArray<bhkWorldObject*>, kEntities, std::nullopt, std::nullopt);
	kEntities.m_data = nullptr;
	kEntities.m_size = 0;
	kEntities.m_capacityAndFlags = static_cast<int32_t>(hkArrayBase<bhkWorldObject*>::DONT_DEALLOCATE_FLAG);
	kEntities.setSize(pInfo->EntitiesA.m_size);
	for (int32_t i = 0; i < pInfo->EntitiesA.m_size; ++i) {
		hkpEntity* pEntity = pInfo->EntitiesA.m_data[i];
		kEntities.m_data[i] = pEntity ? pEntity->m_userData : nullptr;
	}

	pInfo->EntitiesA.clear();
	pInfo->EntitiesA.setSize(kEntities.m_size);
	for (int32_t i = 0; i < kEntities.m_size; ++i) {
		NiTPointerMap<NiObject*, NiObject*>* pCloneMap = arCloneData.m_pkCloneMap;
		NiObject* pkEntity = kEntities.m_data[i];
		// EXE-INLINED 0xA5D3F0
		// EXE-INLINED 0xA53C30
		NiTMapItem<NiObject*, NiObject*>* pkItem = pCloneMap->m_ppkHashTable[pCloneMap->KeyToHashIndex(pkEntity)];
		while (pkItem && !pCloneMap->IsKeysEqual(pkEntity, pkItem->m_key)) {
			pkItem = pkItem->m_pkNext;
		}
		NiObject* pkClone;
		if (pkItem) {
			pkClone = pkItem->m_val;
		} else {
			pkClone = static_cast<NiObject*>(kEntities.m_data[i])->CreateClone(arCloneData);
		}
		pInfo->EntitiesA.m_data[i] = static_cast<hkpEntity*>(static_cast<bhkWorldObject*>(pkClone)->phkObject);
	}

	CopyCloneMembers(pDest, arCloneData);
	// EXE-INLINED none: hkArray<bhkWorldObject*>::~hkArray (the SDK's releaseMemory: no stores after the free)
	hkMemoryAllocator& rHeap = hkMemoryRouter::getInstance().heap();
	kEntities.m_size = 0;
	if ((kEntities.m_capacityAndFlags & hkArrayBase<bhkWorldObject*>::DONT_DEALLOCATE_FLAG) == 0) {
		rHeap._bufFree<bhkWorldObject*>(kEntities.m_data, kEntities.getCapacity());
	}
	pTLSData->eMemContext = eMemContext;
}

// bhkConstraint's members, then the chain instance's entities, with the array grown the SDK's way (to the larger of
// the count and double the capacity). The exe tests this for null before reading the wrapped object, a test clang
// drops; the count is read once, the source array on every pass.
void bhkConstraintChain::CopyCinfo(hkConstraintChainCinfo* arInfo) {
	bhkConstraint::CopyCinfo(*arInfo);
	hkpConstraintChainInstance* pInstance = static_cast<hkpConstraintChainInstance*>(phkObject);
	if (!pInstance) {
		return;
	}
	int32_t iNumEntities = pInstance->m_chainedEntities.m_size;
	hkArray<hkpEntity*>& rEntities = arInfo->EntitiesA;
	hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
	int32_t iCapacity = static_cast<int32_t>(rEntities.m_capacityAndFlags & hkArrayBase<hkpEntity*>::CAPACITY_MASK);
	if (iCapacity < iNumEntities) {
		int32_t iReserve = 2 * iCapacity;
		if (iNumEntities >= iReserve) {
			iReserve = iNumEntities;
		}
		hkArrayUtil::_reserve(pHeap, &rEntities, iReserve, sizeof(hkpEntity*));
	}
	rEntities.m_size = static_cast<uint32_t>(iNumEntities);
	for (int32_t i = 0; i < pInstance->m_chainedEntities.m_size; ++i) {
		rEntities.m_data[i] = pInstance->m_chainedEntities.m_data[i];
	}
}

// Reads the entities' link IDs, then hkConstraintCinfo's part. The exe has no vtable of this class:
// hkBallSocketChainCinfo's LoadBinary calls it directly.
void hkConstraintChainCinfo::LoadBinary(NiStream& arStream) {
	arStream.ReadMultipleLinkIDs();
	hkConstraintCinfo::LoadBinary(arStream);
}

// Writes the entity count and a link ID per entity (the bhkWorldObject each Havok entity holds as its user data,
// null for none), then hkConstraintCinfo's part. The exe has no vtable of this class: hkBallSocketChainCinfo's
// SaveBinary calls it directly (IDA: hkConstraintCinfo::sub_D53360).
void hkConstraintChainCinfo::SaveBinary(NiStream& arStream) {
	NiBinaryStream* pOstr = arStream.Ostr();
	uint32_t uiCount = static_cast<uint32_t>(EntitiesA.m_size);
	NiBinaryStream::NIBINARYSTREAMWRITEFN pfnWrite = pOstr->m_pfnWrite;
	uint32_t uiSize = 4;
	pOstr->m_uiAbsoluteCurrentPos += pfnWrite(pOstr, &uiCount, 4, &uiSize, 1);
	for (uint32_t i = 0; i < uiCount; ++i) {
		hkpEntity* pEntity = EntitiesA.m_data[i];
		arStream.SaveLinkID(pEntity ? pEntity->getUserData() : nullptr);
	}
	hkConstraintCinfo::SaveBinary(arStream);
}
