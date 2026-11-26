#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"

class NiFile;
class NiPersistentSrcTextureRendererData;

// A texture loaded from a file (or given pixel data).
class NiSourceTexture : public NiTexture {
public:
    NiSourceTexture();
    ~NiSourceTexture() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
    void PostLinkObject(NiStream& kStream) override;
    void Upgrade() override;
    void Degrade() override;
    uint32_t GetWidth() const override;
    uint32_t GetHeight() const override;
    const NiFixedString& GetFilename() const override {
        return m_kFilename;
    }
    virtual void LoadPixelDataFromFile();
    virtual void DestroyAppPixelData();
    virtual bool CreateRendererData();

    void SetLookNormal();
    // Takes kFilename, standardizes it and derives the platform-specific name.
    void SetFilename(const NiFixedString& kFilename);
    bool QLookNormal();
    void* GetSrcPixelData();
    const NiFixedString& GetPlatformSpecificFilename() {
        return m_kPlatformFilename;
    }

    static void SetUsePreloading(bool bPreload);
    static void SetDestroyAppDataFlag(bool bDestroy);
    // A texture of apRawData named apcName (no name when null).
    static NiSourceTexture* Create(NiPixelData* apRawData, const char* apcName, NiTexture::FormatPrefs& arPrefs);
    // A texture of the file kFilename.
    static NiSourceTexture* Create(
        const NiFixedString& kFilename,
        NiTexture::FormatPrefs& kPrefs,
        bool bLoadDirectToRendererHint,
        bool bSrcRendererDataIsPersistent
    );
    // A texture read from the open file pFile, named apFileName (no name when null).
    static NiSourceTexture*
    Create(NiFile* pFile, const char* apFileName, NiTexture::FormatPrefs& kPrefs, bool bLoadDirectToRendererHint);
    static NiObject* CreateObject();

    NiFixedString m_kFilename;
    NiFixedString m_kPlatformFilename;
    NiPointer<NiPersistentSrcTextureRendererData> m_spPersistentSrcRendererData;
    NiPointer<NiPixelData> m_spSrcPixelData;
    bool m_bStatic;
    bool m_bLoadDirectToRendererHint;
    bool m_bSrcRendererDataIsPersistent;
    uint8_t cLooked;
    NiFile* m_pFile;

    static NiRTTI ms_RTTI;
    // The format preferences textures are created with when the caller has none.
    static FormatPrefs ms_kDefaultPrefs;
    static inline uint8_t ms_bPreload = 1;
    static inline uint8_t ms_bDestroyAppData;
    static inline bool ms_bMipmap = true;
};

NiSmartPointer(NiSourceTexture);

inline void NiSourceTexture::SetLookNormal() {}

// Always false: cLooked is read and masked with zero.
inline bool NiSourceTexture::QLookNormal() {
    return false;
}

inline void NiSourceTexture::SetUsePreloading(bool bPreload) {
    NiSourceTexture::ms_bPreload = bPreload;
}

inline void NiSourceTexture::SetDestroyAppDataFlag(bool bDestroy) {
    NiSourceTexture::ms_bDestroyAppData = bDestroy;
}
