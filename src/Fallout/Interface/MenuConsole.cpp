#include "Fallout/Interface/MenuConsole.h"

#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSMenu/DebugText.h"
#include "BSMenu/Font.h"
#include "BSMenu/TileImage.h"
#include "BSMenu/TileMenu.h"
#include "BSShader/BSShaderProperty.h"
#include "BSSystem/BSFile.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/Date.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESScriptCompiler.h"
#include "Fallout Shared/TESScriptFunctions.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/BookMenu.h"
#include "Fallout/Interface/Menus/StartMenu.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SceneGraph.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/string.h"

#include <stdint.h>

// The game settings this file defines, in their initialization order.
SettingT<INISettingCollection> iConsoleHistorySize("iConsoleHistorySize:Menu", 50);
SettingT<INISettingCollection> iConsoleVisibleLines("iConsoleVisibleLines:Menu", 15);
SettingT<INISettingCollection> iConsoleFont("iConsoleFont:Menu", 2);
SettingT<INISettingCollection> iConsoleTextXPos("iConsoleTextXPos:Menu", 30);
SettingT<INISettingCollection> iConsoleTextYPos("iConsoleTextYPos:Menu", 940);
SettingT<INISettingCollection> iConsoleLineSpacing("iConsoleLineSpacing:Menu", 35);

MenuConsole::MenuConsole()
	: pConsoleCompiler(nullptr), xStrings(), xInputs(), iInputsPos(0), iLastString(0), cVisible(0), bIsActive(0),
	  bRecordingBatch(0), iRecordingLineCount(0) {
	int32_t iFont = iConsoleFont.Int() - 1;
	FontManager* pFontManager = FontManager::Instance();
	iConsoleLineSpacing = static_cast<int32_t>(pFontManager->GetFont(iFont)->GetBaseLine()) + 3;
	iConsoleXPos = iConsoleTextXPos.Int();
	iConsoleYPos = iConsoleTextYPos.Int();
	ourMemset(pOutputFileName, 0, 260u);
}

MenuConsole::~MenuConsole() {
	if (pConsoleCompiler) {
		delete static_cast<Date*>(pConsoleCompiler);
	}
}

void MenuConsole::Init() {
	ScriptCompiler* v1;
	void* v3 = ourMalloc(sizeof(ScriptCompiler));
	if (v3) {
		v1 = new (v3) ScriptCompiler();
	} else {
		v1 = nullptr;
	}
	pConsoleCompiler = v1;
}

void MenuConsole::ScrollBy(int32_t aiNumLines) {
	int32_t formFlags = (int32_t)(aiNumLines + iLastString);
	if (formFlags > ((int32_t)(&xStrings)->m_uiCount)) {
		formFlags = (int32_t)(&xStrings)->m_uiCount;
	}
	if (((int32_t)(formFlags - ((&::iConsoleVisibleLines)->UInt()))) <= 0) {
		formFlags = (int32_t)((&::iConsoleVisibleLines)->UInt());
	}
	if (formFlags > ((int32_t)(&xStrings)->m_uiCount)) {
		formFlags = (int32_t)(&xStrings)->m_uiCount;
	}
	iLastString = (uint32_t)formFlags;
}

void MenuConsole::RefreshLines() {
	DebugText* v1;
	BSStringT<char>* v2;
	DebugText* v3;
	char* v4;
	float a3;
	float a3a;
	float a4;
	float a4a;
	uint32_t uint;
	int32_t j;
	NiTListItem<BSStringT<char>>* a1;
	int32_t iLastString;
	int32_t i;
	a1 = (&xStrings)->NiTListBase<BSStringT<char>>::GetHeadPos();
	int32_t v14 = (int32_t)(iConsoleYPos - (((int32_t)iConsoleLineSpacing) * ((&::iConsoleVisibleLines)->UInt())));
	iLastString = (int32_t)this->iLastString;
	for (i = (int32_t)(iLastString - ((&::iConsoleVisibleLines)->UInt())); i < 0; ++i) {
		a4 = (float)(double)v14;
		a3 = (float)(double)iConsoleXPos;
		v1 = DebugText::Instance(1);
		v1->Print(nullptr, a3, a4, 1, (-1), (-0x1p+0f), 0, nullptr);
		v14 += ((int32_t)iConsoleLineSpacing);
	}
	for (j = 0; j < i; ++j) {
		a1 = (&xStrings)->NiTListBase<BSStringT<char>>::GetNextPos(a1);
	}
	while (i < iLastString) {
		uint = (&::iConsoleFont)->UInt();
		a4a = (float)(double)v14;
		a3a = (float)(double)iConsoleXPos;
		v2 = &(&xStrings)->NiTPointerListBase<BSStringT<char>>::Get(a1);
		v4 = v2->pString;
		v3 = DebugText::Instance(1);
		v3->Print(v4, a3a, a4a, 1, (-1), (-0x1p+0f), uint, nullptr);
		a1 = (&xStrings)->NiTListBase<BSStringT<char>>::GetNextPos(a1);
		++i;
		v14 += ((int32_t)iConsoleLineSpacing);
	}
}

