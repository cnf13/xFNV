#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class NiPalette;

// Application-side pixels of an image: every mipmap level of every face in one block.
class NiPixelData : public NiObject {
public:
    NiPixelData();
    // Builds an image of auiWidth x auiHeight pixels of akFormat with auiMipmapLevels levels per face.
    NiPixelData(
        uint32_t auiWidth,
        uint32_t auiHeight,
        const NiPixelFormat& akFormat,
        uint32_t auiMipmapLevels,
        uint32_t auiFaces
    );
    NiPixelData(const NiPixelData& a1);
    ~NiPixelData() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;

    void MarkAsChanged();
    uint32_t GetWidth(uint32_t uiMipmapLevel, uint32_t uiFace) const;
    uint8_t* GetPixels(uint32_t uiMipmapLevel, uint32_t uiFace);

    const uint8_t* GetPixels(uint32_t uiMipmapLevel, uint32_t uiFace) const {
        return const_cast<NiPixelData*>(this)->GetPixels(uiMipmapLevel, uiFace);
    }

    // The bytes of mipmap level uiMipmapLevel of one face.
    uint32_t GetSizeInBytes(uint32_t uiMipmapLevel, uint32_t uiFace) const;
    void SetNoConvert(bool abTF);
    void AllocateData(uint32_t uiLevels, uint32_t uiFaces, uint32_t uiBytes);
    void FreeData();
    uint32_t GetHeight(uint32_t uiMipmapLevel, uint32_t uiFace) const;
    // Swaps in the palette through the smart pointer assignment.
    void SetPalette(NiPalette* pkPal);

    static uint32_t ComputeMipmapLevels(uint32_t uiWidth, uint32_t uiHeight);
    static NiObject* CreateObject();

    NiPixelFormat m_kPixelFormat;
    NiPointer<NiPalette> m_spPalette;
    uint8_t* m_pucPixels;
    uint32_t* m_puiWidth;
    uint32_t* m_puiHeight;
    uint32_t* m_puiOffsetInBytes;
    uint32_t m_uiMipmapLevels;
    uint32_t m_uiPixelStride;
    uint32_t m_uiRevID;
    uint32_t m_uiFaces;
    bool bNoConvert;

    static NiRTTI ms_RTTI;
};
