#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/BitField/hkBitField.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppModifier.h"

class hkAabb;
class hkpMoppCode;
class hkpShapeContainer;

// A MOPP modifier that removes the terminals of the listed shape keys from a MOPP code, and remembers where they were
// (m_removeInfo) so that the removal can be undone.
class hkpRemoveTerminalsMoppModifier : public hkReferencedObject, public hkpMoppModifier {
protected:
	// The SDK's protected constructor for the derived modifiers, which walk the code themselves.
	hkpRemoveTerminalsMoppModifier() : m_tempShapesToRemove(nullptr) {}

public:
	// The precalculated-AABB constructor: the modifier over moppCode that removes the terminals of shapesToRemove,
	// walked inside aabb. The shape-container overload is a different function.
	hkpRemoveTerminalsMoppModifier(const hkpMoppCode* moppCode, const hkAabb& aabb,
		const hkArray<uint32_t>& shapesToRemove);
	hkpRemoveTerminalsMoppModifier(const hkpMoppCode* moppCode, const hkpShapeContainer* shapeContainer,
		const hkArray<uint32_t>& keysToRemove);

	// The SDK's finish constructor of a loaded object.
	hkpRemoveTerminalsMoppModifier(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag), m_removeInfo(aFlag) {}

	~hkpRemoveTerminalsMoppModifier() override;

	// hkpMoppModifier's vtable at +0x8 (protected in the SDK).
	hkBool shouldTerminalBeRemoved(uint32_t id, const uint32_t* properties) override;
	// hkpMoppModifier's vtable at +0x8 (protected in the SDK).
	void addTerminalRemoveInfo(int32_t relativeMoppAddress) override;

	// Havok SDK.
	void applyRemoveTerminals(hkpMoppCode* moppCode);
	void undoRemoveTerminals(hkpMoppCode* moppCode);

	// Both bases free through the heap: hkReferencedObject's is the one this class's deleting destructor uses.
	static void operator delete(void* apMemory) { hkReferencedObject::deleteObject(apMemory); }
	// Havok SDK signature (hkpCollide.lib, tied by target).
	static const hkClass& staticClass();

	// The relative MOPP addresses of the removed terminals.
	hkArray<uint32_t> m_removeInfo;
	// The shape keys to remove while the constructor runs (a pointer to the caller's array).
	const hkArray<uint32_t>* m_tempShapesToRemove;
};

// The modifier that keeps the terminals whose shape keys are set in a bit field.
class hkpRemoveTerminalsMoppModifier2 : public hkpRemoveTerminalsMoppModifier {
public:
	// The bit-field constructor: walks the whole code.
	hkpRemoveTerminalsMoppModifier2(const hkpMoppCode* moppCode, const hkBitField& enabledKeys);
	~hkpRemoveTerminalsMoppModifier2() override;

	// hkpMoppModifier's vtable at +0x8 (protected in the SDK).
	hkBool shouldTerminalBeRemoved(uint32_t id, const uint32_t* properties) override;

	const hkBitField* m_enabledKeys;
};
