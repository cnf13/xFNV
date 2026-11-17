#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class NiObjectNET;
class NiStream;

// One furniture marker: where an actor sits, lies or stands on the furniture, and which way it faces.
struct FurnitureMark {
	// Inlined everywhere: the default (all bits set) marks a loaded slot, NiTArray's T(0) an empty one.
	FurnitureMark(uint32_t aiNumber = 0xFFFFFFFFu) : sHeading(0), cNumber(static_cast<uint8_t>(aiNumber)) {}

	// Position, then all other fields; NaN coordinates compare unequal.
	bool operator==(const FurnitureMark& arSource) const;
	// Wraps afHeading (radians) into [0, 2 pi) and stores it in thousandths.
	void SetHeading(float afHeading);
	void LoadBinary(NiStream& arStream);
	void SaveBinary(NiStream& arStream);
	// Position, through the exe's shared one-field getter (thiscall, 0x6815C0).
	const NiPoint3& QPosition();
	// The heading in radians, rounded to float and returned in ST0 (thiscall, 0x568650).
	float GetHeading();
	// The type byte zero-extended into eax (0x5094BA movzx eax, byte ptr); callers push or compare the whole eax
	// (0x50964C push eax to TESFurniture::IsSleepMarker).
	uint32_t GetType() const;
	void SetType(uint8_t a2);
	// Copies arPosition into Position; the exe's body is shared with
	// MultiBoundMarkerData::SetHalfExtents (thiscall, ret 4, 0x98DDD0).
	void SetPosition(const NiPoint3& arPos);

	NiPoint3 Position;
	// The heading in thousandths of a radian.
	uint16_t sHeading;
	uint8_t cNumber;
};

// The "FRN" extra data of a model: where an actor stands, sits or lies when using the furniture.
class BSFurnitureMarker : public NiExtraData {
public:
	BSFurnitureMarker();
	~BSFurnitureMarker() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void LinkObject(NiStream& arStream) override;
	bool RegisterStreamables(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	bool IsEqual(NiObject* apObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;

	// Marker auiIndex.
	FurnitureMark& QMark(uint32_t aiMark);
	// The number of markers, aMarks' effective size.
	uint32_t QNumMarks();
	void Init();
	void Add(const FurnitureMark& arMark);

	// Creates the "FRN" tag.
	static void InitSDM();
	// The furniture marker extra data of apObject, or null.
	static BSFurnitureMarker* Find(NiObjectNET* apObject);
	static NiObject* CreateObject();
	static void KillSDM();

	NiTObjectArray<FurnitureMark> aMarks;

	static inline NiFixedString* pTag;
	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable BSFurnitureMarker_vftable{};
