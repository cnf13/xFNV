#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "Fallout Shared/PackageData.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESLoadScreenType.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/stdlib.h"

#include <cstring>
#include <type_traits>

class BGSImpactData;
class BGSMusicType;
class BSFile;
class NavMesh;
class SpellItem;
class TESBitArrayFile;
class TESEffectShader;
class TESFaction;
class TESIdleForm;
class TESObjectCELL;
class TESObjectREFR;
class TESSound;
class TESTopic;

// The engine's byte swap of a value read from a big-endian file. Callers pass a second, bool argument that the body
// never reads.
template <class T>
void ENDIAN(T& arValue, bool abUnused);
void* simpleMalloc(void* opaque, uint32_t aiNumItems, uint32_t aSize);
void simpleFree(void* opaque, void* apPtr);

// The chunk record codes, as the little-endian 32-bit value read from a file. XLCR_ID is the name for the code 'XCLP'.
enum CHUNK_ID : uint32_t {
    NO_CHUNK = 0,
    HEDR_ID = 0x52444548,
    ONAM_ID = 0x4D414E4F,
    EDID_ID = 0x44494445,
    AMRK_ID = 0x4B524D41,
    MMRK_ID = 0x4B524D4D,
    RNAM_ID = 0x4D414E52,
    XACR_ID = 0x52434158,
    XACT_ID = 0x54434158,
    XAMC_ID = 0x434D4158,
    XAMT_ID = 0x544D4158,
    XAPD_ID = 0x44504158,
    XAPR_ID = 0x52504158,
    XATO_ID = 0x4F544158,
    XCAS_ID = 0x53414358,
    XCCM_ID = 0x4D434358,
    XCET_ID = 0x54454358,
    XCHG_ID = 0x47484358,
    XCIM_ID = 0x4D494358,
    XCLM_ID = 0x4D4C4358,
    XCLR_ID = 0x524C4358,
    XCMO_ID = 0x4F4D4358,
    XCMT_ID = 0x544D4358,
    XCNT_ID = 0x544E4358,
    XCWT_ID = 0x54574358,
    XDCR_ID = 0x52434458,
    XEDL_ID = 0x4C444558,
    XEMI_ID = 0x494D4558,
    XESP_ID = 0x50534558,
    XEZN_ID = 0x4E5A4558,
    XGLB_ID = 0x424C4758,
    XHLP_ID = 0x504C4858,
    XHLT_ID = 0x544C4858,
    XHRS_ID = 0x53524858,
    XIBS_ID = 0x53424958,
    XLCM_ID = 0x4D434C58,
    XLKR_ID = 0x524B4C58,
    XLMB_ID = 0x424D4C58,
    XLOC_ID = 0x434F4C58,
    XLOD_ID = 0x444F4C58,
    XLTW_ID = 0x57544C58,
    XMBO_ID = 0x4F424D58,
    XMBP_ID = 0x50424D58,
    XMBR_ID = 0x52424D58,
    XMRC_ID = 0x43524D58,
    XMRK_ID = 0x4B524D58,
    XNDP_ID = 0x50444E58,
    XNVP_ID = 0x50564E58,
    XOCP_ID = 0x50434F58,
    XORD_ID = 0x44524F58,
    XOWN_ID = 0x4E574F58,
    XPCI_ID = 0x49435058,
    XPOD_ID = 0x444F5058,
    XPPA_ID = 0x41505058,
    XPRD_ID = 0x44525058,
    XPRM_ID = 0x4D525058,
    XPSL_ID = 0x4C535058,
    XPSN_ID = 0x4E535058,
    XPTL_ID = 0x4C545058,
    XPWR_ID = 0x52575058,
    XRAD_ID = 0x44415258,
    XRDO_ID = 0x4F445258,
    XRDS_ID = 0x53445258,
    XRGB_ID = 0x42475258,
    XRGD_ID = 0x44475258,
    XRMR_ID = 0x524D5258,
    XRNK_ID = 0x4B4E5258,
    XROO_ID = 0x4F4F5258,
    XRTM_ID = 0x4D545258,
    XSED_ID = 0x44455358,
    XSOL_ID = 0x4C4F5358,
    XSRD_ID = 0x44525358,
    XSRF_ID = 0x46525358,
    XTEL_ID = 0x4C455458,
    XTIM_ID = 0x4D495458,
    XTRG_ID = 0x47525458,
    XTRI_ID = 0x49525458,
    XUSE_ID = 0x45535558,
    XWLT_ID = 0x544C5758,
    XCLC_ID = 0x434C4358,
    OBND_ID = 0x444E424F,
    IMPF_ID = 0x46504D49,
    IMPS_ID = 0x53504D49,
    LNAM_ID = 0x4D414E4C,
    XCLL_ID = 0x4C4C4358,
    FULL_ID = 0x4C4C5546,
    XNAM_ID = 0x4D414E58,
    LTMP_ID = 0x504D544C,
    XCLW_ID = 0x574C4358,
    DATA_ID = 0x41544144,
    NAME_ID = 0x454D414E,
    XSCL_ID = 0x4C435358,
    XLCR_ID = 0x504C4358,
    // A reference's linked-reference colour ('RCLR'); TESObjectREFR::Load skips it.
    RCLR_ID = 0x524C4352,
    CNAM_ID = 0x4D414E43,
    IDLA_ID = 0x414C4449,
    IDLB_ID = 0x424C4449,
    IDLC_ID = 0x434C4449,
    IDLF_ID = 0x464C4449,
    IDLN_ID = 0x4E4C4449,
    IDLT_ID = 0x544C4449,
    PKDD_ID = 0x44444B50,
    PKDT_ID = 0x54444B50,
    PKES_ID = 0x32454B50,
    PKFD_ID = 0x44464B50,
    PKPT_ID = 0x54504B50,
    PKW2_ID = 0x32574B50,
    PKW3_ID = 0x33574B50,
    PLD2_ID = 0x32444C50,
    PLDT_ID = 0x54444C50,
    POBA_ID = 0x41424F50,
    POCA_ID = 0x41434F50,
    POEA_ID = 0x41454F50,
    PSDT_ID = 0x54445350,
    PTD2_ID = 0x32445450,
    PTDT_ID = 0x54445450,
    // CIDA_ID is the condition code 'CTDA'.
    CIDA_ID = 0x41445443,
    PRKE_ID = 0x454B5250,
    // The equip type code the alchemy item's loader compares.
    ETYP_ID = 0x50595445,
    BPTN_ID = 0x4E545042,
    BPNN_ID = 0x4E4E5042,
    BPNT_ID = 0x544E5042,
    BPNI_ID = 0x494E5042,
    BPND_ID = 0x444E5042,
    NAM1_ID = 0x314D414E,
    NAM2_ID = 0x324D414E,
    NAM3_ID = 0x334D414E,
    NAM4_ID = 0x344D414E,
    NAM5_ID = 0x354D414E,
    RAGA_ID = 0x41474152,
    SCDA_ID = 0x41444353,
    EPF2_ID = 0x32465045,
    EPF3_ID = 0x33465045,
    SLSD_ID = 0x44534C53,
    SCRO_ID = 0x4F524353,
    SCHR_ID = 0x52484353,
    SCVR_ID = 0x52564353,
    SCRV_ID = 0x56524353,
    NVER_ID = 0x5245564E,
    NVVX_ID = 0x5856564E,
    NVTR_ID = 0x5254564E,
    NVEX_ID = 0x5845564E,
    NVGD_ID = 0x4447564E,
    NVCA_ID = 0x4143564E,
    NVDP_ID = 0x5044564E,
    NVMI_ID = 0x494D564E,
    NVCI_ID = 0x4943564E,
    XXXX_ID = 0x58585858,
    INFC_ID = 0x43464E49,
    INFX_ID = 0x58464E49,
    PNAM_ID = 0x4D414E50,
    QSTI_ID = 0x49545351,
    QSTA_ID = 0x41545351,
    TDUM_ID = 0x4D554454,
    XIDX_ID = 0x58444958,
    PRKF_ID = 0x464B5250,
    MODL_ID = 0x4C444F4D,
    MODD_ID = 0x44444F4D,
    MOSD_ID = 0x44534F4D,
    MODT_ID = 0x54444F4D,
    MO2S_ID = 0x53324F4D,
    MO3S_ID = 0x53334F4D,
    MO4S_ID = 0x53344F4D,
    ANAM_ID = 0x4D414E41,
    RDAT_ID = 0x54414452,
    RDID_ID = 0x44494452,
    NNAM_ID = 0x4D414E4E,
    FNAM_ID = 0x4D414E46,
    MNAM_ID = 0x4D414E4D,
    SNAM_ID = 0x4D414E53,
    MAST_ID = 0x5453414D,
    DNAM_ID = 0x4D414E44,
    GNAM_ID = 0x4D414E47,
    COED_ID = 0x44454F43,
    VNAM_ID = 0x4D414E56,
    INAM_ID = 0x4D414E49,
    WNAM_ID = 0x4D414E57,
    WLST_ID = 0x54534C57,
    TNAM_ID = 0x4D414E54,
    MODS_ID = 0x53444F4D,
    // The leveled-list codes.
    LVLF_ID = 0x464C564C,
    LVLD_ID = 0x444C564C,
    LVLO_ID = 0x4F4C564C,
    LVLG_ID = 0x474C564C,
    // The destructible-object codes.
    DSTD_ID = 0x44545344,
    DEST_ID = 0x54534544,
    // The load screen's type code.
    WMI1_ID = 0x31494D57,
    PFIG_ID = 0x47494650,
    PFPC_ID = 0x43504650,
    INDX_ID = 0x58444E49,
    ICON_ID = 0x4E4F4349,
    JNAM_ID = 0x4D414E4A,
    ENAM_ID = 0x4D414E45,
    EITM_ID = 0x4D544945,
    EAMT_ID = 0x544D4145,
    RDWT_ID = 0x54574452,
    RAFD_ID = 0x44464152,
    RAFB_ID = 0x42464152,
    RAPS_ID = 0x53504152,
    // The quest objective's index code 'QOBJ'.
    QOBJ_ID = 0x4A424F51,
    // The quest, topic info, recipe, skill and sound codes DS509's loaders compare.
    SCRI_ID = 0x49524353,
    QSDT_ID = 0x54445351,
    NAM0_ID = 0x304D414E,
    NEXT_ID = 0x5458454E,
    RCOD_ID = 0x444F4352,
    RCQY_ID = 0x59514352,
    SNDD_ID = 0x44444E53,
    SNDX_ID = 0x58444E53,
    TRDT_ID = 0x54445254,
    EFID_ID = 0x44494645,
    QNAM_ID = 0x4D414E51,
    HNAM_ID = 0x4D414E48,
    KNAM_ID = 0x4D414E4B,
    YNAM_ID = 0x4D414E59,
    ZNAM_ID = 0x4D414E5A,
    NAM6_ID = 0x364D414E,
    NAM7_ID = 0x374D414E,
    RCIL_ID = 0x4C494352,
    DAT2_ID = 0x32544144,
    INTV_ID = 0x56544E49,
    MOD2_ID = 0x32444F4D,
    MOD3_ID = 0x33444F4D,
    TX00_ID = 0x30305854,
    TX01_ID = 0x31305854,
    CARD_ID = 0x44524143,
    RDSI_ID = 0x49534452,
    RPLD_ID = 0x444C5052,
    RDSD_ID = 0x44534452,
    RDMD_ID = 0x444D4452,
    RDMP_ID = 0x504D4452,
    RDLN_ID = 0x4E4C4452,
    RDOJ_ID = 0x4A4F4452,
    RPLI_ID = 0x494C5052,
    RDOB_ID = 0x424F4452,
    RDSB_ID = 0x42534452,
    RDOT_ID = 0x544F4452,
    RDGS_ID = 0x53474452,
    // The codes DS502's loaders compare (the extra texture names and the effect shader's).
    TX02_ID = 0x32305854,
    TX03_ID = 0x33305854,
    TX04_ID = 0x34305854,
    TX05_ID = 0x35305854,
    ESCE_ID = 0x45435345,
    ITXT_ID = 0x54585449,
    DESC_ID = 0x43534544,
    // The texture set's decal data code ('DODT' in the file).
    DODT_ID = 0x54444F44,
    // The biped model, sound and container codes DS507's loaders compare.
    MOD4_ID = 0x34444F4D,
    MO2T_ID = 0x54324F4D,
    MO3T_ID = 0x54334F4D,
    MO4T_ID = 0x54344F4D,
    MIC2_ID = 0x3243494D,
    ICO2_ID = 0x324F4349,
    BMCT_ID = 0x54434D42,
    BMDT_ID = 0x54444D42,
    BNAM_ID = 0x4D414E42,
    REPL_ID = 0x4C504552,
    BIPL_ID = 0x4C504942,
    CNTO_ID = 0x4F544E43,
    // The race saver's codes (the voice type, the second face clamp and the three FaceGen arrays).
    VTCK_ID = 0x4B435456,
    UNAM_ID = 0x4D414E55,
    FGGS_ID = 0x53474746,
    FGGA_ID = 0x41474746,
    FGTS_ID = 0x53544746,
    // The class, challenge and combat style codes DS504's loaders compare.
    ATTR_ID = 0x52545441,
    CSSD_ID = 0x44535343,
    CSTD_ID = 0x44545343,
    CSAD_ID = 0x44415343,
    MICO_ID = 0x4F43494D,
    // The item-mod, static and weapon codes DS508's loaders compare.
    BRUS_ID = 0x53555242,
    VANM_ID = 0x4D4E4156,
    VATS_ID = 0x53544156,
    CRDT_ID = 0x54445243,
    EFSD_ID = 0x44534645,
    NAM8_ID = 0x384D414E,
    NAM9_ID = 0x394D414E,
    WMI2_ID = 0x32494D57,
    WMI3_ID = 0x33494D57,
    WMS1_ID = 0x31534D57,
    WMS2_ID = 0x32534D57,
    WNM1_ID = 0x314D4E57,
    WNM2_ID = 0x324D4E57,
    WNM3_ID = 0x334D4E57,
    WNM4_ID = 0x344D4E57,
    WNM5_ID = 0x354D4E57,
    WNM6_ID = 0x364D4E57,
    WNM7_ID = 0x374D4E57,
    MWD1_ID = 0x3144574D,
    MWD2_ID = 0x3244574D,
    MWD3_ID = 0x3344574D,
    MWD4_ID = 0x3444574D,
    MWD5_ID = 0x3544574D,
    MWD6_ID = 0x3644574D,
    MWD7_ID = 0x3744574D,
    MWT1_ID = 0x3154574D,
    MWT2_ID = 0x3254574D,
    MWT3_ID = 0x3354574D,
    MWT4_ID = 0x3454574D,
    MWT5_ID = 0x3554574D,
    MWT6_ID = 0x3654574D,
    MWT7_ID = 0x3754574D,
    // The first of a weather's per-time image space modifier codes ('\0IAD'; the next ones add 1 each).
    SIAD_ID = 0x44414900,
    // A worldspace's cell offsets.
    OFST_ID = 0x5453464F,
    // The NPC codes TESNPC::Load compares (AI data, base stats, spells, hair color, template, KF files, packages).
    AIDT_ID = 0x54444941,
    ACBS_ID = 0x53424341,
    SPLO_ID = 0x4F4C5053,
    HCLR_ID = 0x524C4348,
    TPLT_ID = 0x544C5054,
    KFFZ_ID = 0x5A46464B,
    PKID_ID = 0x44494B50,
    // The creature codes TESCreature::Load compares (model lists, sound type and sounds, sound creature).
    NIFT_ID = 0x5446494E,
    NIFZ_ID = 0x5A46494E,
    CSDT_ID = 0x54445343,
    CSDI_ID = 0x49445343,
    CSDF_ID = 0x46445343,
    CSCR_ID = 0x52435343,
    DELE_ID = 0x454C4544,
    SCRD_ID = 0x44524353,
    SCRS_ID = 0x53524353,
    CLNM_ID = 0x4D414E51,
    RDMO_ID = 0x4F4D4452,
    XRFB_ID = 0x42465258,
    XLRM_ID = 0x4D524C58,
    XWMS_ID = 0x534D5758,
    ACTN_ID = 0x4E544341,
    STPR_ID = 0x52505453,
    SOUL_ID = 0x4C554F53,
    SLCP_ID = 0x50434C53,
    TCLT_ID = 0x544C4354,
    TCLF_ID = 0x464C4354,
    TCFU_ID = 0x55464354,
    ESCS_ID = 0x53435345,
    ESBS_ID = 0x53425345,
    ESHS_ID = 0x53485345,
    ESAS_ID = 0x53415345,
    WIDX_ID = 0x58444957,
    MODQ_ID = 0x51444F4D,
    ATDM_ID = 0x4D445441,
    STRV_ID = 0x56525453,
    FLTV_ID = 0x56544C46,
    NPAC_ID = 0x4341504E,
    ND3D_ID = 0x4433444E,
    ANIS_ID = 0x53494E41,
    EFIT_ID = 0x54494645,
    AVIT_ID = 0x54495641,
    SCIT_ID = 0x54494353,
    SPIT_ID = 0x54495053,
    ENIT_ID = 0x54494E45,
    PKED_ID = 0x44454B50,
    PKND_ID = 0x444E4B50,
    PUID_ID = 0x44495550,
    PKAM_ID = 0x4D414B50,
    BIDX_ID = 0x58444942,
    BPTM_ID = 0x4D545042,
    BPTF_ID = 0x46545042,
    MVRF_ID = 0x4652564D,
    AGTY_ID = 0x59544741,
    AGFD_ID = 0x44464741,
    AGSC_ID = 0x43534741,
    AGSX_ID = 0x58534741,
    SCTX_ID = 0x58544353,
    SLCS_ID = 0x53434C53,
    SLLD_ID = 0x444C4C53,
    SLFD_ID = 0x44464C53,
    SLUD_ID = 0x44554C53,
    VHGT_ID = 0x54474856,
    VNML_ID = 0x4C4D4E56,
    VCLR_ID = 0x524C4356,
    BTXT_ID = 0x54585442,
    ATXT_ID = 0x54585441,
    VTXT_ID = 0x54585456,
    MPCD_ID = 0x4443504D,
    PGRP_ID = 0x50524750,
    PGRC_ID = 0x52524750,
    PGRI_ID = 0x49524750,
    PGRL_ID = 0x4C524750,
    PGAG_ID = 0x47414750,
    LVCR_ID = 0x5243564C,
    GMDT_ID = 0x54444D47,
    TPIC_ID = 0x43495054,
    QSTR_ID = 0x52545351,
    WHGT_ID = 0x54474857,
    LODH_ID = 0x48444F4C,
    LODV_ID = 0x56444F4C,
    LODS_ID = 0x53444F4C,
    LODT_ID = 0x54444F4C,
    LODC_ID = 0x43444F4C,
    LODN_ID = 0x4E444F4C,
    CSDC_ID = 0x43445343,
    LVSR_ID = 0x5053564C,
    PROD_ID = 0x444F5250,
    NVTX_ID = 0x5854564E,
    NVLX_ID = 0x584C564E,
    DSTF_ID = 0x46545344,
    DMDL_ID = 0x4C444D44,
    DMDT_ID = 0x54444D44,
    DMDS_ID = 0x53444D44,
    TX06_ID = 0x36305854,
    TX07_ID = 0x37305854,
    TX08_ID = 0x38305854,
    TX09_ID = 0x39305854,
    DMTL_ID = 0x4C544D44,
    PRKC_ID = 0x434B5250,
    EPFT_ID = 0x54465045,
    EPFD_ID = 0x44465045,
    ISCR_ID = 0x52435349,
    EIAD_ID = 0x444149FF,
    RAFS_ID = 0x53464152,
    RAFL_ID = 0x4C464152,
    RAFN_ID = 0x4E464152,
    RALH_ID = 0x484C4152,
    RALN_ID = 0x4E4C4152,
    RAGN_ID = 0x4E474152,
    RCID_ID = 0x4C494352,
    RCCF_ID = 0x46434352,
    CQES_ID = 0x53455143,
};

