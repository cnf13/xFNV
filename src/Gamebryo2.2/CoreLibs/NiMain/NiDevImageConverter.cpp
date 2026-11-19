#include "Gamebryo2.2/CoreLibs/NiMain/NiDevImageConverter.h"

#include "BSCore/MemoryManager.h"
#include "BSShader/ParticleShaderProperty.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/Sky/SkyTextureManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBMPReader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDDSReader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiImageReader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTGAReader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFilename.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiPath.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/string.h"

#include <cstdint>

NiPixelFormat NiPixelFormat::RGBA8888{
    {},
    1,
    32,
    NiPixelFormat::FORMAT_RGBA,
    NiPixelFormat::TILE_NONE,
    0xFFFFFFFF,
    0,
    {{NiPixelFormat::COMP_ALPHA, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_RED, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_GREEN, NiPixelFormat::REP_FIXED, 8, true},
     {NiPixelFormat::COMP_BLUE, NiPixelFormat::REP_FIXED, 8, true}}
};

NiDevImageConverter::PixelBits::PixelBits(const NiPixelFormat& kFmt)
  : m_uiRM(kFmt.GetMask(NiPixelFormat::COMP_RED)), m_uiGM(kFmt.GetMask(NiPixelFormat::COMP_GREEN)),
    m_uiBM(kFmt.GetMask(NiPixelFormat::COMP_BLUE)), m_uiAM(kFmt.GetMask(NiPixelFormat::COMP_ALPHA)),
    m_ucRS(kFmt.GetShift(NiPixelFormat::COMP_RED)), m_ucGS(kFmt.GetShift(NiPixelFormat::COMP_GREEN)),
    m_ucBS(kFmt.GetShift(NiPixelFormat::COMP_BLUE)), m_ucAS(kFmt.GetShift(NiPixelFormat::COMP_ALPHA)),
    m_ucRQ(8 - kFmt.GetBits(NiPixelFormat::COMP_RED)), m_ucGQ(8 - kFmt.GetBits(NiPixelFormat::COMP_GREEN)),
    m_ucBQ(8 - kFmt.GetBits(NiPixelFormat::COMP_BLUE)), m_ucAQ(8 - kFmt.GetBits(NiPixelFormat::COMP_ALPHA)) {}

NiDevImageConverter::NiDevImageConverter() {
    m_kReaders.AddHead(new NiBMPReader());
    m_kReaders.AddHead(new NiTGAReader());
    m_kReaders.AddHead(new NiDDSReader());
}

// The readers are owned by the converter: each one is deleted as it leaves the list.
NiDevImageConverter::~NiDevImageConverter() {
    while (m_kReaders.GetSize()) {
        delete m_kReaders.RemoveHead();
    }
}

bool NiDevImageConverter::ReadImageFileInfo(
    const char* pcFilename,
    NiPixelFormat& kFmt,
    bool& bMipmap,
    uint32_t& uiWidth,
    uint32_t& uiHeight,
    uint32_t& uiNumFaces
) {
    NiFile* pkIstr_2;
    char pcDest[260];
    NiTListItem<NiImageReader*>* m_pkHead = m_kReaders.m_pkHead;
    strcpy_s(pcDest, 260, pcFilename);
    NiPath::Standardize(pcDest);
    NiFilename kFileName(pcDest);
    if (!m_pkHead) {
        return false;
    }
    while (true) {
        NiFile* pkIstr;
        bool v11;
        NiImageReader* pkReader = m_pkHead->m_element;
        m_pkHead = m_pkHead->m_pkNext;
        if (!pkReader->CanReadFile(kFileName.m_acExt)) {
            goto LABEL_7;
        }
        pkIstr = NiFile::GetFile(pcDest, NiFile::READ_ONLY, 0x8000u);
        pkIstr_2 = pkIstr;
        if (!pkIstr) {
            return false;
        }
        if (!pkIstr->operator bool()) {
            break;
        }
        v11 = !pkReader->ReadHeader(*pkIstr_2, uiWidth, uiHeight, kFmt, bMipmap, uiNumFaces);
        if (!v11) {
            delete pkIstr_2;
            return true;
        }
        delete pkIstr_2;
    LABEL_7:;
        if (!m_pkHead) {
            return false;
        }
    }
    if (pkIstr_2) {
        delete pkIstr_2;
    }
    return false;
}

NiPixelData* NiDevImageConverter::ReadImageFile(const char* pcFilename, NiPixelData* pkOptDest, NiFile* apFile) {
    char pcDest[260];
    NiTListItem<NiImageReader*>* m_pkHead = m_kReaders.m_pkHead;
    strcpy_s(pcDest, 260, pcFilename);
    NiPath::Standardize(pcDest);
    NiFilename kFileName(pcDest);
    while (m_pkHead) {
        NiImageReader* pkReader;
        pkReader = m_pkHead->m_element;
        m_pkHead = m_pkHead->m_pkNext;
        if (pkReader->CanReadFile(kFileName.m_acExt)) {
            NiFile* pkIstr;
            if (apFile) {
                pkIstr = apFile;
            }
            else {
                // 32768 is the source's default buffer size of GetFile.
                pkIstr = NiFile::GetFile(pcDest, NiFile::READ_ONLY, 32768u);
            }
            if (!pkIstr) {
                return nullptr;
            }
            if (!pkIstr->operator bool()) {
                delete pkIstr;
                return nullptr;
            }
            NiPixelData* pkDest = pkReader->ReadFile(*pkIstr, pkOptDest);
            if (pkDest) {
                delete pkIstr;
                return pkDest;
            }
            if (!apFile) {
                delete pkIstr;
            }
        }
    }
    if (apFile) {
        delete apFile;
    }
    return nullptr;
}

bool NiDevImageConverter::CanConvertPixelData(const NiPixelFormat& kSrcFmt, const NiPixelFormat& kDestFmt) const {
    bool v7;
    NiDevImageConverter::PixelBits kDestBits;
    uint32_t m_eFormat = kDestFmt.m_eFormat;
    int32_t m_ucBitsPerPixel = kDestFmt.m_ucBitsPerPixel;
    kDestBits.~PixelBits();
    ::new (&kDestBits) NiDevImageConverter::PixelBits(kDestFmt);
    if (kSrcFmt.operator==(kDestFmt)) {
        return true;
    }
    if ((kSrcFmt.m_uFlags & 1) == 0) {
        return false;
    }
    if ((kDestFmt.m_uFlags & 1) == 0) {
        return false;
    }
    NiPixelFormat::Format v5 = kDestFmt.m_eFormat;
    if (v5 >= NiPixelFormat::FORMAT_DXT1 && v5 <= NiPixelFormat::FORMAT_DXT5) {
        return false;
    }
    if (m_ucBitsPerPixel == 24 &&
        (kDestBits.m_uiGM != 0x00FF00 || (kDestBits.m_uiRM != 0x0000FF && kDestBits.m_uiRM != 0xFF0000))) {
        return false;
    }
    if (kSrcFmt.operator==(NiPixelFormat_PAL4) || kSrcFmt.operator==(NiPixelFormat_PALA4)) {
        if (m_eFormat == NiPixelFormat::FORMAT_RGB || m_eFormat == NiPixelFormat::FORMAT_RGBA) {
            return m_ucBitsPerPixel == 24 || m_ucBitsPerPixel == 32;
        }
        if (!kDestFmt.operator!=(NiPixelFormat_PAL4)) {
            return true;
        }
        v7 = kDestFmt.operator!=(NiPixelFormat_PALA4);
    }
    else {
        if (!kSrcFmt.operator==(NiPixelFormat_PAL8) && !kSrcFmt.operator==(NiPixelFormat_PALA8)) {
            if (!kSrcFmt.operator==(NiPixelFormat_RGB24) &&
                (!kSrcFmt.operator==(NiPixelFormat_RGBA32) && !static_cast<uint8_t>(kSrcFmt.GetCompressed()))) {
                if (!kSrcFmt.operator==(NiPixelFormat_BUMP16) && !kSrcFmt.operator==(NiPixelFormat_BUMPLUMA32)) {
                    if (!kSrcFmt.operator==(NiPixelFormat_RGBA16) && !kSrcFmt.operator==(NiPixelFormat_BGRA16)) {
                        return false;
                    }
                    return m_ucBitsPerPixel == 16;
                }
                goto LABEL_22;
            }
            if (m_eFormat != NiPixelFormat::FORMAT_RGB && m_eFormat != NiPixelFormat::FORMAT_RGBA) {
            LABEL_22:;
                if (m_eFormat != NiPixelFormat::FORMAT_BUMP && m_eFormat != NiPixelFormat::FORMAT_BUMPLUMA) {
                    return false;
                }
            }
        LABEL_24:;
            if (m_ucBitsPerPixel == 16 || m_ucBitsPerPixel == 24) {
                return true;
            }
            return m_ucBitsPerPixel == 32;
        }
        if (m_eFormat == NiPixelFormat::FORMAT_RGB || m_eFormat == NiPixelFormat::FORMAT_RGBA) {
            goto LABEL_24;
        }
        if (!kDestFmt.operator!=(NiPixelFormat_PAL8)) {
            return true;
        }
        v7 = kDestFmt.operator!=(NiPixelFormat_PALA8);
    }
    return !v7;
}

// Converts akSrc to akDestFmt: expands compressed data first, generates mipmap levels when asked, converts into
// apOptDest where it fits, else into a new pixel data (bump formats through GenerateBumpMap).
NiPixelData* NiDevImageConverter::ConvertPixelData(
    const NiPixelData& kSrc,
    const NiPixelFormat& kDestFmt,
    NiPixelData* pkOptDest,
    bool bMipmap
) {
    NiPointer<NiPixelData> spCompressionTmp;
    const NiPixelData* pkSrc = &kSrc;
    // Bit 0 of m_uFlags (NiPixelFormat::GetBit).
    if (!(kSrc.m_kPixelFormat.m_uFlags & 1)) {
        return nullptr;
    }
    if (!(kDestFmt.m_uFlags & 1)) {
        return nullptr;
    }
    NiPixelFormat::Format eSrcFormat = kSrc.m_kPixelFormat.m_eFormat;
    if (eSrcFormat >= NiPixelFormat::FORMAT_DXT1 && eSrcFormat <= NiPixelFormat::FORMAT_DXT5 &&
        kDestFmt.m_eFormat != kSrc.m_kPixelFormat.m_eFormat) {
        spCompressionTmp.SetNew(m_kCodec.ExpandCompressedData(kSrc));
        pkSrc = spCompressionTmp.m_pObject;
    }

    NiPointer<NiPixelData> spMipmapTmp;
    if (bMipmap && pkSrc->m_uiMipmapLevels <= 1) {
        // EXE-INLINED 0xA77920
        uint32_t uiWidth = *pkSrc->m_puiWidth;
        if (uiWidth && !(uiWidth & (uiWidth - 1)) && NiIsPowerOf2(*pkSrc->m_puiHeight)) {
            NiPixelFormat::Format eDestFormat = kDestFmt.m_eFormat;
            if (eDestFormat == NiPixelFormat::FORMAT_RGB || eDestFormat == NiPixelFormat::FORMAT_RGBA ||
                eDestFormat == NiPixelFormat::FORMAT_BUMP || eDestFormat == NiPixelFormat::FORMAT_BUMPLUMA) {
                spMipmapTmp.SetNew(NiDevImageConverter::GenerateMipmapLevels(pkSrc, pkOptDest));
                if (spMipmapTmp.m_pObject) {
                    pkSrc = spMipmapTmp.m_pObject;
                }
            }
        }
    }

    if (pkOptDest && pkOptDest->m_kPixelFormat == kDestFmt && (!bMipmap || pkOptDest->m_uiMipmapLevels > 1) &&
        (pkOptDest == pkSrc || ConvertPixelDataFormat(*pkOptDest, *pkSrc, -1))) {
        return pkOptDest;
    }

    NiPointer<NiPixelData> spBumpTmp;
    if (kDestFmt.m_eFormat == NiPixelFormat::FORMAT_BUMP || kDestFmt.m_eFormat == NiPixelFormat::FORMAT_BUMPLUMA) {
        spBumpTmp.SetNew(GenerateBumpMap(*pkSrc, kDestFmt));
        if (spBumpTmp.m_pObject) {
            pkSrc = spBumpTmp.m_pObject;
        }
    }
    NiPixelData* pkMemory = static_cast<NiPixelData*>(NiMemObject::operator new(sizeof(NiPixelData)));
    NiPixelData* pkDest;
    if (pkMemory) {
        pkDest = ::new (pkMemory)
            NiPixelData(*pkSrc->m_puiWidth, *pkSrc->m_puiHeight, kDestFmt, pkSrc->m_uiMipmapLevels, pkSrc->m_uiFaces);
    }
    else {
        pkDest = nullptr;
    }
    // A failed conversion returns null and leaves pkDest allocated, as the game does.
    bool bConverted = ConvertPixelDataFormat(*pkDest, *pkSrc, -1);
    return bConverted ? pkDest : nullptr;
}

