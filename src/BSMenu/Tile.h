#pragma once

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSStringT.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTStringMap.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class Menu;
class TileMenu;
class TileShaderProperty;

// A menu file's XML text, cached by Menu::GetXML.
struct XMLStorage {
	~XMLStorage();

	static void operator delete(void* apMemory);

	uint32_t iFileSize;
	char* pXMLData;
};

// A node of the menu tree: a named tile with traits, a parent, children and a Gamebryo model.
class Tile {
public:
	struct Action;
	struct FloatAction;
	struct RefValueAction;
	struct Reaction;

	// How a tile's fade animation runs (values from the Xbox PDB).
	enum TILE_FADE_CONTROL_TYPE : int32_t {
		TFCT_STANDARD = 0,
		TFCT_NONLINEAR_LONG_DARK_REPEATING = 1,
		TFCT_BLINK_THRICE = 2,
		TFCT_BLINK_FAST_FADE = 3,
		TFCT_FADE_IN_HOLD_FADE_OUT = 4,
	};

	// uiFlags bits. The low ten bits mark pending trait changes, 0x400 puts the tile in the dirty list, 0x800 in
	// the hibernating list. DIRTY..TRI_UPDATE_MANUAL follow the PDB's IsDirty..IsTriUpdateManual order; the exe adds
	// 0x10000 (no PDB accessor) before the model-loader bit, which ~Tile3D tests to give the model back.
	enum Flags : uint32_t {
		CHANGE_POSITION = 0x1,
		CHANGE_CREATE = 0x2,
		CHANGE_VISIBILITY = 0x4,
		CHANGE_COLOR = 0x8,
		CHANGE_GEOMETRY = 0x10,
		CHANGE_TEXTURE = 0x20,
		CHANGE_NIF_FILE = 0x40,
		CHANGE_SCISSOR_WINDOW = 0x80,
		CHANGE_SCISSOR = 0x100,
		CHANGE_LOCUS = 0x200,
		CHANGE_MASK = 0x3FF,
		DIRTY = 0x400,
		HIBERNATED = 0x800,
		PROMOTED = 0x1000,
		RELEASED = 0x2000,
		MENU_DELETING = 0x4000,
		TRI_UPDATE_MANUAL = 0x8000,
		// ConnectTraitsToTree asks for it (through AddNeedsUpdate, which refuses it) while it applies a tile's
		// traits, and Value::CalculateValue skips every trait but class while it is set.
		CONNECTING_TRAITS = 0x10000,
		MODEL_FROM_MODEL_LOADER = 0x20000,
	};