// The header of a record in a plugin file: its type code, size, flags and form ID (24 bytes). For a group (form 'GRUP')
// `flags` holds the group's label and `iFormID` its FORM_GROUP_TYPE.
struct FORM {
    // Byte swaps every field of a big-endian header.
    void Endian();

    uint32_t form;
    uint32_t length;
    uint32_t flags;
    uint32_t iFormID;
    uint32_t iVersionControl;
    uint16_t sFormVersion;
    uint16_t sVCVersion;
};

// The kinds of record group (a group header's FORM::iFormID); FGT_TYPE is a top-level group of one form type.
enum FORM_GROUP_TYPE : uint32_t {
    FGT_TYPE = 0,
    FGT_WORLDCHILD = 1,
    FGT_INTCELLBLOCK = 2,
    FGT_INTCELLSUBBLOCK = 3,
    FGT_EXTCELLBLOCK = 4,
    FGT_EXTCELLSUBBLOCK = 5,
    FGT_CELLCHILD = 6,
    FGT_TOPICCHILD = 7,
    FGT_PERSISTENTCHILD = 8,
    FGT_TEMPCHILD = 9,
};

// A record group as the save and load code passes it: the group's header and its offset in the file.
struct FORM_GROUP {
    FORM GroupData;
    uint32_t iGroupOffset;
};

