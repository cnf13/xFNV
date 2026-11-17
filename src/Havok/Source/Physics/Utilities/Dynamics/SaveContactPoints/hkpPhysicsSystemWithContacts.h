#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Dynamics/World/hkpPhysicsSystem.h"

#include <stdint.h>

struct hkpSerializedAgentNnEntry;

// Havok SDK signature (hkpUtilities.lib, tied by bytes,reloc).
const void* getVtablehkpPhysicsSystemWithContacts();
void finishLoadedObjecthkpPhysicsSystemWithContacts(void* p, int32_t finishing);

// A physics system that also keeps the saved contacts (agent entries) between its bodies.
class hkpPhysicsSystemWithContacts : public hkpPhysicsSystem {
public:
	hkpPhysicsSystemWithContacts() {}
	hkpPhysicsSystemWithContacts(hkFinishLoadedObjectFlag aFlag) : hkpPhysicsSystem(aFlag), m_contacts(aFlag) {}
	~hkpPhysicsSystemWithContacts() override;

	hkpPhysicsSystem* clone() const override;
	hkBool* hasContacts(hkBool* arResult) override;

	// Adds the contact to m_contacts.
	void addContact(hkpSerializedAgentNnEntry* c);
	// Havok SDK signature (hkpUtilities.lib, tied by bytes).
	void removeContact(int32_t i);
	void copy(const hkpPhysicsSystemWithContacts& toCopy);

	static const hkClass& staticClass();

	hkArray<hkpSerializedAgentNnEntry*> m_contacts;
};

extern hkClass hkpPhysicsSystemWithContactsClass;
extern hkTypeInfo hkpPhysicsSystemWithContactsTypeInfo;
