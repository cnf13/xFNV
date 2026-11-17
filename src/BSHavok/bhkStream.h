#pragma once

#include "BSHavok/bhkTransform.h"
#include "BSHavok/bhkWorldObject.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

#include <bit>

class hkpConvexShape;
class hkpMoppCode;
class hkpShape;
class hkQsTransform;
class NiStream;

int32_t LoadUIntArray(NiStream& arStream, hkArray<uint32_t>& arArray);

// The Havok data helpers of the NIF stream. Its base and fields are not declared: only some static helpers are declared
// here.
class bhkStream {
public:
    // Swaps the byte order of a creation info's words, keeping its hkArray member's data, size and capacity words as
    // they were.
    template <class T>
    static void SwapHKArray(hkSerializableCinfo* apCInfo, int32_t aiSize, hkArray<T>* apArray);
    // Reads the array's element count, then each vector, byte-swapped for a big-endian source. Returns the count times
    // 16.
    static int32_t LoadBinary(NiStream& arStream, hkArray<hkVector4>& arArray);
    // True when the two arrays hold the same spheres.
    static bool IsEqual(const hkArray<hkVector4>& arA, const hkArray<hkVector4>& arB);
    // Reads a transform: three floats of translation (w 0), four of rotation, three of scale (w 0).
    static void LoadBinary(NiStream& arStream, hkQsTransform& arTransf);
    // Reads a MOPP code into a new hkpMoppCode stored in arMoppCode. Returns the code's byte count.
    static int32_t LoadBinary(NiStream& arStream, hkpMoppCode*& arMoppCode);
    // Writes the array's size, then each vector. Returns the size times 16.
    static int32_t SaveBinary(NiStream& arStream, hkArray<hkVector4>& arArray);
    // Writes a MOPP code's byte count, its offset vector and, when it has any, its bytes. Returns 0 for no code, else
    // the byte count.
    static int32_t SaveBinary(NiStream& arStream, const hkpMoppCode* apMoppCode);
    // Writes the array's size, then each element, byte-swapped for a big-endian save. Returns the size times 4.
    static int32_t SaveBinary(NiStream& arStream, hkArray<uint32_t>& arArray);

    // Swaps the byte order of each of the vector's four words, x to w, and returns the vector.
    static hkVector4& Endian(hkVector4& arVector) {
        for (int32_t i = 0; i < 4; ++i) {
            arVector(i) = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(arVector(i))));
        }
        return arVector;
    }

    // Swaps the byte order of each of the rotation's twelve words, column by column, and returns the rotation.
    static hkRotation& Endian(hkRotation& arRot);

    // Swaps the byte order of each of the vector's four words, x to w, and returns the vector.
    static bhkVector4& Endian(bhkVector4& arVector) {
        arVector.fX = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(arVector.fX)));
        arVector.fY = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(arVector.fY)));
        arVector.fZ = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(arVector.fZ)));
        arVector.fW = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(arVector.fW)));
        return arVector;
    }

    // The same swap of the three columns as Endian(hkRotation&).
    static bhkMatrix3& Endian(bhkMatrix3& arRot);

    // Swaps the translation's words, then the rotation's, and returns the transform.
    static bhkTransform& Endian(bhkTransform& arTransf) {
        Endian(arTransf.Trans);
        Endian(arTransf.Rot);
        return arTransf;
    }
};
