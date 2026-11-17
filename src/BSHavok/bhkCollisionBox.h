#pragma once

#include "BSHavok/bhkSimpleShapePhantom.h"
#include "Reconstruction.hpp"

class bhkCharacterController;

// A phantom shaped like a box that follows a character controller; Add registers it with the world.
class bhkCollisionBox : public bhkSimpleShapePhantom {
public:
	bhkCollisionBox(hkSerializableCinfo* a1, bhkCharacterController* a2);
	~bhkCollisionBox() override;

	const NiRTTI* GetRTTI() const override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	bool Add(bhkWorld* apWorld) override;
	virtual void Init(const hkSerializableCinfo& arInfo, bhkCharacterController* apCharController);
	using bhkSimpleShapePhantom::Init;

	// Moves the box and its shape's radius to the controller's phantom or position.
	void Update();

	// Frees the aligned block the object sits in: the byte in front of the object gives the block's start.
	static void operator delete(void* apMemory);

	bhkCharacterController* pCharController;
	bool bDisabled;
	uint8_t padding1D[3];

	static NiRTTI ms_RTTI;
	static inline bool bNoCollision;
};
inline constexpr fnvd::Vtable bhkCollisionBox_vftable{};
