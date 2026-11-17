#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/SceneData/Mesh/hkxVertexDescription.h"

#include <stdint.h>

// Scene vertices stored in separate arrays by scalar type.
class hkxVertexBuffer : public hkReferencedObject {
public:
	struct VertexData {
		// The hybrid's implicit default constructor: empty arrays (hkxVertexBuffer's default constructor).
		VertexData() = default;

		// The finish constructor the vertex buffer's needs: the loaded arrays are kept as they are.
		VertexData(hkFinishLoadedObjectFlag flag)
			: m_vectorData(flag), m_floatData(flag), m_uint32Data(flag), m_uint16Data(flag), m_uint8Data(flag) {}

		~VertexData();

		void clear();
		void setSize(int32_t n);

		static const hkClass& staticClass();

		hkArray<hkVector4> m_vectorData;
		hkArray<float> m_floatData;
		hkArray<uint32_t> m_uint32Data;
		hkArray<uint16_t> m_uint16Data;
		hkArray<uint8_t> m_uint8Data;
		uint32_t m_numVerts;
		uint32_t m_vectorStride;
		uint32_t m_floatStride;
		uint32_t m_uint32Stride;
		uint32_t m_uint16Stride;
		uint32_t m_uint8Stride;
	};

	// The reflection's member table (the PDB's hkxVertexBuffer::Members), as the exe's .rdata holds it.
	using MembersArray = const hkClassMember[2];

	// The hybrid's implicit default constructor: the getVtable function declares the local, then constructs it
	// again in place. Every member's own default leaves it destructible.
	hkxVertexBuffer() = default;
	hkxVertexBuffer(hkFinishLoadedObjectFlag flag);
	~hkxVertexBuffer() override;

	int32_t getNumVertices() const;
	void setNumVertices(int32_t n, const hkxVertexDescription& format);
	const void* getVertexDataPtr(const hkxVertexDescription::ElementDecl& elem) const;
	void* getVertexDataPtr(const hkxVertexDescription::ElementDecl& elem);
	const hkxVertexDescription& getVertexDesc() const;
	void copy(const hkxVertexBuffer& other, bool resize);
	// The PDB's copyVertex(const hkxVertexBuffer&, int, int) (hkSceneData.lib): thiscall ret 0xC; vertFrom is
	// checked against other's vertex count, vertTo against this one's. Its caller, copy (0xE3E74B), reads no result.
	void copyVertex(const hkxVertexBuffer& other, int32_t vertFrom, int32_t vertTo);

	static const hkClass& staticClass();

	VertexData m_data;
	hkxVertexDescription m_desc;

	static MembersArray Members;
};

extern hkClass hkxVertexBufferVertexDataClass;
extern hkClass hkxVertexBufferClass;
extern hkTypeInfo hkxVertexBufferTypeInfo;
inline constexpr fnvd::Vtable hkxVertexBuffer_vftable{};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkxVertexBufferVertexDataTypeInfo;
