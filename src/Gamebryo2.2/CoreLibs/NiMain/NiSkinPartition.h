#pragma once

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTSet.h"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class NiGeometryBufferData;
class NiSkinData;
class NiTriBasedGeomData;

// The hardware skinning split of a skinned mesh: partitions, each a self-contained slice of the vertices with its own
// bone list, per-vertex bone indices and weights, and its triangles (or strips).
class NiSkinPartition : public NiObject {
public:
	// One bone's weight on a vertex.
	class Interaction : public NiMemObject {
	public:
		uint32_t m_uiBone;
		float m_fWeight;
	};

	// The bones that influence one vertex, with their weights.
	class VertexInteractions : public NiTPrimitiveSet<Interaction> {
	public:
		VertexInteractions();

		void Unitize();

		// The qsort compare of two Interactions: descending weight.
		static int32_t CompareWeights(const void* pv0, const void* pv1);
	};

	// A set of bone indices and the weight value the equal-set grouping of MakePartitions works with.
	class BoneSet : public NiTPrimitiveSet<uint16_t> {
	public:
		bool Contains(const NiSkinPartition::BoneSet& kThat) const;
		// The size of the union with kThat.
		uint32_t MergedSize(const BoneSet& kThat) const;
		// Adds kThat's bones to this set.
		void Merge(const BoneSet& kThat);
		// Whether each bone kVertex interacts with is in this set.
		bool ContainsVertex(const VertexInteractions& kVertex) const;
		// Whether the three vertices of pusTriangle are all ContainsVertex.
		bool ContainsTriangle(const uint16_t* pusTriangle, const VertexInteractions* pkVertex) const;

		// The qsort and bsearch compare of two bone indices (uint16_t): ascending.
		static int32_t CompareBones(const void* pv0, const void* pv1);

		// Weight value; MakePartitions hands its low word out as the partition number.
		int32_t iWValue;
	};

	// Walks two sorted bone sets in step, handing out their union in ascending order.
	class BoneSetMerger : public NiMemObject {
	public:
		// The next bone of the union; a set that runs out reads as USHRT_MAX.
		uint16_t GetNext();

		uint32_t m_uiIndex[2];
		uint16_t m_usEl[2];
		const BoneSet* m_kSet0;
		const BoneSet* m_kSet1;
	};

	// The triangle indices of one partition.
	class TriangleSet : public NiTPrimitiveSet<uint16_t> {
	public:
		// Adds every triangle i of the uiTriangles whose puiTriangleToPartition[i] is uiPartition.
		void AddTriangles(uint32_t uiPartition, const uint32_t* puiTriangleToPartition, uint32_t uiTriangles);
	};

	// One partition: m_pusVertexMap maps a partition vertex to a model vertex, m_pucBonePalette holds
	// m_usBonesPerVertex bone indices per vertex into m_pusBones, m_pusStripLengths is set when it has strips.
	class Partition : public NiMemObject {
	public:
		Partition();
		virtual ~Partition();

		// The index count of all strips together.
		uint32_t GetStripLengthSum() const;
		// Fills this partition from one bone set. bNeedPalette, bForcePalette and ucBonesPerVertex are declared int32_t
		// rather than bool/uint8_t because the caller passes a whole 32-bit value on, which a bool would zero-extend.
		// Create reads only the low byte of bNeedPalette.
		void Create(const TriangleSet& kTriangles, const BoneSet& kBones, const NiTriBasedGeomData* pkTriData,
			const VertexInteractions* pkVertexInteraction, int32_t bNeedPalette, int32_t bForcePalette,
			int32_t ucBonesPerVertex, uint32_t uiMaxBones);
		void LoadBinary(NiStream& kStream);
		void SaveBinary(NiStream& kStream);
		// Compares the counts and the contents of every array.
		bool IsEqual(const Partition& kPartition);
		uint16_t* CreateVertexMap(const NiSkinPartition::TriangleSet& kTriangles, const NiTriBasedGeomData* pkTriData);
		void CreateTriangleList(const NiSkinPartition::TriangleSet& kTriangles, const NiTriBasedGeomData* pkTriData,
			const uint16_t* pusVertexPartitionIndex);