// The header of a chunk (subrecord): its type code and size. It is 6 bytes, so the size is not padded.
#pragma pack(push, 2)
struct CHUNK {
    // Byte swaps both fields of a big-endian header.
    void Endian();

    uint32_t chunk;
    uint16_t length;
};
#pragma pack(pop)

// A pointer holds the 4-byte form ID Load read until InitItem resolves it, so its four low bytes are swapped.
template <class T>
void ENDIAN(T& arValue, bool abUnused) {
    if constexpr (std::is_pointer_v<T>) {
        // The pointer field still holds the untyped form ID that was read into it, not an object's address.
        uint32_t uiFormID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(arValue));
        arValue = reinterpret_cast<T>(static_cast<uintptr_t>(std::byteswap<uint32_t>(uiFormID)));
    }
    else if constexpr (sizeof(T) == 2) {
        uint16_t usValue;
        std::memcpy(&usValue, &arValue, sizeof(usValue));
        usValue = std::byteswap<uint16_t>(usValue);
        std::memcpy(&arValue, &usValue, sizeof(usValue));
    }
    else if constexpr (sizeof(T) == 4) {
        uint32_t uiValue;
        std::memcpy(&uiValue, &arValue, sizeof(uiValue));
        uiValue = std::byteswap<uint32_t>(uiValue);
        std::memcpy(&arValue, &uiValue, sizeof(uiValue));
    }
    else {
        static_assert(sizeof(T) == 8, "ENDIAN swaps 2, 4 or 8 bytes");
        uint64_t ullValue;
        std::memcpy(&ullValue, &arValue, sizeof(ullValue));
        ullValue = std::byteswap<uint64_t>(ullValue);
        std::memcpy(&arValue, &ullValue, sizeof(ullValue));
    }
}
// The version, record count and next free form ID stored in a plugin's header record.
struct TESFileHeaderInfo {
    // Byte-swaps the three dwords.
    void Endian();