bool MenuConsole::IsVisible() {
	// The state byte is sign-extended: zero or negative reads as closed.
	return static_cast<int8_t>(this->cVisible) > 0;
}

void MenuConsole::OnEnterMenuMode() {
	if (cVisible > 0) {
		int32_t v5 = iConsoleFont.Int();
		float a4 = static_cast<float>(iConsoleYPos);
		float a3 = static_cast<float>(iConsoleXPos);
		DebugText* v1 = DebugText::Instance(1);
		v1->Print("|", a3, a4, 1, -1, -1.0f, v5, 0);
		uint32_t FormFlags = xStrings.m_uiCount;
		ScrollBy(FormFlags);
		RefreshLines();
		Interface::SetCrosshairRef(0);
	}
}

bool MenuConsole::ToggleVisible() {
	BSShaderProperty::PreviousTransform::ToggleConsole();
	if (!cVisible) {
		SetVisible(1);
		return 1;
	}
	// Only state 1 may switch to state 2 (shift held, one menu entered, none on the second stack); every other
	// state closes the console.
	if (cVisible == 1 && InterfaceManager::Instance()->IsShiftDown() &&
		InterfaceManager::Instance()->GetEnterStack(0) == 3 && !InterfaceManager::Instance()->GetEnterStack(1)) {
		SetVisible(2);
	} else {
		SetVisible(0);
	}
	return 0;
}