	// Trait ids, tile type ids and XML operator codes (the PDB has no names for them; these are the menu XML's).
	enum eTileValue : int32_t {
		kTileValue_Rect = 0x385,
		kTileValue_Image = 0x386,
		kTileValue_Text = 0x387,
		kTileValue_Nif = 0x388,
		kTileValue_Menu = 0x389,
		kTileValue_Hotrect = 0x38A,
		kTileValue_Window = 0x38B,
		kTileValue_Radial = 0x38C,
		kTileValue_Template = 0x3E7,
		kTileValue_Copy = 0x7D0,
		kTileValue_Add = 0x7D1,
		kTileValue_Sub = 0x7D2,
		kTileValue_Mul = 0x7D3,
		kTileValue_Div = 0x7D4,
		kTileValue_Min = 0x7D5,
		kTileValue_Max = 0x7D6,
		kTileValue_Mod = 0x7D7,
		kTileValue_Floor = 0x7D8,
		kTileValue_Ceil = 0x7D9,
		kTileValue_Abs = 0x7DA,
		kTileValue_Round = 0x7DB,
		kTileValue_Gt = 0x7DC,
		kTileValue_Gte = 0x7DD,
		kTileValue_Eq = 0x7DE,
		kTileValue_Neq = 0x7DF,
		kTileValue_Lt = 0x7E0,
		kTileValue_Lte = 0x7E1,
		kTileValue_And = 0x7E2,
		kTileValue_Or = 0x7E3,
		kTileValue_Not = 0x7E4,
		kTileValue_OnlyIf = 0x7E5,
		kTileValue_OnlyIfNot = 0x7E6,
		kTileValue_Ref = 0x7E7,
		// Internal operators with no XML name: open and close a nested action group (ConnectTraitsToTree).
		kTileValue_ActionBegin = 0x7E8,
		kTileValue_ActionEnd = 0x7E9,
		kTileValue_Value = 0xBB9,
		kTileValue_Name = 0xBBA,
		kTileValue_Src = 0xBBB,
		kTileValue_Trait = 0xBBC,
		kTileValue_X = 0xFA1,
		kTileValue_Y = 0xFA2,
		kTileValue_Visible = 0xFA3,
		kTileValue_Class = 0xFA4,
		kTileValue_ClipWindow = 0xFA6,
		kTileValue_StackingType = 0xFA7,
		kTileValue_Locus = 0xFA8,
		kTileValue_Alpha = 0xFA9,
		kTileValue_ID = 0xFAA,
		kTileValue_DisableFade = 0xFAB,
		kTileValue_ListIndex = 0xFAC,
		kTileValue_Depth = 0xFAD,
		kTileValue_Clips = 0xFAE,
		kTileValue_Target = 0xFAF,
		kTileValue_Height = 0xFB0,
		kTileValue_Width = 0xFB1,
		kTileValue_Red = 0xFB2,
		kTileValue_Green = 0xFB3,
		kTileValue_Blue = 0xFB4,
		kTileValue_Tile = 0xFB5,
		kTileValue_ChildCount = 0xFB6,
		kTileValue_Child_Count = 0xFB6,
		kTileValue_Justify = 0xFB7,
		kTileValue_Zoom = 0xFB8,
		kTileValue_Font = 0xFB9,
		kTileValue_WrapWidth = 0xFBA,
		kTileValue_WrapLimit = 0xFBB,
		kTileValue_WrapLines = 0xFBC,
		kTileValue_PageNum = 0xFBD,
		kTileValue_IsHTML = 0xFBE,
		kTileValue_CropOffsetY = 0xFBF,
		kTileValue_CropY = 0xFBF,
		kTileValue_CropOffsetX = 0xFC0,
		kTileValue_CropX = 0xFC0,
		kTileValue_MenuFade = 0xFC1,
		kTileValue_ExploreFade = 0xFC2,
		kTileValue_Mouseover = 0xFC3,
		kTileValue_String = 0xFC4,
		kTileValue_ShiftClicked = 0xFC5,
		kTileValue_Clicked = 0xFC7,
		kTileValue_ClickSound = 0xFCB,
		kTileValue_Filename = 0xFCC,
		kTileValue_FileWidth = 0xFCD,
		kTileValue_FileHeight = 0xFCE,
		kTileValue_RepeatVertical = 0xFCF,
		kTileValue_RepeatHorizontal = 0xFD0,
		kTileValue_Animation = 0xFD2,
		kTileValue_LineCount = 0xFD4,
		kTileValue_PageCount = 0xFD5,
		kTileValue_xdefault = 0xFD6,
		kTileValue_xUp = 0xFD7,
		kTileValue_xDown = 0xFD8,
		kTileValue_xLeft = 0xFD9,
		kTileValue_xRight = 0xFDA,
		kTileValue_xButtonA = 0xFDD,
		kTileValue_xButtonB = 0xFDE,
		kTileValue_xButtonX = 0xFDF,
		kTileValue_xButtonY = 0xFE0,
		kTileValue_xButtonLT = 0xFE1,
		kTileValue_xButtonRT = 0xFE2,
		kTileValue_xButtonLB = 0xFE3,
		kTileValue_xButtonRB = 0xFE4,
		kTileValue_xButtonStart = 0xFE7,
		kTileValue_MouseoverSound = 0xFE8,
		kTileValue_Draggable = 0xFE9,
		kTileValue_DragStartX = 0xFEA,
		kTileValue_DragStartY = 0xFEB,
		kTileValue_DragOffsetX = 0xFEC,
		kTileValue_DragOffsetY = 0xFED,
		kTileValue_DragDeltax = 0xFEE,
		kTileValue_DragDeltay = 0xFEF,
		kTileValue_DragX = 0xFF0,
		kTileValue_DragY = 0xFF1,
		kTileValue_Wheelable = 0xFF2,
		kTileValue_Wheelmoved = 0xFF3,
		kTileValue_SystemColor = 0xFF4,
		kTileValue_Brightness = 0xFF5,
		kTileValue_LineGap = 0xFF7,
		kTileValue_ResolutionConverter = 0xFF8,
		kTileValue_Texatlas = 0xFF9,
		kTileValue_RotateAngle = 0xFFA,
		kTileValue_RotateAxisX = 0xFFB,
		kTileValue_RotateAxisY = 0xFFC,
		kTileValue_User0 = 0x1004,
		kTileValue_User1 = 0x1005,
		kTileValue_User2 = 0x1006,
		kTileValue_User3 = 0x1007,
		kTileValue_User4 = 0x1008,
		kTileValue_User5 = 0x1009,
		kTileValue_User6 = 0x100A,
		kTileValue_User7 = 0x100B,
		kTileValue_User8 = 0x100C,
		kTileValue_User9 = 0x100D,
		kTileValue_User10 = 0x100E,
		kTileValue_User11 = 0x100F,
		kTileValue_User12 = 0x1010,
		kTileValue_User13 = 0x1011,
		kTileValue_User14 = 0x1012,
		kTileValue_User15 = 0x1013,
		kTileValue_User16 = 0x1014,
		kTileValue_End = 0x1015,
		// The tile references of the menu XML (LoadTextTable's "parent", "me", ...).
		kTileValue_Parent = 0x1389,
		kTileValue_Me = 0x138A,
		kTileValue_Sibling = 0x138C,
		kTileValue_Child = 0x138D,
		kTileValue_Screen = 0x138E,
		kTileValue_Globals = 0x138F,
		kTileValue_IO = 0x1390,
		kTileValue_Grandparent = 0x1391,
		// The menu traits of Tile::enumTrait (PDB: eMenuLevel = 6001 ... eMenuVisible = 6009). The exe keeps the
		// class of the pip-boy menu shown in the menus' root's kTileValue_MenuLevel.
		kTileValue_MenuLevel = 0x1771,
		kTileValue_DeleteOnFade = 0x1772,
		kTileValue_MenuThickness = 0x1773,
		kTileValue_NoClickPastDone = 0x1776,
		kTileValue_MixedMenuDone = 0x1777,
		kTileValue_DoesNotStack = 0x1778,
		kTileValue_MenuVisible = 0x1779,
		// A tile's visibility while the menus are isolated:
		// Interface::IsolateElementsOfMenu sets it per glow/no-glow tile, RestoreMenuElements
		// sets it back to 1, and both cull a tile's model when it or kTileValue_Visible is 0.
		kTileValue_IsolatedVisible = 0x1779,
		// No trait: what the trait lookups return for a name that is no
		// trait, and the key Tile::GetOrCreateValue searches the value list with.
		kTileValue_Invalid = -0x7FFFFFFF - 1,
	};