    float fVersion;
    uint32_t uiFormCount;
    uint32_t uiNextFormID;
};

// The file's find data, the Win32 WIN32_FIND_DATAA (0x140 bytes), filled from the directory search's entry by
// OpenTES. TESDataHandler::BuildFileList compares GetFileData()->ftLastWriteTime (+0x14) with a found file's.
using TESFileFindData = _WIN32_FIND_DATAA;

// The result of a file operation, kept in TESFile::m_lastError.
enum TES_RETURN_CODE : int32_t {
    TES_ERROR_NONE = 0,
    TES_ERROR_NOT_FOUND = 1,
    TES_ERROR_NO_FILE = 2,
    TES_ERROR_NO_FORM = 3,
    TES_ERROR_NO_CHUNK = 4,
    TES_ERROR_NO_ID = 5,
    TES_ERROR_BAD_FILE = 6,
    TES_ERROR_BAD_ID = 7,
    TES_ERROR_FORM_OPEN = 8,
    TES_ERROR_FILE_OPEN = 9,
    TES_ERROR_WRITE_FAILURE = 10,
    TES_ERROR_INVALID_FILE = 11,
    TES_ERROR_FILE_IN_USE = 12,
    TES_ERROR_CREATE_FAILURE = 13,
};

// One plugin (.esm / .esp) or save file, read record by record.
class TESFile {
public:
    // Bits of m_Flags (the top byte holds the file's language).
    enum FILE_FLAG : uint32_t {
        FILE_MASTER = 0x1,
        FILE_CHECKED = 0x4,
        FILE_ACTIVE = 0x8,
        FILE_OPTIMIZED = 0x10,
        FILE_TEMP_ID_OWNER = 0x20,
        FILE_LITTLE_ENDIAN = 0x40,
    };