bool NiDevImageConverter::CanReadImageFile(const char* pcFilename) {
    char pcDest[260];
    NiTListItem<NiImageReader*>* m_pkHead = m_kReaders.m_pkHead;
    strcpy_s(pcDest, 260, pcFilename);
    NiPath::Standardize(pcDest);
    NiFilename kFileName(pcDest);
    if (m_pkHead) {
        while (true) {
            NiImageReader* pkReader = m_pkHead->m_element;
            m_pkHead = m_pkHead->m_pkNext;
            if (pkReader->CanReadFile(kFileName.m_acExt)) {
                break;
            }
            if (!m_pkHead) {
                return false;
            }
        }
        NiFile* pkIstr = NiFile::GetFile(pcDest, NiFile::READ_ONLY, 0x8000u);
        if (pkIstr) {
            bool v6 = !pkIstr->operator bool();
            if (!v6) {
                delete pkIstr;
                return true;
            }
            delete pkIstr;
        }
    }
    return false;
}

bool NiDevImageConverter::IsASupportedMipmapGenerationFormat(const NiPixelFormat& kSrcFmt) {
    return IsAnAlphaSupportedMipmapGenerationFormat(kSrcFmt) || IsANonAlphaSupportedMipmapGenerationFormat(kSrcFmt);
}

bool NiDevImageConverter::IsAnAlphaSupportedMipmapGenerationFormat(const NiPixelFormat& kSrcFmt) {
    if (kSrcFmt == NiPixelFormat_PALA4) {
        return true;
    }
    if (kSrcFmt == NiPixelFormat_PALA8) {
        return true;
    }
    return kSrcFmt == NiPixelFormat_RGBA32;
}

bool NiDevImageConverter::IsANonAlphaSupportedMipmapGenerationFormat(const NiPixelFormat& kSrcFmt) {
    if (kSrcFmt == NiPixelFormat_PAL4) {
        return true;
    }
    if (kSrcFmt == NiPixelFormat_PAL8) {
        return true;
    }
    return kSrcFmt == NiPixelFormat_RGB24;
}

NiPixelData* NiDevImageConverter::GenerateMipmapLevels(const NiPixelData* pkSrc, NiPixelData* pkOptDest) {
    NiPixelFormat* v5;
    const NiPixelData* v3 = pkSrc;
    NiPixelFormat kDestFmt;
    if (IsANonAlphaSupportedMipmapGenerationFormat(pkSrc->m_kPixelFormat)) {
        v5 = &NiPixelFormat_RGB24;
    }
    else {
        if (!IsAnAlphaSupportedMipmapGenerationFormat(pkSrc->m_kPixelFormat)) {
            return nullptr;
        }
        v5 = &NiPixelFormat_RGBA32;
    }
    NiPixelData* pkDest = pkOptDest;
    kDestFmt = *v5;
    if (!pkOptDest ||
        (!pkOptDest->m_kPixelFormat.operator==(kDestFmt) ||
         (pkOptDest->m_uiMipmapLevels <= 1 || pkOptDest->m_uiFaces != pkSrc->m_uiFaces))) {
        NiPixelData* v7 = static_cast<NiPixelData*>(NiMemObject::operator new(sizeof(NiPixelData)));
        if (v7) {
            pkDest = ::new (v7) NiPixelData(*pkSrc->m_puiWidth, *pkSrc->m_puiHeight, kDestFmt, 0, pkSrc->m_uiFaces);
        }
        else {
            pkDest = nullptr;
        }
    }
    uint32_t m_uiMipmapLevels = pkDest->m_uiMipmapLevels;
    uint32_t v52 = m_uiMipmapLevels;
    if (pkSrc != pkDest && !ConvertPixelDataFormat(*pkDest, *pkSrc, 0)) {
        return nullptr;
    }
    uint32_t apkOptDesta = 0;
    if (pkSrc->m_uiFaces) {
        while (true) {
            if (kDestFmt.operator==(NiPixelFormat_RGB24)) {
                uint32_t v10 = 1;
                int32_t v61 = 1;
                if (m_uiMipmapLevels > 1) {
                    do {
                        uint32_t* m_puiOffsetInBytes = pkDest->m_puiOffsetInBytes;
                        uint32_t v12 = apkOptDesta * m_puiOffsetInBytes[pkDest->m_uiMipmapLevels];
                        uint8_t* m_pucPixels = pkDest->m_pucPixels;
                        uint8_t* v14 = &m_pucPixels[m_puiOffsetInBytes[v10 - 1]];
                        uint8_t* v15 = &m_pucPixels[v12 + m_puiOffsetInBytes[v10]];
                        uint8_t* v16 = &v14[v12];
                        uint32_t* v17 = &pkDest->m_puiWidth[v10];
                        uint32_t v18 = *v17;
                        int32_t v19 = *(v17 - 1);
                        uint32_t v56 = v18;
                        uint32_t* m_puiHeight = pkDest->m_puiHeight;
                        uint32_t v21 = m_puiHeight[v10];
                        uint32_t* v22 = &m_puiHeight[v10];
                        uint32_t v53 = v21;
                        int32_t v23 = 3 * v19;
                        int32_t v64 = 3 * v19;
                        if (v19 == 1) {
                            for (uint32_t i = v53; i; --i) {
                                *v15 = (*v16 + static_cast<uint32_t>(v16[3])) >> 1;
                                v15[1] = (v16[1] + static_cast<uint32_t>(v16[4])) >> 1;
                                uint8_t* v25 = v15 + 2;
                                *v25 = (v16[2] + static_cast<uint32_t>(v16[5])) >> 1;
                                v15 = v25 + 1;
                                v16 += 6;
                            }
                        }
                        else {
                            if (*(v22 - 1) == 1) {
                                for (uint32_t j = v56; j; --j) {
                                    *v15 = (*v16 + static_cast<uint32_t>(v16[3])) >> 1;
                                    v15[1] = (v16[1] + static_cast<uint32_t>(v16[4])) >> 1;
                                    uint8_t* v27 = v15 + 2;
                                    *v27 = (v16[2] + static_cast<uint32_t>(v16[5])) >> 1;
                                    v15 = v27 + 1;
                                    v16 += 6;
                                }
                            }
                            else {
                                if (v53) {
                                    uint32_t v58 = v53;
                                    do {
                                        if (v56) {
                                            uint8_t* v28 = &v16[v23];
                                            int32_t v29 = -v23;
                                            uint32_t v54 = v56;
                                            do {
                                                *v15 =
                                                    (*v16 + (*v28 + (v28[3] + static_cast<uint32_t>(v28[v29 + 3])))) >>
                                                    2;
                                                v15[1] =
                                                    (v28[1] +
                                                     (v28[4] + (v28[v29 + 4] + static_cast<uint32_t>(v28[v29 + 1])))) >>
                                                    2;
                                                uint8_t* v30 = v15 + 2;
                                                *v30 =
                                                    (v28[2] +
                                                     (v28[5] + (v28[v29 + 5] + static_cast<uint32_t>(v28[v29 + 2])))) >>
                                                    2;
                                                v15 = v30 + 1;
                                                v16 += 6;
                                                v28 += 6;
                                                --v54;
                                            }
                                            while (v54);
                                            v23 = v64;
                                            v10 = v61;
                                        }
                                        v16 += v23;
                                        --v58;
                                    }
                                    while (v58);
                                }
                            }
                        }
                        v61 = ++v10;
                    }
                    while (v10 < v52);
                }
            }
            else {
                if (!kDestFmt.operator==(NiPixelFormat_RGBA32)) {
                    goto LABEL_56;
                }
                uint32_t v31 = 1;
                int32_t v55 = 1;
                if (m_uiMipmapLevels > 1) {
                    do {
                        uint32_t* v32 = pkDest->m_puiOffsetInBytes;
                        uint32_t v33 = apkOptDesta * v32[pkDest->m_uiMipmapLevels];
                        uint8_t* v34 = pkDest->m_pucPixels;
                        uint8_t* v35 = &v34[v32[v31 - 1]];
                        uint8_t* v36 = &v34[v33 + v32[v31]];
                        uint8_t* v37 = &v35[v33];
                        uint32_t* v38 = &pkDest->m_puiWidth[v31];
                        uint32_t v39 = *v38;
                        int32_t v40 = *(v38 - 1);
                        uint32_t v59 = v39;
                        uint32_t* v41 = pkDest->m_puiHeight;
                        uint32_t v42 = v41[v31];
                        uint32_t* v43 = &v41[v31];
                        uint32_t v62 = v42;
                        int32_t v44 = 4 * v40;
                        int32_t v65 = 4 * v40;
                        if (v40 == 1) {
                            for (uint32_t k = v62; k; --k) {
                                *v36 = (*v37 + static_cast<uint32_t>(v37[4])) >> 1;
                                v36[1] = (v37[1] + static_cast<uint32_t>(v37[5])) >> 1;
                                uint8_t* v46 = v36 + 1;
                                v46[1] = (v37[2] + static_cast<uint32_t>(v37[6])) >> 1;
                                v46 += 2;
                                *v46 = (v37[3] + static_cast<uint32_t>(v37[7])) >> 1;
                                v36 = v46 + 1;
                                v37 += 8;
                            }
                        }
                        else {
                            if (*(v43 - 1) == 1) {
                                for (uint32_t m = v59; m; --m) {
                                    *v36 = (*v37 + static_cast<uint32_t>(v37[4])) >> 1;
                                    v36[1] = (v37[1] + static_cast<uint32_t>(v37[5])) >> 1;
                                    uint8_t* v48 = v36 + 1;
                                    v48[1] = (v37[2] + static_cast<uint32_t>(v37[6])) >> 1;
                                    v48 += 2;
                                    *v48 = (v37[3] + static_cast<uint32_t>(v37[7])) >> 1;
                                    v36 = v48 + 1;
                                    v37 += 8;
                                }
                            }
                            else {
                                if (v62) {
                                    uint32_t v57 = v62;
                                    do {
                                        if (v59) {
                                            uint8_t* v49 = &v37[v44];
                                            int32_t v50 = -v44;
                                            uint32_t v63 = v59;
                                            do {
                                                *v36 =
                                                    (*v49 + (*v37 + (v49[4] + static_cast<uint32_t>(v49[v50 + 4])))) >>
                                                    2;
                                                v36[1] =
                                                    (v49[1] +
                                                     (v49[5] + (v49[v50 + 5] + static_cast<uint32_t>(v49[v50 + 1])))) >>
                                                    2;
                                                uint8_t* v51 = v36 + 1;
                                                v51[1] =
                                                    (v49[2] +
                                                     (v49[6] + (v49[v50 + 6] + static_cast<uint32_t>(v49[v50 + 2])))) >>
                                                    2;
                                                v51 += 2;
                                                *v51 =
                                                    (v49[3] +
                                                     (v49[7] + (v49[v50 + 7] + static_cast<uint32_t>(v49[v50 + 3])))) >>
                                                    2;
                                                v36 = v51 + 1;
                                                v37 += 8;
                                                v49 += 8;
                                                --v63;
                                            }
                                            while (v63);
                                            v44 = v65;
                                            v31 = v55;
                                        }
                                        v37 += v44;
                                        --v57;
                                    }
                                    while (v57);
                                }
                            }
                        }
                        v55 = ++v31;
                    }
                    while (v31 < v52);
                }
            }
            v3 = pkSrc;
        LABEL_56:;
            if (++apkOptDesta >= v3->m_uiFaces) {
                return pkDest;
            }
            m_uiMipmapLevels = v52;
        }
    }
    return pkDest;
}

// Reads one file per mipmap level into a new pixel data (the first level from apkSrc when given) and copies each
// level's pixels in; null when a file is missing, unreadable or of another level size.
NiPixelData* NiDevImageConverter::GenerateMipmapPyramidFromFiles(
    const char** apcFilenames,
    uint32_t uiNumLevels,
    const NiPixelData* pkSrc
) {
    bool bMipmap;
    uint32_t uiNumFaces;
    uint32_t uiHeight;
    uint32_t uiWidth;
    if (!*apcFilenames) {
        return nullptr;
    }
    NiPixelFormat kFmt;
    if (!ReadImageFileInfo(*apcFilenames, kFmt, bMipmap, uiWidth, uiHeight, uiNumFaces) || uiNumFaces != 1) {
        return nullptr;
    }
    NiPixelData* pkMemory = static_cast<NiPixelData*>(NiMemObject::operator new(sizeof(NiPixelData)));
    if (!pkMemory) {
        return nullptr;
    }
    NiPixelData* pkDest = ::new (pkMemory) NiPixelData(uiWidth, uiHeight, kFmt, uiNumLevels, 1u);
    if (!pkDest) {
        return nullptr;
    }
    for (uint32_t i = 0; i < uiNumLevels; ++i) {
        const char* pcFilename = apcFilenames[i];
        if (!pcFilename) {
            delete pkDest;
            return nullptr;
        }
        NiPointer<NiPixelData> spLevel;
        NiPixelData* pkRead;
        if (!i && pkSrc) {
            pkRead = const_cast<NiPixelData*>(pkSrc);
        }
        else {
            pkRead = ReadImageFile(pcFilename, nullptr, nullptr);
        }
        spLevel.SetNew(pkRead);
        NiPixelData* pkLevel = spLevel.m_pObject;
        if (!pkLevel) {
            delete pkDest;
            return nullptr;
        }
        uint32_t uiSize = pkDest->m_puiOffsetInBytes[i + 1] - pkDest->m_puiOffsetInBytes[i];
        if (uiSize != pkLevel->m_puiOffsetInBytes[1] - pkLevel->m_puiOffsetInBytes[0]) {
            delete pkDest;
            return nullptr;
        }
        rt::memcpy(
            &pkDest->m_pucPixels[pkDest->m_puiOffsetInBytes[i]], &pkLevel->m_pucPixels[pkLevel->m_puiOffsetInBytes[0]],
            uiSize
        );
        if ((!i) && (static_cast<uint8_t>(kFmt.GetPalettized()))) {
            pkDest->SetPalette(pkLevel->m_spPalette.m_pObject);
        }
    }
    return pkDest;
}

