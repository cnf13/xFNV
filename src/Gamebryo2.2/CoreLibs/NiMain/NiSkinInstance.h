#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class BSNiBound;
class NiAVObject;
class NiCloningProcess;
class NiGeometryData;
class NiGeometryDataIteratorNiBlendIndices;
class NiGeometryDataIteratorNiBlendWeights;
class NiGeometryDataIteratorNiPoint3;
class NiPoint3;
class NiSkinData;
class NiSkinPartition;

// Binds a mesh to its skeleton: the skin data, the partitions, the root parent and one bone node per skin-data bone.
// The root parent and the bones are not owned.
class NiSkinInstance : public NiObject {
public:
	NiSkinInstance();
	NiSkinInstance(NiSkinData* apSkinData, NiAVObject* apRootParent, NiAVObject** appBones);
	~NiSkinInstance() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void PostLinkObject(NiStream& kStream) override;
	void SetBound(BSNiBound*) override {}

	// Skins auiVertices model vertices and normals into the destination arrays, auiStride bytes apart.
	void Deform(const NiPoint3* apSrcVertex, const NiPoint3* apSrcNormal, uint32_t auiVertices, NiPoint3* apDstVertex,
		NiPoint3* apDstNormal, NiPoint3* apDstBinormal, NiPoint3* apDstTangent, uint32_t auiStride);
	// Skins the model data's active vertices and normals into the destination arrays, auiStride bytes apart.
	void Deform(const NiGeometryData* apModelData, NiPoint3* apDstVertex, NiPoint3* apDstNormal,
		NiPoint3* apDstBinormal, NiPoint3* apDstTangent, uint32_t auiStride);
	// The Xenon build's skinning of uiVertices vertices and normals read through the
	// iterators; an empty function in this build.
	void Deform(NiGeometryDataIteratorNiPoint3* pkSrcVertex, NiGeometryDataIteratorNiBlendWeights* pkWeights,
		NiGeometryDataIteratorNiBlendIndices* pkIndices, NiGeometryDataIteratorNiPoint3* pkSrcNormal,
		uint32_t uiVertices, NiPoint3* pkDstVertex, NiPoint3* pkDstNormal, uint32_t uiStride);
	NiSkinPartition* GetSkinPartition() const;
	NiSkinData* GetSkinData() const;
	NiAVObject** GetBones() const;
	void SetBone(uint32_t i, NiAVObject* pkBone);
	void SetRootParent(NiAVObject* pkRootParent);
	// The skinned mesh's bound in skin space: the bones' bounds in world space, merged, then moved by the inverse of
	// the root parent's world transform and the skin data's root-parent-to-skin transform.
	void UpdateModelBound(NiBound& arBound);
	void SetSkinPartition(NiSkinPartition* pkSkinPartition);

protected:
	// Shares the skin data and partition with apDest and records apDest in the clone map.
	void CopyMembers(NiSkinInstance* pkDest, NiCloningProcess& kCloning);

public:
	// Whether the root parent or any bone is being cloned (in the clone map).
	bool NeedClone(NiCloningProcess& kCloning);

	static NiObject* CreateObject();

	NiPointer<NiSkinData> m_spSkinData;
	NiPointer<NiSkinPartition> m_spSkinPartition;
	NiAVObject* m_pkRootParent;
	NiAVObject** m_ppkBones;
	uint32_t m_uiFrameID;
	uint32_t m_uiNumMatrices;
	uint32_t m_uiNumRegisters;
	uint32_t m_uiAllocatedSize;
	void* m_pvBoneMatrices;
	void* m_pvPrevBoneMatrices;
	void* m_pvSkinToWorldWorldToSkinMatrix;

	static NiRTTI ms_RTTI;
};

NiSmartPointer(NiSkinInstance);