    // The loaded file that owns the temporary form ID aiTempID.
    static TESFile* GetFileForTempID(uint32_t aiID);
    static inline int32_t iOwnerFileCount;
    TES_RETURN_CODE m_lastError;
    TESFile* pThreadSafeParent;
    // the copies of this file made for other threads, by thread ID
    NiTPointerMap<uint32_t, TESFile*>* pThreadSafeFileMap;
    BSFile* m_pLockedFile;
    BSFile* m_pFile;
    TESBitArrayFile* pFormUserDataBitArray;
    TESBitArrayFile* pFormVersionBitArray;
    TESBitArrayFile* pFormIDBitArray;
    char m_Filename[260];
    char m_Path[260];
    char* m_pBuffer;
    uint32_t m_uiBufferAllocSize;
    // The offsets of the file's first cell, its current cell and its current reference, and the cell being loaded;
    // the constructor clears all four.
    uint32_t m_firstCellOffset;
    uint32_t m_currCellOffset;
    TESObjectCELL* m_pCurrCell;
    uint32_t m_currRefOffset;
    FORM m_currentform;
    CHUNK_ID m_currentchunkID;
    uint32_t m_actualChunkSize;
    uint32_t m_filesize;
    uint32_t m_fileoffset;
    uint32_t m_formoffset;
    uint32_t m_chunkoffset;
    // The header of the form being written, and where it and its current chunk start (CloseForm writes the size
    // m_savechunkoffset into m_saveform.length and seeks back to m_saveformoffset).
    FORM m_saveform;
    uint32_t m_saveformoffset;
    uint32_t m_savechunkoffset;
    // the groups that are currently open
    BSSimpleList<FORM_GROUP*> m_grouplist;
    bool bHasGroups;
    bool bMustEndianConvert;
    bool bCloseFileOverride;
    uint8_t padding29B[1];
    TESFileFindData m_FileInfo;
    TESFileHeaderInfo fileHeaderInfo;
    uint32_t m_Flags;
    BSSimpleList<char*> listMasters;
    BSSimpleList<_ULARGE_INTEGER*> listMastersData;
    uint32_t iMasterCount;
    TESFile** m_pMasterPtrs;
    _FILETIME DeletedFormTime;
    uint8_t cCompileIndex;
    uint8_t padding40D[3];
    // the CNAM (author) string LoadTESInfo reads
    BSStringT<char> cCreatedBy;
    BSStringT<char> cSummary;
    char* pDecompressedFormBuffer;
    uint32_t iDecompressedFormBufferSize;
    bool bCached;
    bool bCaching;
    bool bDLC;
    uint8_t padding42B[1];

