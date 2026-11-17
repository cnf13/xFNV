#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineBasis.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

// The degree-3 basis B-spline interpolators evaluate their control points with.
class NiBSplineBasisData : public NiObject {
public:
	NiBSplineBasisData();
	~NiBSplineBasisData() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& stream) override;
	bool RegisterStreamables(NiStream& stream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;

	NiBSplineBasis<float, 3>& GetDegree3Basis();

	// The stream's factory for the class: a basis of no control points.
	static NiObject* CreateObject();

	NiBSplineBasis<float, 3> m_kBasisDegree3;

	static NiRTTI ms_RTTI;
};

NiSmartPointer(NiBSplineBasisData);