// Copies or converts mipmap levels iMipmapLevel (all of kDest's levels for -1) of every face from kSrc into kDest.
bool NiDevImageConverter::ConvertPixelDataFormat(NiPixelData& kDest, const NiPixelData& kSrc, int32_t iMipmapLevel) {
    if (*kSrc.m_puiWidth != *kDest.m_puiWidth || *kSrc.m_puiHeight != *kDest.m_puiHeight) {
        return false;
    }
    uint32_t uiFaces = kSrc.m_uiFaces;
    if (uiFaces != kDest.m_uiFaces) {
        return false;
    }

    uint32_t uiStart;
    uint32_t uiEnd;
    if (iMipmapLevel == -1) {
        uiEnd = kDest.m_uiMipmapLevels;
        if (kSrc.m_uiMipmapLevels < uiEnd) {
            return false;
        }
        uiStart = 0;
        uiEnd--;
    }
    else {
        uiEnd = static_cast<uint32_t>(iMipmapLevel);
        if (uiEnd >= kSrc.m_uiMipmapLevels || uiEnd >= kDest.m_uiMipmapLevels) {
            return false;
        }
        uiStart = uiEnd;
    }

    auto CopyLevels = [&] {
        for (uint32_t uiFace = 0; uiFace < uiFaces; uiFace++) {
            for (uint32_t i = uiStart; i <= uiEnd; i++) {
                const uint32_t* puiSrcOffsets = kSrc.m_puiOffsetInBytes;
                rt::memcpy(
                    kDest.m_pucPixels + kDest.m_puiOffsetInBytes[i] +
                        uiFace * kDest.m_puiOffsetInBytes[kDest.m_uiMipmapLevels],
                    kSrc.m_pucPixels + puiSrcOffsets[i] + uiFace * puiSrcOffsets[kSrc.m_uiMipmapLevels],
                    puiSrcOffsets[i + 1] - puiSrcOffsets[i]
                );
            }
        }
    };
    // The source palette copied (or assigned) to akDest's; the first entry's alpha is then made opaque.
    auto CopyPaletteOpaque = [&] {
        if (!kSrc.m_spPalette.m_pObject) {
            return false;
        }
        if (!kDest.m_spPalette.m_pObject) {
            void* pMemory = NiMemObject::operator new(sizeof(NiPalette));
            if (pMemory) {
                kDest.SetPalette(::new (pMemory) NiPalette(*kSrc.m_spPalette.m_pObject));
            }
            else {
                kDest.SetPalette(nullptr);
            }
        }
        else {
            *kDest.m_spPalette.m_pObject = *kSrc.m_spPalette.m_pObject;
        }
        NiPalette* pkPalette = kDest.m_spPalette.m_pObject;
        if (pkPalette->m_uiEntries > 0) {
            pkPalette->m_pkPalette[0].m_ucAlpha = 0xFF;
        }
        return true;
    };
    // A palette of auiEntries entries without alpha, built from the source palette's entries.
    auto NewPalette = [&](uint32_t auiEntries) {
        if (!kSrc.m_spPalette.m_pObject) {
            return false;
        }
        void* pMemory = NiMemObject::operator new(sizeof(NiPalette));
        if (pMemory) {
            kDest.SetPalette(::new (pMemory) NiPalette(false, auiEntries, kSrc.m_spPalette.m_pObject->m_pkPalette));
        }
        else {
            kDest.SetPalette(nullptr);
        }
        return true;
    };

    if (kSrc.m_kPixelFormat == kDest.m_kPixelFormat) {
        CopyLevels();
        NiPalette* pkSrcPalette = kSrc.m_spPalette.m_pObject;
        if (pkSrcPalette) {
            if (!kDest.m_spPalette.m_pObject) {
                void* pMemory = NiMemObject::operator new(sizeof(NiPalette));
                if (pMemory) {
                    kDest.SetPalette(::new (pMemory) NiPalette(*kSrc.m_spPalette.m_pObject));
                }
                else {
                    kDest.SetPalette(nullptr);
                }
                return true;
            }
            *kDest.m_spPalette.m_pObject = *pkSrcPalette;
        }
        return true;
    }

    NiPixelFormat::Format eSrcFormat = kSrc.m_kPixelFormat.m_eFormat;
    uint32_t uiSrcBitsPerPixel = kSrc.m_kPixelFormat.m_ucBitsPerPixel;
    NiPixelFormat::Format eDestFormat = kDest.m_kPixelFormat.m_eFormat;
    uint32_t uiDestBitsPerPixel = kDest.m_kPixelFormat.m_ucBitsPerPixel;
    PixelBits kDestBits;
    kDestBits.~PixelBits();
    ::new (&kDestBits) PixelBits(kDest.m_kPixelFormat);
    PixelBits kSrcBits;
    kSrcBits.~PixelBits();
    ::new (&kSrcBits) PixelBits(kSrc.m_kPixelFormat);

    if (eDestFormat == NiPixelFormat::FORMAT_DXT1 || eDestFormat == NiPixelFormat::FORMAT_DXT3 ||
        eDestFormat == NiPixelFormat::FORMAT_DXT5) {
        return false;
    }
    if (uiDestBitsPerPixel == 24 &&
        (kDestBits.m_uiGM != 0xFF00 || (kDestBits.m_uiRM != 0xFF && kDestBits.m_uiRM != 0xFF0000))) {
        return false;
    }

    bool bDestIsRGB = eDestFormat == NiPixelFormat::FORMAT_RGB || eDestFormat == NiPixelFormat::FORMAT_RGBA;
    ConvertFunction pfnConvert;
    if (kSrc.m_kPixelFormat == NiPixelFormat_PAL4) {
        if (!kSrc.m_spPalette.m_pObject) {
            return false;
        }
        if (!bDestIsRGB) {
            if (!(kDest.m_kPixelFormat == NiPixelFormat_PALA4)) {
                return false;
            }
            CopyLevels();
            return CopyPaletteOpaque();
        }
        if (uiDestBitsPerPixel == 24) {
            pfnConvert = &ConvertPAL4To24;
        }
        else if (uiDestBitsPerPixel == 32) {
            pfnConvert = &ConvertPAL4To32;
        }
        else {
            return false;
        }
    }
    else if (kSrc.m_kPixelFormat == NiPixelFormat_PALA4) {
        if (!kSrc.m_spPalette.m_pObject) {
            return false;
        }
        if (!bDestIsRGB) {
            if (!(kDest.m_kPixelFormat == NiPixelFormat_PAL4)) {
                return false;
            }
            CopyLevels();
            return NewPalette(0x10);
        }
        // ConvertPALA4To24 (same function as ConvertPAL4To24).
        if (uiDestBitsPerPixel == 24) {
            pfnConvert = &ConvertPAL4To24;
        }
        else if (uiDestBitsPerPixel == 32) {
            pfnConvert = &ConvertPALA4To32;
        }
        else {
            return false;
        }
    }
    else if (kSrc.m_kPixelFormat == NiPixelFormat_PAL8) {
        if (!kSrc.m_spPalette.m_pObject) {
            return false;
        }
        if (!bDestIsRGB) {
            if (!(kDest.m_kPixelFormat == NiPixelFormat_PALA8)) {
                return false;
            }
            CopyLevels();
            return CopyPaletteOpaque();
        }
        if (uiDestBitsPerPixel == 16) {
            pfnConvert = &ConvertPAL8To16;
        }
        else if (uiDestBitsPerPixel == 24) {
            pfnConvert = &ConvertPAL8To24;
        }
        else if (uiDestBitsPerPixel == 32) {
            pfnConvert = &ConvertPAL8To32;
        }
        else {
            return false;
        }
    }
    else if (kSrc.m_kPixelFormat == NiPixelFormat_PALA8) {
        if (!kSrc.m_spPalette.m_pObject) {
            return false;
        }
        if (!bDestIsRGB) {
            if (!(kDest.m_kPixelFormat == NiPixelFormat_PAL8)) {
                return false;
            }
            CopyLevels();
            return NewPalette(0x100);
        }
        // ConvertPALA8To24 (same function as ConvertPAL8To24).
        if (uiDestBitsPerPixel == 16) {
            pfnConvert = &ConvertPALA8To16;
        }
        else if (uiDestBitsPerPixel == 24) {
            pfnConvert = &ConvertPAL8To24;
        }
        else if (uiDestBitsPerPixel == 32) {
            pfnConvert = &ConvertPALA8To32;
        }
        else {
            return false;
        }
    }
    else if (kSrc.m_kPixelFormat == NiPixelFormat_RGB24) {
        if (!bDestIsRGB) {
            return false;
        }
        if (uiDestBitsPerPixel == 16) {
            pfnConvert = &ConvertRGB24To16;
        }
        else if (uiDestBitsPerPixel == 24) {
            pfnConvert = &ConvertRGB24To24;
        }
        else if (uiDestBitsPerPixel == 32) {
            pfnConvert = &ConvertRGB24To32;
        }
        else {
            return false;
        }
    }
    else if (kSrc.m_kPixelFormat == NiPixelFormat_RGBA32) {
        if (!bDestIsRGB) {
            return false;
        }
        if (uiDestBitsPerPixel == 16) {
            pfnConvert = &ConvertRGBA32To16;
        }
        else if (uiDestBitsPerPixel == 24) {
            pfnConvert = &ConvertRGBA32To24;
        }
        else if (uiDestBitsPerPixel == 32) {
            pfnConvert = &ConvertRGBA32To32;
        }
        else {
            return false;
        }
    }
    else if (kSrc.m_kPixelFormat == NiPixelFormat_BUMP16) {
        if (eDestFormat != NiPixelFormat::FORMAT_BUMP && eDestFormat != NiPixelFormat::FORMAT_BUMPLUMA) {
            return false;
        }
        if (uiDestBitsPerPixel == 16) {
            pfnConvert = &ConvertBUMP16To16;
        }
        else if (uiDestBitsPerPixel == 32) {
            pfnConvert = &ConvertBUMP16To32;
        }
        else {
            return false;
        }
    }
    else if (kSrc.m_kPixelFormat == NiPixelFormat_BUMPLUMA32) {
        if (eDestFormat != NiPixelFormat::FORMAT_BUMP && eDestFormat != NiPixelFormat::FORMAT_BUMPLUMA) {
            return false;
        }
        if (uiDestBitsPerPixel == 16) {
            pfnConvert = &ConvertBUMPLUMA32To16;
        }
        else if (uiDestBitsPerPixel == 32) {
            pfnConvert = &ConvertBUMPLUMA32To32;
        }
        else {
            return false;
        }
    }
    else if (kSrc.m_kPixelFormat == NiPixelFormat_RGBA16 && kDest.m_kPixelFormat == NiPixelFormat_BGRA16) {
        pfnConvert = &ConvertRGBA16ToBGRA16;
    }
    else if (kSrc.m_kPixelFormat == NiPixelFormat_BGRA16 && kDest.m_kPixelFormat == NiPixelFormat_RGBA16) {
        pfnConvert = &ConvertBGRA16ToRGBA16;
    }
    else {
        if (eSrcFormat != NiPixelFormat::FORMAT_RGB && eSrcFormat != NiPixelFormat::FORMAT_RGBA) {
            return false;
        }
        if (kDest.m_kPixelFormat == NiPixelFormat_RGBA32) {
            if (uiSrcBitsPerPixel == 16) {
                pfnConvert = kSrcBits.m_uiAM ? &Convert16AlphaToRGBA32 : &Convert16NoAlphaToRGBA32;
            }
            else if (uiSrcBitsPerPixel == 24) {
                pfnConvert = &Convert24ToRGBA32;
            }
            else if (uiSrcBitsPerPixel == 32) {
                pfnConvert = kSrcBits.m_uiAM ? &Convert32AlphaToRGBA32 : &Convert32NoAlphaToRGBA32;
            }
            else {
                return false;
            }
        }
        else if (kDest.m_kPixelFormat == NiPixelFormat_RGB24) {
            if (uiSrcBitsPerPixel == 16) {
                pfnConvert = &Convert16ToRGB24;
            }
            else if (uiSrcBitsPerPixel == 24) {
                pfnConvert = &Convert24ToRGB24;
            }
            else if (uiSrcBitsPerPixel == 32) {
                pfnConvert = &Convert32ToRGB24;
            }
            else {
                return false;
            }
        }
        else {
            return false;
        }
    }

    for (uint32_t uiFace = 0; uiFace < uiFaces; uiFace++) {
        for (uint32_t i = uiStart; i <= uiEnd; i++) {
            const uint32_t* puiDestOffsets = kDest.m_puiOffsetInBytes;
            pfnConvert(
                kDest.m_puiWidth[i], kDest.m_puiHeight[i], puiDestOffsets[i + 1] - puiDestOffsets[i],
                kDest.m_pucPixels + puiDestOffsets[i] + uiFace * puiDestOffsets[kDest.m_uiMipmapLevels],
                kSrc.m_spPalette.m_pObject, kDestBits,
                kSrc.m_pucPixels + kSrc.m_puiOffsetInBytes[i] + uiFace * kSrc.m_puiOffsetInBytes[kSrc.m_uiMipmapLevels],
                kSrcBits
            );
        }
    }
    return true;
}

