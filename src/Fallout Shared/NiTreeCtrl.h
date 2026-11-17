#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class NiNode;

int32_t TreeCtrlCallback(NiObject* apObj, int32_t iMessage, uint32_t uiParam1, uint32_t uiParam2);

// The SDK's tree item handle (commctrl.h's HTREEITEM is `struct _TREEITEM *`): opaque. The exe only passes handles
// around (NiTreeCtrl::AddObjectPalette, 0x4D8A40, and AddControllerSequence, 0x4D8B90, take one as their parent) and
// never dereferences one, and the PDB has no member for it.
struct _TREEITEM {};

// A Win32 tree view of a scene graph (a debug viewer), which is not shown: its window has no SDL form (NiTreeCtrl.cpp).
// IDA's class and field names; the PDB has only the source file (Fallout Shared\NiTreeCtrl.cpp).
class NiTreeCtrl {
public:
	// What the tree reports its notifications to: the item's object (the item's lParam, which CreateTreeRecursive
	// sets to the object), the notification, two values; cdecl, returns an int. TreeCtrlCallback (0x4D78C0), the one
	// the exe sets (0x5BA8F0, 0x5BAC90), takes the same.
	using CallbackFunction = int32_t(NiObject*, int32_t, uint32_t, uint32_t);

	// Builds the viewer of pObject named pName: the item map, the windows at the given position and size, the
	// image list, then the tree of pObject when there is one. IDA's parameter names.
	NiTreeCtrl(void* hInstance, void* hParentWnd, NiAVObject* pObject, const char* pName, int32_t iXPos, int32_t iYPos,
		int32_t iWidth, int32_t iHeight);
	~NiTreeCtrl();

	// IDA name; not in the PDB. The exe creates the tree and command windows and returns the tree window (HWND)
	// (thiscall, ret 0x10, 0x4D6340).
	void* CreateWindows(int32_t X, int32_t Y, int32_t nWidth, int32_t nHeight);
	// IDA name; not in the PDB. The exe loads the image list (thiscall, ret 0, 0x4D6490).
	void SetImageList();
	// IDA name; not in the PDB. The exe adds the tree items of apObject (thiscall, ret 8, 0x4D64C0).
	void CreateTree(NiAVObject* apObject, const char* apName);
	void DestroyWindows();
	// Expands a1's item and each of its ancestors, root first; its
	// closing scroll-and-select runs only when the parent walk ends on an item, which it never does.
	void BuildTree(NiNode* a1);
	int32_t DestroyImageList();
	int32_t DestroyTree();

	// Stores the notification callback.
	static void SetCallback(CallbackFunction* apCallback);

	// The module, parent window, tree window, tree view, commands window and image list (HINSTANCE, HWND and
	// HIMAGELIST handles).
	void* m_hInstance;
	void* m_hParentWnd;
	void* m_hTreeWnd;
	void* m_hTreeView;
	void* m_hCommands;
	void* m_hImageList;
	// The tree item (HTREEITEM) of each scene graph object shown. The exe's map is NiTMap<NiAVObject*, _TREEITEM*>
	// (its constructor 0x4D8FA0 writes that instance's vtable, 0x1020FE4).
	NiTMap<NiAVObject*, _TREEITEM*> TreeItemMap;
	NiAVObject* m_pObject;
	char* m_pName;

	// The notification callback set by SetCallback (stored without a check).
	static inline CallbackFunction* stpCallback;
};
