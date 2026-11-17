#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Reconstruction.hpp"

class hkDataObject;

// The interface of the tagfile writers. Only the listener type hkSerializeUtil's save functions pass is declared; the
// virtuals stay with the exe.
class hkTagfileWriter : public hkReferencedObject {
public:
	// Told about each object as the writer adds it, and may replace it. The PDB's class has this one pure virtual and
	// no members, and the exe holds no code for it (the game passes no listener). The callback's `this` is const in
	// the PDB (the SDK header lacks the const).
	class AddDataObjectListener : public hkReferencedObject {
	public:
		virtual hkDataObject* addDataObjectCallback(hkDataObject* apResult, const hkDataObject& object) const = 0;
	};
};
