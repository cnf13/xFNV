#include "Fallout Shared/FaceGen/BSFaceGenImage.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/XINISetting.h"
#include "BSShader/BSShaderUtil.h"
#include "BSShader/BoltShader.h"
#include "Fallout Shared/FaceGen/BSFaceGenModel.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDevImageConverter.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "SDK/FaceGen/TriFileIO.h"
#include "SDK/MSVC/crtdefs.h"

#include <cstdint>
#include <memory>
#include <vector>

SettingT<INISettingCollection> BSFaceGenImage::bFaceMipMaps("bFaceMipMaps:General", true);

// The const pixel iterator's operator*: reports a missing container or an end position through
// _invalid_parameter_noinfo, then reads the element.
const BSFaceGenRGBA& VectorIteratorDeref(const BSFaceGenRGBAVector::const_iterator* apThis) {
    // The iterator's own operator* (its checks are the standard library's).
    return **apThis;
}

// The pixel iterator's operator*: the const iterator's on the same object, its reference made writable.
BSFaceGenRGBA& VectorIteratorDeref(const BSFaceGenRGBAVector::iterator* apThis) {
    return **apThis;
}

BSFaceGenImage::BSFaceGenImage(NiSourceTexture* apImage) : spCompressedImage(apImage) {
    if (spCompressedImage.m_pObject) {
        iWidth = spCompressedImage->GetWidth();
        iHeight = spCompressedImage->GetHeight();
    }
    else {
        iHeight = 0;
        iWidth = 0;
    }
}

// The members' own destructors clear the pixels and release the texture, in that order.
BSFaceGenImage::~BSFaceGenImage() {}

// Each channel is clamped to [afUncompressedMin, afUncompressedMax] through float temporaries, floored, offset and
// scaled, then truncated to an int whose low byte is stored.
NiSourceTexture* BSFaceGenImage::GetCompressedImage(
    float afUncompressedMin,
    float afUncompressedMax,
    float afScale,
    const NiPixelFormat& aFormat
) {
    BSFaceGenRGBAVector::iterator kIter;
    if (!spCompressedImage.m_pObject &&
        !UncompressedImage.empty() &&
        this->iWidth &&
        this->iHeight &&
        afUncompressedMax > static_cast<double>(afUncompressedMin)) {
        float fChannel;
        NiDevImageConverter kImageConverter;
        NiPointer<NiSourceTexture> spTex(static_cast<NiSourceTexture*>(nullptr));
        NiPixelData* pPixelData = nullptr;
        NiPixelData* pPixelDataDest = nullptr;
        kIter = this->UncompressedImage.begin();
        uint32_t uiPixels = this->iHeight * this->iWidth;
        uint8_t* pBuffer = nullptr;
        if (aFormat.m_eFormat) {
            pPixelData = new NiPixelData(this->iWidth, this->iHeight, NiPixelFormat::RGBA32, 1u, 1u);
            pBuffer = pPixelData->GetPixels(0u, 0u);
            for (uint32_t i = 0; i < uiPixels; ++i) {
                float fClamped;
                double dFloor;
                fChannel = VectorIteratorDeref(&kIter).pColor[0];
                if (!(afUncompressedMin <= static_cast<double>(fChannel))) {
                    fClamped = afUncompressedMin;
                }
                else if (!(afUncompressedMax >= static_cast<double>(fChannel))) {
                    fClamped = afUncompressedMax;
                }
                else {
                    fClamped = fChannel;
                }
                dFloor = FloatFloor(fClamped);
                pBuffer[0] = static_cast<int32_t>((dFloor - afUncompressedMin) * afScale);
                fChannel = VectorIteratorDeref(&kIter).pColor[1];
                if (!(afUncompressedMin <= static_cast<double>(fChannel))) {
                    fClamped = afUncompressedMin;
                }
                else if (!(afUncompressedMax >= static_cast<double>(fChannel))) {
                    fClamped = afUncompressedMax;
                }
                else {
                    fClamped = fChannel;
                }
                dFloor = FloatFloor(fClamped);
                pBuffer[1] = static_cast<int32_t>((dFloor - afUncompressedMin) * afScale);
                fChannel = VectorIteratorDeref(&kIter).pColor[2];
                if (!(afUncompressedMin <= static_cast<double>(fChannel))) {
                    fClamped = afUncompressedMin;
                }
                else if (!(afUncompressedMax >= static_cast<double>(fChannel))) {
                    fClamped = afUncompressedMax;
                }
                else {
                    fClamped = fChannel;
                }
                dFloor = FloatFloor(fClamped);
                pBuffer[2] = static_cast<int32_t>((dFloor - afUncompressedMin) * afScale);
                fChannel = VectorIteratorDeref(&kIter).pColor[3];
                if (!(afUncompressedMin <= static_cast<double>(fChannel))) {
                    fClamped = afUncompressedMin;
                }
                else if (!(afUncompressedMax >= static_cast<double>(fChannel))) {
                    fClamped = afUncompressedMax;
                }
                else {
                    fClamped = fChannel;
                }
                dFloor = FloatFloor(fClamped);
                pBuffer[3] = static_cast<int32_t>((dFloor - afUncompressedMin) * afScale);
                pBuffer += 4;
                kIter++;
            }
        }
        else {
            pPixelData = new NiPixelData(this->iWidth, this->iHeight, NiPixelFormat::RGB24, 1u, 1u);
            pBuffer = pPixelData->GetPixels(0u, 0u);
            for (uint32_t j = 0; j < uiPixels; ++j) {
                float fClamped;
                double dFloor;
                fChannel = VectorIteratorDeref(&kIter).pColor[0];
                if (!(afUncompressedMin <= static_cast<double>(fChannel))) {
                    fClamped = afUncompressedMin;
                }
                else if (!(afUncompressedMax >= static_cast<double>(fChannel))) {
                    fClamped = afUncompressedMax;
                }
                else {
                    fClamped = fChannel;
                }
                dFloor = FloatFloor(fClamped);
                pBuffer[0] = static_cast<int32_t>((dFloor - afUncompressedMin) * afScale);
                fChannel = VectorIteratorDeref(&kIter).pColor[1];
                if (!(afUncompressedMin <= static_cast<double>(fChannel))) {
                    fClamped = afUncompressedMin;
                }
                else if (!(afUncompressedMax >= static_cast<double>(fChannel))) {
                    fClamped = afUncompressedMax;
                }
                else {
                    fClamped = fChannel;
                }
                dFloor = FloatFloor(fClamped);
                pBuffer[1] = static_cast<int32_t>((dFloor - afUncompressedMin) * afScale);
                fChannel = VectorIteratorDeref(&kIter).pColor[2];
                if (!(afUncompressedMin <= static_cast<double>(fChannel))) {
                    fClamped = afUncompressedMin;
                }
                else if (!(afUncompressedMax >= static_cast<double>(fChannel))) {
                    fClamped = afUncompressedMax;
                }
                else {
                    fClamped = fChannel;
                }
                dFloor = FloatFloor(fClamped);
                pBuffer[2] = static_cast<int32_t>((dFloor - afUncompressedMin) * afScale);
                pBuffer += 3;
                kIter++;
            }
        }
        if (BSFaceGenImage::bFaceMipMaps) {
            pPixelDataDest = kImageConverter.NiDevImageConverter::GenerateMipmapLevels(pPixelData, nullptr);
            if (pPixelData) {
                delete pPixelData;
            }
            if (BSShaderManager::GetTintMipMaps()) {
                BSShaderUtil::TintUncompressedPixelDataByMipMap(pPixelDataDest);
            }
            spTex = NiSourceTexture::Create(pPixelDataDest, nullptr, NiSourceTexture::ms_kDefaultPrefs);
        }
        else {
            if (BSShaderManager::GetTintMipMaps()) {
                BSShaderUtil::TintUncompressedPixelDataByMipMap(pPixelData);
            }
            spTex = NiSourceTexture::Create(pPixelData, nullptr, NiSourceTexture::ms_kDefaultPrefs);
        }
        if (aFormat.m_eFormat != NiPixelFormat::FORMAT_RGBA && aFormat.m_eFormat) {
            this->spCompressedImage = BSShaderUtil::ConvertTexture(spTex.m_pObject, aFormat);
        }
        else {
            this->spCompressedImage = spTex;
        }
        if (!this->spCompressedImage.m_pObject) {
            this->spCompressedImage = spTex;
        }
    }

    return this->spCompressedImage.m_pObject;
}