NiPixelData* NiDevImageConverter::GenerateBumpMap(const NiPixelData& kSrc, const NiPixelFormat& kDestFmt) {
    NiPixelData* pkDest;
    uint32_t uiWidth = *kSrc.m_puiWidth;
    uint32_t uiHeight = *kSrc.m_puiHeight;
    uint32_t uiLevels = kSrc.m_uiMipmapLevels;
    uint32_t uiSrcPixelStride;
    uint32_t uiDestPixelStride;
    uint32_t l;
    uint32_t i;
    uint32_t j;
    if (kSrc.m_kPixelFormat.operator==(NiPixelFormat_RGBA32)) {
        NiPixelData* pkMem = static_cast<NiPixelData*>(NiMemObject::operator new(sizeof(NiPixelData)));
        pkDest = pkMem ? ::new (pkMem) NiPixelData(uiWidth, uiHeight, NiPixelFormat_BUMPLUMA32, uiLevels, 1u) : nullptr;
        for (l = 0; l < uiLevels; l++) {
            uint8_t* pucDest = &pkDest->m_pucPixels[pkDest->m_puiOffsetInBytes[l]];
            uiWidth = kSrc.m_puiWidth[l];
            uiHeight = kSrc.m_puiHeight[l];
            const uint8_t* pucSrc = &kSrc.m_pucPixels[kSrc.m_puiOffsetInBytes[l]];
            for (j = 0; j < uiHeight; j++) {
                for (i = 0; i < uiWidth; i++) {
                    pucDest[2] = pucSrc[3];
                    pucDest[3] = pucSrc[0];
                    pucSrc += 4;
                    pucDest += 4;
                }
            }
        }
        uiSrcPixelStride = 4;
        uiDestPixelStride = 4;
    }
    else if (kSrc.m_kPixelFormat.operator==(NiPixelFormat_RGB24) && kDestFmt.operator==(NiPixelFormat_BUMPLUMA32)) {
        // The NiOutputDebugString warning is deliberately omitted here, as the game does.
        NiPixelData* pkMem = static_cast<NiPixelData*>(NiMemObject::operator new(sizeof(NiPixelData)));
        pkDest = pkMem ? ::new (pkMem) NiPixelData(uiWidth, uiHeight, NiPixelFormat_BUMPLUMA32, uiLevels, 1u) : nullptr;
        for (l = 0; l < uiLevels; l++) {
            uint8_t* pucDest = &pkDest->m_pucPixels[pkDest->m_puiOffsetInBytes[l]];
            uiWidth = kSrc.m_puiWidth[l];
            uiHeight = kSrc.m_puiHeight[l];
            const uint8_t* pucSrc = &kSrc.m_pucPixels[kSrc.m_puiOffsetInBytes[l]];
            for (j = 0; j < uiHeight; j++) {
                for (i = 0; i < uiWidth; i++) {
                    pucDest[2] = 0xFF;
                    pucDest[3] = pucSrc[0];
                    pucSrc += 3;
                    pucDest += 4;
                }
            }
        }
        uiSrcPixelStride = 3;
        uiDestPixelStride = 4;
    }
    else if (kSrc.m_kPixelFormat.operator==(NiPixelFormat_RGB24)) {
        NiPixelData* pkMem = static_cast<NiPixelData*>(NiMemObject::operator new(sizeof(NiPixelData)));
        pkDest = pkMem ? ::new (pkMem) NiPixelData(uiWidth, uiHeight, NiPixelFormat_BUMP16, uiLevels, 1u) : nullptr;
        uiSrcPixelStride = 3;
        uiDestPixelStride = 2;
    }
    else {
        return nullptr;
    }
    for (l = 0; l < uiLevels; l++) {
        uint8_t* pucDest = &pkDest->m_pucPixels[pkDest->m_puiOffsetInBytes[l]];
        uiWidth = kSrc.m_puiWidth[l];
        uiHeight = kSrc.m_puiHeight[l];
        const uint8_t* pucSrc = &kSrc.m_pucPixels[kSrc.m_puiOffsetInBytes[l]];
        const uint8_t* pucSrcU = pucSrc + uiSrcPixelStride;
        const uint8_t* pucSrcV = pucSrc + uiWidth * uiSrcPixelStride;
        uiWidth--;
        uiHeight--;
        for (j = 0; j < uiHeight; j++) {
            for (i = 0; i < uiWidth; i++) {
                int16_t s00 = *pucSrc;
                int16_t s01 = *pucSrcU;
                int16_t s10 = *pucSrcV;
                int16_t sDU = s01 - s00;
                int16_t sDV = s10 - s00;
                pucDest[0] = static_cast<uint8_t>(sDU);
                pucDest[1] = static_cast<uint8_t>(sDV);
                pucSrc += uiSrcPixelStride;
                pucSrcU += uiSrcPixelStride;
                pucSrcV += uiSrcPixelStride;
                pucDest += uiDestPixelStride;
            }
            // The last column has no right neighbour: the U derivative looks back one pixel.
            if (i) {
                pucDest[0] = *pucSrc - *(pucSrc - uiSrcPixelStride);
            }
            else {
                pucDest[0] = 0;
            }
            pucDest[1] = *pucSrcV - *pucSrc;
            pucSrc += uiSrcPixelStride;
            pucSrcU += uiSrcPixelStride;
            pucSrcV += uiSrcPixelStride;
            pucDest += uiDestPixelStride;
        }
        // The last row has no lower neighbour: the V derivative looks up one row.
        pucSrcV = pucSrc - (uiWidth + 1) * uiSrcPixelStride;
        for (i = 0; i < uiWidth; i++) {
            int16_t s00 = *pucSrc;
            int16_t s01 = *pucSrcU;
            int16_t s10 = *pucSrcV;
            int16_t sDU = s01 - s00;
            int16_t sDV = s00 - s10;
            pucDest[0] = static_cast<uint8_t>(sDU);
            pucDest[1] = static_cast<uint8_t>(sDV);
            pucSrc += uiSrcPixelStride;
            pucSrcU += uiSrcPixelStride;
            pucSrcV += uiSrcPixelStride;
            pucDest += uiDestPixelStride;
        }
        if (i) {
            pucDest[0] = *pucSrc - *(pucSrc - uiSrcPixelStride);
        }
        else {
            pucDest[0] = 0;
        }
        if (j) {
            pucDest[1] = *pucSrc - *pucSrcV;
        }
        else {
            pucDest[1] = 0;
        }
    }
    return pkDest;
}

// Adds apkReader at the head of the reader list.
void NiDevImageConverter::AddReader(NiImageReader* pkReader) {
    // EXE-INLINED none: NiTPointerListBase<NiImageReader *>::AddHead
    NiTListItem<NiImageReader*>* pkNode = m_kReaders.NewItem();
    pkNode->m_element = pkReader;
    pkNode->m_pkPrev = nullptr;
    pkNode->m_pkNext = m_kReaders.m_pkHead;
    if (m_kReaders.m_pkHead) {
        m_kReaders.m_pkHead->m_pkPrev = pkNode;
        ++m_kReaders.m_uiCount;
    }
    else {
        ++m_kReaders.m_uiCount;
        m_kReaders.m_pkTail = pkNode;
    }
    m_kReaders.m_pkHead = pkNode;
}

const NiPixelFormat* NiDevImageConverter::FindClosestPixelFormat(
    Platform ePlatform,
    const NiTexture::FormatPrefs& kFmtPrefs,
    const NiPixelFormat& kSrcFmt,
    const NiPixelFormat& kDesiredFmt
) {
    const NiPixelFormat* result;
    int32_t m_eFormat;
    switch (ePlatform) {
        case ANY: {
            result = &kDesiredFmt;
            m_eFormat = (int32_t)(&kDesiredFmt)->m_eFormat;
            if (((m_eFormat == 2) || ((m_eFormat == 3) || ((&kDesiredFmt)->m_ucBitsPerPixel == 16))) &&
                (((int32_t)(&kFmtPrefs)->m_ePixelLayout) == 4)) {
                result = &NiPixelFormat_RGBA32;
            }
            break;
        }
        case XENON: {
            result = FindClosestPixelFormatXenon(kFmtPrefs, kSrcFmt, kDesiredFmt);
            break;
        }
        case PLAYSTATION3: {
            result = FindClosestPixelFormatPS3(kFmtPrefs, kSrcFmt, kDesiredFmt);
            break;
        }
        case DX9: {
            result = FindClosestPixelFormatDX9(kFmtPrefs, kSrcFmt, kDesiredFmt);
            break;
        }
        default: {
            result = nullptr;
            break;
        }
    }
    return result;
}

NiPixelData* NiDevImageConverter::ConvertForPlatform(
    Platform ePlatform,
    const NiTexture::FormatPrefs& kPrefs,
    const NiPixelData* pkSrc,
    const NiPixelFormat& kDesiredFormat
) {
    NiPixelData* v11;
    const NiPixelFormat* p_m_kPixelFormat = &pkSrc->m_kPixelFormat;
    const NiPixelFormat* v8 = FindClosestPixelFormat(ePlatform, kPrefs, pkSrc->m_kPixelFormat, kDesiredFormat);
    NiPixelFormat* v9 = const_cast<NiPixelFormat*>(v8);
    // The source is taken as const but a non-const NiPixelData* is returned: the source itself when no conversion is
    // needed.
    if (static_cast<const void*>(v8) == static_cast<const void*>(&pkSrc->m_kPixelFormat)) {
        return const_cast<NiPixelData*>(pkSrc);
    }
    if (!v8) {
        return nullptr;
    }
    if (v8->operator==(*p_m_kPixelFormat)) {
        return const_cast<NiPixelData*>(pkSrc);
    }
    if (!CanConvertPixelData(*p_m_kPixelFormat, *v9)) {
        return nullptr;
    }
    void* pkSrca = NiMemObject::operator new(sizeof(NiPixelData));
    if (pkSrca) {
        v11 = ::new (pkSrca)
            NiPixelData(*pkSrc->m_puiWidth, *pkSrc->m_puiHeight, *v9, pkSrc->m_uiMipmapLevels, pkSrc->m_uiFaces);
    }
    else {
        v11 = nullptr;
    }
    return ConvertPixelDataFormat(*v11, *pkSrc, (-1)) ? v11 : nullptr;
}

NiTPointerList<NiImageReader*>& NiDevImageConverter::GetImageReaderList() {
    return m_kReaders;
}

