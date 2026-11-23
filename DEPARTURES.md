# Departures from the exe

The deliberate ways this program differs from the original 32-bit `FalloutNV.exe`. Each one also has a comment at its
site; this file collects them. A new departure gets both: the site comment saying what differs and why, and an entry
here.

Not listed: code shape the compiler decides (inlining, evaluation order with the same result, register use), and
comments that only describe the exe's own layout.

## Platform

- **Window and messages.** WinMain creates an SDL3 window (icon 101, 320x240 at 0,0, Alt+F4 off through a hint); on
  Linux it starts hidden and `Renderer::Init` shows it at its final size. `WinProc_SDL` handles SDL events in place of
  WinProc's messages; the messages WinProc only answers have no counterpart. On Linux an HWND is the SDL window.
  Mouse positions are scaled from the window's size to the screen's, which differ once the window manager resizes the
  window or the display is scaled. `GetActiveWindow_SDL` is null off the main thread, as GetActiveWindow is for a thread
  without windows. `src/Fallout/Misc/Main.cpp`, `src/Fallout Shared/Renderer.cpp`.
- **Modal loops** (message boxes, movies) only pump SDL events; the events are handled when WinMain's loop runs again.
  `src/Fallout/Interface/Interface.cpp`, `src/BSMovie/MoviePlayer.cpp`.
- **Displays.** The monitor count and desktop size come from SDL, not GetSystemMetrics; a full-screen window is not
  re-fitted, as the device owns it. `Renderer.cpp`, `Main.cpp`.
- **Window features not reproduced.** A second instance ends through `platform::SingleInstance` (a lock file on POSIX)
  instead of bringing the first one's window forward; the cell test sends no SendInput mouse nudge; the NiTreeCtrl debug
  viewer window is never created. `Main.cpp`, `src/Fallout Shared/TES.cpp`, `src/Fallout Shared/NiTreeCtrl.cpp`.
- **Direct3D 9.** Linux loads dxvk-native (`D3D9.DLL` maps to `libdxvk_d3d9.so.0`); Windows uses the system d3d9. The
  SDK's d3d9 handles are pointer-sized, as dxvk-native has them. `src/platform/README`, `src/SDK/DirectX/d3d9.h`.