	// A single trait value of this tile. Only the traits the menu XML asks for exist.
	struct Value {
		~Value();

		// Sets the string value; abClearActions also drops the actions.
		void SetString(const char* apString, bool abClearActions);
		// Recomputes the value from its action list; abForceUpdate reports it even when unchanged.
		void CalculateValue(bool abForceUpdate);
		// Appends an action with a constant operand (an operator kTileValue_* id and its value).
		void AddAction(int32_t aiActionType, float afActionVal);
		// Appends an action whose operand is another tile's trait.
		void AddAction(int32_t aiActionType, Tile* apSourceTile, int32_t aeSourceTrait);
		// fValue, through the one-field getter the exe shares with
		// BSFaceGenKeyframe::GetTime (thiscall, ST0, 0x488D50).
		float QFloat();
		void ClearActions();
		void SetFloat(float afValue, bool abClearActions);

		// The trait id (PDB: Tile::enumTrait, whose values are src's trait kTileValue_* ids: eX = 4001 =
		// kTileValue_X) or a user trait id from 10000 up.
		eTileValue eIndex;
		Tile* pParent;
		float fValue;
		char* strValue;
		Action* pActionListA;
	};

	// A trait fade in progress: one linear interpolation of a trait from a start to an end value.
	struct FadeControl {
		float fStartValue;
		float fEndValue;
		uint32_t uiStartTime;
		float fDurationMillis;
		eTileValue iTrait;
		Tile* pParent;
		TILE_FADE_CONTROL_TYPE eFadeType;
	};