// KEEP: uninitialised reads. With an empty console line (Source[0] == 0) the exe tests Source[ilen - 1], the byte
// before the buffer (ebp-0x81D: the top byte of ipos, not yet written); and mbsnbcpy (0x5CBD90) leaves the buffer
// unterminated past 2046 bytes, which the length scan and the key insert then read.
// The exe's cleanup-only C++ EH frame is left out.
bool MenuConsole::Idle(int32_t aiKey) {
	float fval;
	int32_t aiClass;
	int32_t isizey;
	int32_t isizex;
	float frange;
	NiFrustum xfrust;
	if (cVisible <= 0) {
		return 0;
	}
	InterfaceManager* v2 = InterfaceManager::Instance();
	if (!v2->IsInMenuMode()) {
		return 0;
	}
	if (aiKey) {
		char apBuf[2052];
		int32_t ipos;
		char Source[2052];
		int32_t ilen;
		float afY = static_cast<float>(iConsoleYPos);
		float a7 = static_cast<float>(iConsoleXPos);
		DebugText* v4 = DebugText::Instance(1);
		v4->GetDebugText_0(apBuf, a7, afY, 1);
		mbsnbcpy_(Source, apBuf, 2046u);
		for (ilen = 0; Source[ilen]; ++ilen) {
			;
		}
		if (Source[ilen - 1] == 124) {
			Source[--ilen] = 0;
		}
		for (ipos = 0; ipos < ilen && Source[ipos] != 124; ++ipos) {
			;
		}
		if (ipos == ilen) {
			Source[ipos] = 124;
			++ilen;
		}
		if (aiKey == 0x80000008u) {
			for (int32_t a = 0; a < ilen + 1; ++a) {
				if (Source[a] == 124) {
					Source[a] = Source[a + 1];
					Source[a + 1] = 124;
				}
			}
			for (int32_t a_2 = 0; a_2 < ilen + 1; ++a_2) {
				if (Source[a_2] == 10) {
					if (Source[a_2 - 1] == 45) {
						for (int32_t b = a_2 - 1; b < ilen + 1; ++b) {
							Source[b] = Source[b + 2];
						}
					} else {
						Source[a_2] = 32;
					}
				}
				if (Source[a_2] == 10) {
					Source[a_2] = 32;
				}
			}
			if (Source[0]) {
				g_bHasPlayerUsedConsole = 1;
				InterfaceManager* pim = InterfaceManager::Instance();
				uint32_t FormFlags = xStrings.m_uiCount;
				ScrollBy(FormFlags);
				PrintLine(Source);
				BSStringT<char> xstr(static_cast<const char*>(Source));
				xInputs.AddHead(xstr);
				iInputsPos = 0;
				int32_t afYa = iConsoleFont.Int();
				float a4 = static_cast<float>(iConsoleYPos);
				float a3 = static_cast<float>(iConsoleXPos);
				DebugText* v6 = DebugText::Instance(1);
				v6->Print("|", a3, a4, 1, -1, -1.0f, afYa, 0);
				if (_tcsnicmp(Source, "dof", 3u)) {
					if (_tcsnicmp(Source, "usz", 3u)) {
						if (_tcsicmp(Source, "clr") && _tcscmp(Source, "cls")) {
							if (_tcsicmp(Source, "tag")) {
								if (_tcsnicmp(Source, "reload strings", 14u)) {
									if (_tcsnicmp(Source, "reload HUDReticle", 17u)) {
										if (_tcsnicmp(Source, "reload HUDSafeZone", 18u)) {
											if (_tcsicmp(Source, "reload mainmenu")) {
												if (_tcsnicmp(Source, "reload ", 7u)) {
													if (_tcsnicmp(Source, "delete ", 7u)) {
														if (_tcsncmp(Source, "front", 5u)) {
															if (_tcsncmp(Source, "stack", 5u)) {
																if (_tcsncmp(Source, "visible", 7u)) {
																	if (_tcsncmp(Source, "exit menus", 10u)) {
																		if (_tcsncmp(Source,
																				"emergency texture release", 25u)) {
																			if (_tcsncmp(Source, "set trait", 9u)) {
																				if (_tcsncmp(Source, "depth", 5u)) {
																					if (_tcsncmp(Source, "new fonts",
																							9u)) {
																						if (_tcsncmp(Source,
																								"old fonts", 9u)) {
																							if (_tcsncmp(Source, "iso",
																									3u)) {
																								if (_tcsncmp(Source,
																										"restore menus",
																										13u)) {
																									if (_tcsncmp(Source,
																											"use "
																											"workbook",
																											12u)) {
																										if (_tcsncmp(
																												Source,
																												"dump "
																												"textur"
																												"es",
																												13u)) {
																											if (_tcsncmp(
																													Source,
																													"ba"
																													"t",
																													3u)) {
																												if (_tcsncmp(
																														Source,
																														"rbat",
																														3u)) {
																													if (bRecordingBatch) {
																														if (iRecordingLineCount >=
																															20) {
																															Interface::
																																PrintLine(
																																	"Recording full; command ignored. Use rbat to cancel or rbat {filename} to save.");
																														} else {
																															char* Destination = cRecording
																																[iRecordingLineCount++];
																															ourStrcpy(
																																Destination,
																																Source);
																															Interface::
																																PrintLine(
																																	"Recorded.");
																														}
																													}
																													for (
																														uint32_t
																															a_5 =
																																0;
																														a_5 <
																														std::strlen(
																															Source);
																														++a_5) {
																														if (Source
																																[a_5] ==
																															10) {
																															Source[a_5] =
																																32;
																														}
																													}
																													Script
																														pScript;
																													pScript
																														.SetTemporary();
																													pScript
																														.SetText(
																															Source);
																													InterfaceManager*
																														v20 = InterfaceManager::
																															Instance();
																													pScript
																														.CompileAndRun(
																															static_cast<
																																ScriptCompiler*>(
																																pConsoleCompiler),
																															SYSTEM_WINDOW_COMPILER,
																															v20->GetConsoleSelection());
																												} else {
																													if (bRecordingBatch) {
																														char v71
																															[132];
																														std::memset(
																															v71,
																															0,
																															128);
																														std::sscanf(
																															Source,
																															"rbat %s",
																															v71);
																														if (std::strlen(
																																v71)) {
																															if (iRecordingLineCount >
																																0) {
																																BSFile*
																																	pfile_5;
																																BSFile* v40 = static_cast<
																																	BSFile*>(ourMalloc(
																																	sizeof(
																																		BSFile)));
																																if (v40) {
																																	pfile_5 = ::new (
																																		v40) BSFile(v71,
																																		NiFile::
																																			WRITE_ONLY,
																																		0x1000u,
																																		1);
																																} else {
																																	pfile_5 =
																																		0;
																																}
																																if (pfile_5 &&
																																	pfile_5
																																		->Open(
																																			0,
																																			0)) {
																																	for (
																																		int32_t i =
																																			0;
																																		i <
																																		iRecordingLineCount;
																																		++i) {
																																		char apDest
																																			[104];
																																		BSsprintf(
																																			apDest,
																																			100,
																																			"%s\r\n",
																																			cRecording
																																				[i]);
																																		uint32_t v19 =
																																			BSstrlen(
																																				apDest);
																																		pfile_5
																																			->WriteHere(
																																				apDest,
																																				v19);
																																	}
																																	pfile_5
																																		->Close();
																																	Interface::PrintLine(
																																		"Batch file '%s' created.",
																																		v71);
																																} else {
																																	Interface::PrintLine(
																																		"The file '%s' could not be opened.",
																																		v71);
																																}
																																if (pfile_5) {
																																	delete pfile_5;
																																}
																															} else {
																																Interface::
																																	PrintLine(
																																		"No commands recorded; recording cancelled.");
																															}
																														} else {
																															Interface::
																																PrintLine(
																																	"No filename specified; recording cancelled.");
																														}
																														bRecordingBatch =
																															0;
																														iRecordingLineCount =
																															0;
																													} else {
																														Interface::
																															PrintLine(
																																"Batch Recording Started.");
																														Interface::
																															PrintLine(
																																"Use rbat {filename} to save batch file or rbat with no filename to cancel.");
																														bRecordingBatch =
																															1;
																														iRecordingLineCount =
																															0;
																														for (
																															int32_t j =
																																0;
																															j <
																															20;
																															++j) {
																															ourStrcpy(
																																cRecording
																																	[j],
																																"");
																														}
																													}
																												}
																											} else {
																												char Str
																													[132];
																												std::memset(
																													Str,
																													0,
																													128);
																												std::sscanf(
																													Source,
																													"ba"
																													"t "
																													"%"
																													"s",
																													Str);
																												if (std::strlen(
																														Str)) {
																													BSFile*
																														pfile;
																													char Dst
																														[264];
																													char Src
																														[264];
																													std::memset(
																														Src,
																														0,
																														260);
																													std::memset(
																														Dst,
																														0,
																														260);
																													BSstrcat(
																														Src,
																														260,
																														Str);
																													void* apMemory =
																														ourMalloc(sizeof(
																															BSFile));
																													if (apMemory) {
																														pfile = ::new (
																															apMemory)
																															BSFile(
																																Src,
																																NiFile::
																																	READ_ONLY,
																																0x4000u,
																																0);
																													} else {
																														pfile =
																															0;
																													}
																													BSFile* pfile_2 =
																														pfile;
																													uint32_t ilength =
																														BSstrlen(
																															Str);
																													if ((!pfile_2 ||
																															!pfile_2
																																->Open(
																																	0,
																																	0)) &&
																														(ilength <=
																																4 ||
																															BSstrnicmp(
																																&Str[ilength -
																																	4],
																																".txt",
																																4u))) {
																														BSFile*
																															pfile_4;
																														BSFile* v45 =
																															pfile_2;
																														BSFile* pfile_3 =
																															pfile_2;
																														if (pfile_2) {
																															delete pfile_3;
																														}
																														BSstrcpy(
																															Dst,
																															260,
																															Src);
																														BSstrcat(
																															Dst,
																															260,
																															".txt");
																														BSFile* v43 = static_cast<
																															BSFile*>(ourMalloc(
																															sizeof(
																																BSFile)));
																														if (v43) {
																															pfile_4 = ::new (
																																v43) BSFile(Dst,
																																NiFile::
																																	READ_ONLY,
																																0x4000u,
																																0);
																														} else {
																															pfile_4 =
																																0;
																														}
																														pfile_2 =
																															pfile_4;
																													}
																													if (pfile_2 &&
																														pfile_2
																															->Open(
																																0,
																																0)) {
																														char newText
																															[2600];
																														while (
																															pfile_2
																																->GetLine(
																																	newText,
																																	2600u,
																																	13)) {
																															for (
																																size_t k =
																																	0;
																																k <
																																std::strlen(
																																	newText);
																																++k) {
																																if (newText
																																		[k] ==
																																	10) {
																																	newText
																																		[k] =
																																			32;
																																}
																															}
																															if (std::strlen(
																																	newText) >
																																1) {
																																Interface::PrintLine(
																																	"> %s",
																																	newText);
																																Script
																																	xScript;
																																xScript
																																	.SetTemporary();
																																xScript
																																	.SetText(
																																		newText);
																																InterfaceManager*
																																	v18 = InterfaceManager::
																																		Instance();
																																xScript
																																	.CompileAndRun(
																																		static_cast<
																																			ScriptCompiler*>(
																																			pConsoleCompiler),
																																		SYSTEM_WINDOW_COMPILER,
																																		v18->GetConsoleSelection());
																															}
																														}
																													} else {
																														if (BSstrlen(
																																Dst)) {
																															Interface::PrintLine(
																																"Neither file '%s' or '%s' could be opened.",
																																Src,
																																Dst);
																														} else {
																															Interface::PrintLine(
																																"The file '%s' could not be opened.",
																																Src);
																														}
																													}
																													if (pfile_2) {
																														delete pfile_2;
																													}
																												} else {
																													Interface::
																														PrintLine(
																															"You must enter a filename.");
																												}
																											}
																										} else {
																											DumpTextures();
																										}
																									} else {
																										Tile* MenuByClass =
																											Menu::
																												GetMenuByClass(
																													1026);
																										TileMenu* ptile_2 =
																											dynamic_cast<
																												TileMenu*>(
																												MenuByClass);
																										if (ptile_2) {
																											BookMenu* pbook = dynamic_cast<
																												BookMenu*>(
																												ptile_2
																													->pMenu);
																											if (pbook) {
																												pbook
																													->UseWorkbook();
																											}
																										} else {
																											PrintLine(
																												"You "
																												"must "
																												"open "
																												"a "
																												"book "
																												"(or "
																												"scroll"
																												") "
																												"before"
																												" using"
																												" the "
																												"workbo"
																												"ok.");
																										}
																									}
																								} else {
																									Interface::
																										RestoreMenuElements();
																								}
																							} else {
																								bool bfirst =
																									Source[3] == 49;
																								bool bsecond =
																									Source[4] == 49;
																								Interface::
																									IsolateMenuElements(
																										bfirst,
																										bsecond);
																							}
																						} else {
																							FontManager* Singleton =
																								FontManager::Instance();
																							Singleton->SetNewFonts(0);
																						}
																					} else {
																						FontManager* v15 =
																							FontManager::Instance();
																						v15->SetNewFonts(1);
																					}
																				} else {
																					PrintLine("Max Depth: %i",
																						static_cast<int32_t>(Tile::
																								GetMaximumDepth()));
																					InterfaceManager* v14 =
																						InterfaceManager::Instance();
																					PrintLine("Cursor Depth: %i",
																						static_cast<int32_t>(v14
																								->pCursor
																								->GetFloat(4013)));
																				}
																			} else {
																				char ptarget[128];
																				char ptrait[128];
																				fval = 0.0f;
																				std::sscanf(Source,
																					"set trait %s %s %f", ptarget,
																					ptrait, &fval);
																				Tile* ptile =
																					Tile::GetTileByName(0, ptarget);
																				int32_t itrait =
																					Tile::TextToTrait(ptrait);
																				if (ptile && itrait > 0) {
																					PrintLine("Trait set.");
																					ptile->Set(itrait, fval, 1);
																				} else {
																					if (ptile) {
																						PrintLine("Unknown trait '%s'",
																							ptrait);
																					} else {
																						PrintLine("Can't find tile "
																								  "'%s'",
																							ptarget);
																					}
																				}
																			}
																		} else {
																			Interface::
																				SetFirstChanceTextureRelease_00703920();
																			PrintLine("First change release of hidden "
																					  "textures activated.");
																		}
																	} else {
																		for (int32_t top = pim->GetEnterStackTop(); top;
																			top = pim->GetEnterStackTop()) {
																			pim->PopFromEnterStack(top, 0);
																		}
																	}
																} else {
																	aiClass = 0;
																	std::sscanf(Source, "visible %i", &aiClass);
																	PrintLine("Is Visible: %i",
																		MenuManager::Instance(1)->IsMenuOpen(aiClass));
																}
															} else {
																for (int32_t a_4 = 0; pim->GetEnterStack(a_4++);
																	PrintLine("%i: %i", a_4, pim->GetEnterStack(a_4))) {
																	;
																}
															}
														} else {
															Menu* FrontmostMenu =
																MenuManager::Instance(1)->GetFrontmostMenu();
															PrintLine("Frontmost: %s", FrontmostMenu->pMenu->GetName());
														}
													} else {
														char v93[1028];
														std::sscanf(Source, "delete %s", v93);
														int32_t v11 = Tile::TextToTrait(v93);
														MenuManager::Instance(1)->DeleteMenu(v11);
													}
												} else {
													char apText[1028];
													Menu::FreeXMLArchive();
													std::sscanf(Source, "reload %s", apText);
													if (apText[0] != 38) {
														int32_t a_3;
														for (a_3 = 0; apText[a_3]; ++a_3) {
															;
														}
														apText[a_3 + 2] = 0;
														apText[a_3 + 1] = 59;
														while (a_3 > 0) {
															apText[a_3] = apText[a_3 - 1];
															--a_3;
														}
														apText[0] = 38;
													}
													int32_t v10 = Tile::TextToTrait(apText);
													MenuManager::Instance(1)->ReloadMenu(v10);
												}
											} else {
												Menu::FreeXMLArchive();
												int32_t ClassStatic = StartMenu::GetClassStatic();
												Tile* v49 = Menu::GetMenuByClass(ClassStatic);
												if (v49) {
													delete v49;
												}
												StartMenu::Create(0, 0);
											}
										} else {
											Menu::FreeXMLArchive();
											Interface::ToggleSafeZone(3);
										}
									} else {
										Menu::FreeXMLArchive();
									}
								} else {
									Tile* StringRoot = pim->pStringRoot;
									if (StringRoot) {
										delete StringRoot;
									}
									Menu::RemoveFromXMLArchive("Data\\Menus\\globals.xml", 0);
									Tile* pstring = pim->GetMenusRoot()->ReadFile("Data\\Menus\\globals.xml");
									pstring->SetParent(0, 0);
									pim->pStringRoot = pstring;
								}
							} else {
								TESObjectREFR* ppickRef = pim->GetConsoleSelection();
								if (ppickRef) {
									BSStringT<char> xMessage;
									int32_t afDie = ppickRef->iFormID;
									xMessage.SPrintF("\"%s\" (%08x)", ppickRef->GetFullName(), afDie);
									NiNode* v7 = ppickRef->Get3D();
									NiNode* pnode =
										static_cast<NiNode*>(NiObject::DynamicCastStatic(&NiNode::ms_RTTI, v7));
									if (!pnode) {
										NiNode* v8 = ppickRef->Get3D();
										pnode = static_cast<NiNode*>(NiObject::DynamicCastStatic(&NiNode::ms_RTTI,
											v8->m_pkParent));
									}
									DebugText::AddFloatingText(pnode, xMessage.pString, 0, -1.0f);
								}
							}
						} else {
							xStrings.RemoveAll();
							iLastString = 0;
							RefreshLines();
						}
					} else {
						isizex = 0;
						isizey = 0;
						std::sscanf(Source, "usz %i %i", &isizex, &isizey);
						Interface::UpdateScreenShape(isizex, isizey);
					}
				} else {
					frange = 0.0f;
					std::sscanf(Source, "dof %f", &frange);
					xfrust = pim->GetSceneGraph()->GetCamera()->GetViewFrustum();
					xfrust.m_fFar = frange;
					pim->GetSceneGraph()->GetCamera()->SetViewFrustum(xfrust);
				}
			}
		} else {
			int32_t a_8;
			switch (aiKey) {
			case (-2147483647 - 1): {
				if (ilen) {
					if (ipos) {
						--ilen;
						for (int32_t itpos = ipos; itpos < ilen; ++itpos) {
							Source[itpos] = Source[itpos + 1];
						}
						Source[ilen] = 0;
						Source[ipos - 1] = 124;
					}
				}
				break;
			}
			case -0x7FFFFFF9: {
				if (ipos < ilen) {
					--ilen;
					for (int32_t itpos_2 = ipos; itpos_2 < ilen; ++itpos_2) {
						Source[itpos_2] = Source[itpos_2 + 1];
					}
					Source[ilen] = 0;
					Source[ipos] = 124;
				}
				break;
			}
			case -0x7FFFFFF7: {
				ScrollBy(1 - iConsoleVisibleLines.Int());
				RefreshLines();
				break;
			}
			case -0x7FFFFFF6: {
				ScrollBy(iConsoleVisibleLines.Int() - 1);
				RefreshLines();
				break;
			}
			case -0x7FFFFFFD: {
				if (xInputs.m_uiCount) {
					++iInputsPos;
					uint32_t v21 = xInputs.m_uiCount;
					iInputsPos = BSMIN(iInputsPos, static_cast<int32_t>(v21 - 1));
					void* xelem = (void*)xInputs.m_pkHead;
					for (int32_t a_6 = 0; a_6 < iInputsPos; ++a_6) {
						xelem = (void*)xInputs.GetNextPos(static_cast<NiTListItem<BSStringT<char>>*>(xelem));
					}
					std::sprintf(Source, "%s",
						(&xInputs.Get(static_cast<NiTListItem<BSStringT<char>>*>(xelem)))->String());
				}
				break;
			}
			case -0x7FFFFFFC: {
				if (xInputs.m_uiCount) {
					char* v32;
					--iInputsPos;
					iInputsPos = BSMAX(iInputsPos, 0);
					BSStringT<char> xstr_2;
					void* xelem_2 = 0;
					if (iInputsPos) {
						xelem_2 = (void*)xInputs.m_pkHead;
						for (int32_t a_7 = 0; a_7 < iInputsPos; ++a_7) {
							xelem_2 = (void*)xInputs.GetNextPos(static_cast<NiTListItem<BSStringT<char>>*>(xelem_2));
						}
					}
					if (xelem_2) {
						xstr_2.Copy(xInputs.Get(static_cast<NiTListItem<BSStringT<char>>*>(xelem_2)));
					} else {
						xstr_2.operator=("");
					}
					if (xstr_2.GetLength()) {
						v32 = xstr_2.pString;
					} else {
						v32 = const_cast<char*>("");
					}
					std::sprintf(Source, "%s", v32);
				}
				break;
			}
			case -0x7FFFFFFF: {
				if (ilen) {
					if (ipos) {
						Source[ipos] = Source[ipos - 1];
						Source[ipos - 1] = 124;
					}
				}
				break;
			}
			case -0x7FFFFFFE: {
				if (ilen) {
					if (ipos + 1 < ilen) {
						Source[ipos] = Source[ipos + 1];
						Source[ipos + 1] = 124;
					}
				}
				break;
			}
			case -0x7FFFFFFB: {
				if (ilen) {
					if (ipos) {
						for (int32_t c = ipos; c > 0; --c) {
							Source[c] = Source[c - 1];
						}
						Source[0] = 124;
					}
				}
				break;
			}
			case -0x7FFFFFFA: {
				if (ilen) {
					if (ipos + 1 < ilen) {
						for (int32_t c_2 = ipos; c_2 < ilen - 1; ++c_2) {
							Source[c_2] = Source[c_2 + 1];
						}
						Source[ilen - 1] = 124;
					}
				}
				break;
			}
			default: {
				Source[ipos] = aiKey;
				for (int32_t itpos_3 = ++ilen; itpos_3 > ipos; --itpos_3) {
					Source[itpos_3 + 1] = Source[itpos_3];
				}
				Source[ilen + 1] = 0;
				Source[ipos + 1] = 124;
				break;
			}
			}
			bool bhas_bar = 0;
			for (a_8 = 0; Source[a_8]; ++a_8) {
				if (Source[a_8] == 124) {
					bhas_bar = 1;
					break;
				}
			}
			if (!bhas_bar) {
				Source[a_8] = 124;
				Source[a_8 + 1] = 0;
			}
			BSStringT<char> xstr_3(static_cast<const char*>(Source));
			if (xInputs.m_uiCount) {
				BSStringT<char> kRemoved = xInputs.RemoveHead();
				xInputs.AddHead(xstr_3);
			} else {
				xInputs.AddHead(xstr_3);
			}
			int32_t afYb = iConsoleFont.Int();
			float a4a = static_cast<float>(iConsoleYPos);
			float a3a = static_cast<float>(iConsoleXPos);
			DebugText* v22 = DebugText::Instance(1);
			v22->Print(Source, a3a, a4a, 1, -1, -1.0f, afYb, 0);
		}
	}
	return 1;
}