		static void operator delete[](void* apvMem) { MemoryManager_Singleton.Deallocate(apvMem); }
		// Frees with ourFree (the array form is declared above).
		static void operator delete(void* apMemory);

		uint16_t* m_pusBones;
		float* m_pfWeights;
		uint16_t* m_pusVertexMap;
		uint8_t* m_pucBonePalette;
		uint16_t* m_pusTriList;
		uint16_t* m_pusStripLengths;
		uint16_t m_usVertices;
		uint16_t m_usTriangles;
		uint16_t m_usBones;
		uint16_t m_usStrips;
		uint16_t m_usBonesPerVertex;
		NiGeometryBufferData* m_pkBuffData;
	};

protected:
	typedef NiTPrimitiveSet<BoneSet*> NiBoneSetCollection;

public:
	// No partitions; the count is left as allocated.
	NiSkinPartition();
	~NiSkinPartition() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;

	void ReplacePartitions(NiSkinPartition::Partition* pkPartitions, uint32_t uiPartitions);
	uint32_t GetPartitionCount() const;
	NiSkinPartition::Partition* GetPartitions() const;
	// Splits the mesh into hardware partitions of at most ucBonesPerPartition bones each and replaces the partition
	// array; the three trailing arrays receive each partition's number, bone count and state-change flag.
	// ucBonesPerPartition, ucBonesPerVertex and bForcePalette are int32_t rather than uint8_t and bool so the caller's
	// whole 32-bit value is passed on to RemoveExcessInteractions, MakeBoneSets and Partition::Create; a bool would be
	// zero-extended.
	bool MakePartitions(const NiTriBasedGeomData* pkTriData, const NiSkinData* pkSkinData, int32_t ucBonesPerPartition,
		int32_t ucBonesPerVertex, int32_t bForcePalette, int32_t* apWArray, uint16_t* apusPartNumA,
		bool* apbStateChangeA);

protected:
	// Merges bone sets that fit into one partition, then sorts the collection so equal sets are adjacent. Only the low
	// byte of ucBonesPerPartition is used.
	void MergeBoneSets(int32_t ucBonesPerPartition, NiBoneSetCollection& kBoneSets);
	// One VertexInteractions per vertex.
	VertexInteractions* CreateVertexInteractions(uint32_t uiVertices, const NiSkinData* pkSkinData);
	// Drops the weights past ucBonesPerVertex. ucBonesPerPartition and ucBonesPerVertex are declared int32_t rather
	// than uint8_t because the caller's whole 32-bit value is passed on.
	bool RemoveExcessInteractions(const NiTriBasedGeomData* pkTriData, int32_t ucBonesPerPartition,
		int32_t ucBonesPerVertex, VertexInteractions* pkVertexInteraction);
	// Builds the bone sets and the triangle-to-partition map. ucBonesPerPartition is int32_t rather than uint8_t
	// because the caller's whole value is passed on.
	bool MakeBoneSets(const NiTriBasedGeomData* pkTriData, const NiSkinData* pkSkinData, int32_t ucBonesPerPartition,
		VertexInteractions* pkVertexInteraction, NiBoneSetCollection& kBoneSets, uint32_t*& puiTriangleToPartition,
		const int32_t* apWArray);

public:
	void MakeBoneMap(NiTPointerMap<uint32_t, float>& kMap, const uint16_t* pusTriangle,
		const NiSkinPartition::VertexInteractions* pkVertexInteraction);
	void CreateInitialBoneSets(const NiTriBasedGeomData* pkTriData,
		NiSkinPartition::VertexInteractions* pkVertexInteraction, NiTPrimitiveSet<NiSkinPartition::BoneSet*>& kBoneSets,
		const int32_t* apWArray);

	static NiObject* CreateObject();
	static int32_t AssignTriangle(NiTPrimitiveSet<NiSkinPartition::BoneSet*>& kBoneSets, const uint16_t* pusTriangle,
		NiSkinPartition::VertexInteractions* pkVertexInteraction, bool* pbAssigned, bool bAssign);

	uint32_t m_uiPartitions;
	Partition* m_pkPartitions;

	static NiRTTI ms_RTTI;
};

NiSmartPointer(NiSkinPartition);