	// One parsed XML command of a menu template: a tag opening or closing, a trait, or an action on a trait.
	struct TileTemplateItem {
		TileTemplateItem(int32_t aiCmd, float afVal, const char* axStr, int32_t aiTA, int32_t aiLine);

		// The command: 2/3 begin/end an action, 4/5 open/close a trait, 6/7 open/close a tile, 8 a trait value,
		// 9 a constant action operand, 10 an action operand taken from another tile.
		int32_t iCmd;
		float fVal;
		BSStringT<char> xStr;

		// The built tile for commands 6, the trait or operator id for the others.
		union {
			Tile* pTile;
			int32_t iTrait;
		} u;

		int32_t iLine;
	};

	struct BuildStorage;

	// A named list of template commands, built by ParseFile and replayed by BuildAndNameTree/ConnectTraitsToTree.
	struct TileTemplate {
		// Guessed bound: the database gives none; the next data item starts 0x2c bytes on, 11 four-byte pointers.
		// The exe stores and reloads TileTemplateItem pointers here.
		using pItemsA_t = TileTemplateItem* [11];

		TileTemplate(const char* apString, BuildStorage* apParent);
		~TileTemplate();

		// Appends a command; aiPreserveStrings keeps the string's spaces.
		void AddPair(int32_t aiCmd, const char* apStr, int32_t aiLine, bool aiPreserveStrings);
		// Empties the template's command list, freeing the items.
		void Clear();

		// A new command item (cdecl, 0xA0B950; aiTA is the trait or action id).
		static TileTemplateItem* GetItem(int32_t aiCmd, float afVal, const char* apStr, int32_t aiTA, int32_t aiLine);
		// Destroys and frees a command item.
		static void ThrowAway(TileTemplateItem* apItem);
		// Frees the pooled command items of pItemsA and empties the pool (cdecl, ret 0, 0xA0BA50; Menu's
		// LoadFinalMasterXML calls it after parsing the packed menus).
		static void ClearAll();

		NiFixedString xName;
		// The parse result that owns the template: both of the exe's callers (BuildStorage::Construct, 0xA0AC80,
		// and NewSubTemplate, 0xA0AE70) pass their BuildStorage.
		BuildStorage* pParent;
		NiTPointerListBase<TileTemplateItem*> xList;

		static inline int32_t iNumItems;
		static inline pItemsA_t pItemsA{};
	};

	// The parse result of a menu XML file: the main template, the sub-templates and the template being filled.
	struct BuildStorage {
		BuildStorage();
		~BuildStorage();

		// The template being filled, else the main one (inlined in the exe).
		TileTemplate* GetCurrentTemplate() { return this->pCurrentTemplate ? this->pCurrentTemplate : this->pTemplate; }

		// Deletes the main template, then makes apTemplate the main one.
		void SetTemplate(TileTemplate* apTemplate);

		// The sub-template named pcString, or null: matched on the interned handle first, else on the contents.
		TileTemplate* GetSubTemplateByName(const char* pName);
		// Creates the sub-template pName and adds it to xSubTemplates.
		TileTemplate* NewSubTemplate(const char* pName);

		TileTemplate* pTemplate;
		BSSimpleList<TileTemplate*> xSubTemplates;
		TileTemplate* pCurrentTemplate;
		bool bDeleteTemplates;
	};

	// An entry of a loaded texture-atlas file: the atlas file it came from, the subtexture's name, the atlas's texture
	// and the subtexture's rectangle in it (Tile::GetTextureAtlasInfo builds them; FreeTraitList destroys them).
	class TextureAtlasEntry {
	public:
		TextureAtlasEntry();
		~TextureAtlasEntry();

		BSStringT<char> strAtlasFilename;
		BSStringT<char> strSubtexture;
		BSStringT<char> strAtlasTexture;
		NiRect<float> SubRect;
	};

	// The class of the trait name table (the exe's xTextTable, 0x11F32F4): the string pointer map with
	// case-insensitive hashing and compares.
	class MenuStringMap;
	using TraitExtraDataMap = NiTPointerMap<int32_t, int32_t>;
	// Per trait value, the list of values whose actions read it (ValueChangeEvent recalculates them).
	using ValueReactionMap = NiTPointerMap<Value*, Reaction*>;
	// The extra data that ties a tile's Gamebryo node back to the tile.
	class Extra;

