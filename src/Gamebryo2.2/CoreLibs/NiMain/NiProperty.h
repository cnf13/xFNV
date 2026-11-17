#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

struct NiUpdateData;

// A render state attached to a scene graph object.
class NiProperty : public NiObjectNET {
public:
	enum PropertyType : uint32_t {
		ALPHA = 0,
		CULLING = 1,
		MATERIAL = 2,
		SHADE = 3,
		STENCIL = 4,
		TEXTURING = 5,
		WIREFRAME = 6,
		MAX_TYPES = 7,
		DITHER = 8,
		SPECULAR = 9,
		VERTEX_COLOR = 10,
		ZBUFFER = 11,
		REND_SPEC = 12,
		FOG = 13,
	};

	NiProperty();
	~NiProperty() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	// The PropertyType of the property.
	virtual int32_t Type() const = 0;
	virtual void Update(NiUpdateData& UpdateData);

	void CopyMembers(NiProperty* pkDest, NiCloningProcess& kCloning);

	static NiRTTI ms_RTTI;
};

NiSmartPointer(NiProperty);

inline constexpr fnvd::Vtable NiProperty_vftable{};