- **Input.** `src/platform/dinput_sdl` stands in for dinput8 and XInput on both platforms: keyboard and mouse from SDL
  (exclusive mode is SDL's relative mouse mode on the cooperative window, set on the main thread), no DirectInput
  joysticks, XInput as SDL's first gamepad (user 0 only). `Controls::SetMouseBackgroundStatus` called on another thread
  (a message box's) is applied by the main thread's next `UpdateValues`, so the device is not freed while the main
  thread reads it. `src/BSDevices/Controls.cpp`.
- **Audio** (always on Linux, `XFNV_SDL_AUDIO` on Windows). `src/platform/dsound` stands in for DirectSound, mmio and
  DirectShow on SDL3's audio: SetFX effects are recorded but have no effect, 3D sound uses its own two-speaker model,
  music decodes MP3 and WAV only, on its own thread, and mmio cannot write.
- **d3dx9** (always on Linux, `XFNV_D3DX` on Windows). `src/platform/d3dx9`: textures load from DDS only and a texture
  that needs resizing fails; shader compile and assemble return E_NOTIMPL; generated DXT levels come from its own
  encoder. Measured against the DLL in `src/platform/d3dx9/README.md`.
- **Bink.** `src/platform/bink` is a stub: BinkOpen returns null, so no movie plays.
- **Steam.** `src/platform/steam` stands in for the game's 32-bit `steam_api.dll`; Steam is never started
  (`XFNV_NO_STEAM`), so getters return null and no callback runs.
- **POSIX layer** (`src/platform/README`): paths match case-insensitively; FindFiles orders as NTFS does and has no 8.3
  names; user folders are XDG (`$XDG_DATA_HOME/FalloutNV/Documents`); the install folder is the executable's; the
  registry is `Registry.ini`; INI files are parsed to Wine's rules and edited in place; thread priority and ideal
  processor do nothing; message boxes are SDL's, and another thread's box is shown by the main thread (which runs no
  frames meanwhile) when it takes the box within half a second; there is no launcher, so the game writes
  FalloutPrefs.ini itself
  (`Renderer.cpp`); opening a `.exe` fails; a lost X11 display is a quit event; `_CxxThrowException` aborts.
- **Windows APIs in standard C++.** Interlocked operations, critical sections, TLS and time
  (`src/SDK/Windows/winbase.h`); kernel objects, where the requested stack size is ignored and Suspend takes effect at
  the thread's next wait (`src/BSCore/BSSystemObjects.h`); Havok's threads (`hkWin32Thread.cpp`) and DirectShow's
  handles (`src/SDK/DirectShow`). Files open through fopen, which also opens folders CreateFileA refuses
  (`src/BSSystem/Xenon/BSSystemFile_Xenon.cpp`). Sockets are POSIX's and WSAStartup is a stub
  (`src/SDK/Windows/winsock2.cpp`). The SEH-guarded cpuid probes run unguarded (`NiProcessor.cpp`).
- **C runtime.** On both platforms `rand` is a per-thread `minstd_rand` over [0, 32767] (`src/SDK/MSVC/stdlib.h`). On
  Linux (`src/SDK/MSVC`): fopen_s shares the file (VC9 used _SH_SECURE); printf prints two exponent digits, not three;
  `_stat64i32` gives the write time as the access and creation times; text-mode fgets maps CR LF to LF and stops at
  Ctrl-Z, as the Windows CRT does. FaceGen's file streams lose the share flag (`src/SDK/FaceGen/algs.cpp`).
- **Floating-point precision.** The exe's device thread runs the x87 at 24-bit precision (the device is created without
  `D3DCREATE_FPU_PRESERVE`); the program computes in SSE at the declared types, so values written as `double` keep 53
  bits where the exe rounded each step to 24 (e.g. `Calendar::Update`).

## Memory

- **Pools.** MemoryManager makes only the pools whose block size is a multiple of 16; other sizes go to the next larger
  pool or to the heap. Pools lie below 4 GB; a reservation above is released. `src/BSCore/MemoryManager.cpp`,
  `MemoryPool.cpp`.
- **Block heap.** 16-byte headers, 16-byte smallest blocks and 16-byte alignment, where the exe has 8, 8 and 4.
  `src/BSCore/AbstractHeap.h`.
- **ZeroOverheadHeap.** Blocks are 16-aligned with their size word just before them; the exe packs them back to back.
  `src/BSCore/ZeroOverheadHeap.cpp`.
- **No hand-aligned blocks.** Every MemoryManager block is 16-aligned, so the blocks the exe over-allocates and aligns
  by hand (a padding byte before the object) are plain allocations: Havok creation infos, materials and bones, bhkWorld,
  the character controller, the casters, the ragdoll classes, bhkMemoryAllocator's blocks. `src/BSCore/MemoryManager.h`.
- **Over-aligned new.** Global `operator new`/`delete` with `std::align_val_t` (for `alignas` above 16, such as
  NiCriticalSection) over-allocate from MemoryManager and keep the padding length in the byte before the block.
  `MemoryManager.cpp`.
- **Debug allocators.** `XFNV_SYSTEM_ALLOC` sends MemoryManager, ScrapHeap, the Havok memory systems and bhkWorld's
  stack straight to the C runtime; `XFNV_GUARD_ALLOC` adds a guard-page heap and a fault log (`src/xfnv/debugheap`).

## 64-bit layouts

- **Form IDs in pointer fields.** File records keep 4-byte form IDs where the native structure has a pointer:
  `TESFile::WidenFormIDRecords` spreads the 32-bit records over the native ones (about 45 calls in 30 files, e.g.
  `ExtraDataList.cpp`, `TESObjectWEAP.cpp`, `MagicItemObject.cpp`), and `NarrowFormIDRecords` packs them back for
  writing (`TESConditionItem.cpp`, `ExtraDataList.cpp`, `TESObjectWEAP.cpp` and the other saves of such records).
  Definitions in `src/Fallout Shared/TESFile.h`. Save loads widen the IDs read into pointer fields the same way
  (`BGSLoadGameBuffer.h`).
- **NiString's buffer** asks for its three size_t header words and the terminator on top of the length. The exe
  adds 13 bytes, its 12-byte header and the terminator, which on x64 would leave strings of 8 to 18 characters
  overflowing their buffer. `NiString.cpp`.
- **TESRegionDataSound** keeps the sound's form ID, not a `TESSound*` stored over it, and looks the sound up on use.
  `Region System/TESRegionDataSound.h`, `src/Fallout/Audio/FalloutAudio.cpp`.
- **Pointer-sized values.** Fields and keys holding addresses are `uintptr_t`: A* node keys (`Pathfinding/AStarNode.h`,
  `NavMeshPOVSearch.h`, `TeleportDoorSearch.cpp`), crosshair reference ids (`InterfaceManager.h`), Havok property
  values, which the exe sign-extends from 32 bits (`bhkCharacterController.cpp`, `bhkWorldObject.cpp`), the KD tree's id
  halves (32 bits each, the exe's 16: `hkKdTree.h`). LockFreeMap's mark clears bit 0 only (`LockFreeMap.h`). Package
  locations and targets test their object pointer itself, where the exe tests the union's 4-byte type member over it
  (`PackageLocation.cpp`, `PackageTarget.cpp`).
- **Havok sizes.** The SDK's 8-byte-pointer values: agent slots and sectors 160/960 (the exe's 128/512,
  `hkpAgentNnSector.h`), job queue entries 256 (128, `hkJobQueue.h`), MAX_NUM_ATOM_INFOS 80 (144, `hkpDynamicsJobs.h`),
  wider Jacobian schemas (`hkpJacobianSchema.h`, `hkpJacobian2dFrictionSchema.h`), 64-bit pair-filter keys
  (`hkpPairCollisionFilter.h`), the pointer map's 64-bit methods (`hkMap.h`), the block size the serialized data
  objects record, their sizeof (the exe stores 44 for hkDataArrayNative, `hkDataObjectImpl.h`). `removeDuplicates`
  hashes both handles into the map key, where the exe packs two 32-bit addresses, and compares the handles on every
  lookup; it keeps a matched entry instead of removing it (`hkpTypedBroadPhaseDispatcher.cpp`). Arrays whose element
  holds a pointer are indexed by element, not by the exe's byte stride (`hkThreadMemory.cpp`, `hkJobQueue.cpp`,
  `hkCpuJobThreadPool.cpp`, `hkStackTracer.cpp`, `hkDataObjectDict.cpp`).