	Tile();
	virtual ~Tile();

	// Attaches the tile under apParent, copying apReplacedTile's traits when given.
	virtual void Init(Tile* apParent, const char* apName, Tile* apReplacedTile);
	// Builds this tile's Gamebryo node; null for a tile without one.
	virtual NiNode* MakeNode() { return nullptr; }
	// The tile type (a kTileValue_* id).
	virtual int32_t GetType() { return 0; }
	// The XML tag name of the tile type.
	virtual const char* GetTypeName();
	// True when the point (x, y) is inside the tile's own bounds test.
	virtual bool SpecialBoundsCheck(float, float) { return true; }
	// Reacts to a trait just parsed from the XML (trait, value, string); the tile to update, or null.
	virtual Tile* PostParse(int32_t, float, const char*) { return nullptr; }
	virtual void ForceTextureRelease();
	virtual TileShaderProperty* GetShaderProperty();
	// Sets the tile alpha and overlay colour on every geometry child of apNode.
	virtual void SetAlphaAndColor(NiNode* apNode, float afAlpha, const NiColorA& aColor);

	// Assigns the tile's name (the string is copied).
	void SetName(const char* apName);
	// Assigns a copy of arName's text as the tile's name.
	void SetName(const BSStringT<char>& arName);
	// The tile's name: xName's text, read through the one-field getter the exe shares with
	// NiControllerSequence::GetTextKeys (thiscall, ret 0, 0x700300).
	const char* GetName();
	// The Gamebryo model of this tile, or null.
	NiNode* GetModel();
	// True once Release has taken the tile out of the tree.
	bool IsReleased();
	// Clears the "an Ni update is pending" mark.
	void ClearNeedsNiUpdate();
	// Flags the tile's traits as changed; a change flags the tile dirty.
	void AddNeedsUpdate(int32_t aiFlag);
	// Flags the tile's colour as changed when its system colour (or, without one, afInheritedVal) is aiColorID, then
	// does the same for every descendant.
	void RecursiveColorUpdate(int32_t aiColorID, float afInheritedVal);
	// The same, for a list of colour ids.
	void RecursiveColorUpdate(BSSimpleList<int32_t>* pColorIDList, float afInheritedVal);
	// Clears the aiFlag trait-change bits when any of them is set.
	void ClearNeedsUpdate(int32_t aiFlag);
	// Rebuilds every descendant's Gamebryo geometry.
	void UpdateChildren(bool abParentWillUpdate);
	// The menu fade state of the root menu above this tile.
	bool GetRenderedMenu();
	// The nearest ancestor that owns a model, or null.
	NiNode* GetParentModel();
	// True while no model in the chain from this tile upward is culled.
	bool IsVisible();
	// The menu that owns this tile.
	Menu* GetMenu();
	// The parent tile (pParent).
	Tile* GetParent();
	// The root tile of the menu that owns this tile.
	Tile* GetMenuTile();
	// bNeedsNiUpdate, through the one-byte getter the exe shares with
	// QueuedFileLoad::GetFileIndex (thiscall, 0x712380).
	bool GetNeedsNiUpdate();
	// The first descendant (depth first) with the given name, or null.
	Tile* GetChildByName(const char* apName);
	// The first descendant with the given id trait, or null.
	Tile* GetChildByID(int32_t aiID);
	// The nearest tile at or above this one whose aiTrait holds a reference action; aiType receives the referenced
	// trait id, and the tile the value belongs to is the result.
	Tile* GetFirstRefCopy(int32_t aiTrait, int32_t& aiType);
	// Releases every child tile and empties the child list.
	void DeleteChildren();
	// Rebuilds the tile's geometry for its pending changes; true when the Ni scene needs an update.
	bool UpdateTile(bool abClearUpdateFlags);
	// Creates the tiles of a template under this tile and names them; the first tile created, or null.
	Tile* BuildAndNameTree(TileTemplate* apTemplate);
	// Applies a template's traits and actions to the tiles BuildAndNameTree created.
	void ConnectTraitsToTree(TileTemplate* apTemplate);
	// Flags the changes a trait set during parsing implies; this tile, or null when the trait implies none.
	Tile* FinalPostParse(int32_t aiTrait, float afValue, const char* axString);
	// True when a fade control already exists for the given trait on this tile.
	bool HasFadeControl(int32_t aiTrait);
	// The value of the given trait, or null when the tile does not have it.
	Value* GetValue(int32_t aiTrait);
	// The value of the given trait, created when the tile does not have it yet.
	Value* GetOrCreateValue(int32_t aiTrait);
	// The value of the given trait, or null; takes the tile tree's lock.
	Value* GetValueQ(int32_t aiTrait);
	// The value of the trait named aiBaseTrait's name followed by aiID (TraitToText, "%s%d", TextToTrait), or null.
	Value* GetUnderscoreValue(int32_t aiBaseTrait, int32_t aiID);
	// The numeric value of the given trait, or zero.
	float GetFloat(int32_t aiTrait);
	// The string value of the given trait, or null.
	const char* GetString(int32_t aiTrait);
	// True when the given trait's numeric value is non-zero.
	bool IsTrue(int32_t aeTrait);
	// Sets a numeric trait and immediately reports it back as zero.
	void Poke(int32_t aiTrait, float afValue);
	// Sets a numeric trait from a signed integer value.
	void Set(int32_t aiTrait, int32_t aiValue);
	// Sets a numeric trait from an unsigned integer value (zero-extended to 64 bits before the conversion).
	void Set(int32_t aiTrait, uint32_t auiValue);
	// Sets a numeric trait and fires its actions.
	void Set(int32_t aiTrait, float afValue, bool abClearActions);
	// Sets a string trait and fires its actions.
	void Set(int32_t aiTrait, const char* apString, bool abClearActions);
	// Replaces the pending trait changes with the given ones (the flag bits above 0x3FF are refused).
	void SetNeedsUpdate(int32_t aiFlag);
	// The tile's x position in its menu: its own x plus the x of every ancestor that is a locus.
	float GetRealX();
	float GetRealY();
	// Flags the clip window of every descendant that has none of its own as changed.
	void UpdateClipwindows();
	// Marks the tile's triangles as updated by hand (TRI_UPDATE_MANUAL), or clears the mark.
	void SetManualTriUpdate(bool abOn);
	// The speech-challenge failure flag of speech-check tiles.
	void SetSpeechChallengeFailure(bool bValue);
	bool GetSpeechChallengeFailure();
	// Sets the scissor rectangle of every clipping child's geometry, clamped to the screen, through the subtree.
	void ReClipChildren(float afx, float afy, float afx2, float afy2);
	// Starts a fade of the trait from afStart to afEnd over afLength seconds (replacing any fade of the trait).
	void AddFadeControl(int32_t aiTrait, float afStart, float afEnd, float afLength,
		Tile::TILE_FADE_CONTROL_TYPE aeFadeType);
	// Drops this tile's fades of the trait (every trait for 0x80000000).
	void RemoveFadeControl(int32_t aiTrait);
	// False while a fade of the trait runs on this tile; looping fades (type 1) never finish.
	bool GetFadeFinished(int32_t aiTrait);
	// Moves the tile under apParent, after apReplacedTile when it is one of the children, else first.
	void SetParent(Tile* apParent, Tile* apReplacedTile);
	// Detaches the tile's model from its parent node and releases it.
	void DeleteModel();
	// Plays the sound the string trait names, when the tile's menu is fading in.
	void PlayTileSound(int32_t aiSoundType);
	// Sets the alpha of a 3D tile's model.
	void FadeIn3D(NiAVObject* apObject, float afPct);
	// Takes the tile out of the tree for deletion: drops its traits, model attachment and child tiles.
	void Release();
	// Flags this tile and every descendant MENU_DELETING; a released or already deleting tile stops the walk.
	void SetMenuDeleting();
	// Clears the tile pointer of the model's tile extra data.
	void SeverExtraData();
	// Builds the tile tree of a menu XML file under this tile; the root of the built tree, or null on an error.
	Tile* ReadFile(const char* apName);
	bool IsInGlowBranch();
	float GetRealDepth();
	float GetFadeEndFor(int32_t aiTrait);
	Tile::TILE_FADE_CONTROL_TYPE GetFadeType(int32_t aiTrait);
	void RecursiveRotationUpdate();

