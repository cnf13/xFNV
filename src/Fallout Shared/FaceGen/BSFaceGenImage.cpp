#include "Fallout Shared/FaceGen/BSFaceGenImage.h"

#include "BSCore/MemoryManager.h"
#include "BSShader/BSShaderUtil.h"
#include "BSShader/BoltShader.h"
#include "Fallout Shared/FaceGen/BSFaceGenModel.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDevImageConverter.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Reconstruction.hpp"
#include "SDK/FaceGen/TriFileIO.h"
#include "SDK/MSVC/crtdefs.h"

#include <memory>
#include <stdint.h>
#include <vector>

SettingT<INISettingCollection> gs_bFaceMipMaps_General("bFaceMipMaps:General", true);

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
        iWidth = spCompressedImage.m_pObject->GetWidth();
        iHeight = spCompressedImage.m_pObject->GetHeight();
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
    BSFaceGenRGBAVector::iterator v65;
    BSFaceGenRGBAVector::iterator v66;
    if (!spCompressedImage.m_pObject && !UncompressedImage.empty()) {
        if (this->iWidth) {
            if (this->iHeight) {
                if (afUncompressedMax > static_cast<double>(afUncompressedMin)) {
                    float v58;
                    NiDevImageConverter imageConverter;
                    NiPointer<NiSourceTexture> sptex(static_cast<NiSourceTexture*>(0));
                    NiPixelData* ppixeldata = 0;
                    NiPixelData* ppixeldataDest = 0;
                    BSFaceGenRGBAVector::iterator* v6 =
                        ::new (&v66) BSFaceGenRGBAVector::iterator(this->UncompressedImage.begin());
                    v65 = *v6;
                    uint32_t v64 = this->iHeight * this->iWidth;
                    uint8_t* pbuffer = 0;
                    if (aFormat.m_eFormat) {
                        NiPixelData* ppixeldata_3;
                        NiPixelData* v41 = static_cast<NiPixelData*>(NiMemObject::operator new(sizeof(NiPixelData)));
                        if (v41) {
                            ppixeldata_3 =
                                ::new (v41) NiPixelData(this->iWidth, this->iHeight, NiPixelFormat_RGBA32, 1u, 1u);
                        }
                        else {
                            ppixeldata_3 = 0;
                        }
                        ppixeldata = ppixeldata_3;
                        pbuffer = ppixeldata_3->GetPixels(0u, 0u);
                        for (uint32_t i = 0; i < v64; ++i) {
                            float v21;
                            float v26;
                            float v31;
                            float v36;
                            v58 = VectorIteratorDeref(&v65).pColor[0];
                            if (!(afUncompressedMin <= static_cast<double>(v58))) {
                                v36 = afUncompressedMin;
                            }
                            else {
                                float v37;
                                if (!(afUncompressedMax >= static_cast<double>(v58))) {
                                    v37 = afUncompressedMax;
                                }
                                else {
                                    v37 = v58;
                                }
                                v36 = v37;
                            }
                            double v11 = FloatFloor(v36);
                            int32_t v33 = static_cast<int32_t>((v11 - afUncompressedMin) * afScale);
                            *pbuffer = v33;
                            v58 = VectorIteratorDeref(&v65).pColor[1];
                            if (!(afUncompressedMin <= static_cast<double>(v58))) {
                                v31 = afUncompressedMin;
                            }
                            else {
                                float v32;
                                if (!(afUncompressedMax >= static_cast<double>(v58))) {
                                    v32 = afUncompressedMax;
                                }
                                else {
                                    v32 = v58;
                                }
                                v31 = v32;
                            }
                            double v12 = FloatFloor(v31);
                            int32_t v28 = static_cast<int32_t>((v12 - afUncompressedMin) * afScale);
                            pbuffer[1] = v28;
                            v58 = VectorIteratorDeref(&v65).pColor[2];
                            if (!(afUncompressedMin <= static_cast<double>(v58))) {
                                v26 = afUncompressedMin;
                            }
                            else {
                                float v27;
                                if (!(afUncompressedMax >= static_cast<double>(v58))) {
                                    v27 = afUncompressedMax;
                                }
                                else {
                                    v27 = v58;
                                }
                                v26 = v27;
                            }
                            double v13 = FloatFloor(v26);
                            int32_t v23 = static_cast<int32_t>((v13 - afUncompressedMin) * afScale);
                            pbuffer[2] = v23;
                            v58 = VectorIteratorDeref(&v65).pColor[3];
                            if (!(afUncompressedMin <= static_cast<double>(v58))) {
                                v21 = afUncompressedMin;
                            }
                            else {
                                float v22;
                                if (!(afUncompressedMax >= static_cast<double>(v58))) {
                                    v22 = afUncompressedMax;
                                }
                                else {
                                    v22 = v58;
                                }
                                v21 = v22;
                            }
                            double v14 = FloatFloor(v21);
                            int32_t v18 = static_cast<int32_t>((v14 - afUncompressedMin) * afScale);
                            pbuffer[3] = v18;
                            pbuffer += 4;
                            v65.operator++(0);
                        }
                    }
                    else {
                        NiPixelData* ppixeldata_2;
                        NiPixelData* v62 = static_cast<NiPixelData*>(NiMemObject::operator new(sizeof(NiPixelData)));
                        if (v62) {
                            ppixeldata_2 =
                                ::new (v62) NiPixelData(this->iWidth, this->iHeight, NiPixelFormat_RGB24, 1u, 1u);
                        }
                        else {
                            ppixeldata_2 = 0;
                        }
                        ppixeldata = ppixeldata_2;
                        pbuffer = ppixeldata_2->GetPixels(0u, 0u);
                        for (uint32_t j = 0; j < v64; ++j) {
                            float v46;
                            float v51;
                            float v56;
                            v58 = VectorIteratorDeref(&v65).pColor[0];
                            if (!(afUncompressedMin <= static_cast<double>(v58))) {
                                v56 = afUncompressedMin;
                            }
                            else {
                                float v57;
                                if (!(afUncompressedMax >= static_cast<double>(v58))) {
                                    v57 = afUncompressedMax;
                                }
                                else {
                                    v57 = v58;
                                }
                                v56 = v57;
                            }
                            double v8 = FloatFloor(v56);
                            int32_t v53 = static_cast<int32_t>((v8 - afUncompressedMin) * afScale);
                            *pbuffer = v53;
                            v58 = VectorIteratorDeref(&v65).pColor[1];
                            if (!(afUncompressedMin <= static_cast<double>(v58))) {
                                v51 = afUncompressedMin;
                            }
                            else {
                                float v52;
                                if (!(afUncompressedMax >= static_cast<double>(v58))) {
                                    v52 = afUncompressedMax;
                                }
                                else {
                                    v52 = v58;
                                }
                                v51 = v52;
                            }
                            double v9 = FloatFloor(v51);
                            int32_t v48 = static_cast<int32_t>((v9 - afUncompressedMin) * afScale);
                            pbuffer[1] = v48;
                            v58 = VectorIteratorDeref(&v65).pColor[2];
                            if (!(afUncompressedMin <= static_cast<double>(v58))) {
                                v46 = afUncompressedMin;
                            }
                            else {
                                float v47;
                                if (!(afUncompressedMax >= static_cast<double>(v58))) {
                                    v47 = afUncompressedMax;
                                }
                                else {
                                    v47 = v58;
                                }
                                v46 = v47;
                            }
                            double v10 = FloatFloor(v46);
                            int32_t v43 = static_cast<int32_t>((v10 - afUncompressedMin) * afScale);
                            pbuffer[2] = v43;
                            pbuffer += 3;
                            v65.operator++(0);
                        }
                    }
                    if (gs_bFaceMipMaps_General.operator bool()) {
                        ppixeldataDest = imageConverter.NiDevImageConverter::GenerateMipmapLevels(ppixeldata, 0);
                        if (ppixeldata) {
                            delete ppixeldata;
                        }
                        if (BSShaderManager::GetTintMipMaps()) {
                            BSShaderUtil::TintUncompressedPixelDataByMipMap(ppixeldataDest);
                        }
                        sptex.operator=(NiSourceTexture::Create(ppixeldataDest, 0, NiSourceTexture::ms_kDefaultPrefs));
                    }
                    else {
                        if (BSShaderManager::GetTintMipMaps()) {
                            BSShaderUtil::TintUncompressedPixelDataByMipMap(ppixeldata);
                        }
                        sptex.operator=(NiSourceTexture::Create(ppixeldata, 0, NiSourceTexture::ms_kDefaultPrefs));
                    }
                    if (aFormat.m_eFormat != NiPixelFormat::FORMAT_RGBA && aFormat.m_eFormat) {
                        NiSourceTexture* x = BSShaderUtil::ConvertTexture(sptex.m_pObject, aFormat);
                        this->spCompressedImage.operator=(x);
                    }
                    else {
                        this->spCompressedImage.operator=(sptex);
                    }
                    if (!this->spCompressedImage.m_pObject) {
                        this->spCompressedImage.operator=(sptex);
                    }
                }
            }
        }
    }
    return this->spCompressedImage.m_pObject;
}

