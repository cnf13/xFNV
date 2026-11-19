#include "SDK/NVAPI/nvapi.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "platform/System.h"
#include <stdint.h>

int32_t g_NvAPI_Initialized;
int32_t g_NvAPI_InitStatus;
int32_t g_NvAPI_StubStatus = -4;
// Every entry starts at the not-implemented stub; the interface ids are NVAPI's (nvapi.lib's table).
NvAPI_InterfaceTable_t g_NvAPI_Interfaces{
	{NvAPI_Stub, 0x6C2D048C}, {NvAPI_Stub, 0x1053FA5}, {NvAPI_Stub, 0xF951A4D1}, {NvAPI_Stub, 0x9ABDD40D},
	{NvAPI_Stub, 0x20DE9260}, {NvAPI_Stub, 0xE5AC921F}, {NvAPI_Stub, 0x48B3EA59}, {NvAPI_Stub, 0x34EF9506},
	{NvAPI_Stub, 0x5018ED61}, {NvAPI_Stub, 0x63F9799E}, {NvAPI_Stub, 0xEE1370CF}, {NvAPI_Stub, 0xADD604D1},
	{NvAPI_Stub, 0xAEA3FA32}, {NvAPI_Stub, 0x1890E8DA}, {NvAPI_Stub, 0x5380AD1A}, {NvAPI_Stub, 0x6533EA3E},
	{NvAPI_Stub, 0x35C29134}, {NvAPI_Stub, 0x22A78B05}, {NvAPI_Stub, 0x4888D790}, {NvAPI_Stub, 0x2863148D},
	{NvAPI_Stub, 0xAB163097}, {NvAPI_Stub, 0x67B5DB55}, {NvAPI_Stub, 0x3092AC32}, {NvAPI_Stub, 0xD995937E},
	{NvAPI_Stub, 0xC64FF367}, {NvAPI_Stub, 0xFA13E65A}, {NvAPI_Stub, 0x6AE16EC3}, {NvAPI_Stub, 0x9734F1D},
	{NvAPI_Stub, 0x69C6F365}, {NvAPI_Stub, 0x67EFD887}, {NvAPI_Stub, 0x41511594}, {NvAPI_Stub, 0x7F9B368},
	{NvAPI_Stub, 0x774AA982}, {NvAPI_Stub, 0x2DC95125}, {NvAPI_Stub, 0x4085DE45}, {NvAPI_Stub, 0x172409B4},
	{NvAPI_Stub, 0x9FB063DF}, {NvAPI_Stub, 0x3FC9A59C}, {NvAPI_Stub, 0x23B19713}, {NvAPI_Stub, 0x357CFB24},
	{NvAPI_Stub, 0x2F948148}, {NvAPI_Stub, 0x38AC3081}, {NvAPI_Stub, 0xFA2F1791}, {NvAPI_Stub, 0xE0C76DFD},
	{NvAPI_Stub, 0xFF8BD545}, {NvAPI_Stub, 0xE563AF29}, {NvAPI_Stub, 0x4B708B54}, {NvAPI_Stub, 0xF2DD3F2},
	{NvAPI_Stub, 0x6800F5FC}, {NvAPI_Stub, 0xC7985ED5}, {NvAPI_Stub, 0x80C9FD3B}, {NvAPI_Stub, 0x6D6FDAD4},
	{NvAPI_Stub, 0x754033F0}, {NvAPI_Stub, 0xCDE4A28A}, {NvAPI_Stub, 0x2AB714AB}, {NvAPI_Stub, 0xDBC803EC},
	{NvAPI_Stub, 0x12EE68F2}, {NvAPI_Stub, 0x18CDF365}, {NvAPI_Stub, 0xD00B8317}, {NvAPI_Stub, 0x3D7A86BB},
	{NvAPI_Stub, 0x22CAD61}, {NvAPI_Stub, 0xB380F218}, {NvAPI_Stub, 0xFC5A155B}, {NvAPI_Stub, 0x72B19155},
	{NvAPI_Stub, 0xD5E13573}, {NvAPI_Stub, 0x13C7112E}, {NvAPI_Stub, 0xF7029C5}, {NvAPI_Stub, 0x84C9D553},
	{NvAPI_Stub, 0xB476BF61}, {NvAPI_Stub, 0x89FFD9A3}, {NvAPI_Stub, 0x3111BED1}, {NvAPI_Stub, 0x5CF7F862},
	{NvAPI_Stub, 0x2BBDA32E}, {NvAPI_Stub, 0x3D596B93}, {NvAPI_Stub, 0x7C20C5BE}, {NvAPI_Stub, 0x905F5C27},
	{NvAPI_Stub, 0x6317345C}, {NvAPI_Stub, 0xC182027E}, {NvAPI_Stub, 0xA4527BF8}, {NvAPI_Stub, 0xBD4BC56F},
	{NvAPI_Stub, 0x694BFF4D}, {NvAPI_Stub, 0xBFDC062C}, {NvAPI_Stub, 0xDDF5643C}, {NvAPI_Stub, 0x9D2509EF},
	{NvAPI_Stub, 0xAEAECD41}, {NvAPI_Stub, 0xA4AD0870}, {NvAPI_Stub, 0x64D6C83D}, {NvAPI_Stub, 0xF0F4A5E0},
	{NvAPI_Stub, 0x8AAC133D}, {NvAPI_Stub, 0x73EB9329}, {NvAPI_Stub, 0xAE9C2019}, {NvAPI_Stub, 0xCAC3CE5D},
	{NvAPI_Stub, 0x4EADF5D2}, {NvAPI_Stub, 0x2DE11D61}, {NvAPI_Stub, 0x5B803DAF}, {NvAPI_Stub, 0x102A1456},
	{NvAPI_Stub, 0xA5BB8295}, {NvAPI_Stub, 0x8547410}, {NvAPI_Stub, 0x5F7C0788}, {NvAPI_Stub, 0x3595E4C7},
	{NvAPI_Stub, 0x3119F36E}, {NvAPI_Stub, 0x919B3136}, {NvAPI_Stub, 0x7562E947}, {NvAPI_Stub, 0x988AEA78},
	{NvAPI_Stub, 0x9D772BBA}, {NvAPI_Stub, 0xF5579360}, {NvAPI_Stub, 0xE24CEEE}, {NvAPI_Stub, 0x633252D8},
	{NvAPI_Stub, 0xE0B1DCE9}, {NvAPI_Stub, 0x5D857A00}, {NvAPI_Stub, 0xD8CBF37B}, {NvAPI_Stub, 0x63E2F56F},
	{NvAPI_Stub, 0xBE17923}, {NvAPI_Stub, 0x2EB3C140}, {NvAPI_Stub, 0x86F05D7A}, {NvAPI_Stub, 0x329D77CD},
	{NvAPI_Stub, 0x4A35DF54}, {NvAPI_Stub, 0xEB7AF173}, {NvAPI_Stub, 0x4E2F76A8}, {NvAPI_Stub, 0xAE5FBCFE},
	{NvAPI_Stub, 0xB6D62591}, {NvAPI_Stub, 0xE76ADA52}, {NvAPI_Stub, 0x7D554F8E}, {NvAPI_Stub, 0x1730BFC9},
	{NvAPI_Stub, 0x680DE09}, {NvAPI_Stub, 0xA3EA6F1D}, {NvAPI_Stub, 0xCF8CAF39}, {NvAPI_Stub, 0x96043CC7},
	{NvAPI_Stub, 0x62FB1592}, {NvAPI_Stub, 0xB7476D15}, {NvAPI_Stub, 0xBAAABFCC}, {NvAPI_Stub, 0xE3E89B6F},
	{NvAPI_Stub, 0x37D32E69}, {NvAPI_Stub, 0xE83D6456}, {NvAPI_Stub, 0x40A505E4}, {NvAPI_Stub, 0xD2277E3A},
	{NvAPI_Stub, 0x36CFF969}, {NvAPI_Stub, 0x34C9C2D4}, {NvAPI_Stub, 0x4ECA2C10}, {NvAPI_Stub, 0xD988F0F3},
	{NvAPI_Stub, 0xCEEE8E9F}, {NvAPI_Stub, 0x2DDFB66E}, {NvAPI_Stub, 0xC33BAEB1}, {NvAPI_Stub, 0x1BB18724},
	{NvAPI_Stub, 0x1BE0B8E5}, {NvAPI_Stub, 0x2A0A350F}, {NvAPI_Stub, 0xE4715417}, {NvAPI_Stub, 0xACC3DA0A},
	{NvAPI_Stub, 0x2D43FB31}, {NvAPI_Stub, 0xFC13EE11}, {NvAPI_Stub, 0xB7AB19B9}, {NvAPI_Stub, 0xA561FD7D},
	{NvAPI_Stub, 0x6E042794}, {NvAPI_Stub, 0xC74925A0}, {NvAPI_Stub, 0xD048C3B1}, {NvAPI_Stub, 0x3F28E1B9},
	{NvAPI_Stub, 0x3BD32008}, {NvAPI_Stub, 0xE3795199}, {NvAPI_Stub, 0x8456FF3D}, {NvAPI_Stub, 0x521566BB},
	{NvAPI_Stub, 0x524B9773}, {NvAPI_Stub, 0x387B2E41}, {NvAPI_Stub, 0xED9E8057}, {NvAPI_Stub, 0x46FBEB03},
	{NvAPI_Stub, 0x5A04B644}, {NvAPI_Stub, 0xE4B701E3}, {NvAPI_Stub, 0xE332FA47}, {NvAPI_Stub, 0xDEE047AB},
	{NvAPI_Stub, 0xD4F3944C}, {NvAPI_Stub, 0x14B83A5F}, {NvAPI_Stub, 0x57F7CAAC}, {NvAPI_Stub, 0x51CCDB2A},
	{NvAPI_Stub, 0x7975C581}, {NvAPI_Stub, 0x17073A3C}, {NvAPI_Stub, 0xD8265D24}, {NvAPI_Stub, 0x25F17421},
	{NvAPI_Stub, 0x32E1D697}, {NvAPI_Stub, 0xF91E777B}, {NvAPI_Stub, 0x1BD69F49}, {NvAPI_Stub, 0x6F151055},
	{NvAPI_Stub, 0x7BC207F8}, {NvAPI_Stub, 0x5D7CCAEB}, {NvAPI_Stub, 0x1EA54A3B}, {NvAPI_Stub, 0x7BCF4AC},
	{NvAPI_Stub, 0xDA141340}, {NvAPI_Stub, 0x891FA0AE}, {NvAPI_Stub, 0x8F6ED0FB}, {NvAPI_Stub, 0x518A32C},
	{NvAPI_Stub, 0x987947CD}, {NvAPI_Stub, 0xD8C4FE63}, {NvAPI_Stub, 0xA69F8E29}, {NvAPI_Stub, 0x927DA4F6},
	{NvAPI_Stub, 0x189A1FDF}, {NvAPI_Stub, 0xE3640A56}, {NvAPI_Stub, 0x1B71D425}, {NvAPI_Stub, 0x6683EE65},
	{NvAPI_Stub, 0x4FCB326E}, {NvAPI_Stub, 0x30C805D5}, {NvAPI_Stub, 0x5757474A}, {NvAPI_Stub, 0xCA36A3AB},
	{NvAPI_Stub, 0x6DFD1C8C}, {NvAPI_Stub, 0xAED02700}, {NvAPI_Stub, 0x6BB1EE5D}, {NvAPI_Stub, 0x57D9060F},
	{NvAPI_Stub, 0x5DFAB48A}, {NvAPI_Stub, 0x79AD0307}, {NvAPI_Stub, 0x15658BE6}, {NvAPI_Stub, 0x25201F3D},
	{NvAPI_Stub, 0xA81F8992}, {NvAPI_Stub, 0x64F7D53C}, {NvAPI_Stub, 0x21E2CF70}, {NvAPI_Stub, 0x6A8937C6},
	{NvAPI_Stub, 0xB4B26B65}, {NvAPI_Stub, 0x4DB019E6}, {NvAPI_Stub, 0x8A50F126}, {NvAPI_Stub, 0xF409D5E5},
	{NvAPI_Stub, 0x813D89A8}, {NvAPI_Stub, 0x2FDE12C5}, {NvAPI_Stub, 0xE812EB07}, {NvAPI_Stub, 0x283AC65A},
	{NvAPI_Stub, 0x4D7B0709}, {NvAPI_Stub, 0x76BFA16B}, {NvAPI_Stub, 0x50016C78}, {NvAPI_Stub, 0xB8249127},
	{NvAPI_Stub, 0x2040AC5D}, {NvAPI_Stub, 0x1EAD124C}, {NvAPI_Stub, 0x5CAC215E}, {NvAPI_Stub, 0x18A35445},
	{NvAPI_Stub, 0x493584D6}, {NvAPI_Stub, 0x4306DEAC}, {NvAPI_Stub, 0x75DD3E6A}, {NvAPI_Stub, 0x25BFB10},
	{NvAPI_Stub, 0xE7B1198D}, {NvAPI_Stub, 0xFE0E5187}, {NvAPI_Stub, 0x77D8F573}, {NvAPI_Stub, 0x58337FA3},
	{NvAPI_Stub, 0xDC2BD4A6}, {NvAPI_Stub, 0xA91F88EB}, {NvAPI_Stub, 0xD9C0E326}, {NvAPI_Stub, 0xE5A518DC},
	{NvAPI_Stub, 0x38A18C40}, {NvAPI_Stub, 0x2FA7E0EB}, {NvAPI_Stub, 0x3F2C5B97}, {NvAPI_Stub, 0xF0B18453},
	{NvAPI_Stub, 0x578B6382}, {NvAPI_Stub, 0x8AE4B4CA}, {NvAPI_Stub, 0x3AB6A38A}, {NvAPI_Stub, 0xB7FA5440},
	{NvAPI_Stub, 0x6AE878A4}, {NvAPI_Stub, 0x8682711B}, {NvAPI_Stub, 0xA5B31EB8}, {NvAPI_Stub, 0x29FBC5EA},
	{NvAPI_Stub, 0x53DABBCA}, {NvAPI_Stub, 0xCDA14D8A}, {NvAPI_Stub, 0xCE8200E8}, {NvAPI_Stub, 0xCF00D48E},
	{NvAPI_Stub, 0x1003328A}, {NvAPI_Stub, 0x6579F1C8}, {NvAPI_Stub, 0xF8181529}, {NvAPI_Stub, 0xB6EF8B9},
	{NvAPI_Stub, 0x9321CA5B}, {NvAPI_Stub, 0x18919887}, {NvAPI_Stub, 0x1BB5779A}, {NvAPI_Stub, 0xCAE9A69},
	{NvAPI_Stub, 0x3805EF7A}, {NvAPI_Stub, 0x22ED9516}, {NvAPI_Stub, 0xB47A657E}, {NvAPI_Stub, 0xAE921F12},
	{NvAPI_Stub, 0x7EB95503}, {NvAPI_Stub, 0x271EBE10}, {NvAPI_Stub, 0x1B765642}, {NvAPI_Stub, 0xE04F3D86},
	{NvAPI_Stub, 0xDC27D5D4}, {NvAPI_Stub, 0xAFC4833E}, {NvAPI_Stub, 0x42892957}, {NvAPI_Stub, 0xBF6C1762},
	{NvAPI_Stub, 0x854BA405}, {NvAPI_Stub, 0xE7CB998D}, {NvAPI_Stub, 0xA9062C78}, {NvAPI_Stub, 0x61D7B624},
	{NvAPI_Stub, 0x4144111A}, {NvAPI_Stub, 0xF98854C8}, {NvAPI_Stub, 0x3EFADA1D}, {NvAPI_Stub, 0x8E985CCD},
	{NvAPI_Stub, 0xF36A668D}, {NvAPI_Stub, 0xF5F4D01}, {NvAPI_Stub, 0x957D7B6}, {NvAPI_Stub, 0xD6B99D89},
	{NvAPI_Stub, 0x6B89E68}, {NvAPI_Stub, 0xDBBC0AF4}, {NvAPI_Stub, 0x66FB7FC0}, {NvAPI_Stub, 0xB68F3440},
	{NvAPI_Stub, 0x45AEA29B}, {NvAPI_Stub, 0xB3BB0772}, {NvAPI_Stub, 0x4ECBEA37}, {NvAPI_Stub, 0x55A45AD2},
	{NvAPI_Stub, 0x8B8C90E4}, {NvAPI_Stub, 0xC0C4C680}, {NvAPI_Stub, 0xACCE03F4}, {NvAPI_Stub, 0x7DEF8998},
	{NvAPI_Stub, 0x29E0A109}, {NvAPI_Stub, 0x25ABDA26}, {NvAPI_Stub, 0xFB90BDC1}, {NvAPI_Stub, 0xFA66ED29},
	{NvAPI_Stub, 0x14FF532D}, {NvAPI_Stub, 0xA97EBEFA}, {NvAPI_Stub, 0x95256D18}, {NvAPI_Stub, 0x28A480CF},
	{NvAPI_Stub, 0x1C0F37AB}, {NvAPI_Stub, 0x130E96D7}, {NvAPI_Stub, 0x6A56E20}, {NvAPI_Stub, 0x5A96D0F4},
	{NvAPI_Stub, 0x84244524}, {NvAPI_Stub, 0xCB7B09D2}, {NvAPI_Stub, 0x977C45E6}, {NvAPI_Stub, 0xF74926F5},
	{NvAPI_Stub, 0x872B4463}, {NvAPI_Stub, 0xFB22D656}, {NvAPI_Stub, 0xE23B68C1}, {NvAPI_Stub, 0x106EC09A},
	{NvAPI_Stub, 0x2DF387AD}, {NvAPI_Stub, 0x5C909191}, {NvAPI_Stub, 0x952E408F}, {NvAPI_Stub, 0xE8DC7ACF},
	{NvAPI_Stub, 0xB94B341B}, {NvAPI_Stub, 0x1C5659CD}, {NvAPI_Stub, 0x54FE75A}, {NvAPI_Stub, 0x58934E9A},
	{NvAPI_Stub, 0x638CD19C}, {NvAPI_Stub, 0xC96C4920}, {NvAPI_Stub, 0x410B5C25}, {NvAPI_Stub, 0xF60852BD},
	{NvAPI_Stub, 0xD54B8989}, {NvAPI_Stub, 0x74073CC9}, {NvAPI_Stub, 0xBD669B00}, {NvAPI_Stub, 0xFF4E7D40},
	{NvAPI_Stub, 0x3C6346CC}, {NvAPI_Stub, 0xA2A9F4AD}, {NvAPI_Stub, 0x38913465}, {NvAPI_Stub, 0xBB9EF1C3},
	{NvAPI_Stub, 0x26C63DE3}, {NvAPI_Stub, 0xA3E36F0A}, {NvAPI_Stub, 0x50F44FEF}, {NvAPI_Stub, 0xBF739FFF},
	{NvAPI_Stub, 0xB0C0A5FD}, {NvAPI_Stub, 0x4AF0011D}, {NvAPI_Stub, 0x9962C97C}, {NvAPI_Stub, 0xFAB69565},
	{NvAPI_Stub, 0xF5E10439}, {NvAPI_Stub, 0xF3B11E68}, {NvAPI_Stub, 0xF089EEF5}, {NvAPI_Stub, 0x625D3B94},
	{NvAPI_Stub, 0xC5AD6E48}, {NvAPI_Stub, 0x6F83FB5B}, {NvAPI_Stub, 0x1D5B485B}, {NvAPI_Stub, 0xE44BD59D},
	{NvAPI_Stub, 0xC9116276}, {NvAPI_Stub, 0xD8BF768C}, {NvAPI_Stub, 0x680649D7}, {NvAPI_Stub, 0x25EEB2C4}, {}};