	// Orders trait values by their index; qsort-style.
	static int32_t ValueSortFunc(const void* apVal1, const void* apVal2);
	// True when the name is a user trait name: it starts with an underscore or with "&_".
	static bool IsValidUserTraitName(const char* apTraitName);
	// Reads the extra-data id out of a trait name ("user7_42" reads 42).
	static int32_t GetExtraDataFromTraitName(const char* apTraitName);
	// Deepest menu level in the whole menu tree.
	static float GetMaximumDepth();
	// The tile's depth plus the deepest of its children's thicknesses (added to its own depth where its trait 4008
	// is set).
	static double GetTileThickness(Tile* apTile);
	// Drops the per-frame count of updated tiles.
	static void ResetUpdateCount();
	// The critical section that guards the whole tile tree.
	static BSCriticalSection* GetTileUpdateAllCriticalSectionLock();
	// Creates a tile of the given type (a kTileValue_* tile type id), or null.
	static Tile* BuildTile(int32_t aiID);
	// The tile named apName, looked up relative to apTarget.
	static Tile* GetTileByName(Tile* apTarget, const char* apName);
	// Checks the hibernating tiles for ones that must wake.
	static void CheckHibernatingTiles();
	// Puts the tile in the hibernating list, unless it is hibernated already or the list's last tile.
	static void AddHibernatingTile(Tile* apTile);
	// Advances every fade one step and drops the finished ones.
	static void UpdateFadeControls();
	// The first tile named apName in apTarget's subtree, apTarget included.
	static Tile* SearchForTileByName(Tile* apTarget, const char* apName);
	// Wakes hibernating tiles while the frame's update budget allows.
	static void PromoteHibernators();
	// The first dirty tile, or null; abRemove takes it off the dirty list.
	static Tile* GetNextDirtyTile(bool abRemove);
	// Puts a tile on the dirty list and wakes it from hibernation.
	static void AddDirtyTile(Tile* apTile);
	// Updates every dirty tile; abDoNiUpdate also refreshes the tile models.
	static void UpdateAll(bool abDoNiUpdate);
	// Enters (leaves) the critical section that guards the whole tile tree.
	static void Lock();
	static void Unlock();
	// Maps a trait name to its id; a user trait name also records the next free user id and its extra data.
	static void AddToTable(const char* apText, int32_t aiIndex);
	// Fills the trait name table with every built-in name.
	static void LoadTextTable();
	// The id of a user trait name, adding it (with aiIndex, or the next free id for -1) when it is new;
	// 0x80000000 for a name that is no user trait name.
	static int32_t AddUserTrait(const char* apString, int32_t aiIndex);
	// The id of a trait name, or 0x80000000.
	static int32_t TextToTrait(const char* apText);
	// The id of the name before "(", copying the text between "(" and ")" to apParam.
	static int32_t TextToTraitAndParam(const char* apText, char* apParam);
	// The name of a trait id, or null.
	static const char* TraitToText(int32_t aiTrait);
	// Recalculates every value whose actions read apChangedVal.
	static void ValueChangeEvent(Value* apChangedVal);
	// The tile of a model node (or of its parent node), from its tile extra data; null when it has none.
	static Tile* GetTileFromNode(NiNode* apNode);
	// Parses a menu XML file (or apData's apSize bytes when given) into templates; null on an error.
	static BuildStorage* ParseFile(const char* apName, char* apData, uint32_t apSize);
	// Splices the files named by a menu XML's include directives into its text.
	static bool InsertIncludedFiles(XMLStorage* apFile);
	// The atlas texture path and sub-rectangle of a sub-texture listed in an atlas file.
	static void GetTextureAtlasInfo(const char* strAtlasFilename, const char* strSubtextureFilename,
		BSStringT<char>* strAtlasTextureFilename, NiRect<float>* ResultRect);
	static void FreeMaybePostUpdateList();
	// Empties the trait tables, the texture atlas entries and the fade controls, under the tile lock.
	static void FreeTraitList();
	static void operator delete(void* apMemory);
	static void AddReaction(Tile::Value* apRefVal, Tile::Value* apDependantVal);