    // the default size of a file's read buffer
    static inline uint32_t uiDefaultBufferSize = 10240;
    // The map from temporary form IDs to the file that owns them (null when unused).
    using OwnedTempIDMap = NiTMap<uint32_t, TESFile*>;
    static inline OwnedTempIDMap* pOwnedTempIDMap;

    uint32_t GetFORMSize() const;
    const char* GetFilename() const;
    const char* GetPath() const;
    TESFileFindData* GetFileData();
    FORM* GetFormData();
    BSSimpleList<char*>* GetMasterList();
    BSSimpleList<_ULARGE_INTEGER*>* GetMasterDataList();
    FORM_GROUP* GetCurrentGroup() const;
    uint32_t GetChunkSize() const;
    bool QEndian() const;
    int32_t GetFormCount() const;
    uint32_t GetOffset() const;
    // Seeks the file to record offset newoffset and reads its header.
    bool SetOffset(uint32_t newoffset);
    // Seeks the file to apForm's record; false when it is not in this file.
    bool FindForm(TESForm* apForm);
    TES_RETURN_CODE AddTESForm(TESForm* apForm);
    static void DeleteFileList(BSSimpleList<TESFile*>* apFileList);
    float GetFileVersion() const;
    uint32_t GetMasterCount() const;
    uint32_t GetIndexCount() const;
    uint32_t GetDecompressedBufferSize() const;
    uint8_t GetCompileIndex() const;
    void SetCompileIndex(uint8_t acIndex);
    bool GetHasGroups() const;
    void SetHasGroups(bool abVal);
    uint16_t GetFormVersion() const;
    void ChangeBufferSize(int32_t aiNewSize);
    TESFile* GetThreadSafeParent() const;
    void SetThreadSafeParent(TESFile* apFile);
    TESFile* GetIndexFile(uint32_t index);

    bool GetMaster() const;
    bool GetOptimizedFile() const;
    bool GetChecked() const;
    bool GetActive() const;
    bool GetTempIDOwner() const;
    bool GetLittleEndian() const;
    void SetChecked(bool checked);
    void SetLittleEndian(bool abLittle);
    // Sets or clears the master file flag (bit 0 of m_Flags).
    void SetMaster(bool master);
    void SetFileLanguage(LANGUAGE language);