void MenuConsole::PrintLine(const char* apString, va_list xlist) {
	if (!Interface::IsInGameLoadingMenuOpen() || StartMenu::MainMenuIsActive()) {
		BSStringT<char> xstring;
		xstring.VSPrintF(apString, xlist);
		WriteToOutputFile(xstring.pString);
		int32_t iwidth = 1200;
		int32_t iheight = 0;
		int32_t v5 = iConsoleFont.Int() - 1;
		FontManager* Singleton = FontManager::Instance();
		int32_t inum_lines = Singleton->GetFont(v5)->LineBreakText(xstring, iwidth, iheight, 0, 0);
		if (inum_lines) {
			char v14[2052];
			for (uint32_t b = 0; b <= xstring.GetLength(); ++b) {
				v14[b] = xstring.pString[b];
			}
			char* pstring = v14;
			for (int32_t a = 0; a < inum_lines; ++a) {
				bool bat_end = iLastString == xStrings.m_uiCount;
				BSStringT<char> psave_me;
				psave_me.operator=(pstring);
				xStrings.AddTail(psave_me);
				if (bat_end) {
					iLastString = xStrings.m_uiCount;
				}
				while (1) {
					int32_t FormFlags = xStrings.m_uiCount;
					if (FormFlags <= iConsoleHistorySize.Int()) {
						break;
					}
					BSStringT<char> kRemoved = xStrings.RemoveHead();
					--iLastString;
				}
				if (a + 1 < inum_lines) {
					while (*pstring) {
						++pstring;
					}
					++pstring;
				}
			}
			if (iLastString == xStrings.m_uiCount) {
				RefreshLines();
			}
		} else {
		}
	}
}