	// The list of child tiles.
	NiTPointerList<Tile*> xChildren;
	// The tile's trait values, ordered by trait index.
	BSSimpleArray<Value*, 8> xTraits;
	// The tile's name.
	BSStringT<char> xName;
	Tile* pParent;
	NiPointer<NiNode> spModel;
	uint32_t uiFlags;
	bool bNeedsNiUpdate;
	bool bSpeechChallengeFailure;

	static inline int32_t iTilesUpdatedThisFrame;
	static BSSimpleList<FadeControl*> xFadeControls;
	// The entries the loaded texture-atlas files created; FreeTraitList drops them.
	static BSSimpleList<TextureAtlasEntry*> TextureEntryList;
	static BSCriticalSection TileUpdateAllCritSect;
	static NiTPointerListBase<Tile*> xDirtyTiles;
	static inline bool bNeedsCheckHibernate;
	// The tiles waiting for a visible model before they are updated.
	static NiTPointerListBase<Tile*> xHibernatingTiles;
	static inline int32_t iHibernatingTileCount;
	// The trait name table (it copies its keys) and the extra data ids of the user traits. The exe constructs a
	// MenuStringMap here (its initialiser stores that class's vtable, 0x1094D9C).
	static MenuStringMap xTextTable;
	static TraitExtraDataMap xTraitExtraData;
	static ValueReactionMap ValueReactionList;
	// The next free user trait id.
	static inline int32_t iTextTableUserNextID = 10000;
	// The parse result the templates being built read from (Menu::RenderTemplate swaps one in).
	static inline BuildStorage* pBuild;
};