struct IUnknown;

// The entry points below jump through untyped table entries (nvapi_QueryInterface's void*, NVAPI SDK): each is cast to
// its function's type, as the SDK's own callers do.

// Not registered (0x9FEAD0): as NvAPI_EnumNvidiaDisplayHandle below, the exe's `jmp [slot]` hands the caller's ecx to
// the entry point unchanged; src cannot promise that, and the body check compares ecx at the call (it differs).
NvAPI_Status NvAPI_GetDisplayDriverVersion(NvDisplayHandle hNvDisplay, NV_DISPLAY_DRIVER_VERSION* pVersion)
{
	return reinterpret_cast<NvAPI_GetDisplayDriverVersion_t>(
		g_NvAPI_Interfaces[NvAPI_Index_GetDisplayDriverVersion].pEntry)(hNvDisplay, pVersion);
}

// Not registered (0x9FEAD6): the exe's `jmp [slot]` hands the caller's ecx to the entry point unchanged; src cannot
// promise that, and the body check compares ecx at the call (it differs).
NvAPI_Status NvAPI_EnumNvidiaDisplayHandle(uint32_t thisEnum, NvDisplayHandle* pNvDispHandle)
{
	return reinterpret_cast<NvAPI_EnumNvidiaDisplayHandle_t>(
		g_NvAPI_Interfaces[NvAPI_Index_EnumNvidiaDisplayHandle].pEntry)(thisEnum, pNvDispHandle);
}

