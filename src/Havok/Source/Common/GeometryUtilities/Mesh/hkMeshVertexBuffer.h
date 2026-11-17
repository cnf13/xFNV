#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

class hkClassEnum;

// The layout of a vertex buffer's vertices: up to 32 elements, each a data type, a usage and hint flags.
struct hkVertexFormat {
	enum {
		MAX_ELEMENTS = 32,
	};

	// The type of an element's values.
	enum DataType : int32_t {
		TYPE_NONE = 0x0,
		TYPE_INT8 = 0x1,
		TYPE_UINT8 = 0x2,
		TYPE_INT16 = 0x3,
		TYPE_UINT16 = 0x4,
		TYPE_INT32 = 0x5,
		TYPE_UINT32 = 0x6,
		TYPE_UINT8_DWORD = 0x7,
		TYPE_ARGB32 = 0x8,
		TYPE_FLOAT16 = 0x9,
		TYPE_FLOAT32 = 0xA,
		TYPE_VECTOR4 = 0xB,
		TYPE_LAST = 0xC,
	};

	// What an element holds.
	enum DataUsage : int32_t {
		USAGE_NONE = 0x0,
		USAGE_POSITION = 0x1,
		USAGE_NORMAL = 0x2,
		USAGE_COLOR = 0x3,
		USAGE_TANGENT = 0x4,
		USAGE_BINORMAL = 0x5,
		USAGE_BLEND_MATRIX_INDEX = 0x6,
		USAGE_BLEND_WEIGHTS = 0x7,
		USAGE_BLEND_WEIGHTS_LAST_IMPLIED = 0x8,
		USAGE_TEX_COORD = 0x9,
		USAGE_POINT_SIZE = 0xA,
		USAGE_USER = 0xB,
		USAGE_LAST = 0xC,
	};

	// The flags an element may carry.
	enum HintFlags : int32_t {
		FLAG_READ = 0x1,
		FLAG_WRITE = 0x2,
		FLAG_DYNAMIC = 0x4,
		FLAG_NOT_SHARED = 0x8,
	};

	// How a format's elements are shared between buffers.
	enum SharingType : int32_t {
		SHARING_ALL_SHARED = 0x0,
		SHARING_ALL_NOT_SHARED = 0x1,
		SHARING_MIXTURE = 0x2,
	};

	// One element of a vertex: its type, how many values, what they mean and their flags.
	struct Element {
		hkEnum<DataType, uint8_t> m_dataType;
		uint8_t m_numValues;
		hkEnum<DataUsage, uint8_t> m_usage;
		uint8_t m_subUsage;
		hkFlags<HintFlags, uint8_t> m_flags;
		uint8_t m_pad[3];
	};

	// The constructors, set, clear, addElement and the other methods are not declared here.
	Element m_elements[MAX_ELEMENTS];
	int32_t m_numElements;
};

// A vertex buffer a mesh section locks: its format, its vertices and the buffers it locks them in. No data of its own
// (the layout is the hkReferencedObject base); its virtuals come after the base's getClassType and
// calcContentStatistics, in this order.
class hkMeshVertexBuffer : public hkReferencedObject {
public:
	// What a lock may do with the buffer.
	enum Flags : int32_t {
		ACCESS_READ = 0x1,
		ACCESS_WRITE = 0x2,
		ACCESS_READ_WRITE = 0x3,
		ACCESS_WRITE_DISCARD = 0x4,
		ACCESS_ELEMENT_ARRAY = 0x8,
	};

	// The result of a lock.
	enum LockResult : int32_t {
		RESULT_FAILURE = 0x0,
		RESULT_SUCCESS = 0x1,
		RESULT_IN_USE = 0x2,
	};

	// What to lock: a vertex range and the lock's flags.
	struct LockInput {
		int32_t m_startVertex;
		int32_t m_numVertices;
		hkBool m_noWait;
		hkBool m_contiguousAccess;
		int32_t m_lockFlags;
	};

	// The locked vertices: one buffer per element.
	struct LockedVertices {
		// One element's vertices: where they start, the distance between two and which element they are.
		struct Buffer {
			void* m_start;
			int32_t m_stride;
			hkVertexFormat::Element m_element;
		};

		Buffer m_buffers[hkVertexFormat::MAX_ELEMENTS];
		int32_t m_numBuffers;
		int32_t m_numVertices;
		hkBool m_isInterleaved;
	};

	// What to lock in a partial lock: the elements and the flags of each.
	struct PartialLockInput {
		int32_t m_numLockFlags;
		int32_t m_elementIndices[hkVertexFormat::MAX_ELEMENTS];
		uint8_t m_lockFlags[hkVertexFormat::MAX_ELEMENTS];
	};

	virtual hkMeshVertexBuffer* clone() = 0;
	virtual bool isSharable() = 0;
	virtual void getVertexFormat(hkVertexFormat& formatOut) = 0;
	virtual int32_t getNumVertices() = 0;
	virtual LockResult lock(const LockInput& input, LockedVertices& lockedVerticesOut) = 0;
	virtual LockResult partialLock(const LockInput& input, const PartialLockInput& partialInput,
		LockedVertices& lockedOut) = 0;
	virtual void getElementVectorArray(const LockedVertices& lockedVertices, int32_t elementIndex,
		hkVector4* compData) = 0;
	virtual void setElementVectorArray(const LockedVertices& lockedVertices, int32_t elementIndex,
		const hkVector4* compData) = 0;
	virtual void getElementIntArray(const LockedVertices& lockedVertices, int32_t elementIndex, int32_t* compData) = 0;
	virtual void setElementIntArray(const LockedVertices& lockedVertices, int32_t elementIndex,
		const int32_t* compData) = 0;
	virtual void unlock(const LockedVertices& lockedVertices) = 0;
};

// The class's reflection record.
extern hkClass hkMeshVertexBufferClass;

// The records of hkMeshVertexBuffer's two enums (entries of hkMeshVertexBufferClass.cpp's enum table), by pointer.
extern const hkClassEnum* hkMeshVertexBufferFlagsEnum;
extern const hkClassEnum* hkMeshVertexBufferLockResultEnum;