void au_re_LeaveCriticalSection_1_0();
Tile* FindTileByID(Tile* th_s, int32_t a2);
// The exe calls it thiscall and its class is not known yet: the object comes first, then the unused edx.
void RemoveAllAudioDebugStrings(void* apThis, [[maybe_unused]] void* apEdx);

// One step of a trait value's action list: its action type and the next action; QFloat and QRefValue give its
// operand as a float or as another trait value (both pure in the exe).
struct Tile::Action {
	Action();

	virtual float QFloat() = 0;
	virtual Tile::Value* QRefValue() = 0;

	eTileValue eActionType;
	Action* pnext;
};

// An action whose operand is a constant float.
struct Tile::FloatAction : public Tile::Action {
	FloatAction();

	float QFloat() override;
	Tile::Value* QRefValue() override;

	float fValue;
};

// An action whose operand is another trait value.
struct Tile::RefValueAction : public Tile::Action {
	RefValueAction();

	// The referenced value's float, or a default when there is none (thiscall, ST0, 0xA0C020).
	float QFloat() override;
	Tile::Value* QRefValue() override;

	Value* pRefValue;
};

// One entry of a value's reaction list: a value that reads it, and the next entry.
struct Tile::Reaction {
	Value* preactionValue;
	Reaction* pnext;
};

// The extra data a tile adds to its node, so the node leads back to the tile.
class Tile::Extra : public NiExtraData {
public:
	Extra(Tile* apTile, NiNode* apNode);

	~Extra() override;

	Tile* pTile;
	NiNode* pNode;
};

// The trait name table's map: NiTStringPointerMap<int> with keys hashed and compared case-insensitively (tolower
// hash at 0xA0D1B0, stricmp at 0xA0D180). Its constructor (0xFB3250, inlined into xTextTable's initialiser) builds
// xTextTable with 0x101 buckets and m_bCopy set.
class Tile::MenuStringMap : public NiTStringPointerMap<int32_t> {
public:
	~MenuStringMap() override;

	uint32_t KeyToHashIndex(const char* apKey) const override;
	bool IsKeysEqual(const char* apKey1, const char* apKey2) const override;

	static void operator delete(void* apMemory);
};

inline constexpr fnvd::Vtable Tile_vftable{};
inline constexpr fnvd::Vtable Tile_Action_vftable{};
inline constexpr fnvd::Vtable Tile_FloatAction_vftable{};
inline constexpr fnvd::Vtable Tile_RefValueAction_vftable{};
inline constexpr fnvd::Vtable Tile_Extra_vftable{};
inline constexpr fnvd::Vtable Tile_MenuStringMap_vftable{};