const NiPixelFormat* NiDevImageConverter::FindClosestPixelFormatDX9(
    const NiTexture::FormatPrefs& kFmtPrefs,
    const NiPixelFormat& kSrcFmt,
    const NiPixelFormat& kDesiredFmt
) {
    NiTexture::FormatPrefs::PixelLayout m_ePixelLayout = kFmtPrefs.m_ePixelLayout;
    if (kFmtPrefs.m_ePixelLayout == NiTexture::FormatPrefs::SINGLE_COLOR_8) {
        if (kFmtPrefs.m_eAlphaFmt) {
            return &NiPixelFormat_A8;
        }

        return &NiPixelFormat::L8;
    }
    switch (m_ePixelLayout) {
        case NiTexture::FormatPrefs::SINGLE_COLOR_16: {
            return &NiPixelFormat_R16;
        }
        case NiTexture::FormatPrefs::SINGLE_COLOR_32: {
            return &NiPixelFormat_R32;
        }
        case NiTexture::FormatPrefs::DOUBLE_COLOR_32: {
            return &NiPixelFormat_RG32;
        }
        case NiTexture::FormatPrefs::DOUBLE_COLOR_64: {
            return &NiPixelFormat_RG64;
        }
        case NiTexture::FormatPrefs::FLOAT_COLOR_128: {
            return &NiPixelFormat_RGBA128;
        }
        case NiTexture::FormatPrefs::FLOAT_COLOR_64:
        case NiTexture::FormatPrefs::FLOAT_COLOR_32: {
            return &NiPixelFormat_RGBA64;
        }
        case NiTexture::FormatPrefs::TRUE_COLOR_32: {
            if (kFmtPrefs.m_eAlphaFmt) {
                return &NiPixelFormat_BGRA8888;
            }

            return &NiPixelFormat_BGRX8888;
        }
        case NiTexture::FormatPrefs::HIGH_COLOR_16: {
            NiTexture::FormatPrefs::AlphaFormat m_eAlphaFmt = kFmtPrefs.m_eAlphaFmt;
            if (m_eAlphaFmt == NiTexture::FormatPrefs::BINARY) {
                return &NiPixelFormat_BGRA5551;
            }
            if (m_eAlphaFmt == NiTexture::FormatPrefs::SMOOTH) {
                return &NiPixelFormat_BGRA4444;
            }
            return &NiPixelFormat_BGR565;
        }
        case NiTexture::FormatPrefs::PALETTIZED_8:
        case NiTexture::FormatPrefs::PALETTIZED_4: {
            goto LABEL_36;
        }
        case NiTexture::FormatPrefs::COMPRESSED: {
            NiPixelFormat::Format m_eFormat;
            m_eFormat = kDesiredFmt.m_eFormat;
            switch (m_eFormat) {
                case NiPixelFormat::FORMAT_DXT1: {
                    return &NiPixelFormat_DXT1;
                }
                case NiPixelFormat::FORMAT_DXT3: {
                    return &NiPixelFormat_DXT3;
                }
                case NiPixelFormat::FORMAT_DXT5: {
                    return &NiPixelFormat_DXT5;
                }
            }
        LABEL_36:;
            if (kFmtPrefs.m_eAlphaFmt != NiTexture::FormatPrefs::SMOOTH) {
                return &NiPixelFormat_DXT1;
            }
            return &NiPixelFormat_DXT5;
        }
        case NiTexture::FormatPrefs::BUMPMAP: {
            return &NiPixelFormat_BUMPLUMA32;
        }
    }
    if (m_ePixelLayout != NiTexture::FormatPrefs::PIX_DEFAULT) {
        return &kDesiredFmt;
    }
    if (!static_cast<uint8_t>(kDesiredFmt.GetCompressed()) && !static_cast<uint8_t>(kDesiredFmt.GetPalettized())) {
        uint8_t m_ucBitsPerPixel = kDesiredFmt.m_ucBitsPerPixel;
        if (m_ucBitsPerPixel == 16) {
            bool v9;
            if (static_cast<uint8_t>(kDesiredFmt.GetPalettized())) {
                v9 = kDesiredFmt.m_eFormat == NiPixelFormat::FORMAT_PALALPHA;
            }
            else {
                v9 = kDesiredFmt.GetBits(NiPixelFormat::COMP_ALPHA) != 0;
            }
            if (static_cast<uint8_t>(kDesiredFmt.GetCompressed())) {
                v9 = true;
            }
            NiTexture::FormatPrefs::AlphaFormat v10 = kFmtPrefs.m_eAlphaFmt;
            if (v10 == NiTexture::FormatPrefs::BINARY) {
                return &NiPixelFormat_BGRA5551;
            }
            if (v10 == NiTexture::FormatPrefs::SMOOTH || (v10 && v9)) {
                return &NiPixelFormat_BGRA4444;
            }
            return &NiPixelFormat_BGR565;
        }
        if (m_ucBitsPerPixel == 32) {
            NiTexture::FormatPrefs::AlphaFormat v11 = kFmtPrefs.m_eAlphaFmt;
            if (v11 == NiTexture::FormatPrefs::NONE) {
                return &NiPixelFormat_BGRX8888;
            }
            if (v11 == NiTexture::FormatPrefs::BINARY || v11 == NiTexture::FormatPrefs::SMOOTH) {
                return &NiPixelFormat_BGRA8888;
            }
            NiPixelFormat::Format v12 = kDesiredFmt.m_eFormat;
            if (v12 == NiPixelFormat::FORMAT_RGB) {
                return &NiPixelFormat_BGRX8888;
            }
            if (v12 == NiPixelFormat::FORMAT_RGBA) {
                return &NiPixelFormat_BGRA8888;
            }
        }
        return &kDesiredFmt;
    }
    NiPixelFormat::Format v13 = kDesiredFmt.m_eFormat;
    if (v13 == NiPixelFormat::FORMAT_DXT1) {
        return &NiPixelFormat_DXT1;
    }
    if (v13 == NiPixelFormat::FORMAT_DXT3) {
        return &NiPixelFormat_DXT3;
    }
    if (v13 == NiPixelFormat::FORMAT_DXT5 || kFmtPrefs.m_eAlphaFmt == NiTexture::FormatPrefs::SMOOTH) {
        return &NiPixelFormat_DXT5;
    }

    return &NiPixelFormat_DXT1;
}

const NiPixelFormat* NiDevImageConverter::FindClosestPixelFormatXenon(
    const NiTexture::FormatPrefs& kFmtPrefs,
    const NiPixelFormat& kSrcFmt,
    const NiPixelFormat& kDesiredFmt
) {
    bool v5;
    NiPixelFormat* result;
    NiPixelFormat::Format v11;
    bool v12;
    NiPixelFormat::Format m_eFormat = kSrcFmt.m_eFormat;
    if (m_eFormat == NiPixelFormat::FORMAT_PAL || m_eFormat == NiPixelFormat::FORMAT_PALALPHA) {
        v5 = m_eFormat == NiPixelFormat::FORMAT_PALALPHA;
    }
    else {
        uint8_t m_ucBitsPerComponent;
        uint32_t v6 = 0;
        const NiPixelFormat::NiComponentSpec* m_akComponents = kSrcFmt.m_akComponents;
        while (m_akComponents->m_eComponent != NiPixelFormat::COMP_ALPHA) {
            ++v6;
            ++m_akComponents;
            if (v6 >= 4) {
                m_ucBitsPerComponent = 0;
                goto LABEL_8;
            }
        }
        m_ucBitsPerComponent = kSrcFmt.m_akComponents[v6].m_ucBitsPerComponent;
    LABEL_8:;
        v5 = m_ucBitsPerComponent != 0;
    }
    if (m_eFormat >= NiPixelFormat::FORMAT_DXT1 && m_eFormat <= NiPixelFormat::FORMAT_DXT5) {
        v5 = true;
    }
    NiTexture::FormatPrefs::PixelLayout m_ePixelLayout = kFmtPrefs.m_ePixelLayout;
    if (kFmtPrefs.m_ePixelLayout == NiTexture::FormatPrefs::FLOAT_COLOR_128) {
        return &NiPixelFormat_RGBA128;
    }
    switch (m_ePixelLayout) {
        case NiTexture::FormatPrefs::FLOAT_COLOR_64: {
            return &NiPixelFormat_RGBA64;
        }
        case NiTexture::FormatPrefs::TRUE_COLOR_32: {
            return &NiPixelFormat_BGRA8888;
        }
        case NiTexture::FormatPrefs::HIGH_COLOR_16: {
            goto LABEL_19;
        }
        case NiTexture::FormatPrefs::PALETTIZED_8:
        case NiTexture::FormatPrefs::PALETTIZED_4: {
            result = &NiPixelFormat_DXT5;
            goto LABEL_41;
        }
        case NiTexture::FormatPrefs::COMPRESSED: {
            v11 = kDesiredFmt.m_eFormat;
            goto LABEL_25;
        }
        case NiTexture::FormatPrefs::BUMPMAP: {
            return &NiPixelFormat_BUMPLUMA32;
        }
    }
    if (m_ePixelLayout != NiTexture::FormatPrefs::PIX_DEFAULT) {
        return &NiPixelFormat_BGRA8888;
    }
    if (!static_cast<uint8_t>(kDesiredFmt.GetPalettized()) && !static_cast<uint8_t>(kDesiredFmt.GetCompressed())) {
        uint8_t m_ucBitsPerPixel;
        m_ucBitsPerPixel = kDesiredFmt.m_ucBitsPerPixel;
        switch (m_ucBitsPerPixel) {
            case 16: {
            LABEL_19:;
                result = &NiPixelFormat_BGRA4444;
                if (!v5) {
                    return &NiPixelFormat_BGR565;
                }
                return result;
            }
            case 64: {
                return &NiPixelFormat_RGBA64;
            }
            case 128: {
                return &NiPixelFormat_RGBA128;
            }
        }
        return &NiPixelFormat_BGRA8888;
    }
    v11 = kDesiredFmt.m_eFormat;
LABEL_25:;
    if (v11 == NiPixelFormat::FORMAT_DXT1) {
        return &NiPixelFormat_DXT1;
    }
    if (v11 == NiPixelFormat::FORMAT_DXT3) {
        return &NiPixelFormat_DXT3;
    }
    v12 = v11 == NiPixelFormat::FORMAT_DXT5;
    result = &NiPixelFormat_DXT5;
    if (!v12) {
    LABEL_41:;
        if (v5) {
            return result;
        }
        return &NiPixelFormat_DXT1;
    }
    return result;
}

const NiPixelFormat* NiDevImageConverter::FindClosestPixelFormatPS3(
    const NiTexture::FormatPrefs& kFmtPrefs,
    const NiPixelFormat& kSrcFmt,
    const NiPixelFormat& kDesiredFmt
) {
    bool v5;
    bool v12;
    bool v13;
    NiPixelFormat::Format m_eFormat = kSrcFmt.m_eFormat;
    if (m_eFormat == NiPixelFormat::FORMAT_PAL || m_eFormat == NiPixelFormat::FORMAT_PALALPHA) {
        v5 = m_eFormat == NiPixelFormat::FORMAT_PALALPHA;
    }
    else {
        uint8_t m_ucBitsPerComponent;
        uint32_t v6 = 0;
        const NiPixelFormat::NiComponentSpec* m_akComponents = kSrcFmt.m_akComponents;
        while (m_akComponents->m_eComponent != NiPixelFormat::COMP_ALPHA) {
            ++v6;
            ++m_akComponents;
            if (v6 >= 4) {
                m_ucBitsPerComponent = 0;
                goto LABEL_8;
            }
        }
        m_ucBitsPerComponent = kSrcFmt.m_akComponents[v6].m_ucBitsPerComponent;
    LABEL_8:;
        v5 = m_ucBitsPerComponent != 0;
    }
    if (m_eFormat >= NiPixelFormat::FORMAT_DXT1 && m_eFormat <= NiPixelFormat::FORMAT_DXT5) {
        v5 = true;
    }
    NiTexture::FormatPrefs::PixelLayout m_ePixelLayout = kFmtPrefs.m_ePixelLayout;
    if (kFmtPrefs.m_ePixelLayout == NiTexture::FormatPrefs::FLOAT_COLOR_128) {
        return &NiPixelFormat_RGBA128;
    }
    switch (m_ePixelLayout) {
        case NiTexture::FormatPrefs::FLOAT_COLOR_64:
        case NiTexture::FormatPrefs::FLOAT_COLOR_32: {
            return &NiPixelFormat_RGBA64;
        }
        case NiTexture::FormatPrefs::HIGH_COLOR_16: {
            NiPixelFormat* result = &NiPixelFormat_BGRA4444;
            if (!v5) {
                return &NiPixelFormat_BGR565;
            }
            return result;
        }
        case NiTexture::FormatPrefs::PALETTIZED_8:
        case NiTexture::FormatPrefs::PALETTIZED_4: {
        LABEL_30:;
            if (kFmtPrefs.m_eAlphaFmt != NiTexture::FormatPrefs::SMOOTH) {
                return &NiPixelFormat_DXT1;
            }
            return &NiPixelFormat_DXT5;
        }
        case NiTexture::FormatPrefs::COMPRESSED: {
            NiPixelFormat::Format v11 = kDesiredFmt.m_eFormat;
            if (v11 == NiPixelFormat::FORMAT_DXT1) {
                return &NiPixelFormat_DXT1;
            }
            if (v11 == NiPixelFormat::FORMAT_DXT3) {
                return &NiPixelFormat_DXT3;
            }
            v12 = v11 == NiPixelFormat::FORMAT_DXT5;
            goto LABEL_29;
        }
    }
    if (m_ePixelLayout != NiTexture::FormatPrefs::PIX_DEFAULT) {
        return &NiPixelFormat::RGBA8888;
    }
    if (static_cast<uint8_t>(kDesiredFmt.GetCompressed()) || static_cast<uint8_t>(kDesiredFmt.GetPalettized())) {
        NiPixelFormat::Format v15;
        v15 = kDesiredFmt.m_eFormat;
        if (v15 == NiPixelFormat::FORMAT_DXT1) {
            return &NiPixelFormat_DXT1;
        }
        if (v15 == NiPixelFormat::FORMAT_DXT3) {
            return &NiPixelFormat_DXT3;
        }
        v12 = v15 == NiPixelFormat::FORMAT_DXT5;
    LABEL_29:;
        if (v12) {
            return &NiPixelFormat_DXT5;
        }
        goto LABEL_30;
    }
    if (kDesiredFmt.m_ucBitsPerPixel != 16) {
        return &NiPixelFormat::RGBA8888;
    }
    if (static_cast<uint8_t>(kDesiredFmt.GetPalettized())) {
        v13 = kDesiredFmt.m_eFormat == NiPixelFormat::FORMAT_PALALPHA;
    }
    else {
        v13 = kDesiredFmt.GetBits(NiPixelFormat::COMP_ALPHA) != 0;
    }
    if (static_cast<uint8_t>(kDesiredFmt.GetCompressed())) {
        v13 = true;
    }
    NiTexture::FormatPrefs::AlphaFormat m_eAlphaFmt = kFmtPrefs.m_eAlphaFmt;
    if (m_eAlphaFmt == NiTexture::FormatPrefs::BINARY ||
        (m_eAlphaFmt == NiTexture::FormatPrefs::SMOOTH || (m_eAlphaFmt && v13))) {
        return &NiPixelFormat_BGRA4444;
    }

    return &NiPixelFormat_BGR565;
}

