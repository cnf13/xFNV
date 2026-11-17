#include "Havok/Source/Physics/Utilities/Dynamics/SaveContactPoints/hkpPhysicsSystemWithContacts.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpAction.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/World/hkpPhysicsSystem.h"
#include "Havok/Source/Physics/Utilities/Dynamics/SaveContactPoints/hkpSerializedAgentNnEntry.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Utilities/Dynamics/SaveContactPoints/hkpSerializedAgentNnEntryClass.h"

static void cleanupLoadedObjecthkpPhysicsSystemWithContacts(void* p);

// The member table of hkpPhysicsSystemWithContacts's class record, as the Havok SDK's class file has it. Not named in
// the PDB or IDA; name chosen here.
using hkpPhysicsSystemWithContactsClass_MembersArray = const hkClassMember[1];
static hkpPhysicsSystemWithContactsClass_MembersArray hkpPhysicsSystemWithContactsClass_Members{
	{"contacts", &hkpSerializedAgentNnEntryClass, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_POINTER},
		0, {0}, 0x44, nullptr}};
hkClass hkpPhysicsSystemWithContactsClass("hkpPhysicsSystemWithContacts", &hkpPhysicsSystemClass, 80, nullptr, 0,
	nullptr, 0, hkpPhysicsSystemWithContactsClass_Members, 1, nullptr, nullptr, 0, 0);
hkTypeInfo hkpPhysicsSystemWithContactsTypeInfo = {"hkpPhysicsSystemWithContacts",
	finishLoadedObjecthkpPhysicsSystemWithContacts, cleanupLoadedObjecthkpPhysicsSystemWithContacts,
	getVtablehkpPhysicsSystemWithContacts(), &typeid(hkpPhysicsSystemWithContacts),
	sizeof(hkpPhysicsSystemWithContacts)};

static void cleanupLoadedObjecthkpPhysicsSystemWithContacts(void* p) {
	static_cast<hkpPhysicsSystemWithContacts*>(p)->~hkpPhysicsSystemWithContacts();
}

// Each saved contact loses its reference; the array's storage goes with m_contacts.
hkpPhysicsSystemWithContacts::~hkpPhysicsSystemWithContacts() {
	for (int32_t i = 0; i < m_contacts.m_size; ++i) {
		m_contacts.m_data[i]->removeReference();
	}
}

hkpPhysicsSystem* hkpPhysicsSystemWithContacts::clone() const {
	return nullptr;
}

void hkpPhysicsSystemWithContacts::addContact(hkpSerializedAgentNnEntry* c) {
	hkArray<hkpSerializedAgentNnEntry*>* p_m_contacts;
	hkMemoryRouter* instance;
	if (c) {
		c->addReference();
		p_m_contacts = &m_contacts;
		instance = &hkMemoryRouter::getInstance();
		if (p_m_contacts->m_size == (p_m_contacts->m_capacityAndFlags & 1073741823)) {
			hkArrayUtil::_reserveMore(*instance->m_heap, p_m_contacts, sizeof(hkpSerializedAgentNnEntry*));
		}
		p_m_contacts->m_data[p_m_contacts->m_size++] = c;
	}
}

void hkpPhysicsSystemWithContacts::removeContact(int32_t a1) {
	m_contacts.m_data[a1]->removeReference();
	uint32_t result = --m_contacts.m_size;
	if (result != ((uint32_t)a1)) {
		m_contacts.m_data[a1] = m_contacts.m_data[result];
	}
}

// The base copy, then m_contacts copied as plain pointers (no references added).
void hkpPhysicsSystemWithContacts::copy(const hkpPhysicsSystemWithContacts& toCopy) {
	hkpPhysicsSystem::copy(toCopy);
	// EXE-INLINED none: hkArray<hkpSerializedAgentNnEntry*, hkContainerHeapAllocator>::operator=
	m_contacts.copyFromArray(*hkMemoryRouter::getInstance().m_heap, toCopy.m_contacts, hkTypeIsPod());
}

const hkClass& hkpPhysicsSystemWithContacts::staticClass() {
	return hkpPhysicsSystemWithContactsClass;
}
