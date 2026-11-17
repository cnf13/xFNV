#pragma once

#include "BSHavok/bhkWorldObject.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "BSHavok/bhkTransform.h"
#include "Reconstruction.hpp"

#include <bit>

class NiStream;
class hkQsTransform;
class hkpConvexShape;
class hkpMoppCode;
class hkpShape;

int32_t LoadUIntArray(NiStream& arStream, hkArray<uint32_t>& arArray);

// The Havok data helpers of the NIF stream. Its base and fields are not modelled yet: only the static helpers the
// reconstructed bodies call are declared here.
class bhkStream {
public:
	// Swaps the byte order of a creation info's words, keeping its hkArray member's data, size and capacity words as
	// they were (cdecl, ret 0, 0xCF0930). One exe function for every element type.
	template <class T> static void SwapHKArray(hkSerializableCinfo* apCInfo, int32_t aiSize, hkArray<T>* apArray);
	// Reads the array's element count, then each vector, byte-swapped for a big-endian source (cdecl, ret 0,
	// 0xCF0F40). Returns the count times 16 (the PDB's isize; the exe's eax).
	static int32_t LoadBinary(NiStream& arStream, hkArray<hkVector4>& arArray);
	// True when the two arrays hold the same spheres.
	static bool IsEqual(const hkArray<hkVector4>& arA, const hkArray<hkVector4>& arB);
	// Reads a transform: three floats of translation (w 0), four of rotation, three of scale (w 0).
	static void LoadBinary(NiStream& arStream, hkQsTransform& arTransf);
	// Reads a MOPP code into a new hkpMoppCode stored in arMoppCode (cdecl, ret 0, 0xCF1270). Returns the code's
	// byte count (the exe's eax).
	static int32_t LoadBinary(NiStream& arStream, hkpMoppCode*& arMoppCode);
	// Writes the array's size, then each vector (cdecl, ret 0, 0xCF0CF0). Returns the size times 16: the exe keeps
	// that product and returns it in eax.
	static int32_t SaveBinary(NiStream& arStream, hkArray<hkVector4>& arArray);
	// Writes a MOPP code's byte count, its offset vector and, when it has any, its bytes (cdecl, ret 0, 0xCF0DC0).
	// Returns 0 for no code, else the byte count (the exe's eax).
	static int32_t SaveBinary(NiStream& arStream, const hkpMoppCode* apMoppCode);
	// Writes the array's size, then each element, byte-swapped for a big-endian save (cdecl, ret 0, 0xCF0E90).
	// Returns the size times 4: the exe keeps that product and returns it in eax.
	static int32_t SaveBinary(NiStream& arStream, hkArray<uint32_t>& arArray);

	// Swaps the byte order of each of the vector's four words, x to w, and returns the vector. The exe has no
	// out-of-line copy of this overload: its callers inline it.
	static hkVector4& Endian(hkVector4& arVector) {
		for (int32_t i = 0; i < 4; ++i) {
			arVector(i) = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(arVector(i))));
		}
		return arVector;
	}

	// Swaps the byte order of each of the rotation's twelve words, column by column, and returns the rotation.
	static hkRotation& Endian(hkRotation& arRot);

	// Swaps the byte order of each of the vector's four words, x to w, and returns the vector. The exe has no
	// out-of-line copy of this overload: its callers inline it.
	static bhkVector4& Endian(bhkVector4& arVector) {
		arVector.fX = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(arVector.fX)));
		arVector.fY = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(arVector.fY)));
		arVector.fZ = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(arVector.fZ)));
		arVector.fW = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(arVector.fW)));
		return arVector;
	}

	// The same swap of the three columns as Endian(hkRotation&) (cdecl, ret 0, 0xC8C020; the exe's callers pass it a
	// bhkTransform's Rot).
	static bhkMatrix3& Endian(bhkMatrix3& arRot);

	// Swaps the translation's words, then the rotation's, and returns the transform. The exe has no out-of-line copy
	// of this overload: its callers inline it, calling Endian(bhkMatrix3&) for the rotation.
	static bhkTransform& Endian(bhkTransform& arTransf) {
		Endian(arTransf.Trans);
		Endian(arTransf.Rot);
		return arTransf;
	}
};
