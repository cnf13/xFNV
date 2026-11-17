# idakit: the harness's IDA tools in C++ on the IDA SDK (idalib), a library (include/idakit, src/) and its CLI
# (tools/: build/idakit/idakit).
#   cmake -S harness/idakit -B build/idakit -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang \
#       -DCMAKE_CXX_COMPILER=clang++ && cmake --build build/idakit
# IDA_DIR: the IDA installation (its libidalib.so is what runs; the SDK is IDA_DIR/ida-sdk-9.3).
cmake_minimum_required(VERSION 3.20)
project(idakit CXX)

if(DEFINED ENV{FNVD_USER_HOME})
	set(_home "$ENV{FNVD_USER_HOME}")
else()
	set(_home "$ENV{HOME}")
endif()
set(IDA_DIR "${_home}/ida-pro-9.3" CACHE PATH "IDA installation")
set(IDA_SDK "${IDA_DIR}/ida-sdk-9.3/src" CACHE PATH "IDA SDK (its src/ directory)")
if(NOT EXISTS "${IDA_SDK}/include/pro.h")
	message(FATAL_ERROR "no IDA SDK at ${IDA_SDK}")
endif()
find_package(SQLite3 REQUIRED)
# the exe facts (harness/exefacts, on the venv's capstone): what the exe's code says about a function's interface
add_subdirectory(../exefacts "${CMAKE_BINARY_DIR}/exefacts" EXCLUDE_FROM_ALL)

add_library(idakit STATIC
	src/Address.cpp
	src/Args.cpp
	src/Asm.cpp
	src/AsmFacts.cpp
	src/Database.cpp
	src/Emit.cpp
	src/EmitAdjustedNullTests.cpp
	src/EmitAccessors.cpp
	src/EmitAliases.cpp
	src/EmitAlignedLocals.cpp
	src/EmitAllocations.cpp
	src/EmitByteParameters.cpp
	src/EmitCalls.cpp
	src/EmitConst.cpp
	src/EmitContainingRecords.cpp
	src/EmitConversions.cpp
	src/EmitDeclarations.cpp
	src/EmitDynamicCast.cpp
	src/EmitEhState.cpp
	src/EmitEnums.cpp
	src/EmitFoldedAccessors.cpp
	src/EmitFloats.cpp
	src/EmitFormIds.cpp
	src/EmitHiddenResults.cpp
	src/EmitFields.cpp
	src/EmitGlobals.cpp
	src/EmitIdioms.cpp
	src/EmitImplicit.cpp
	src/EmitInlinedLocals.cpp
	src/EmitIntrinsics.cpp
	src/EmitNiPointer.cpp
	src/EmitNoOps.cpp
	src/EmitOutgoingArguments.cpp
	src/EmitOverloads.cpp
	src/EmitParts.cpp
	src/EmitPointerCalls.cpp
	src/EmitPointerMasks.cpp
	src/EmitPrinter.cpp
	src/EmitRefCounts.cpp
	src/EmitReview.cpp
	src/EmitRounding.cpp
	src/EmitRules.cpp
	src/EmitScopes.cpp
	src/EmitSelf.cpp
	src/EmitShadowing.cpp
	src/EmitSimpleList.cpp
	src/EmitStructCopies.cpp
	src/EmitStructorLocals.cpp
	src/EmitThreadData.cpp
	src/EmitTypes.cpp
	src/EmitValue.cpp
	src/EmitVariadicArguments.cpp
	src/EmitVectorLanes.cpp
	src/EmitVtableStores.cpp
	src/EmitWalk.cpp
	src/ExePrototype.cpp
	src/ExeSlotTargets.cpp
	src/ExeVtables.cpp
	src/Frames.cpp
	src/FramesRealigned.cpp
	src/FunctionList.cpp
	src/IdaSupport.cpp
	src/InferRead.cpp
	src/InferSolve.cpp
	src/LocalTrials.cpp
	src/SrcEdit.cpp
	src/SrcIda.cpp
	src/SrcModel.cpp
	src/SrcPlace.cpp
	src/SrcPush.cpp
	src/SrcPushConvert.cpp
	src/SrcPushPops.cpp
	src/SrcTypes.cpp
	src/SyncCheck.cpp
	src/SyncExe.cpp
	src/SyncFunctions.cpp
	src/SyncTypes.cpp
	src/UnfoldFoldOwners.cpp
	src/UnfoldLocals.cpp
	src/UnfoldVariadicCalls.cpp)
set_target_properties(idakit PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON POSITION_INDEPENDENT_CODE ON)
target_include_directories(idakit PUBLIC include)
target_include_directories(idakit SYSTEM PUBLIC "${IDA_SDK}/include")
# the SDK's platform (64-bit addresses on Linux x64) and the C library names it otherwise poisons
target_compile_definitions(idakit PUBLIC __LINUX__ __EA64__ USE_STANDARD_FILE_FUNCTIONS USE_DANGEROUS_FUNCTIONS)
target_link_libraries(idakit PUBLIC
	exefacts-core
	SQLite3::SQLite3
	"${IDA_SDK}/lib/x64_linux_gcc_64/libidalib.so"
	"${IDA_SDK}/lib/x64_linux_gcc_64/libida.so")

add_executable(idakit_cli tools/main.cpp tools/Commands.cpp tools/AllocatorsCommand.cpp tools/DeclareCommand.cpp
	tools/DeclareCallees.cpp
	tools/DeclareHavok.cpp
	tools/DeclareFields.cpp
	tools/DeclarePdb.cpp
	tools/DeclareRetype.cpp
	tools/DeclareSignatures.cpp
	tools/DeclareSlotRows.cpp
	tools/EmitCommand.cpp
	tools/FramesCommand.cpp
	tools/FunctionsCommand.cpp
	tools/InferCommand.cpp
	tools/LocalsCommand.cpp
	tools/MergeCommand.cpp
	tools/SyncCommand.cpp)
set_target_properties(idakit_cli PROPERTIES OUTPUT_NAME idakit CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON
	# the installation's libidalib.so and libida.so at run time (the SDK's are link stubs)
	BUILD_RPATH "${IDA_DIR}" INSTALL_RPATH "${IDA_DIR}")
target_link_libraries(idakit_cli PRIVATE idakit)
# the library's internal helpers (IdaSupport.hpp: IDA's names as the PDB spells them) for the commands that share them
target_include_directories(idakit_cli PRIVATE src)