    void ClearCurrentChunk();
    bool IsCurrentFormCompressed() const;
    char* GetDecompressedBuffer();
    void FreeDecompressedBuffer();
    uint32_t GetOffsetChunk() const;
    CHUNK_ID GetTESChunk();
    ENUM_FORM_ID GetTESForm();
    void GetTESFormNoRet();
    bool NextForm(bool abSkipIgnored);
    bool NextGroup();

    bool GetNextForm();
    bool ReadFormHeader();
    bool ReadChunkHeader();
    void DecompressCurrentForm();
    // Frees every master name and master data entry and forgets the master count.
    void ClearMasterLists();
    // Reads the file's first record type to learn whether it is byte swapped (sets bMustEndianConvert).
    void InitEndian();
    // Seeks to chunk offset newoffset inside the current record and reads that chunk's header.
    bool SetOffsetChunk(uint32_t newoffset);
    // Deletes the per-thread copies of this file and their map.
    void ClearThreadSafeFiles();
    // Empty on PC, because the aux buffer code is compiled out there, as AddAuxBuffer shows. ClearThreadSafeFiles calls
    // it on each copy before deleting it.
    void ClearAuxBuffer();

    // the form flags of the record being read
    uint32_t GetCurrentFormFlags() const;
    // the form ID of the record being read (the engine's TESFile::GetID)
    uint32_t GetID() const;
    // Reads a 2 byte chunk, byte swapped when the file is big endian.
    bool LoadWord(int16_t* apValue);
    void CloseAllOpenGroups();
    // Rewinds the stream and forgets the current record, optionally reading the next form header.
    void TESRewind(bool getform);
    // Gives apForm its type, flags, ID and this file from the record being read.
    void LoadForm(TESForm* apForm);
    // whether a temporary form ID may be loaded from apFile (false when another file owns it)
    static bool GetTempIDCanLoadFromFile(uint32_t aiID, TESFile* apFile);

    // Copies the current chunk into apData, at most auiMaxSize bytes (0: the chunk's size); false when it is
    // missing. An empty chunk returns early without writing apData.
    bool GetChunkData(void* apData, uint32_t auiMaxSize);
    // Reads the current chunk into arData.
    template <class T>
    bool GetChunkData(T& arData);
#if XFNV_64BIT
    // A pointer field of a record read from a file, which the file keeps as a 4-byte form ID until InitItem
    // resolves it: its offset in the file's record and in memory.
    struct FormIDField {
        uint32_t uiFileOffset;
        size_t uiOffset;
    };
    // The 64-bit program's form of a read into records whose pointer fields the file keeps as form IDs: the chunk is
    // read into the start of apRecords in the file's 32-bit layout (auiFileSize bytes a record), then spread over
    // the auiCount records (auiSize bytes each), last first. Each record's bytes move to its own offset, each form
    // ID listed in aFields (in their order in the record) fills the whole pointer field, and the bytes between two
    // such fields keep their layout.
    template <size_t N>
    static void WidenFormIDRecords(
        void* apRecords,
        uint32_t auiCount,
        uint32_t auiFileSize,
        size_t auiSize,
        const FormIDField (&aFields)[N]
    );
    // The write's counterpart: auiCount records of auiSize bytes at apRecords are packed, from the first, into the
    // file's 32-bit layout (auiFileSize bytes a record) at the start of apRecords; each field in aFields keeps the
    // low 4 bytes of its pointer field (the form ID the writer stored there).
    template <size_t N>
    static void NarrowFormIDRecords(
        void* apRecords,
        uint32_t auiCount,
        uint32_t auiFileSize,
        size_t auiSize,
        const FormIDField (&aFields)[N]
    );
#endif
    // Ends the innermost open group of a save file.
    void EndGroup();
    // Closes the innermost open group.
    void CloseGroup();
    // Moves to the next chunk of the current record; false at its end.
    bool NextChunk();
    // Records apFile as the owner of temporary form ID aiID.
    static void SetTempIDOwnedByFile(uint32_t aiID, TESFile* apFile);

