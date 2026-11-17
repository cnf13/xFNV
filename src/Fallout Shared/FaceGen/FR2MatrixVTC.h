#pragma once

#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

#include <memory>
#include <stdint.h>
#include <vector>

// The float vector under one name (a macro argument can't hold the template's commas): FR2MatrixVTC<float>'s storage
// and FaceGen's coordinate data.
using FloatVector = std::vector<float, std::allocator<float>>;

// The out-of-line members of the PDB's std::vector<float>, _Vector_const_iterator<float> and
// _Vector_iterator<float> that the matrix arithmetic calls. Each is the body every element type of this layout
// shares, so src keeps them as free forwarders per element type (thiscall), named by address as BSFaceGenModel.h's
// are. An iterator result comes back through the hidden result pointer the exe returns in eax. vector::begin and
// vector::end on a const vector (0x65FD10, 0x65FD40) are FloatVector's begin() const and end() const (std's).
// const_iterator::operator!= (ret 4, 0x65DFD0).
bool VectorIteratorNotEqual(const FloatVector::const_iterator* apThis, const FloatVector::const_iterator& _Right);
// const_iterator::operator*, checked.
const float& VectorIteratorDeref(const FloatVector::const_iterator* apThis);
// iterator::operator*, through the const one.
float& VectorIteratorDeref(const FloatVector::iterator* apThis);
// const_iterator::operator++(int): the iterator before the step, built in apResult and returned.
FloatVector::const_iterator* VectorIteratorPostIncrement(FloatVector::const_iterator* apThis,
	FloatVector::const_iterator* apResult, int32_t _Postfix);
// iterator::operator++(int): the iterator before the step, built in apResult and returned.
FloatVector::iterator* VectorIteratorPostIncrement(FloatVector::iterator* apThis, FloatVector::iterator* apResult,
	int32_t _Postfix);
// The float vector's out-of-line helpers that FloatVector::_Insert_n (FR2MatrixVTC.cpp) calls, declared here so that
// other files' bodies can call them and their own bodies can be defined with qualified names. Not named in the PDB
// or IDA; names chosen here.
// _Umove(_First, _Last, _Ptr) (thiscall, ret 0xC, 0x60BB40).
float* VectorUmove(FloatVector* _This, float* _First, float* _Last, float* _Ptr);
// fill(_First, _Last, _Val).
void Fill(float* _First, float* _Last, const float& _Val);
// _Umove_backward(_First, _Last, _Dest).
void UncheckedMoveBackward(float* _First, float* _Last, float* _Dest);
// _Destroy(_First, _Last) of the float vector, which the byte vector shares.
void VectorDestroy(FloatVector* _This, float* _First, float* _Last);
// VC9's _Fill of floats: void.
void FillUnchecked(float* a1, float* a2, const float* a3);
// Not named in the PDB or IDA; name chosen here. _Move_backward_opt(_First, _Last, _Dest, _IterCat, _MoveCat,
// _Checked) of floats (cdecl, 0x60BE80): the exe pushes six arguments and reads the first four.
float* MoveOpt(float* _First, float* _Last, float* _Dest, uint8_t _IterCat, uint8_t _MoveCat, uint8_t _Checked);

