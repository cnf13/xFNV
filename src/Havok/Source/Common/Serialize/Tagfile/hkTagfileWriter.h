#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"

class hkDataObject;

// The interface of the tagfile writers. Only the listener type hkSerializeUtil's save functions pass is declared; the
// virtual functions are not declared.
class hkTagfileWriter : public hkReferencedObject {
public:
	// Told about each object as the writer adds it, and may replace it. The class has this one pure virtual and no
	// members; the game passes no listener. The callback's `this` is const.
	class AddDataObjectListener : public hkReferencedObject {
	public:
		virtual hkDataObject* addDataObjectCallback(hkDataObject* apResult, const hkDataObject& object) const = 0;
	};
};