// An unused empty string is built before the call and destroyed after it, as the game does.
void MenuConsole::PrintLine(const char* apString, ...) {
	BSStringT<char> kUnused;
	va_list xlist;
	va_start(xlist, apString);
	// The va_list overload: an exact match for xlist, where the variadic one takes it through the ellipsis.
	this->PrintLine(apString, xlist);
	va_end(xlist);
}

void MenuConsole::SetVisible(char acState) {
	char cold_state = cVisible;
	cVisible = acState;
	if (acState > 0 != cold_state > 0) {
		if (acState <= 0) {
			InitializationFlow();
			DebugText* v6 = DebugText::Instance(1);
			v6->DeleteAllDebugText();
			InterfaceManager* v7 = InterfaceManager::Instance();
			v7->ClearTargetREFR();
		} else {
			InitializationFlow();
			InterfaceManager* v2 = InterfaceManager::Instance();
			if (v2->IsInMenuMode()) {
				int32_t v12 = iConsoleFont.Int();
				float a4 = static_cast<float>(iConsoleYPos);
				float a3 = static_cast<float>(iConsoleXPos);
				DebugText* v3 = DebugText::Instance(1);
				v3->Print("|", a3, a4, 1, -1, -1.0f, v12, 0);
				uint32_t FormFlags = xStrings.m_uiCount;
				ScrollBy(FormFlags);
			} else {
				float a4a = static_cast<float>(iConsoleYPos);
				float a3a = static_cast<float>(iConsoleXPos);
				DebugText* v5 = DebugText::Instance(1);
				v5->Print("", a3a, a4a, 1, -1, -1.0f, 0, 0);
			}
			RefreshLines();
		}
	}
}