// A dense matrix of T, stored row by row in one vector (FaceGen's coordinate arrays).
template <class T> class FR2MatrixVTC {
public:
	// One row of a matrix: the address of its first element and the row length.
	class RowT {
	public:
		RowT() = default;
		RowT(T* ap, uint32_t an) : p(ap), n(an) {}

		// The address of element i.
		T& operator[](uint32_t i);

		T* p;
		uint32_t n;
	};

	// One row of a const matrix, read-only: the address of its first element and the row length (the PDB's
	// RowConstT).
	class RowConstT {
	public:
		// The element i, read-only.
		const T& operator[](uint32_t i) const;

		const T* p;
		uint32_t n;
	};

	FR2MatrixVTC() : nrows(0), ncols(0) { data.clear(); }
	FR2MatrixVTC(uint32_t nr, uint32_t nc) : nrows(nr), ncols(nc) { data.resize(nrows * ncols); }
	// The exe sizes the copy to m's size and copies the elements over: the vector's copy does the same.
	FR2MatrixVTC(const FR2MatrixVTC& m) : data(m.data), nrows(m.nrows), ncols(m.ncols) {}
	~FR2MatrixVTC() {}

	// Sets the size to nr x nc and sizes the storage to fit.
	void resize(int32_t nr, int32_t nc);
	// Row i, returned through a hidden result pointer.
	RowT operator[](uint32_t i);
	// Row i of a const matrix, returned through a hidden result pointer. The exe folds it with the non-const
	// operator[] (0x60B370): the const members (lengthSqr 0x6588A0, subMatrix 0xAAF110) call that copy.
	RowConstT operator[](uint32_t i) const;
	// The number of rows; the exe's shared one-field getter 0x9611E0 (thiscall, ret 0).
	uint32_t numRows() const;
	// The number of columns; the exe's shared one-field getter 0x441110 (thiscall, ret 0).
	uint32_t numCols() const;
	// Copies the size and elements of m; self-assignment does nothing.
	FR2MatrixVTC& operator=(const FR2MatrixVTC& m);
	// True when the size or an element differs from mm's.
	bool operator!=(const FR2MatrixVTC& mm) const;
	// True when the size and the elements (one memcmp) equal mm's; two empty matrices of one
	// size are equal.
	bool operator==(const FR2MatrixVTC& m) const;
	// Adds m element by element; the sizes must match (fr2Assert; thiscall, ret 4, 0x6586C0).
	FR2MatrixVTC& operator+=(const FR2MatrixVTC& m);
	// Subtracts m element by element; the sizes must match (fr2Assert; thiscall, ret 4,
	// 0xAAEF90).
	FR2MatrixVTC& operator-=(const FR2MatrixVTC& m);
	// Zeroes every element.
	void setZero();
	// The exe's vector deleting destructor: with auiFlags & 2 ~FR2MatrixVTC on each of the block's counted elements
	// and with auiFlags & 1 ourFree of the counted block, else the destructor of this and ourFree of it; returns the
	// block, the cookie in front of this in the array case (thiscall, ret 4, 0x5D9FF0).
	void* DeleteArray(uint32_t auiFlags);
	// The sum of the squared elements (thiscall, ret 0, the float in ST0, 0x6588A0).
	float lengthSqr() const;
	// Multiplies every element by s.
	FR2MatrixVTC& operator*=(float d);
	// The PDB's operator*(float): the matrix times s, built in apResult (the by-value result; thiscall, ret 8,
	// 0x658260). src names it MulScalar because it takes the hidden result pointer.
	FR2MatrixVTC* MulScalar(FR2MatrixVTC* apResult, float s) const;
	// The PDB's operator*(const FR2MatrixVTC&): the matrix product with m, built in apResult (the by-value result;
	// thiscall, ret 8, 0x658360). src names it Mul because it takes the hidden result pointer.
	FR2MatrixVTC* Mul(FR2MatrixVTC* apResult, const FR2MatrixVTC& m) const;
	// The PDB's operator+: the element-wise sum with m, built in apResult (the by-value result; thiscall, ret 8,
	// 0x658500). src names it Add because it takes the hidden result pointer.
	FR2MatrixVTC* Add(FR2MatrixVTC* apResult, const FR2MatrixVTC& m) const;
	// The transpose, built in apResult (the by-value result; thiscall, ret 4, 0x6587A0).
	FR2MatrixVTC* transpose(FR2MatrixVTC* apResult) const;
	// The PDB's operator/(float): the matrix times 1/s through MulScalar, built in apResult
	// (the by-value result; thiscall, ret 8, 0xAADDA0). src names it DivScalar because it takes the hidden result
	// pointer.
	FR2MatrixVTC* DivScalar(FR2MatrixVTC* apResult, float s) const;
	// The PDB's operator-(const FR2MatrixVTC&): the element-wise difference with m, built in
	// apResult (the by-value result; thiscall, ret 8, 0xAAED70). src names it Sub because it takes the hidden result
	// pointer.
	FR2MatrixVTC* Sub(FR2MatrixVTC* apResult, const FR2MatrixVTC& m) const;
	// The nr x nc block starting at row r and column c, built in apResult (the by-value
	// result); the block must fit (fr2Assert; thiscall, ret 0x14, 0xAAF110).
	FR2MatrixVTC* subMatrix(FR2MatrixVTC* apResult, uint32_t r, uint32_t c, uint32_t nr, uint32_t nc) const;

	// DeleteArray, the exe's vector deleting destructor, frees with ourFree.
	static void operator delete(void* apMemory);
	static void operator delete[](void* apMemory);

	// The elements, row by row: the exe's VC9 std::vector<T> (checked iterators).
	std::vector<T, std::allocator<T>> data;
	uint32_t nrows;
	uint32_t ncols;
};
// The generic members are defined in FR2MatrixVTC.inl and instantiated in FR2MatrixVTC.cpp only.
extern template class FR2MatrixVTC<float>;
