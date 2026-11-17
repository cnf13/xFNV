#pragma once

#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Reconstruction.hpp"

#include <__stddef_size_t.h>
#include <memory>
#include <vector>

class NiPixelFormat;
class NiSourceTexture;

// One pixel of the uncompressed face image: red, green, blue and alpha as floats (GetUncompressedImage 0x64D880
// stores the RGBA32 bytes at +0, +4, +8, +0xC).
struct BSFaceGenRGBA {
	float pColor[4];
};

// The pixel vector under one name: a macro argument can't hold the template's commas.
using BSFaceGenRGBAVector = std::vector<BSFaceGenRGBA, std::allocator<BSFaceGenRGBA>>;

// The out-of-line VC9 helpers of UncompressedImage's vector<BSFaceGenRGBA> (16-byte elements). Their bodies live in
// BSFaceGenImage.cpp. All are IDA names, not in the PDB; _Al is the vector's allocator and the tags are bytes.
// _Destroy_range(_First, _Last, _Al, _Nonscalar_ptr_iterator_tag): destroys the pixels one by
// one through _Al.destroy (cdecl, 0x64E600).
void DestroyRange(BSFaceGenRGBA* _First, BSFaceGenRGBA* _Last, std::allocator<BSFaceGenRGBA>& _Al, uint8_t _Cat);
// _Destroy_range(_First, _Last, _Al): passes the pixels on with their category tag.
void DestroyRange(BSFaceGenRGBA* _First, BSFaceGenRGBA* _Last, std::allocator<BSFaceGenRGBA>& _Al);
// _Copy_opt(_First, _Last, _Dest, forward_iterator_tag, _Nonscalar_ptr_iterator_tag,
// _Range_checked_iterator_tag): copies the pixels one by one and returns the end of the copy.
BSFaceGenRGBA* CopyOpt(BSFaceGenRGBA* _First, BSFaceGenRGBA* _Last, BSFaceGenRGBA* _Dest, uint8_t _IterCat,
	uint8_t _PtrCat, uint8_t _Checked);
// _Copy_opt(_First, _Last, _Dest, random_access_iterator_tag, _Nonscalar_ptr_iterator_tag,
// _Range_checked_iterator_tag): unchecks _Dest and copies; returns _Dest + (_Last - _First).
BSFaceGenRGBA* Copy(BSFaceGenRGBA* _First, BSFaceGenRGBA* _Last, BSFaceGenRGBA* _Dest, uint8_t _IterCat,
	uint8_t _PtrCat, uint8_t _Checked);
// unchecked_copy(_First, _Last, _Dest): unchecks the source and copies with the category tags.
BSFaceGenRGBA* _Move(BSFaceGenRGBA* _First, BSFaceGenRGBA* _Last, BSFaceGenRGBA* _Dest);
// _Uninit_copy(_First, _Last, _Dest, _Al, _Nonscalar_ptr_iterator_tag,
// _Range_checked_iterator_tag): copy-constructs the pixels at _Dest; returns the end of the copy.
BSFaceGenRGBA* UninitMove(BSFaceGenRGBA* _First, BSFaceGenRGBA* _Last, BSFaceGenRGBA* _Dest,
	std::allocator<BSFaceGenRGBA>& _Al, uint8_t _PtrCat, uint8_t _Checked);
// _Uninitialized_copy(_First, _Last, _Dest, _Al): unchecks the source and copy-constructs with the category tags.
BSFaceGenRGBA* UninitializedMove(BSFaceGenRGBA* _First, BSFaceGenRGBA* _Last, BSFaceGenRGBA* _Dest,
	std::allocator<BSFaceGenRGBA>& _Al);
// unchecked_uninitialized_copy(_First, _Last, _Dest, _Al), the callee of UninitMove above:
// unchecks the source and hands it to _Uninit_copy with the pointer category tag of _First and _Dest and a
// _Range_checked_iterator_tag.
BSFaceGenRGBA* UncheckedUninitializedCopy(BSFaceGenRGBA* _First, BSFaceGenRGBA* _Last, BSFaceGenRGBA* _Dest,
	std::allocator<BSFaceGenRGBA>& _Al);
// _Umove(_First, _Last, _Ptr) of the pixel vector: moves the pixels into the raw storage at
// _Ptr and returns the end. src's vector models no _Umove member, so the vector comes
// first.
BSFaceGenRGBA* VectorUmove(BSFaceGenRGBAVector* _This, BSFaceGenRGBA* _First, BSFaceGenRGBA* _Last,
	BSFaceGenRGBA* _Ptr);