void ConvertPAL4To24(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
) {
    NiPalette::PaletteEntry* m_pkPalette = a5->m_pkPalette;
    if (a6.m_uiGM == 0xFF00) {
        uint32_t v9 = a2;
        uint32_t v10 = a1;
        uint8_t* v11 = static_cast<uint8_t*>(_NiMalloc(a2 * a1));
        uint32_t v12 = a2 * (a1 >> 1);
        uint32_t v13 = 0;
        uint8_t* pvMem = v11;
        if (v12) {
            do {
                *v11 = a7[v13] >> 4;
                uint8_t* v14 = v11 + 1;
                *v14 = a7[v13++] & 15;
                v11 = v14 + 1;
            }
            while (v13 < v12);
            v10 = a1;
        }
        uint8_t* v15 = pvMem;
        if (a6.m_uiRM == 255) {
            if (a2) {
                uint8_t* v16 = a4;
                do {
                    if (v10) {
                        uint32_t v17 = v10;
                        do {
                            *v16 = m_pkPalette[*v15].m_ucRed;
                            uint8_t* v18 = v16 + 1;
                            *v18++ = m_pkPalette[*v15].m_ucGreen;
                            *v18 = m_pkPalette[*v15].m_ucBlue;
                            v16 = v18 + 1;
                            ++v15;
                            --v17;
                        }
                        while (v17);
                    }
                    --v9;
                }
                while (v9);
            }
        }
        else {
            if ((a6.m_uiRM == 0x00FF0000) && (a2)) {
                uint8_t* v19 = a4;
                do {
                    if (v10) {
                        uint32_t v20 = v10;
                        do {
                            *v19 = m_pkPalette[*v15].m_ucBlue;
                            uint8_t* v21 = v19 + 1;
                            *v21++ = m_pkPalette[*v15].m_ucGreen;
                            *v21 = m_pkPalette[*v15].m_ucRed;
                            v19 = v21 + 1;
                            ++v15;
                            --v20;
                        }
                        while (v20);
                    }
                    --v9;
                }
                while (v9);
            }
        }
        _NiFree(pvMem);
    }
}

void ConvertPAL4To32(
    uint32_t uiWidth,
    uint32_t uiHeight,
    uint32_t uiDestSize,
    uint8_t* pucDest,
    NiPalette* pkSrcPal,
    const NiDevImageConverter::PixelBits& kDestBits,
    const uint8_t* pucSrc,
    const NiDevImageConverter::PixelBits& kSrcBits
) {
    const NiPalette::PaletteEntry* pkPal = pkSrcPal->m_pkPalette;
    // Expand the 4-bit indices to one byte each.
    uint8_t* pucExpanded = static_cast<uint8_t*>(_NiMalloc(uiHeight * uiWidth));
    uint8_t* pucStart = pucExpanded;
    uint32_t uiCount = uiHeight * (uiWidth >> 1);
    for (uint32_t i = 0; i < uiCount; i++) {
        *pucExpanded++ = pucSrc[i] >> 4;
        *pucExpanded++ = pucSrc[i] & 0x0F;
    }
    pucExpanded = pucStart;
    uint32_t aulLut[16];
    for (uint32_t i = 0; i < 16; i++) {
        aulLut[i] =
            ((static_cast<uint32_t>(pkPal[i].m_ucRed) >> kDestBits.m_ucRQ << kDestBits.m_ucRS) & kDestBits.m_uiRM) |
            ((static_cast<uint32_t>(pkPal[i].m_ucGreen) >> kDestBits.m_ucGQ << kDestBits.m_ucGS) & kDestBits.m_uiGM) |
            ((static_cast<uint32_t>(pkPal[i].m_ucBlue) >> kDestBits.m_ucBQ << kDestBits.m_ucBS) & kDestBits.m_uiBM) |
            ((255u >> kDestBits.m_ucAQ << kDestBits.m_ucAS) & kDestBits.m_uiAM);
    }
    // The destination is raw pixel memory the source writes as 32-bit pixels (`(unsigned int*)pucDest`).
    uint32_t* puiDW = reinterpret_cast<uint32_t*>(pucDest);
    for (uint32_t y = 0; y < uiHeight; y++) {
        for (uint32_t x = 0; x < uiWidth; x++) {
            *puiDW++ = aulLut[*pucExpanded++];
        }
    }
    _NiFree(pucStart);
}

void ConvertPALA4To32(
    uint32_t uiWidth,
    uint32_t uiHeight,
    uint32_t uiDestSize,
    uint8_t* pucDest,
    NiPalette* pkSrcPal,
    const NiDevImageConverter::PixelBits& kDestBits,
    const uint8_t* pucSrc,
    const NiDevImageConverter::PixelBits& kSrcBits
) {
    const NiPalette::PaletteEntry* pkPal = pkSrcPal->m_pkPalette;
    // Expand the 4-bit indices to one byte each.
    uint8_t* pucExpanded = static_cast<uint8_t*>(_NiMalloc(uiHeight * uiWidth));
    uint8_t* pucStart = pucExpanded;
    uint32_t uiCount = uiHeight * (uiWidth >> 1);
    for (uint32_t i = 0; i < uiCount; i++) {
        *pucExpanded++ = pucSrc[i] >> 4;
        *pucExpanded++ = pucSrc[i] & 0x0F;
    }
    pucExpanded = pucStart;
    uint32_t aulLut[16];
    for (uint32_t i = 0; i < 16; i++) {
        aulLut[i] =
            ((static_cast<uint32_t>(pkPal[i].m_ucRed) >> kDestBits.m_ucRQ << kDestBits.m_ucRS) & kDestBits.m_uiRM) |
            ((static_cast<uint32_t>(pkPal[i].m_ucGreen) >> kDestBits.m_ucGQ << kDestBits.m_ucGS) & kDestBits.m_uiGM) |
            ((static_cast<uint32_t>(pkPal[i].m_ucBlue) >> kDestBits.m_ucBQ << kDestBits.m_ucBS) & kDestBits.m_uiBM) |
            ((static_cast<uint32_t>(pkPal[i].m_ucAlpha) >> kDestBits.m_ucAQ << kDestBits.m_ucAS) & kDestBits.m_uiAM);
    }
    // The destination is raw pixel memory the source writes as 32-bit pixels (`(unsigned int*)pucDest`).
    uint32_t* puiDW = reinterpret_cast<uint32_t*>(pucDest);
    for (uint32_t y = 0; y < uiHeight; y++) {
        for (uint32_t x = 0; x < uiWidth; x++) {
            *puiDW++ = aulLut[*pucExpanded++];
        }
    }
    _NiFree(pucStart);
}

void ConvertPAL8To16(
    uint32_t uiWidth,
    uint32_t uiHeight,
    uint32_t uiDestSize,
    uint8_t* pucDest,
    NiPalette* pkSrcPal,
    const NiDevImageConverter::PixelBits& kDestBits,
    const uint8_t* pucSrc,
    const NiDevImageConverter::PixelBits& kSrcBits
) {
    const NiPalette::PaletteEntry* pkPal = pkSrcPal->m_pkPalette;
    uint16_t ausLut[256];
    for (uint32_t i = 0; i < 256; i++) {
        ausLut[i] = static_cast<uint16_t>(
            ((static_cast<uint32_t>(pkPal[i].m_ucRed) >> kDestBits.m_ucRQ << kDestBits.m_ucRS) & kDestBits.m_uiRM) |
            ((static_cast<uint32_t>(pkPal[i].m_ucGreen) >> kDestBits.m_ucGQ << kDestBits.m_ucGS) & kDestBits.m_uiGM) |
            ((static_cast<uint32_t>(pkPal[i].m_ucBlue) >> kDestBits.m_ucBQ << kDestBits.m_ucBS) & kDestBits.m_uiBM) |
            ((255u >> kDestBits.m_ucAQ << kDestBits.m_ucAS) & kDestBits.m_uiAM)
        );
    }
    // The destination is raw pixel memory the source writes as 16-bit pixels (`(unsigned short*)pucDest`).
    uint16_t* pusW = reinterpret_cast<uint16_t*>(pucDest);
    for (uint32_t y = 0; y < uiHeight; y++) {
        for (uint32_t x = 0; x < uiWidth; x++) {
            *pusW++ = ausLut[*pucSrc++];
        }
    }
}

void ConvertPAL8To32(
    uint32_t uiWidth,
    uint32_t uiHeight,
    uint32_t uiDestSize,
    uint8_t* pucDest,
    NiPalette* pkSrcPal,
    const NiDevImageConverter::PixelBits& kDestBits,
    const uint8_t* pucSrc,
    const NiDevImageConverter::PixelBits& kSrcBits
) {
    const NiPalette::PaletteEntry* pkPal = pkSrcPal->m_pkPalette;
    uint32_t aulLut[256];
    for (uint32_t i = 0; i < 256; i++) {
        aulLut[i] =
            ((static_cast<uint32_t>(pkPal[i].m_ucRed) >> kDestBits.m_ucRQ << kDestBits.m_ucRS) & kDestBits.m_uiRM) |
            ((static_cast<uint32_t>(pkPal[i].m_ucGreen) >> kDestBits.m_ucGQ << kDestBits.m_ucGS) & kDestBits.m_uiGM) |
            ((static_cast<uint32_t>(pkPal[i].m_ucBlue) >> kDestBits.m_ucBQ << kDestBits.m_ucBS) & kDestBits.m_uiBM) |
            ((255u >> kDestBits.m_ucAQ << kDestBits.m_ucAS) & kDestBits.m_uiAM);
    }
    // The destination is raw pixel memory the source writes as 32-bit pixels (`(unsigned int*)pucDest`).
    uint32_t* puiDW = reinterpret_cast<uint32_t*>(pucDest);
    for (uint32_t y = 0; y < uiHeight; y++) {
        for (uint32_t x = 0; x < uiWidth; x++) {
            *puiDW++ = aulLut[*pucSrc++];
        }
    }
}

void ConvertPALA8To16(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
) {
    uint16_t v26[256];
    int16_t m_uiAM = a6.m_uiAM;
    uint8_t m_ucBQ = a6.m_ucBQ;
    uint8_t m_ucAQ = a6.m_ucAQ;
    uint8_t m_ucBS = a6.m_ucBS;
    int16_t m_uiBM = a6.m_uiBM;
    uint8_t m_ucGQ = a6.m_ucGQ;
    uint8_t m_ucGS = a6.m_ucGS;
    int16_t m_uiGM = a6.m_uiGM;
    uint8_t m_ucAS = a6.m_ucAS;
    uint8_t m_ucRQ = a6.m_ucRQ;
    int16_t m_uiRM = a6.m_uiRM;
    uint32_t v10 = 0;
    uint8_t m_ucRS = a6.m_ucRS;
    uint8_t* p_m_ucBlue = &a5->m_pkPalette->m_ucBlue;
    do {
        v26[v10] = (m_uiBM & static_cast<uint8_t>(*p_m_ucBlue >> m_ucBQ) << m_ucBS) |
            ((m_uiAM & static_cast<uint8_t>(p_m_ucBlue[1] >> m_ucAQ) << m_ucAS) |
             ((m_uiGM & static_cast<uint8_t>(*(p_m_ucBlue - 1) >> m_ucGQ) << m_ucGS) |
              (m_uiRM & static_cast<uint8_t>(*(p_m_ucBlue - 2) >> m_ucRQ) << m_ucRS)));
        v26[v10 + 1] = (m_uiAM & static_cast<uint8_t>(p_m_ucBlue[5] >> m_ucAQ) << m_ucAS) |
            ((m_uiBM & static_cast<uint8_t>(p_m_ucBlue[4] >> m_ucBQ) << m_ucBS) |
             ((m_uiGM & static_cast<uint8_t>(p_m_ucBlue[3] >> m_ucGQ) << m_ucGS) |
              (m_uiRM & static_cast<uint8_t>(p_m_ucBlue[2] >> m_ucRQ) << m_ucRS)));
        v26[v10 + 2] = (m_uiAM & static_cast<uint8_t>(p_m_ucBlue[9] >> m_ucAQ) << m_ucAS) |
            ((m_uiBM & static_cast<uint8_t>(p_m_ucBlue[8] >> m_ucBQ) << m_ucBS) |
             ((m_uiGM & static_cast<uint8_t>(p_m_ucBlue[7] >> m_ucGQ) << m_ucGS) |
              (m_uiRM & static_cast<uint8_t>(p_m_ucBlue[6] >> m_ucRQ) << m_ucRS)));
        v26[v10 + 3] = (m_uiAM & static_cast<uint8_t>(p_m_ucBlue[13] >> m_ucAQ) << m_ucAS) |
            ((m_uiBM & static_cast<uint8_t>(p_m_ucBlue[12] >> m_ucBQ) << m_ucBS) |
             ((m_uiGM & static_cast<uint8_t>(p_m_ucBlue[11] >> m_ucGQ) << m_ucGS) |
              (m_uiRM & static_cast<uint8_t>(p_m_ucBlue[10] >> m_ucRQ) << m_ucRS)));
        v10 += 4;
        p_m_ucBlue += 16;
    }
    while (v10 < 256);
    // The destination is raw pixel memory the source writes as 16-bit pixels (`(unsigned short*)pucDest`).
    uint16_t* pusW = reinterpret_cast<uint16_t*>(a4);
    if (a2) {
        uint32_t v13 = a2;
        do {
            if (a1) {
                uint32_t v15 = a1;
                do {
                    *pusW = v26[*a7];
                    ++pusW;
                    ++a7;
                    --v15;
                }
                while (v15);
            }
            --v13;
        }
        while (v13);
    }
}

