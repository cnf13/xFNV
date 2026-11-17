#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiIntegerExtraData.h"
#include "Reconstruction.hpp"

class NiObjectNET;

// The extra data on a model that carries its engine flags (animated, havok, editor marker, ...).
class BSXFlags : public NiIntegerExtraData {
public:
	enum FLAGS : int32_t {
		ANIMATED = 0x1,
		HAVOK = 0x2,
		RAGDOLL = 0x4,
		COMPLEX = 0x8,
		ADDON = 0x10,
		EDITORMARKER = 0x20,
		DYNAMIC = 0x40,
		ARTICULATED = 0x80,
		IKTARGET = 0x100,
		EXTERNAL_EMIT = 0x200,
	};

	BSXFlags();
	BSXFlags(const int32_t a1);
	~BSXFlags() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void LinkObject(NiStream& arStream) override;
	bool RegisterStreamables(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	bool IsEqual(NiObject* apObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;

	// Masks the flags with aiFlags.
	uint32_t GetFlags(uint32_t aiFlags) const;
	bool QAnimated() const;
	bool QEditorMarker() const;
	bool QAddon() const;
	void SetFlags(uint32_t aiFlags, bool abTF);
	void ClearFlags(uint32_t aiFlags);
	// Gives the extra data its engine tag as name.
	void Init();
	bool QIKTarget();

protected:
	void CopyMembers(BSXFlags* pDest, NiCloningProcess& arCloneData);

public:
	static const NiFixedString& Tag();
	// Creates the "BSX" tag string stored in pTag.
	static void InitSDM();
	// The object's BSXFlags extra data, null when it has none.
	static BSXFlags* Find(NiObjectNET* apObject);
	// apObject's flags masked with aiFlags, or 0 without BSXFlags.
	// Inferred type: apObject: NiObjectNET* from 0x62c41a (parameter 1 of TESHavokUtilities::Should3DRagDoll)
	static int32_t FindFlag(NiObjectNET* apObject, FLAGS aiFlags);
	static NiObject* CreateObject();
	static void KillSDM();

	static NiRTTI ms_RTTI;
	static inline NiFixedString* pTag;
};
inline constexpr fnvd::Vtable BSXFlags_vftable{};
