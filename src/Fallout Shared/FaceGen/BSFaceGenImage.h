#pragma once

#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

#include <__stddef_size_t.h>
#include <memory>
#include <vector>

class NiPixelFormat;
class NiSourceTexture;

// One pixel of the uncompressed face image: red, green, blue and alpha as floats.
struct BSFaceGenRGBA {
    float pColor[4];
};

// The pixel vector under one name: a macro argument can't hold the template's commas.
using BSFaceGenRGBAVector = std::vector<BSFaceGenRGBA, std::allocator<BSFaceGenRGBA>>;

// Helpers of UncompressedImage's vector<BSFaceGenRGBA> (16-byte elements). Their bodies live in BSFaceGenImage.cpp. _Al
// is the vector's allocator and the tags are bytes. DestroyRange(_First, _Last, _Al, _Cat): destroys the pixels one by
// one through _Al.destroy.
void DestroyRange(BSFaceGenRGBA* _First, BSFaceGenRGBA* _Last, std::allocator<BSFaceGenRGBA>& _Al, uint8_t _Cat);
// _Destroy_range(_First, _Last, _Al): passes the pixels on with their category tag.
void DestroyRange(BSFaceGenRGBA* _First, BSFaceGenRGBA* _Last, std::allocator<BSFaceGenRGBA>& _Al);
// _Copy_opt(_First, _Last, _Dest, forward_iterator_tag, _Nonscalar_ptr_iterator_tag,
// _Range_checked_iterator_tag): copies the pixels one by one and returns the end of the copy.
BSFaceGenRGBA* CopyOpt(
    BSFaceGenRGBA* _First,
    BSFaceGenRGBA* _Last,
    BSFaceGenRGBA* _Dest,
    uint8_t _IterCat,
    uint8_t _PtrCat,
    uint8_t _Checked
);
// _Copy_opt(_First, _Last, _Dest, random_access_iterator_tag, _Nonscalar_ptr_iterator_tag,
// _Range_checked_iterator_tag): unchecks _Dest and copies; returns _Dest + (_Last - _First).
BSFaceGenRGBA* Copy(
    BSFaceGenRGBA* _First,
    BSFaceGenRGBA* _Last,
    BSFaceGenRGBA* _Dest,
    uint8_t _IterCat,
    uint8_t _PtrCat,
    uint8_t _Checked
);
// unchecked_copy(_First, _Last, _Dest): unchecks the source and copies with the category tags.
BSFaceGenRGBA* _Move(BSFaceGenRGBA* _First, BSFaceGenRGBA* _Last, BSFaceGenRGBA* _Dest);
// _Uninit_copy(_First, _Last, _Dest, _Al, _Nonscalar_ptr_iterator_tag,
// _Range_checked_iterator_tag): copy-constructs the pixels at _Dest; returns the end of the copy.
BSFaceGenRGBA* UninitMove(
    BSFaceGenRGBA* _First,
    BSFaceGenRGBA* _Last,
    BSFaceGenRGBA* _Dest,
    std::allocator<BSFaceGenRGBA>& _Al,
    uint8_t _PtrCat,
    uint8_t _Checked
);
// _Uninitialized_copy(_First, _Last, _Dest, _Al): unchecks the source and copy-constructs with the category tags.
BSFaceGenRGBA* UninitializedMove(
    BSFaceGenRGBA* _First,
    BSFaceGenRGBA* _Last,
    BSFaceGenRGBA* _Dest,
    std::allocator<BSFaceGenRGBA>& _Al
);
// unchecked_uninitialized_copy(_First, _Last, _Dest, _Al), the callee of UninitMove above:
// unchecks the source and hands it to _Uninit_copy with the pointer category tag of _First and _Dest and a
// _Range_checked_iterator_tag.
BSFaceGenRGBA* UncheckedUninitializedCopy(
    BSFaceGenRGBA* _First,
    BSFaceGenRGBA* _Last,
    BSFaceGenRGBA* _Dest,
    std::allocator<BSFaceGenRGBA>& _Al
);
// _Umove(_First, _Last, _Ptr) of the pixel vector: moves the pixels into the raw storage at _Ptr and returns the end.
// The vector type has no _Umove member, so the vector is passed as the first parameter.
BSFaceGenRGBA*
VectorUmove(BSFaceGenRGBAVector* _This, BSFaceGenRGBA* _First, BSFaceGenRGBA* _Last, BSFaceGenRGBA* _Ptr);
// fill(_First, _Last, _Val) of the pixels.
void Fill(BSFaceGenRGBA* _First, BSFaceGenRGBA* _Last, const BSFaceGenRGBA& _Val);
// _Unchecked_move_backward(_First, _Last, _Dest) of the pixels.
void UncheckedMoveBackward(BSFaceGenRGBA* _First, BSFaceGenRGBA* _Last, BSFaceGenRGBA* _Dest);
// The const pixel iterator's checked operator*.
const BSFaceGenRGBA& VectorIteratorDeref(const BSFaceGenRGBAVector::const_iterator* apThis);
// The pixel iterator's operator*, through the checked const one; GetCompressedImage reads each pixel's channels through
// it.
BSFaceGenRGBA& VectorIteratorDeref(const BSFaceGenRGBAVector::iterator* apThis);
// Equivalent of unchecked_uninitialized_fill_n(_First, _Count, _Val, _Al): calls _Uninit_fill_n with the pointer
// category tag of _First and a _Range_checked_iterator_tag, and returns _Uninit_fill_n's result.
BSFaceGenRGBA* UncheckedUninitializedFillN(
    BSFaceGenRGBA* _First,
    size_t _Count,
    const BSFaceGenRGBA& _Val,
    std::allocator<BSFaceGenRGBA>& _Al
);
// _Fill(_First, _Last, _Val) of the pixels.
void FillUnchecked(BSFaceGenRGBA* _First, BSFaceGenRGBA* _Last, const BSFaceGenRGBA& _Val);
// Moves backward by calling CopyBackward with a fresh pointer category tag of _First and _Dest. Of the three tag
// parameters, only the iterator tag is read.
BSFaceGenRGBA* MoveBackwardOpt(
    BSFaceGenRGBA* _First,
    BSFaceGenRGBA* _Last,
    BSFaceGenRGBA* _Dest,
    uint8_t _IterCat,
    uint8_t _MoveCat,
    uint8_t _Checked
);
// Copies [_First, _Last) backward so that it ends at _Dest; returns _Dest - (_Last - _First).
BSFaceGenRGBA* CopyBackward(
    BSFaceGenRGBA* _First,
    BSFaceGenRGBA* _Last,
    BSFaceGenRGBA* _Dest,
    uint8_t _IterCat,
    uint8_t _PtrCat,
    uint8_t _Checked
);
// Copies the pixels backward one by one and returns the start of the copy.
BSFaceGenRGBA* CopyBackwardOpt(
    BSFaceGenRGBA* _First,
    BSFaceGenRGBA* _Last,
    BSFaceGenRGBA* _Dest,
    uint8_t _IterCat,
    uint8_t _PtrCat,
    uint8_t _Checked
);

class BSFaceGenImage : public NiRefObject {
public:
    BSFaceGenImage(NiSourceTexture* apImage);
    ~BSFaceGenImage() override;

    NiSourceTexture*
    GetCompressedImage(float afUncompressedMin, float afUncompressedMax, float afScale, const NiPixelFormat& aFormat);
    std::vector<BSFaceGenRGBA, std::allocator<BSFaceGenRGBA>>* GetUncompressedImage();

    NiPointer<NiSourceTexture> spCompressedImage;
    std::vector<BSFaceGenRGBA, std::allocator<BSFaceGenRGBA>> UncompressedImage;
    uint32_t iWidth;
    uint32_t iHeight;
};

extern SettingT<INISettingCollection> gs_bFaceMipMaps_General;
