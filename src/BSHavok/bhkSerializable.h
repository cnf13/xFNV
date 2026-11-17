#pragma once

#include "BSCore/BSCriticalSection.h"
#include "BSHavok/bhkRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

class bhkWorld;
class hkpWorld;

// The empty tag base of every creation info a bhkSerializable is built from; the Havok cinfo it is paired with
// comes first in the object, so a pointer to this base sits after it.
class hkSerializableCinfo {};

// A Havok object that can be saved to and built from the game's data files, holding the creation info it was loaded
// with until the Havok object exists.
class bhkSerializable : public bhkRefObject {
public:
	bhkSerializable();
	~bhkSerializable() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	void LoadBinary(NiStream& arStream) override;
	void LinkObject(NiStream& arStream) override;
	bool RegisterStreamables(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	bool IsEqual(NiObject* apObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	void SethkObject(hkReferencedObject* aphkObject) override;

	// The Havok world the wrapped object is in, null when it is in none.
	virtual const hkpWorld* GethkWorld() const { return nullptr; }
	virtual hkpWorld* GethkWorldRw() { return nullptr; }

	// Adds the wrapped object to the world or removes it from its world; true when it was done.
	virtual bool Add(bhkWorld*) { return false; }
	virtual bool Remove() { return false; }

	// Releases the creation info (or the Havok object when it was already built).
	virtual void KillCinfo(bool abCreated);

	// The size of the class's creation info.
	virtual uint32_t QCinfoSize() const { return 1; }

	virtual int32_t LoadCInfo(NiStream& arStream);

	// Builds the Havok object from the creation info.
	virtual void Init(const hkSerializableCinfo& arInfo) = 0;

	// Makes the creation info from the wrapped Havok object; arCreated tells whether it was newly allocated.
	virtual hkSerializableCinfo& CreateCinfo(bool& arCreated) = 0;

	// Drops the Havok object and the creation info.
	virtual void Kill();

	// Called once the Havok object is built; drops the creation info.
	virtual void PostInit(const hkSerializableCinfo& apCinfo);

	virtual void CopyCinfo(hkSerializableCinfo&) {}

	void CopyMembers(bhkSerializable* pDest, NiCloningProcess& arCloneData);

	// Lock and unlock the engine's shared clone critical section.
	static void CloneCritLock();
	static void CloneCritUnlock();

	hkSerializableCinfo* pInfo;

	static NiRTTI ms_RTTI;
	static BSCriticalSection CloneCrit;
};
