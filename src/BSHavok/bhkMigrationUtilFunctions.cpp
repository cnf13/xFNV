#include "BSHavok/bhkMigrationUtilFunctions.h"

#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"

void bhkMigrationUtilFunctions::PrepareDebugGeom(NiGeometry* apGeom, NiNode* apNode, HavokMaterialType auiMaterial,
	const NiColor* apColor) {
	NiColor clr = *apColor;
	if (bhkShape::bMaterialColor) {
		bhkShape::GetMaterialColor(auiMaterial, clr);
	}
	void* pMemory = NiMemObject::operator new(sizeof(NiMaterialProperty));
	NiMaterialProperty* pmat = pMemory ? ::new (pMemory) NiMaterialProperty : nullptr;
	// EXE-INLINED 0x4BC450
	pmat->m_emit = clr;
	++pmat->m_uiRevID;
	apGeom->AttachProperty(pmat);
	if (bhkNiCollisionObject::pDebugDispFunc) {
		bhkNiCollisionObject::pDebugDispFunc(apGeom, clr);
	}
	apNode->AttachChild(apGeom, 1);
}