void MenuConsole::DumpTextures() {
	BSFile* pfile;
	NiSourceTexture* pLargestTex[200];
	ourMemset(pLargestTex, 0, sizeof(pLargestTex));
	for (NiTexture* pTex = NiTexture::GetListHead(); pTex; pTex = pTex->m_pkNext) {
		if (NiObject::IsKindOf_0(&NiSourceTexture::ms_RTTI, pTex)) {
			NiSourceTexture* pSTex = static_cast<NiSourceTexture*>(pTex);
			if (pTex->m_pkRendererData) {
				if (bShowMenuTextureUse.Bool() || !IsMenuTexture(pSTex)) {
					int32_t idx = -1;
					for (int32_t i = 0; i < 200 && idx == -1; ++i) {
						if (pLargestTex[i]) {
							NiTexture::RendererData* pRendData2 = pLargestTex[i]->m_pkRendererData;
						} else {
							idx = i;
						}
					}
					if (idx != -1) {
						for (int32_t i_2 = 199; i_2 > idx; --i_2) {
							pLargestTex[i_2] = pLargestTex[i_2 - 1];
						}
						pLargestTex[idx] = pSTex;
					}
				}
			}
		}
	}
	BSFile* apMemory = static_cast<BSFile*>(ourMalloc(sizeof(BSFile)));
	if (apMemory) {
		pfile = ::new (apMemory) BSFile("textures.txt", NiFile::WRITE_ONLY, 0x1000u, 1);
	} else {
		pfile = 0;
	}
	if (pfile && pfile->Open(0, 0)) {
		const char* v8;
		const char* v9;
		char apDest[1024];
		uint32_t ultotalMemory = 0;
		if (TES::pInstance->GetCurrentCell()) {
			const char* FullName;
			TESObjectCELL* CurrentCell = TES::pInstance->GetCurrentCell();
			if (CurrentCell->GetWorldSpace()) {
				TESObjectCELL* v2 = TES::pInstance->GetCurrentCell();
				FullName = v2->GetWorldSpace()->GetFullName();
			} else {
				FullName = "{unknown}";
			}
			v9 = FullName;
		} else {
			v9 = "{unknown}";
		}
		if (TES::pInstance->GetCurrentCell()) {
			v8 = TES::pInstance->GetCurrentCell()->GetFullName();
		} else {
			v8 = "{unknown}";
		}
		int32_t ProcessLevel = TES::pInstance->iCurrentGridY;
		BSsprintf(apDest, 1024,
			"Texture dump for cell: %s grid: (%d, %d) worldspace: %s \n---------------------------------------\n", v8,
			TES::pInstance->iCurrentGridX, ProcessLevel, v9);
		uint32_t v4 = BSstrlen(apDest);
		pfile->WriteHere(apDest, v4);
		for (int32_t j = 0; j < 200 && pLargestTex[j]; ++j) {
			const NiFixedString* v5 = &pLargestTex[j]->GetFilename();
			v5->GetHandleString();
			uint32_t v6 = BSstrlen(apDest);
			pfile->WriteHere(apDest, v6);
		}
		BSsprintf(apDest, 1024, "---------------------------------------\nTotal memory: %dkb", ultotalMemory);
		uint32_t v7 = BSstrlen(apDest);
		pfile->WriteHere(apDest, v7);
		pfile->Close();
		Interface::PrintLine("200 largest loaded textures dumped to the file textures.txt.");
	} else {
		Interface::PrintLine("The file textures.txt could not be opened.");
	}
}