// fill(_First, _Last, _Val) of the pixels.
void Fill(BSFaceGenRGBA* _First, BSFaceGenRGBA* _Last, const BSFaceGenRGBA& _Val);
// _Unchecked_move_backward(_First, _Last, _Dest) of the pixels; unregistered: it passes an
// uninitialised iterator tag on.
void UncheckedMoveBackward(BSFaceGenRGBA* _First, BSFaceGenRGBA* _Last, BSFaceGenRGBA* _Dest);
// The const pixel iterator's checked operator*.
// Same function as VectorIteratorDeref (0x64DDA0), registered there (FR2MatrixVTC.h, the float iterator's).
const BSFaceGenRGBA& VectorIteratorDeref(const BSFaceGenRGBAVector::const_iterator* apThis);
// Same function as VectorIteratorDeref (0x64DBD0), registered there (FR2MatrixVTC.h, the float iterator's). The pixel
// iterator's operator*, through the checked const
// one (thiscall, ret 0); GetCompressedImage (0x64CEB0) reads each pixel's channels through it.
BSFaceGenRGBA& VectorIteratorDeref(const BSFaceGenRGBAVector::iterator* apThis);
// Not named in the PDB or IDA; name chosen here. VC9's unchecked_uninitialized_fill_n(_First, _Count, _Val, _Al):
// _Uninit_fill_n (0x64E710) with the pointer category tag of _First and a _Range_checked_iterator_tag (cdecl, ret 0;
// eax is _Uninit_fill_n's result).
BSFaceGenRGBA* UncheckedUninitializedFillN(BSFaceGenRGBA* _First, size_t _Count, const BSFaceGenRGBA& _Val,
	std::allocator<BSFaceGenRGBA>& _Al);
// Not named in the PDB or IDA; name chosen here. VC9's _Fill(_First, _Last, _Val) of the pixels (cdecl, ret 0; void
// in VC9: eax is whatever the loop left, and the one caller, 0x64E4B0, ignores it).
void FillUnchecked(BSFaceGenRGBA* _First, BSFaceGenRGBA* _Last, const BSFaceGenRGBA& _Val);
// Not named in the PDB or IDA; name chosen here. VC9's _Move_backward_opt(_First, _Last, _Dest, _Iter_cat,
// _Undefined_move_tag, _Range_checked_iterator_tag): _Copy_backward_opt (0x64E860) with a fresh pointer category
// tag of _First and _Dest (cdecl, ret 0). The exe reads only the iterator tag of the three tag bytes its caller
// pushes.
BSFaceGenRGBA* MoveBackwardOpt(BSFaceGenRGBA* _First, BSFaceGenRGBA* _Last, BSFaceGenRGBA* _Dest, uint8_t _IterCat,
	uint8_t _MoveCat, uint8_t _Checked);
// VC9's _Copy_backward_opt(_First, _Last, _Dest,
// random_access_iterator_tag, _Nonscalar_ptr_iterator_tag, _Range_checked_iterator_tag): unchecks _Dest and copies
// backward; returns _Dest - (_Last - _First).
BSFaceGenRGBA* CopyBackward(BSFaceGenRGBA* _First, BSFaceGenRGBA* _Last, BSFaceGenRGBA* _Dest, uint8_t _IterCat,
	uint8_t _PtrCat, uint8_t _Checked);
// VC9's _Copy_backward_opt(_First, _Last, _Dest,
// bidirectional_iterator_tag, _Nonscalar_ptr_iterator_tag, _Range_checked_iterator_tag): copies the pixels backward
// one by one and returns the start of the copy.
BSFaceGenRGBA* CopyBackwardOpt(BSFaceGenRGBA* _First, BSFaceGenRGBA* _Last, BSFaceGenRGBA* _Dest, uint8_t _IterCat,
	uint8_t _PtrCat, uint8_t _Checked);

class BSFaceGenImage : public NiRefObject {
public:
	BSFaceGenImage(NiSourceTexture* apImage);
	~BSFaceGenImage() override;

	// thiscall, ret 0x10: the three floats by value, the format by address.
	NiSourceTexture* GetCompressedImage(float afUncompressedMin, float afUncompressedMax, float afScale,
		const NiPixelFormat& aFormat);
	std::vector<BSFaceGenRGBA, std::allocator<BSFaceGenRGBA>>* GetUncompressedImage();

	NiPointer<NiSourceTexture> spCompressedImage;
	// The PDB's std::vector<BSFaceGenRGBA>. IDA types it std::string_release (0x18 bytes, no aux word), but the exe
	// uses it as the vector: its size is the span from +0xC to +0x10 shifted right by 4 (0x64DC30) and operator[] adds
	// index * 16 to the first element at +0xC (0x64DB20).
	std::vector<BSFaceGenRGBA, std::allocator<BSFaceGenRGBA>> UncompressedImage;
	uint32_t iWidth;
	uint32_t iHeight;
};

inline constexpr fnvd::Vtable BSFaceGenImage_vftable{};

// INISettingCollection: 0xf8160a calls 0x45cf30.
extern SettingT<INISettingCollection> gs_bFaceMipMaps_General;
