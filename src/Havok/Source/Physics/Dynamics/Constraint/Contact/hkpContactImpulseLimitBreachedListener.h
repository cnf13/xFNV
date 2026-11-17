#pragma once

#include "Reconstruction.hpp"

class hkContactPoint;
class hkpConstraintInstance;
class hkpContactImpulseLimitBreachedListenerInfo;
class hkpContactPointProperties;

// The interface a world's contact impulse limit listeners implement: the world hands it the contacts whose impulse
// exceeded their limit. It holds no state; the exe has no vtable for the class (fn.py vtable: none), so none is
// declared here.
class hkpContactImpulseLimitBreachedListener {
public:
	virtual ~hkpContactImpulseLimitBreachedListener();

	virtual void contactImpulseLimitBreachedCallback(const hkpContactImpulseLimitBreachedListenerInfo* breachedContacts,
		int32_t numBreachedContacts) = 0;
};

// One contact whose impulse exceeded its limit, as the world's impulse limit listeners get it. The PDB's
// ListenerData is a union of the solver's hkpImpulseLimitBreachedElem and SingleImpulseElem; only the single form
// is modelled (the solver form has the same 16 bytes).
class alignas(16) hkpContactImpulseLimitBreachedListenerInfo {
public:
	struct SingleImpulseElem {
		// 2 for a time of impact.
		uint16_t m_type;
		uint16_t m_isContact;
		hkpConstraintInstance* m_constraintInstance;
		hkpContactPointProperties* m_properties;
		hkContactPoint* m_contactPoint;
	};

	union ListenerData {
		SingleImpulseElem m_single;
	};

	// The SDK's inline: one contact point, of a time of impact when toi.
	void set(hkpConstraintInstance* constraintInstance, hkpContactPointProperties* properties, hkContactPoint* cp,
		bool toi) {
		m_data.m_single.m_type = 1 + toi;
		m_data.m_single.m_isContact = true;
		m_data.m_single.m_constraintInstance = constraintInstance;
		m_data.m_single.m_contactPoint = cp;
		m_data.m_single.m_properties = properties;
	}

	ListenerData m_data;
};
