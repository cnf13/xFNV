#pragma once

#include "BSShader/SpeedTreeShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"

// The property that selects the branch shader and its wind for a SpeedTree model.
class SpeedTreeBranchShaderProperty : public SpeedTreeShaderPPLightingProperty {
public:
	SpeedTreeBranchShaderProperty(STSPData* apData);
	~SpeedTreeBranchShaderProperty() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	NiAdditionalGeometryData* CreateRendererSpecificProperty(NiGeometry* apGeometry) override;
	SpeedTreeBranchShaderProperty* CreateCopy() override;

	// CreateClone calls it on the clone.
	void CopyMembers(SpeedTreeBranchShaderProperty* apDest, NiCloningProcess& arCloneData);

	static NiRTTI ms_RTTI;
};

NiSmartPointer(SpeedTreeBranchShaderProperty);