void ConvertPAL8To24(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
) {
    NiPalette::PaletteEntry* m_pkPalette = a5->m_pkPalette;
    if (a6.m_uiGM == 0xFF00) {
        if (a6.m_uiRM == 255) {
            if (a2) {
                const uint8_t* v9 = a7;
                uint32_t v10 = a2;
                uint8_t* v11 = a4;
                do {
                    if (a1) {
                        uint32_t v12 = a1;
                        do {
                            *v11 = m_pkPalette[*v9].m_ucRed;
                            uint8_t* v13 = v11 + 1;
                            *v13++ = m_pkPalette[*v9].m_ucGreen;
                            *v13 = m_pkPalette[*v9].m_ucBlue;
                            v11 = v13 + 1;
                            ++v9;
                            --v12;
                        }
                        while (v12);
                    }
                    --v10;
                }
                while (v10);
            }
        }
        else {
            if ((a6.m_uiRM == 0x00FF0000) && (a2)) {
                const uint8_t* v14 = a7;
                uint32_t v15 = a2;
                uint8_t* v16 = a4;
                do {
                    if (a1) {
                        uint32_t v17 = a1;
                        do {
                            *v16 = m_pkPalette[*v14].m_ucBlue;
                            uint8_t* v18 = v16 + 1;
                            *v18++ = m_pkPalette[*v14].m_ucGreen;
                            *v18 = m_pkPalette[*v14].m_ucRed;
                            v16 = v18 + 1;
                            ++v14;
                            --v17;
                        }
                        while (v17);
                    }
                    --v15;
                }
                while (v15);
            }
        }
    }
}

void ConvertPALA8To32(
    uint32_t uiWidth,
    uint32_t uiHeight,
    uint32_t uiDestSize,
    uint8_t* pucDest,
    NiPalette* pkSrcPal,
    const NiDevImageConverter::PixelBits& kDestBits,
    const uint8_t* pucSrc,
    const NiDevImageConverter::PixelBits& kSrcBits
) {
    const NiPalette::PaletteEntry* pkPal = pkSrcPal->m_pkPalette;
    uint32_t aulLut[256];
    for (uint32_t i = 0; i < 256; i++) {
        aulLut[i] =
            ((static_cast<uint32_t>(pkPal[i].m_ucRed) >> kDestBits.m_ucRQ << kDestBits.m_ucRS) & kDestBits.m_uiRM) |
            ((static_cast<uint32_t>(pkPal[i].m_ucGreen) >> kDestBits.m_ucGQ << kDestBits.m_ucGS) & kDestBits.m_uiGM) |
            ((static_cast<uint32_t>(pkPal[i].m_ucBlue) >> kDestBits.m_ucBQ << kDestBits.m_ucBS) & kDestBits.m_uiBM) |
            ((static_cast<uint32_t>(pkPal[i].m_ucAlpha) >> kDestBits.m_ucAQ << kDestBits.m_ucAS) & kDestBits.m_uiAM);
    }
    // The destination is raw pixel memory the source writes as 32-bit pixels (`(unsigned int*)pucDest`).
    uint32_t* puiDW = reinterpret_cast<uint32_t*>(pucDest);
    for (uint32_t y = 0; y < uiHeight; y++) {
        for (uint32_t x = 0; x < uiWidth; x++) {
            *puiDW++ = aulLut[*pucSrc++];
        }
    }
}

void ConvertRGB24To24(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
) {
    if (a6.m_uiGM == 0xFF00) {
        if (a6.m_uiRM == 0x00FF0000) {
            if (a2) {
                const uint8_t* v8 = a7;
                uint32_t v9 = a2;
                uint8_t* v10 = a4;
                do {
                    if (a1) {
                        uint32_t v11 = a1;
                        do {
                            *v10 = v8[2];
                            uint8_t* v12 = v10 + 1;
                            *v12++ = v8[1];
                            *v12 = *v8;
                            v10 = v12 + 1;
                            v8 += 3;
                            --v11;
                        }
                        while (v11);
                    }
                    --v9;
                }
                while (v9);
            }
        }
        else {
            if (a6.m_uiRM == 255) {
                rt::memcpy(a4, a7, 3 * (a2 * a1));
            }
        }
    }
}

void Convert24ToRGB24(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* pvDest,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* pvSrc,
    const NiDevImageConverter::PixelBits& a8
) {
    if (a8.m_uiGM == 0xFF00) {
        if (a8.m_uiRM == 0x00FF0000) {
            if (a2) {
                const uint8_t* v8 = pvSrc;
                uint32_t v9 = a2;
                uint8_t* v10 = pvDest;
                do {
                    if (a1) {
                        uint32_t v11 = a1;
                        do {
                            *v10 = v8[2];
                            uint8_t* v12 = v10 + 1;
                            *v12++ = v8[1];
                            *v12 = *v8;
                            v10 = v12 + 1;
                            v8 += 3;
                            --v11;
                        }
                        while (v11);
                    }
                    --v9;
                }
                while (v9);
            }
        }
        else {
            if (a8.m_uiRM == 255) {
                rt::memcpy(pvDest, pvSrc, 3 * (a2 * a1));
            }
        }
    }
}

void ConvertRGBA16ToBGRA16(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
) {
    // The source and destination are untyped raw pixel memory, read and written a word per pixel through 16-bit pointer
    // casts.
    const uint16_t* pusR = reinterpret_cast<const uint16_t*>(a7);
    uint16_t* pusW = reinterpret_cast<uint16_t*>(a4);
    if (a2) {
        uint32_t v9 = a1;
        do {
            if (v9) {
                uint32_t uiCol = v9;
                do {
                    int32_t v10 = *pusR++;
                    *pusW++ =
                        (static_cast<uint16_t>(a6.m_uiGM) &
                         static_cast<uint8_t>(
                             static_cast<uint8_t>(static_cast<uint8_t>((v10 & a8.m_uiGM) >> a8.m_ucGS) << a8.m_ucGQ) >>
                             a6.m_ucGQ
                         ) << a6.m_ucGS) |
                        ((static_cast<uint16_t>(a6.m_uiRM) &
                          static_cast<uint8_t>(
                              static_cast<uint8_t>(static_cast<uint8_t>((a8.m_uiRM & v10) >> a8.m_ucRS) << a8.m_ucRQ) >>
                              a6.m_ucRQ
                          ) << a6.m_ucRS) |
                         ((static_cast<uint16_t>(a6.m_uiBM) &
                           static_cast<uint8_t>(
                               static_cast<uint8_t>(
                                   static_cast<uint8_t>((a8.m_uiBM & static_cast<uint16_t>(v10)) >> a8.m_ucBS)
                                   << a8.m_ucBQ
                               ) >>
                               a6.m_ucBQ
                           ) << a6.m_ucBS) |
                          (static_cast<uint16_t>(a6.m_uiAM) &
                           static_cast<uint8_t>(
                               static_cast<uint8_t>(
                                   static_cast<uint8_t>((v10 & a8.m_uiAM) >> a8.m_ucAS) << a8.m_ucAQ
                               ) >>
                               a6.m_ucAQ
                           ) << a6.m_ucAS)));
                    --uiCol;
                }
                while (uiCol);
                v9 = a1;
            }
            --a2;
        }
        while (a2);
    }
}

void ConvertBGRA16ToRGBA16(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
) {
    ConvertRGBA16ToBGRA16(a1, a2, a3, a4, a5, a6, a7, a8);
}

void ConvertRGB24To16(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
) {
    // The destination is raw pixel memory, written one 16-bit word per pixel.
    uint16_t* pusW = reinterpret_cast<uint16_t*>(a4);
    if (a2) {
        uint32_t v9 = a1;
        uint32_t v14 = a2;
        do {
            if (v9) {
                uint32_t a2a = v9;
                do {
                    uint8_t v11 = *a7;
                    uint8_t v12 = a7[1];
                    const uint8_t* v13 = a7 + 1;
                    uint8_t a4a = v13[1];
                    a7 = v13 + 2;
                    *pusW++ = (static_cast<uint16_t>(a6.m_uiGM) & static_cast<uint8_t>(v12 >> a6.m_ucGQ) << a6.m_ucGS) |
                        ((static_cast<uint16_t>(a6.m_uiBM) & static_cast<uint8_t>(a4a >> a6.m_ucBQ) << a6.m_ucBS) |
                         ((static_cast<uint16_t>(a6.m_uiRM) & static_cast<uint8_t>(v11 >> a6.m_ucRQ) << a6.m_ucRS) |
                          (static_cast<uint16_t>(a6.m_uiAM) & 255u >> a6.m_ucAQ << a6.m_ucAS)));
                    --a2a;
                }
                while (a2a);
                v9 = a1;
            }
            --v14;
        }
        while (v14);
    }
}

void ConvertRGB24To32(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
) {
    // The destination is raw pixel memory, written as one 32-bit value per pixel.
    uint32_t* puiDW = reinterpret_cast<uint32_t*>(a4);
    if (a2) {
        uint32_t v9 = a1;
        uint32_t v15 = a2;
        do {
            if (v9) {
                uint32_t a2a = v9;
                do {
                    uint8_t v11 = *a7;
                    uint8_t v12 = a7[1];
                    const uint8_t* v13 = a7 + 2;
                    uint8_t a4a = v12;
                    uint8_t v14 = *v13;
                    a7 = v13 + 1;
                    *puiDW++ = (a6.m_uiAM & 255u >> a6.m_ucAQ << a6.m_ucAS) |
                        ((a6.m_uiBM & v14 >> a6.m_ucBQ << a6.m_ucBS) |
                         ((a6.m_uiGM & a4a >> a6.m_ucGQ << a6.m_ucGS) | (a6.m_uiRM & v11 >> a6.m_ucRQ << a6.m_ucRS)));
                    --a2a;
                }
                while (a2a);
                v9 = a1;
            }
            --v15;
        }
        while (v15);
    }
}

void ConvertRGBA32To16(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
) {
    // The destination is raw pixel memory: Gamebryo's source writes it a word per pixel through
    // `(unsigned short*)pucDest`.
    uint16_t* pusW = reinterpret_cast<uint16_t*>(a4);
    if (a2) {
        uint32_t v15 = a2;
        do {
            for (uint32_t i = a1; i; --i) {
                uint8_t v11 = a7[0];
                uint8_t v12 = a7[1];
                uint8_t v14 = a7[2];
                uint8_t x2 = a7[3];
                a7 += 4;
                *pusW++ = (static_cast<uint16_t>(a6.m_uiBM) & static_cast<uint8_t>(v14 >> a6.m_ucBQ) << a6.m_ucBS) |
                    ((static_cast<uint16_t>(a6.m_uiAM) & static_cast<uint8_t>(x2 >> a6.m_ucAQ) << a6.m_ucAS) |
                     ((static_cast<uint16_t>(a6.m_uiRM) & static_cast<uint8_t>(v11 >> a6.m_ucRQ) << a6.m_ucRS) |
                      (static_cast<uint16_t>(a6.m_uiGM) & static_cast<uint8_t>(v12 >> a6.m_ucGQ) << a6.m_ucGS)));
            }
            --v15;
        }
        while (v15);
    }
}

void ConvertRGBA32To24(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
) {
    if (a6.m_uiGM == 0xFF00) {
        if (a6.m_uiRM == 0x00FF0000) {
            if (a2) {
                const uint8_t* v8 = a7;
                uint32_t v9 = a2;
                uint8_t* v10 = a4;
                do {
                    if (a1) {
                        uint32_t v11 = a1;
                        do {
                            *v10 = v8[2];
                            uint8_t* v12 = v10 + 1;
                            *v12++ = v8[1];
                            *v12 = *v8;
                            v10 = v12 + 1;
                            v8 += 4;
                            --v11;
                        }
                        while (v11);
                    }
                    --v9;
                }
                while (v9);
            }
        }
        else {
            if ((a6.m_uiRM == 255) && (a2)) {
                const uint8_t* v13 = a7;
                uint32_t v14 = a2;
                uint8_t* v15 = a4;
                do {
                    if (a1) {
                        uint32_t v16 = a1;
                        do {
                            *v15 = *v13;
                            uint8_t v17 = v13[1];
                            const uint8_t* v18 = v13 + 1;
                            uint8_t* v19 = v15 + 1;
                            *v19++ = v17;
                            *v19 = v18[1];
                            v15 = v19 + 1;
                            v13 = v18 + 3;
                            --v16;
                        }
                        while (v16);
                    }
                    --v14;
                }
                while (v14);
            }
        }
    }
}

void ConvertRGBA32To32(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
) {
    if (a2) {
        uint32_t v9 = a1;
        uint32_t v17 = a2;
        do {
            if (v9) {
                uint32_t v16 = v9;
                do {
                    uint8_t v11 = *a7;
                    uint8_t v12 = a7[1];
                    const uint8_t* v13 = a7 + 2;
                    uint8_t a2a = v12;
                    uint8_t v14 = *v13++;
                    uint8_t a4a = v14;
                    uint8_t v15 = *v13;
                    a7 = v13 + 1;
                    a4 += 4;
                    // The destination is raw pixel memory, written a whole 32-bit pixel at a time.
                    *(reinterpret_cast<uint32_t*>(a4) - 1) = (a6.m_uiAM & v15 >> a6.m_ucAQ << a6.m_ucAS) |
                        ((a6.m_uiGM & a2a >> a6.m_ucGQ << a6.m_ucGS) |
                         ((a6.m_uiBM & a4a >> a6.m_ucBQ << a6.m_ucBS) | (a6.m_uiRM & v11 >> a6.m_ucRQ << a6.m_ucRS)));
                    --v16;
                }
                while (v16);
                v9 = a1;
            }
            --v17;
        }
        while (v17);
    }
}