char MenuConsole::IsMenuTexture(NiSourceTexture* apTexture) {
	bool bretval = 0;
	if (apTexture) {
		if (apTexture->GetFilename().GetHandleString()) {
			const char* HandleString = apTexture->GetFilename().GetHandleString();
			const char* Str = std::strchr(HandleString, 92);
			while (Str) {
				if (!_strnicmp(Str, "\\menus", 6u)) {
					bretval = 1;
					Str = 0;
				} else {
					Str = std::strchr(Str + 1, 92);
				}
			}
		}
	}
	return bretval;
}

void MenuConsole::SetOutputFilename(const char* apOutputFileName) {
	if (apOutputFileName) {
		BSstrcpy(pOutputFileName, 260, apOutputFileName);
	} else {
		pOutputFileName[0] = 0;
	}
}

void MenuConsole::WriteToOutputFile(const char* apOutText) {
	if (pOutputFileName[0]) {
		uint32_t ilen;
		if (apOutText) {
			ilen = std::strlen(apOutText);
		} else {
			ilen = 0;
		}
		if (ilen) {
			BSFile outfile(pOutputFileName, NiFile::WRITE_ONLY, 0x2800u, 1);
			if (outfile.BSFile::Open(0, 1)) {
				outfile.BSFile::Seek(0, NiFile::ms_iSeekEnd);
				outfile.BSFile::WriteF(apOutText, ilen);
				outfile.BSFile::WriteF("\r\n", 2u);
			}
			outfile.Close();
		}
	}
}

MenuConsole* MenuConsole::Instance(bool abCreate) {
	MenuConsole* v2;
	if (MenuConsole::pInstance || (!abCreate)) {
		return MenuConsole::pInstance;
	}
	void* v3 = ourMalloc(sizeof(MenuConsole));
	if (v3) {
		v2 = ::new (v3) MenuConsole;
	} else {
		v2 = nullptr;
	}
	MenuConsole::pInstance = v2;
	v2->Init();
	return MenuConsole::pInstance;
}

void MenuConsole::operator delete(void* apMemory) {
	ourFree(apMemory);
}

void MenuConsole::Free() {
	if (MenuConsole::pInstance) {
		delete MenuConsole::pInstance;
	}
	MenuConsole::pInstance = nullptr;
}