// Not registered (0x9FEAE8): the `jmp [slot]` passes ecx through, as above.
NvAPI_Status NvAPI_EnumLogicalGPUs(NvLogicalGpuHandle* nvGPUHandle, uint32_t* pGpuCount)
{
	return reinterpret_cast<NvAPI_EnumLogicalGPUs_t>(g_NvAPI_Interfaces[NvAPI_Index_EnumLogicalGPUs].pEntry)(
		nvGPUHandle, pGpuCount);
}

// Not registered (0x9FEAE2): the `jmp [slot]` passes ecx through, as above.
NvAPI_Status NvAPI_EnumPhysicalGPUs(NvPhysicalGpuHandle* nvGPUHandle, uint32_t* pGpuCount)
{
	return reinterpret_cast<NvAPI_EnumPhysicalGPUs_t>(g_NvAPI_Interfaces[NvAPI_Index_EnumPhysicalGPUs].pEntry)(
		nvGPUHandle, pGpuCount);
}

// Not registered (0x9FEBD8): the `jmp [slot]` passes ecx through, as above.
NvAPI_Status NvAPI_D3D_GetCurrentSLIState(IUnknown* pDevice, NV_GET_CURRENT_SLI_STATE* pSliState)
{
	return reinterpret_cast<NvAPI_D3D_GetCurrentSLIState_t>(
		g_NvAPI_Interfaces[NvAPI_Index_D3D_GetCurrentSLIState].pEntry)(pDevice, pSliState);
}