void Convert16ToRGB24(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
) {
    // The source is raw pixel memory: Gamebryo's source reads it a word per pixel through
    // `(unsigned short*)pucSrc`.
    const uint16_t* pusSrc = reinterpret_cast<const uint16_t*>(a7);
    if (a2) {
        uint32_t a7a = a2;
        do {
            if (a1) {
                uint32_t v10 = a1;
                do {
                    a4 += 3;
                    *(a4 - 3) = static_cast<uint8_t>((a8.m_uiRM & *pusSrc) >> a8.m_ucRS) << a8.m_ucRQ;
                    uint8_t v11 = static_cast<uint8_t>((a8.m_uiGM & *pusSrc) >> a8.m_ucGS) << a8.m_ucGQ;
                    ++pusSrc;
                    *(a4 - 2) = v11;
                    --v10;
                    *(a4 - 1) = static_cast<uint8_t>((a8.m_uiBM & *(pusSrc - 1)) >> a8.m_ucBS) << a8.m_ucBQ;
                }
                while (v10);
            }
            --a7a;
        }
        while (a7a);
    }
}

void Convert16AlphaToRGBA32(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
) {
    // The source is raw pixel memory: Gamebryo's source reads it a word per pixel through
    // `(unsigned short*)pucSrc`.
    const uint16_t* pusSrc = reinterpret_cast<const uint16_t*>(a7);
    if (a2) {
        uint32_t a7a = a2;
        do {
            if (a1) {
                uint32_t v10 = a1;
                do {
                    uint8_t* v11 = a4 + 3;
                    *(v11 - 3) = static_cast<uint8_t>((a8.m_uiRM & *pusSrc) >> a8.m_ucRS) << a8.m_ucRQ;
                    uint8_t v12 = static_cast<uint8_t>((a8.m_uiGM & *pusSrc) >> a8.m_ucGS) << a8.m_ucGQ;
                    a4 = v11 + 1;
                    ++pusSrc;
                    *(a4 - 3) = v12;
                    *(a4 - 2) = static_cast<uint8_t>((a8.m_uiBM & *(pusSrc - 1)) >> a8.m_ucBS) << a8.m_ucBQ;
                    --v10;
                    *(a4 - 1) = static_cast<uint8_t>((a8.m_uiAM & *(pusSrc - 1)) >> a8.m_ucAS) << a8.m_ucAQ;
                }
                while (v10);
            }
            --a7a;
        }
        while (a7a);
    }
}

void Convert16NoAlphaToRGBA32(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
) {
    // The source is raw pixel memory: Gamebryo's source reads it a word per pixel through
    // `(unsigned short*)pucSrc`.
    const uint16_t* pusSrc = reinterpret_cast<const uint16_t*>(a7);
    if (a2) {
        uint32_t a7a = a2;
        do {
            if (a1) {
                uint32_t v10 = a1;
                do {
                    uint8_t* v11 = a4 + 3;
                    *(v11 - 3) = static_cast<uint8_t>((a8.m_uiRM & *pusSrc) >> a8.m_ucRS) << a8.m_ucRQ;
                    uint8_t v12 = static_cast<uint8_t>((a8.m_uiGM & *pusSrc) >> a8.m_ucGS) << a8.m_ucGQ;
                    a4 = v11 + 1;
                    ++pusSrc;
                    *(a4 - 3) = v12;
                    --v10;
                    *(a4 - 2) = static_cast<uint8_t>((a8.m_uiBM & *(pusSrc - 1)) >> a8.m_ucBS) << a8.m_ucBQ;
                    *(a4 - 1) = 0xFF;
                }
                while (v10);
            }
            --a7a;
        }
        while (a7a);
    }
}

void Convert24ToRGBA32(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
) {
    if (a8.m_uiGM == 0xFF00) {
        if (a8.m_uiRM == 0xFF0000) {
            if (a2) {
                const uint8_t* v8 = a7;
                uint32_t v9 = a2;
                uint8_t* v10 = a4;
                do {
                    if (a1) {
                        uint32_t v11 = a1;
                        do {
                            *v10 = v8[2];
                            uint8_t* v12 = v10 + 1;
                            *v12++ = v8[1];
                            *v12++ = *v8;
                            *v12 = -1;
                            v10 = v12 + 1;
                            v8 += 3;
                            --v11;
                        }
                        while (v11);
                    }
                    --v9;
                }
                while (v9);
            }
        }
        else {
            if (a2) {
                const uint8_t* v13 = a7;
                uint32_t v14 = a2;
                uint8_t* v15 = a4;
                do {
                    if (a1) {
                        uint32_t v16 = a1;
                        do {
                            *v15 = *v13;
                            uint8_t v17 = v13[1];
                            const uint8_t* v18 = v13 + 1;
                            uint8_t* v19 = v15 + 1;
                            *v19++ = v17;
                            *v19++ = v18[1];
                            *v19 = -1;
                            v13 = v18 + 2;
                            v15 = v19 + 1;
                            --v16;
                        }
                        while (v16);
                    }
                    --v14;
                }
                while (v14);
            }
        }
    }
}

void Convert32ToRGB24(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
) {
    // The source is raw pixel memory, read as one 32-bit value per pixel.
    const uint32_t* puiSrc = reinterpret_cast<const uint32_t*>(a7);
    if (a2) {
        uint32_t a7a = a2;
        do {
            if (a1) {
                uint32_t v10 = a1;
                do {
                    a4 += 3;
                    *(a4 - 3) = static_cast<uint8_t>((a8.m_uiRM & *puiSrc) >> a8.m_ucRS) << a8.m_ucRQ;
                    uint8_t v11 = static_cast<uint8_t>((*puiSrc & a8.m_uiGM) >> a8.m_ucGS) << a8.m_ucGQ;
                    ++puiSrc;
                    *(a4 - 2) = v11;
                    --v10;
                    *(a4 - 1) = static_cast<uint8_t>((*(puiSrc - 1) & a8.m_uiBM) >> a8.m_ucBS) << a8.m_ucBQ;
                }
                while (v10);
            }
            --a7a;
        }
        while (a7a);
    }
}

void Convert32AlphaToRGBA32(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
) {
    // The source is raw pixel memory, read as one 32-bit value per pixel.
    const uint32_t* puiSrc = reinterpret_cast<const uint32_t*>(a7);
    if (a2) {
        uint32_t a7a = a2;
        do {
            if (a1) {
                uint32_t v10 = a1;
                do {
                    uint8_t* v11 = a4 + 3;
                    *(v11 - 3) = static_cast<uint8_t>((a8.m_uiRM & *puiSrc) >> a8.m_ucRS) << a8.m_ucRQ;
                    uint8_t v12 = static_cast<uint8_t>((*puiSrc & a8.m_uiGM) >> a8.m_ucGS) << a8.m_ucGQ;
                    a4 = v11 + 1;
                    ++puiSrc;
                    *(a4 - 3) = v12;
                    *(a4 - 2) = static_cast<uint8_t>((*(puiSrc - 1) & a8.m_uiBM) >> a8.m_ucBS) << a8.m_ucBQ;
                    --v10;
                    *(a4 - 1) = static_cast<uint8_t>((*(puiSrc - 1) & a8.m_uiAM) >> a8.m_ucAS) << a8.m_ucAQ;
                }
                while (v10);
            }
            --a7a;
        }
        while (a7a);
    }
}

void Convert32NoAlphaToRGBA32(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
) {
    // The source is raw pixel memory, read as one 32-bit value per pixel.
    const uint32_t* puiSrc = reinterpret_cast<const uint32_t*>(a7);
    if (a2) {
        uint32_t a7a = a2;
        do {
            if (a1) {
                uint32_t v10 = a1;
                do {
                    uint8_t* v11 = a4 + 3;
                    *(v11 - 3) = static_cast<uint8_t>((a8.m_uiRM & *puiSrc) >> a8.m_ucRS) << a8.m_ucRQ;
                    uint8_t v12 = static_cast<uint8_t>((*puiSrc & a8.m_uiGM) >> a8.m_ucGS) << a8.m_ucGQ;
                    a4 = v11 + 1;
                    ++puiSrc;
                    *(a4 - 3) = v12;
                    --v10;
                    *(a4 - 2) = static_cast<uint8_t>((*(puiSrc - 1) & a8.m_uiBM) >> a8.m_ucBS) << a8.m_ucBQ;
                    *(a4 - 1) = 0xFF;
                }
                while (v10);
            }
            --a7a;
        }
        while (a7a);
    }
}

void ConvertBUMPLUMA32To32(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
) {
    // The destination is a raw pixel buffer, written one 32-bit value per pixel.
    uint32_t* puiDW = reinterpret_cast<uint32_t*>(a4);
    if (a2) {
        uint32_t v10 = a1;
        do {
            if (v10) {
                do {
                    uint8_t v11 = a7[1];
                    int32_t v12 = *a7 >> a6.m_ucRQ;
                    const uint8_t* v13 = a7 + 1;
                    uint8_t a4a = v11;
                    uint8_t v14 = v13[1];
                    a7 = v13 + 3;
                    --v10;
                    *puiDW++ = (a6.m_uiGM & a4a >> a6.m_ucGQ << a6.m_ucGS) |
                        ((a6.m_uiAM & v14 >> a6.m_ucAQ << a6.m_ucAS) | (a6.m_uiRM & v12 << a6.m_ucRS));
                }
                while (v10);
                v10 = a1;
            }
            --a2;
        }
        while (a2);
    }
}

void ConvertBUMPLUMA32To16(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
) {
    // The destination is raw pixel memory: Gamebryo's source writes it a word per pixel through
    // `(unsigned short*)pucDest`.
    uint16_t* pusW = reinterpret_cast<uint16_t*>(a4);
    if (a2) {
        uint32_t v9 = a1;
        uint32_t v15 = a2;
        do {
            if (v9) {
                uint32_t a2a = v9;
                do {
                    uint8_t v11 = *a7;
                    uint8_t v12 = a7[1];
                    const uint8_t* v13 = a7 + 1;
                    uint8_t v14 = v13[1];
                    a7 = v13 + 3;
                    *pusW++ = (static_cast<uint16_t>(a6.m_uiRM) & static_cast<uint8_t>(v11 >> a6.m_ucRQ) << a6.m_ucRS) |
                        ((static_cast<uint16_t>(a6.m_uiGM) & static_cast<uint8_t>(v12 >> a6.m_ucGQ) << a6.m_ucGS) |
                         (static_cast<uint16_t>(a6.m_uiAM) & static_cast<uint8_t>(v14 >> a6.m_ucAQ) << a6.m_ucAS));
                    --a2a;
                }
                while (a2a);
                v9 = a1;
            }
            --v15;
        }
        while (v15);
    }
}

void ConvertBUMP16To32(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
) {
    // The destination is raw pixel memory, written as one 32-bit value per pixel.
    uint32_t* puiDW = reinterpret_cast<uint32_t*>(a4);
    if (a2) {
        uint32_t v10 = a1;
        uint32_t a4a = a2;
        do {
            if (v10) {
                do {
                    uint8_t v11 = a7[1];
                    uint32_t v12 = a6.m_uiRM & *a7 >> a6.m_ucRQ << a6.m_ucRS;
                    a7 += 2;
                    --v10;
                    // The alpha channel is fully set: all ones are shifted into the mask.
                    *puiDW++ = (a6.m_uiAM & 0xFFFFFFFFu >> a6.m_ucAQ << a6.m_ucAS) |
                        ((a6.m_uiGM & v11 >> a6.m_ucGQ << a6.m_ucGS) | v12);
                }
                while (v10);
                v10 = a1;
            }
            --a4a;
        }
        while (a4a);
    }
}

void ConvertBUMP16To16(
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint8_t* a4,
    NiPalette* a5,
    const NiDevImageConverter::PixelBits& a6,
    const uint8_t* a7,
    const NiDevImageConverter::PixelBits& a8
) {
    // The destination is raw pixel memory: Gamebryo's source writes it a word per pixel through
    // `(unsigned short*)pucDest`.
    uint16_t* pusW = reinterpret_cast<uint16_t*>(a4);
    if (a2) {
        uint32_t a4a = a2;
        do {
            for (uint32_t i = a1; i; --i) {
                uint16_t v11 = static_cast<uint16_t>(a6.m_uiRM) & static_cast<uint8_t>(*a7 >> a6.m_ucRQ) << a6.m_ucRS;
                uint8_t v12 = a7[1] >> a6.m_ucGQ;
                a7 += 2;
                // The alpha channel is fully set: all ones are shifted into the mask.
                *pusW++ = (static_cast<uint16_t>(a6.m_uiAM) & 0xFFFFFFFFu >> a6.m_ucAQ << a6.m_ucAS) |
                    ((static_cast<uint16_t>(a6.m_uiGM) & v12 << a6.m_ucGS) | v11);
            }
            --a4a;
        }
        while (a4a);
    }
}

bool NiIsPowerOf2(uint32_t uiI) {
    return uiI && (uiI & uiI - 1) == 0;
}
