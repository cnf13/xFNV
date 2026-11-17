#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"

#include <stdint.h>

// The elements and strides of a scene vertex format.
class hkxVertexDescription {
public:
	enum DataType : int32_t {
		HKX_DT_NONE = 0,
		HKX_DT_UINT8 = 1,
		HKX_DT_INT16 = 2,
		HKX_DT_UINT32 = 3,
		HKX_DT_FLOAT = 4,
		HKX_DT_FLOAT2 = 5,
		HKX_DT_FLOAT3 = 6,
		HKX_DT_FLOAT4 = 7,
	};

	enum DataUsage : int32_t {
		HKX_DU_NONE = 0,
		HKX_DU_POSITION = 1,
		HKX_DU_COLOR = 2,
		HKX_DU_NORMAL = 4,
		HKX_DU_TANGENT = 8,
		HKX_DU_BINORMAL = 16,
		HKX_DU_TEXCOORD = 32,
		HKX_DU_BLENDWEIGHTS = 64,
		HKX_DU_BLENDINDICES = 128,
		HKX_DU_USERDATA = 256,
	};

	struct ElementDecl {
		// Havok SDK signature (hkSceneData.lib, tied by target).
		static const hkClass& staticClass();

		uint32_t m_byteOffset;
		hkEnum<DataType, uint16_t> m_type;
		hkEnum<DataUsage, uint16_t> m_usage;
		uint32_t m_byteStride;
	};

	// The reflection's member table (the PDB's hkxVertexDescription::Members; .rdata).
	using MembersArray = const hkClassMember[1];

	// The hybrid's implicit default constructor (hkxVertexBuffer's default constructor).
	hkxVertexDescription() = default;
	hkxVertexDescription(hkFinishLoadedObjectFlag flag);

	// The PDB's operator==: whether other's element declarations equal this one's (type, usage, offset and stride
	// of each); thiscall, ret 4, the result in al.
	bool operator==(const hkxVertexDescription& other) const;
	// Havok SDK signature (hkGeometryUtilities.lib, tied by bytes,reloc).
	hkxVertexDescription::ElementDecl* getElementDecl(hkxVertexDescription::DataUsage a1, int32_t a2);

	// The SDK's inline const overload (hkxVertexDescription.inl): it calls the non-const one, the exe's only copy
	// (hkxVertexBuffer::copyVertex, 0xE3E030, calls 0xE3DF60 on its const source's description).
	const hkxVertexDescription::ElementDecl* getElementDecl(hkxVertexDescription::DataUsage a1, int32_t a2) const {
		return const_cast<hkxVertexDescription*>(this)->getElementDecl(a1, a2);
	}

	// Havok SDK signature (hkSceneData.lib, tied by target).
	static const hkClass& staticClass();

	hkArray<ElementDecl> m_decls;

	static MembersArray Members;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkxVertexDescriptionElementDeclTypeInfo;
extern hkTypeInfo hkxVertexDescriptionTypeInfo;
