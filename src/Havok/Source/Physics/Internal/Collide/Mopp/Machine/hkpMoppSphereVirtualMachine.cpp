#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppSphereVirtualMachine.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/hkMath.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/Types/Geometry/Sphere/hkSphere.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverCheck.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Code/hkpMoppCode.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppVirtualMachine.h"
#include "SDK/MSVC/string.h"

// Collects the primitives the sphere overlaps: its centre less one and its radius plus two in the code's integer
// space, then the walk from the root (the high 16 bits: shift 16).
void hkpMoppSphereVirtualMachine::querySphere(const hkpMoppCode* code, const hkSphere& sphere,
	hkArray<hkpMoppPrimitiveInfo, hkContainerHeapAllocator>* primitiveResults) {
	hkpMoppSphereVirtualMachineQuery query;
	const uint8_t* codeData = code->m_data.m_data;
	m_primitives_out = primitiveResults;
	const float scale = code->m_info.m_offset.m_quad[3];
	const float x = (sphere.m_pos.m_quad[0] - code->m_info.m_offset.m_quad[0]) * scale;
	m_x = hkMath::hkToIntFast(x) - 1;
	const float y = (sphere.m_pos.m_quad[1] - code->m_info.m_offset.m_quad[1]) * scale;
	m_y = hkMath::hkToIntFast(y) - 1;
	const float z = (sphere.m_pos.m_quad[2] - code->m_info.m_offset.m_quad[2]) * scale;
	m_z = hkMath::hkToIntFast(z) - 1;
	const float r = sphere.m_pos.m_quad[3] * scale;
	m_radius = hkMath::hkToIntFast(r) + 2;
	query.m_x = m_x >> 16;
	query.m_y = m_y >> 16;
	query.m_z = m_z >> 16;
	query.m_radius = (m_radius >> 16) + 1;
	query.m_offset_x = 0;
	query.m_offset_y = 0;
	query.m_offset_z = 0;
	query.m_primitiveOffset = 0;
	query.m_shift = 16;
	query.m_properties[0] = 0;
	querySphereOnTree(&query, codeData);
}