    TESFile(const char* apPath, const char* apFilename, NiFile::OpenMode auiAccessMode);
    ~TESFile();
    // Frees through ourFree.
    static void operator delete(void* apMemory);
    // Reads the file's header record; 0 on success.
    TES_RETURN_CODE LoadTESInfo();
    // Resolves the masters against pFileList into the index table; false when a master is missing (warned about
    // when showerror is set).
    bool GenIndexTable(BSSimpleList<TESFile*>* pFileList, bool showerror);
    // Whether the file's version is newer than the game supports.
    bool IsFileVersionTooHigh();
    // The name of master aiIndex (1-based), null when there is none.
    const char* GetIndexName(uint32_t index);
    // Whether a master changed since the file was saved.
    bool MastersChanged();
    // Opens the file at its own path and name.
    bool OpenTES(NiFile::OpenMode auiAccessMode, bool abLock);
    // Opens pfilename in ppath (finding it first and keeping its find data) and reads its first record; reopens
    // with the default version when the header is not usable.
    bool OpenTES(const char* ppath, const char* pfilename, NiFile::OpenMode accessmode, bool lock);
    // Sets the version in the file's header info.
    void SetFileVersion(float afVal);
    // Locks the file for writing.
    bool TakeOwnership();
    // The copy of this file for the calling thread.
    TESFile* GetThreadSafeFile();
    TESFile* GetThreadSafeFileForThread(uint32_t aiThreadID);
    void SetDescription(const char* apDescription);
    // Sets the reload flag at +0x428.
    void SetMaybeShouldBeReloaded(bool abReload);
    // The reload flag at +0x428.
    bool GetMaybeShouldBeReloaded();
    // Registers the record data from aiMinIndex for aiMaxIndex bytes as an auxiliary buffer;
    // an empty body that returns true in this build.
    bool AddAuxBuffer(uint32_t aiMinIndex, uint32_t aiMaxIndex);
    // Closes the file's stream and group state; false when that failed.
    bool CloseTES();
    void SetActive(bool active);
    void SetTempIDOwner(bool abVal);
    static uint16_t GetFileFormVersion();
    // The open groups list.
    BSSimpleList<FORM_GROUP*>* GetGroupList();

protected:
    // Writes count bytes from ptr to the file; on a short write warns and returns TES_ERROR_WRITE_FAILURE, else
    // TES_ERROR_NONE.
    TES_RETURN_CODE EasyWrite(void* ptr, uint32_t count);

public:
    void TESRewindChunk();
    TES_RETURN_CODE StartForm(TESForm* apForm);
    uint32_t CloseForm();
    void StartGroup(FORM* apNewGroupData);
    void OpenGroup(FORM* apGroupData);
    void StartAndEndGroupsForForm(TESForm* apForm);
    void RemoveFromOwnedIDMap();
    // &cSummary.
    BSStringT<char>* GetSummary();
};
// Reads sizeof(T) bytes of the current chunk into arData and swaps them when the file is big-endian; returns the read's
// result.
template <class T>
bool TESFile::GetChunkData(T& arData) {
    bool bResult = GetChunkData(&arData, sizeof(T));
    if (QEndian()) {
        ENDIAN(arData, false);
    }
    return bResult;
}
#if XFNV_64BIT
template <size_t N>
void TESFile::WidenFormIDRecords(
    void* apRecords,
    uint32_t auiCount,
    uint32_t auiFileSize,
    size_t auiSize,
    const FormIDField (&aFields)[N]
) {
    char* pRecords = static_cast<char*>(apRecords);
    // Every byte moves up (a pointer field takes 8 bytes for the file's 4), so working from the end down never
    // overwrites bytes still to be moved.
    for (uint32_t i = auiCount; i-- > 0;) {
        char* pRecord = pRecords + i * auiSize;
        std::memmove(pRecord, pRecords + i * auiFileSize, auiFileSize);
        uint32_t uiEnd = auiFileSize;
        for (size_t j = N; j-- > 0;) {
            const FormIDField& rField = aFields[j];
            // A field past the bytes read (a short chunk) keeps its value, as in the 32-bit read.
            if (rField.uiFileOffset + 4 > uiEnd) {
                continue;
            }
            uint32_t uiFormID;
            std::memcpy(&uiFormID, pRecord + rField.uiFileOffset, sizeof(uiFormID));
            std::memmove(
                pRecord + rField.uiOffset + sizeof(uintptr_t), pRecord + rField.uiFileOffset + 4,
                uiEnd - rField.uiFileOffset - 4
            );
            uintptr_t uiValue = uiFormID;
            std::memcpy(pRecord + rField.uiOffset, &uiValue, sizeof(uiValue));
            uiEnd = rField.uiFileOffset;
        }
    }
}
template <size_t N>
void TESFile::NarrowFormIDRecords(
    void* apRecords,
    uint32_t auiCount,
    uint32_t auiFileSize,
    size_t auiSize,
    const FormIDField (&aFields)[N]
) {
    char* pRecords = static_cast<char*>(apRecords);
    // Every byte moves down (a pointer field's 8 bytes become the file's 4), so working from the start up never
    // overwrites bytes still to be moved.
    for (uint32_t i = 0; i < auiCount; ++i) {
        const char* pRecord = pRecords + i * auiSize;
        char* pFileRecord = pRecords + i * auiFileSize;
        uint32_t uiFileStart = 0;
        size_t uiStart = 0;
        for (const FormIDField& rField : aFields) {
            uintptr_t uiValue;
            std::memcpy(&uiValue, pRecord + rField.uiOffset, sizeof(uiValue));
            std::memmove(pFileRecord + uiFileStart, pRecord + uiStart, rField.uiFileOffset - uiFileStart);
            uint32_t uiFormID = static_cast<uint32_t>(uiValue);
            std::memcpy(pFileRecord + rField.uiFileOffset, &uiFormID, sizeof(uiFormID));
            uiFileStart = rField.uiFileOffset + 4;
            uiStart = rField.uiOffset + sizeof(uintptr_t);
        }
        std::memmove(pFileRecord + uiFileStart, pRecord + uiStart, auiFileSize - uiFileStart);
    }
}
#endif