- **Windows and SDK structures at their x64 size.** Critical sections are 0x28 bytes, the exe's 0x18
  (`BSCriticalSection.h`, `hkCriticalSection.h`); DSBUFFERDESC is 40 bytes and dwSize is its sizeof (`BSWin32Audio.cpp`,
  `src/SDK/DirectX/dsound.h`); WSAData is 0x198 bytes in the 64-bit member order (`src/SDK/Windows/winsock2.h`); d3d9's
  structures keep natural alignment, where the exe's headers pack to 4 (`src/SDK/DirectX/d3d9.h`); likewise
  DIDEVICEOBJECTDATA, IMAGEHLP_LINE64 and SOCKET.
- **Native layouts in written data.** `bhkSerializable::SaveBinary` writes the creation info in its 64-bit layout,
  which the serialized-form loaders (`bhkConvexVerticesShape`, `bhkConvexTransformShape` and 11 others) do not read;
  nothing in the program writes a NIF (`NiTreeCtrl.cpp`'s save is behind the viewer window). Saved contact points hold
  x64-sized agent entries (`hkpSaveContactPointsUtil.cpp`); the game saves none.
- **Fields read by name** where the exe reads a fixed offset or calls a folded getter whose offset differs at x64.
  `BSTextureData.cpp`, `bhkExtendedMeshShape.cpp`, `TES.cpp`, `TESLand.cpp`.

## Data and files

- **NIF Havok blocks.** Model files hold the 32-bit creation infos: each is read through a fixed-width `Serialized` form
  and the native structure filled member by member (`src/BSHavok/bhkSerialized.h`, e.g. `bhkRigidBodyCinfo.h`,
  `bhkMoppBvTreeShape.cpp`). A block of a bhk class outside NiStream's list is skipped by the size table and links to it
  read as null (`src/Gamebryo2.2/CoreLibs/NiMain/NiStream.cpp`).
- **BSA directories.** The archive's 16-byte directory records, which hold a 32-bit pointer slot, are spread into the
  native records and the slot dropped. `src/BSSystem/Archive.cpp`.
- **Save games.** CombatGroupCluster writes 0 for the space pointer's 32 bits. `CombatGroup.cpp`.
- **Short plugin chunks.** Locals that `GetChunkData` fills start at 0 or "", so an empty or short chunk gives defined
  values where the exe reads them uninitialised (about 40 loaders, e.g. `TESSound.cpp`, `TESObjectREFR.cpp`,
  `TESRace.cpp`, `ExtraDataList.cpp`, `BGSMessage.cpp`). Other short reads, where the exe reads particular stack
  contents, are marked KEEP at their sites (e.g. `NiRotData.cpp`, `NiPSysRotationModifier.cpp`,
  `SettingCollection.cpp`).

## Fixes

- **RouletteMenu** does not empty the bet list a second time after freeing it (the exe reads freed memory).
  `RouletteMenu.cpp`.
- **TESDataHandler's destructor** deletes the region data manager; the exe frees it without destroying it.
  `TESDataHandler.cpp`.
- **hkArray's copy constructor** copies exactly the source's elements. The exe sizes the copy by the count the
  allocator rounded the buffer up to and copies that many elements, reading past the end of the source. `hkArray.h`.
- **Uninitialised reads.** Where a path leaves a local unset and the exe reads it anyway, the local gets the value the
  code's other paths give it:
  - Game: WinMain's launcher path starts empty (`Main.cpp`); HighProcess::SaveGame saves its spare word as 0
    (`HighProcess.cpp`); the casino winnings commands start their casino null (`PlayerCharacter.cpp`); the auto-aim hit
    offset starts at 0 (`PlayerCharacter.cpp`); OutputMemStats names its dumps without a suffix when its parameters
    fail to parse, and the low-path script command's "to Goal" message is given its door name and form ID
    (`TESScriptFunctions.cpp`); the destructible-object save zeroes its record's padding
    (`BGSDestructibleObjectForm.cpp`); the save-copy path starts empty (`BGSSaveLoadManager.cpp`); the console tests
    the last character only of a non-empty line, and its line buffer starts zeroed (`MenuConsole.cpp`); the Caravan AI's
    setup package starts zeroed (`CaravanMenu.cpp`); the start node of a smoothed path has radius 0
    (`PathSmootherPOV.cpp`); the link-line walk stores true for each visited reference (`TESObjectREFR.cpp`); a
    package start location is saved with the extra's rotation (`ExtraDataList.cpp`); a gamepad click takes the plain
    click on its target (`InterfaceManager.cpp`); an escort package whose target is a type, or an object that is not
    a bound object, matches no reference, where the exe calls through an unset pointer (`Package.cpp`).
  - Engine: the last save time is zero when no file is found (`BSFile.cpp`); short file extensions hash with zero
    padding (`BSHash.cpp`); particles generated from a ray with a null first target start at the origin and at
    rest (`ParticleShaderProperty.cpp`); a weapon's parent chain that ends before the root starts from the identity
    (`WeaponObject.cpp`); Havok-to-Gamebryo transforms have scale 1 (`bhkCollisionObject.cpp`); a plane shape with no
    wrapped shape shows a zero plane (`bhkPlaneShape.cpp`); a D3D blend mode outside 1 to 11 maps to ALPHA_ONE
    (`ParticleShader.cpp`); an object entering water is recorded at the origin with no sub-shape key, so it takes the
    water shape's own material (`bhkWaterPhantomCallbackShape.cpp`); bhkCharacterController::InitShapes starts the
    half-height at 0 (`bhkCharacterController.cpp`).
  - Gamebryo: the shader declaration packs nothing for inputs its switch has no case for
    (`NiD3DShaderDeclaration.cpp`); shader constants shorter than a register are padded with zeros
    (`NiD3DShaderConstantMap.cpp`); a shader name with no extension is looked up whole
    (`NiD3DShaderProgramFactory.cpp`); a BMP palette's unused entries are zero (`NiBMPReader.cpp`); the Direct3D9Ex
    instance and device pointers start null (`NiDX9Renderer.cpp`); an OBB leaf intersection with no hit time has a zero first point
    (`NiOBBNode.cpp`).
  - Havok: the extended mesh's triangle vertices have w = 0 (`hkpExtendedMeshShape.cpp`); the penetration backup step's
    perturbation has w = 0 (`hkConvexPenetrationUtil.cpp`); the chain solvers' diagonal blocks start at 0
    (`hkpChainBuildJacobian.cpp`); OneBasisFun returns 0 when no loop writes the result (`hkaNURBS.cpp`); the foot
    placement solver's skipped foot normal is 0 (`hkaFootPlacementIkSolver.cpp`); the GSK agent's scratch cache starts
    zeroed (`hkpGskfAgent.cpp`); the manifold's vertex remap starts zeroed (`hkpGskManifoldUtil.cpp`); a contact
    whose flying colours fail is not breached (`hkpSimpleContactConstraintData.cpp`); the chain drawer's pivots start
    at 0 (`hkpConstraintChainDrawer.cpp`); the MOPP assembler's root node starts zeroed (`hkpMoppDefaultAssembler.cpp`);
    the geometry mesh's new vertices and triangles start zeroed, with no plane and, for caps, no material, in
    appendFromGeometry, retriangulateFromPlanes, splitTriangle, fillHoles and cutAndCap, and cutAndCap's projected
    segments have no constraint or data bits (`hkgpMesh.cpp`).
- **MOPP property commands** store only property 0 and keep the copied query's value; the exe stores properties 1 to 3
  past the array and can restore a value nothing wrote. `hkpMoppMachine.cpp`, `hkpMoppLongRayVirtualMachine.cpp`,
  `hkpMoppAabbCastVirtualMachine.cpp`, `hkpMoppKDopGeometriesVirtualMachine.cpp`, `hkpMoppFindAllVirtualMachine.cpp`,
  `hkpMoppSphereVirtualMachine.cpp`, `hkpMoppRayBundleVirtualMachine.cpp`.
- **IsActorOfFactionList** returns false after its loop; the exe returns whatever its return register held, which is
  usually true for an empty list. `Actor.cpp`.
- **Clearing a 3D update flag** masks the flag out of MiddleHighProcess's update flags; the exe multiplies them by the
  flag's complement, setting unrelated bits. Nothing calls it. `MiddleHighProcess.cpp`.
- **Animation::AddAnimation's missing-object warning** names the object: the name is written into a buffer, where the
  exe passes a null pointer and the warning shows none. `Animation.cpp`.
- **The editor-ID guard** (`TESForm::bIsReadingName`) is one flag per thread, where the exe has one shared flag that
  makes another thread's concurrent read return 0. `TESForm.h`.
- **Startup order across files.** `uExteriorCellBuffer`'s default reads `uGridsToLoad`, so it is defined in
  `GridCell.cpp` after that setting instead of among `TES.cpp`'s settings, and registers with the INI collection there;
  the exe's order held only because its object files link in that order. `LockLevelMaxVeryEasy` likewise moves to
  `ExtraDataObjects.cpp`, after the `iLockLevelMax*` settings it reads (no registration).

## Build options and target

- **64-bit only.** Configuring for a 32-bit target fails (`CMakeLists.txt`). Windows keeps a fixed image base, as the
  original has. On Linux `__ImageBase` is the ELF header, and SDL3 and DXVK ship beside the program with an `$ORIGIN`
  rpath.
- **`XFNV_NO_STEAM`** (ON, must stay ON): SteamAPI_Init is never called.
- **`XFNV_FIX_FAST_MAIN_MENU`** (ON): skips the start-up's welcome screens once the main menu exists
  (`src/Fallout/Interface/Menus/StartMenu.cpp`).
- **`XFNV_CRASH_LOG`** (ON): an unhandled exception writes `FalloutNV.crash-<time>.log` and a `.dmp` beside the program
  (`src/xfnv/crashlog`).
- **`XFNV_SYSTEM_ALLOC`, `XFNV_GUARD_ALLOC`** (OFF): see Memory.
- **`XFNV_ASAN`, `XFNV_UBSAN`** (Linux, OFF): AddressSanitizer over everything, SDL3 included;
  UndefinedBehaviorSanitizer checks of every bool and enum the game's code loads.
- **`XFNV_SDL_AUDIO`, `XFNV_D3DX`** (OFF on Windows, always on on Linux): the audio and d3dx9 stand-ins above.