std::vector<BSFaceGenRGBA, std::allocator<BSFaceGenRGBA>>* BSFaceGenImage::GetUncompressedImage() {
    if (UncompressedImage.empty() && spCompressedImage.m_pObject) {
        BSFaceGenRGBA kEmtpyRGBA;
        NiPointer<NiSourceTexture> spUncompressedTex(
            BSShaderUtil::ConvertTexture(spCompressedImage.m_pObject, NiPixelFormat::RGBA32)
        );
        if (!spUncompressedTex.m_pObject) {
            return &UncompressedImage;
        }
        const NiPixelData* pPixelData = static_cast<const NiPixelData*>(spUncompressedTex->GetSrcPixelData());
        const uint8_t* pBuffer = pPixelData->GetPixels(0u, 0u);
        uint32_t uiWidth = pPixelData->GetWidth(0, 0);
        uint32_t uiPixels = pPixelData->GetHeight(0, 0) * uiWidth;
        kEmtpyRGBA.pColor[3] = 0.0f;
        kEmtpyRGBA.pColor[2] = 0.0f;
        kEmtpyRGBA.pColor[1] = 0.0f;
        kEmtpyRGBA.pColor[0] = 0.0f;
        UncompressedImage.assign(uiPixels, kEmtpyRGBA);
        for (uint32_t i = 0; i < uiPixels; ++i) {
            UncompressedImage[i].pColor[0] = static_cast<float>(pBuffer[4 * i]);
            UncompressedImage[i].pColor[1] = static_cast<float>(pBuffer[4 * i + 1]);
            UncompressedImage[i].pColor[2] = static_cast<float>(pBuffer[4 * i + 2]);
            UncompressedImage[i].pColor[3] = static_cast<float>(pBuffer[4 * i + 3]);
        }
    }
    return &UncompressedImage;
}
