#pragma once

#include "BSCore/BSCriticalSection.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSShader/BSShaderTextureSet.h"
#include "BSShader/SpeedTreeLeafShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"

#include <cstdint>

class BSTreeNode;
class CSpeedTreeRT;
class NiTriShapeData;
class NiTriStripsData;
class SpeedTreeBranchShaderProperty;
class TESObjectTREE;

// A SpeedTree model and the geometry built from it.
class BSTreeModel : public NiRefObject {
public:
	// The kind of model: none yet, the original or an instance of one.
	enum BSTREEMODEL_TYPE : uint32_t {
		BTMT_NONE = 0,
		BTMT_ORIGINAL = 1,
		BTMT_INSTANCE = 2,
		BTMT_COUNT = 3,
	};

	BSTreeModel();
	~BSTreeModel() override;

	// The destructor is the implicit one: its sequence is the Destruct static above.
	virtual void Update(BSTreeNode* apNode);
	virtual BSTreeNode* CreateArt(float afScale);
	virtual void ApplyBaseObject(TESObjectTREE* apTreeObj);
	virtual void ClearModel();
	virtual void CreateGeometry(TESObjectTREE* apTreeObj);

	uint16_t GetNumBranchLODLevels();
	uint16_t GetNumLeafLODLevels();
	void SetInitialized(bool abVal);
	bhkRigidBody* GetRigidBody() const;
	void CreateBranchGeometry();
	void CreateLeafGeometry(TESObjectTREE* apTreeObj);
	void CreateBillboardGeometry(TESObjectTREE* apTreeObj);
	bool InitAsInstance(BSTreeModel* apInstanceFrom);
	bool InitFromBase(TESObjectTREE* apTreeObj, uint32_t aiSeed);
	bool IsInstance();

	static NiAlphaProperty* CreateAlphaProperty();

	BSTREEMODEL_TYPE eType;
	CSpeedTreeRT* pSpeedTree;
	NiPointer<BSTreeModel> spTreeParent;
	NiPointer<NiTriStripsData>* pspBranchData;
	NiPointer<NiTriShapeData>* pspLeafData;
	NiPointer<NiTriShape> spBillboard;
	NiPointer<STLSPData> spLeafShaderData;
	NiPointer<SpeedTreeBranchShaderProperty>* pspBranchShaderProperties;
	NiPointer<SpeedTreeLeafShaderProperty>* pspLeafShaderProperties;
	NiPointer<BSShaderTextureSet> spBranchTextureSet;
	NiPointer<NiSourceTexture> spLeafFrondTexture;
	NiPointer<BSShaderTextureSet> spBillboardTextureSet;
	NiPointer<bhkRigidBody> spRigidBody;
	float fCurveScalar;
	uint32_t iSeed;
	bool bInitialized;
	uint8_t padding45[3];
	float fTrunkHeight;
	float fTrunkWidth;

	static BSCriticalSection kCloneLock;
};

NiSmartPointer(BSTreeModel);