// Loads nvapi.dll (nvapi64.dll in a 64-bit process, as the NVAPI SDK's 64-bit nvapi.lib does: a 64-bit process cannot
// load the 32-bit DLL), initialises the driver's NVAPI and resolves every entry of the interface table. The exe saves
// and restores every register around the work (pushal/popal); its one caller, Renderer::Init, reads none after the
// call.
NvAPI_Status NvAPI_Initialize()
{
	if (g_NvAPI_Initialized) {
		return NVAPI_OK;
	}
	void* hModule = platform::LibraryLoad(XFNV_64BIT ? "nvapi64.dll" : "nvapi.dll");
	g_NvAPI_InitStatus = -2;
	if (hModule) {
		// GetProcAddress and nvapi_QueryInterface hand back untyped entry points; cast to the SDK's types.
		nvapi_QueryInterface_t pfnQueryInterface =
			reinterpret_cast<nvapi_QueryInterface_t>(platform::LibrarySymbol(hModule, "nvapi_QueryInterface"));
		NvAPI_Initialize_t pfnInitialize;
		if (pfnQueryInterface
			&& (pfnInitialize = reinterpret_cast<NvAPI_Initialize_t>(pfnQueryInterface(NvAPI_Id_Initialize)))
			&& pfnInitialize() >= 0) {
			g_NvAPI_InitStatus = 0;
			g_NvAPI_StubStatus = -3;
			for (NvAPI_InterfaceEntry* pEntry = g_NvAPI_Interfaces; pEntry->uiId; ++pEntry) {
				void* pResolved = pfnQueryInterface(pEntry->uiId);
				if (pResolved) {
					pEntry->pEntry = reinterpret_cast<NvAPI_Entry_t>(pResolved);
				}
			}
		} else {
			g_NvAPI_InitStatus = -1;
		}
	}
	NvAPI_Status eResult = static_cast<NvAPI_Status>(g_NvAPI_InitStatus);
	if (!eResult) {
		g_NvAPI_Initialized = 1;
	}
	return eResult;
}

// Not registered (0x9FEC80): the `jmp [slot]` hands the entry point whatever arguments its caller pushed (the exe has
// no caller to show them; interface id 0x7C20C5BE) and the caller's ecx, which src cannot forward.
NvAPI_Status NvAPI_D3D9_AliasPrimaryFromDevice()
{
	return reinterpret_cast<NvAPI_Status(*)()>(g_NvAPI_Interfaces[NvAPI_Index_D3D9_AliasPrimaryFromDevice].pEntry)();
}

int32_t NvAPI_Stub() {
	return g_NvAPI_StubStatus;
}