std::vector<BSFaceGenRGBA, std::allocator<BSFaceGenRGBA>>* BSFaceGenImage::GetUncompressedImage() {
    FNVD_INLINED_LOCAL(NiPointer<NiSourceTexture>, spUncompressedTex, std::nullopt, std::nullopt);
    if (UncompressedImage.empty() && spCompressedImage.m_pObject) {
        BSFaceGenRGBA emtpyRGBA;
        ::new (&spUncompressedTex)
            NiPointer<NiSourceTexture>(BSShaderUtil::ConvertTexture(spCompressedImage.m_pObject, NiPixelFormat_RGBA32));
        if (!spUncompressedTex.m_pObject) {
            spUncompressedTex.~NiPointer<NiSourceTexture>();
            return &UncompressedImage;
        }
        const NiPixelData* ppixeldata = static_cast<const NiPixelData*>(spUncompressedTex.m_pObject->GetSrcPixelData());
        const uint8_t* pbuffer = ppixeldata->GetPixels(0u, 0u);
        uint32_t Width = ppixeldata->GetWidth(0, 0);
        uint32_t ipixels = ppixeldata->GetHeight(0, 0) * Width;
        emtpyRGBA.pColor[3] = 0.0f;
        emtpyRGBA.pColor[2] = 0.0f;
        emtpyRGBA.pColor[1] = 0.0f;
        emtpyRGBA.pColor[0] = 0.0f;
        UncompressedImage.assign(ipixels, emtpyRGBA);
        for (uint32_t i = 0; i < ipixels; ++i) {
            UncompressedImage[i].pColor[0] = static_cast<float>(pbuffer[4 * i]);
            UncompressedImage[i].pColor[1] = static_cast<float>(pbuffer[4 * i + 1]);
            UncompressedImage[i].pColor[2] = static_cast<float>(pbuffer[4 * i + 2]);
            UncompressedImage[i].pColor[3] = static_cast<float>(pbuffer[4 * i + 3]);
        }
        spUncompressedTex.~NiPointer<NiSourceTexture>();
    }
    return &UncompressedImage;
}

// Indexes the width table by mipmap level and ignores the face argument.
uint32_t NiPixelData::GetWidth(uint32_t uiMipmapLevel, uint32_t) const {
    return m_puiWidth[uiMipmapLevel];
}

// The face stride uses the offset table's terminal entry at m_uiMipmapLevels.
uint8_t* NiPixelData::GetPixels(uint32_t uiMipmapLevel, uint32_t uiFace) {
    return m_pucPixels + m_puiOffsetInBytes[m_uiMipmapLevels] * uiFace + m_puiOffsetInBytes[uiMipmapLevel];
}