// Walks the MOPP code from PC, appending every terminal whose node the sphere reaches to m_primitives_out. query
// holds the sphere in the current node's integer space; a rescale, a terminal offset or a property command moves it
// into scaledQuery first, which the recursive calls are given.
// KEEP: on a property command (96 to 107) the exe stores the value into scaledQuery's slot for that property
// (properties 1 to 3 land past the 40-byte local, in other frame slots), reads scaledQuery.m_properties[0], copies
// the query into scaledQuery when query is not scaledQuery yet, and writes the value it read back. With the copy
// taken, that read is of a local nothing wrote yet, and the value goes on to the recursive calls in scaledQuery:
// src stores property 0 only.
void hkpMoppSphereVirtualMachine::querySphereOnTree(const hkpMoppSphereVirtualMachineQuery* query, const uint8_t* PC) {
	if (!hkpMoppSphereVirtualMachine_HK_flyingcolors_mopp) {
		hkpProcessFlyingColors(&hkpMoppSphereVirtualMachine_HK_flyingcolors_mopp);
		if (!hkpMoppSphereVirtualMachine_HK_flyingcolors_mopp) {
			return;
		}
	}
	hkpMoppSphereVirtualMachineQuery scaledQuery;
	while (true) {
		const int32_t command = *PC;
		// A split node: the query's coordinate along the split's direction, the sphere's radius along it, the left
		// child's upper bound and the right child's lower bound.
		int32_t value;
		int32_t radius;
		int32_t leftMax;
		int32_t rightMin;
		int32_t terminal;
		int32_t property;
		uint32_t propertyValue;
		switch (command) {
		case 0:
			return;
		case 1:
		case 2:
		case 3:
		case 4: {
			// Rescale: the node's space is finer by command bits.
			const int32_t shift = query->m_shift - command;
			scaledQuery.m_shift = shift;
			scaledQuery.m_offset_x = (query->m_offset_x + PC[1]) << command;
			scaledQuery.m_x = (m_x >> shift) - scaledQuery.m_offset_x;
			scaledQuery.m_offset_y = (query->m_offset_y + PC[2]) << command;
			scaledQuery.m_y = (m_y >> shift) - scaledQuery.m_offset_y;
			scaledQuery.m_offset_z = (query->m_offset_z + PC[3]) << command;
			scaledQuery.m_z = (m_z >> shift) - scaledQuery.m_offset_z;
			scaledQuery.m_radius = (m_radius >> shift) + 1;
			scaledQuery.m_primitiveOffset = query->m_primitiveOffset;
			scaledQuery.m_properties[0] = query->m_properties[0];
			query = &scaledQuery;
			PC += 4;
			continue;
		}
		case 5:
			PC += PC[1] + 2;
			continue;
		case 6:
			PC += (PC[1] << 8) + PC[2] + 3;
			continue;
		case 7:
			PC += (((PC[1] << 8) + PC[2]) << 8) + PC[3] + 4;
			continue;
		case 9: {
			const int32_t offset = PC[1];
			if (query != &scaledQuery) {
				scaledQuery = *query;
				query = &scaledQuery;
			}
			scaledQuery.m_primitiveOffset += offset;
			PC += 2;
			continue;
		}
		case 10: {
			const int32_t offset = (PC[1] << 8) + PC[2];
			if (query != &scaledQuery) {
				scaledQuery = *query;
				query = &scaledQuery;
			}
			scaledQuery.m_primitiveOffset += offset;
			PC += 3;
			continue;
		}
		case 11: {
			const int32_t offset = (((((PC[1] << 8) + PC[2]) << 8) + PC[3]) << 8) + PC[4];
			if (query != &scaledQuery) {
				scaledQuery = *query;
				query = &scaledQuery;
			}
			scaledQuery.m_primitiveOffset = offset;
			PC += 5;
			continue;
		}
		case 16:
		case 17:
		case 18:
			value = (&query->m_x)[command - 16];
			radius = query->m_radius;
			leftMax = PC[1];
			rightMin = PC[2];
			break;
		case 19:
			value = query->m_y + query->m_z;
			radius = query->m_radius + (query->m_radius >> 1) + 1;
			leftMax = 2 * PC[1];
			rightMin = 2 * PC[2];
			break;
		case 20:
			value = query->m_y - query->m_z + 255;
			radius = query->m_radius + (query->m_radius >> 1) + 1;
			leftMax = 2 * PC[1];
			rightMin = 2 * PC[2];
			break;
		case 21:
			value = query->m_x + query->m_z;
			radius = query->m_radius + (query->m_radius >> 1) + 1;
			leftMax = 2 * PC[1];
			rightMin = 2 * PC[2];
			break;
		case 22:
			value = query->m_x - query->m_z + 255;
			radius = query->m_radius + (query->m_radius >> 1) + 1;
			leftMax = 2 * PC[1];
			rightMin = 2 * PC[2];
			break;
		case 23:
			value = query->m_x + query->m_y;
			radius = query->m_radius + (query->m_radius >> 1) + 1;
			leftMax = 2 * PC[1];
			rightMin = 2 * PC[2];
			break;
		case 24:
			value = query->m_x - query->m_y + 255;
			radius = query->m_radius + (query->m_radius >> 1) + 1;
			leftMax = 2 * PC[1];
			rightMin = 2 * PC[2];
			break;
		case 25:
			value = query->m_x + query->m_y + query->m_z;
			radius = 4 * query->m_radius;
			leftMax = 3 * PC[1];
			rightMin = 3 * PC[2];
			break;
		case 26:
			value = query->m_y - query->m_z + query->m_x + 255;
			radius = 4 * query->m_radius;
			leftMax = 3 * PC[1];
			rightMin = 3 * PC[2];
			break;
		case 27:
			value = query->m_z - query->m_y + query->m_x + 255;
			radius = 4 * query->m_radius;
			leftMax = 3 * PC[1];
			rightMin = 3 * PC[2];
			break;
		case 28:
			value = query->m_x - query->m_z - query->m_y + 510;
			radius = 4 * query->m_radius;
			leftMax = 3 * PC[1];
			rightMin = 3 * PC[2];
			break;
		case 32:
		case 33:
		case 34: {
			// A single plane along one axis: the left child follows, the right one an 8-bit offset away.
			const int32_t split = PC[1];
			value = (&query->m_x)[command - 32];
			radius = query->m_radius;
			PC += 3;
			if (radius + value < split) {
				continue;
			}
			const int32_t offset = PC[-1];
			PC += offset;
			if (value > radius + split + 1) {
				continue;
			}
			querySphereOnTree(query, PC - offset);
			continue;
		}
		case 35:
		case 36:
		case 37: {
			// A split along one axis with 16-bit offsets to both children.
			leftMax = PC[1];
			rightMin = PC[2];
			const int32_t leftOffset = (PC[3] << 8) + PC[4];
			value = (&query->m_x)[command - 35];
			radius = query->m_radius;
			PC += 7;
			if (value + radius < rightMin) {
				PC += leftOffset;
				continue;
			}
			const int32_t rightOffset = (PC[-2] << 8) + PC[-1];
			PC += rightOffset;
			if (value > radius + leftMax) {
				continue;
			}
			querySphereOnTree(query, PC + (leftOffset - rightOffset));
			continue;
		}
		case 38:
		case 39:
		case 40:
			// Cuts the sphere against the node's bounds along one axis.
			value = (&query->m_x)[command - 38];
			radius = query->m_radius;
			if (radius + value < PC[1] || value > radius + PC[2]) {
				return;
			}
			PC += 3;
			continue;
		case 41:
		case 42:
		case 43:
			// Cuts the unscaled sphere against 24-bit bounds along one axis.
			value = (&m_x)[command - 41];
			radius = m_radius;
			if (radius + value < (((PC[1] << 8) + PC[2]) << 8) + PC[3] ||
				value > (((PC[4] << 8) + PC[5]) << 8) + radius + PC[6]) {
				return;
			}
			PC += 7;
			continue;
		case 48:
		case 49:
		case 50:
		case 51:
		case 52:
		case 53:
		case 54:
		case 55:
		case 56:
		case 57:
		case 58:
		case 59:
		case 60:
		case 61:
		case 62:
		case 63:
		case 64:
		case 65:
		case 66:
		case 67:
		case 68:
		case 69:
		case 70:
		case 71:
		case 72:
		case 73:
		case 74:
		case 75:
		case 76:
		case 77:
		case 78:
		case 79:
			terminal = command - 48;
			goto addTerminal;
		case 80:
			terminal = PC[1];
			goto addTerminal;
		case 81:
			terminal = (PC[1] << 8) + PC[2];
			goto addTerminal;
		case 82:
			terminal = (((PC[1] << 8) + PC[2]) << 8) + PC[3];
			goto addTerminal;
		case 83:
			terminal = (((((PC[1] << 8) + PC[2]) << 8) + PC[3]) << 8) + PC[4];
		addTerminal:
			{
				const uint32_t id = query->m_primitiveOffset + terminal;
				hkArray<hkpMoppPrimitiveInfo>* primitives = m_primitives_out;
				// EXE-INLINED none: hkArray<hkpMoppPrimitiveInfo>::pushBack
				hkMemoryAllocator& alloc = hkContainerHeapAllocator().get(primitives);
				if (primitives->m_size ==
					(primitives->m_capacityAndFlags & hkArrayBase<hkpMoppPrimitiveInfo>::CAPACITY_MASK)) {
					hkArrayUtil::_reserveMore(alloc, primitives, sizeof(hkpMoppPrimitiveInfo));
				}
				primitives->m_data[primitives->m_size++].ID = id;
				return;
			}
		case 96:
		case 97:
		case 98:
		case 99:
			property = command - 96;
			propertyValue = PC[1];
			PC += 2;
			goto setProperty;
		case 100:
		case 101:
		case 102:
		case 103:
			property = command - 100;
			propertyValue = (PC[1] << 8) + PC[2];
			PC += 3;
			goto setProperty;
		case 104:
		case 105:
		case 106:
		case 107:
			property = command - 104;
			propertyValue = (((((PC[1] << 8) + PC[2]) << 8) + PC[3]) << 8) + PC[4];
			PC += 5;
		setProperty:
			if (query != &scaledQuery) {
				scaledQuery = *query;
				query = &scaledQuery;
			}
			if (property == 0) {
				scaledQuery.m_properties[0] = propertyValue;
			}
			continue;
		default: {
			// HK_ERROR; the same command is then read again, as the game does.
			char buf[512];
			hkErrStream kStream(buf, 512);
			kStream.operator<<("Unknown command - This mopp data has been corrupted (check for memory trashing), or an "
							   "hkpMoppBvTreeShape has been pointed at invalid mopp data.\n");
			if (hkError::s_instance->message(hkError::MESSAGE_ERROR, 0x1298FEDD, buf,
					".\\Collide\\Mopp\\Machine\\hkpMoppSphereVirtualMachine.cpp", 292)) {
				__builtin_debugtrap();
			}
			continue;
		}
		}
		// The splits with an 8-bit offset to the right child (16 to 28): the left child follows.
		PC += 4;
		if (value + radius < rightMin) {
			continue;
		}
		const int32_t offset = PC[-1];
		PC += offset;
		if (value > leftMax + radius) {
			continue;
		}
		querySphereOnTree(query, PC - offset);
	}
}
