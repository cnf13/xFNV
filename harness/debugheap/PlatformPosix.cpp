// Platform.h on POSIX (Linux, x86-64): mmap/mprotect/madvise pages, pthread's stack bounds, the program's executable
// segments from dl_iterate_phdr, a SIGSEGV handler for the fault report (chained to the handler it replaces, so the
// crash log's report follows; or to the one installed after it, which chains back here), and an aligned heap on
// malloc with the size kept in front of each block.

#include "Platform.h"

#include <fcntl.h>
#include <limits.h>
#include <link.h>
#include <malloc.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <ucontext.h>
#include <unistd.h>

namespace fnvd::debugheap::platform {
namespace {

int giLog = -1;
char gcImagePath[PATH_MAX];
struct sigaction gPrevious;
thread_local uintptr_t tuiStackTop = 0;

// The x86-64 page fault's error code: the access was a write.
constexpr uintptr_t kErrorWrite = 0x2;

struct Image {
	uintptr_t uiBase;
	uintptr_t uiCodeBegin;
	uintptr_t uiCodeEnd;
	bool bFound;
};
Image gImage = {};

// The program itself is the first module dl_iterate_phdr reports.
int OnModule(dl_phdr_info* apInfo, size_t, void*)
{
	gImage.uiBase = apInfo->dlpi_addr;
	gImage.uiCodeBegin = UINTPTR_MAX;
	gImage.uiCodeEnd = 0;
	for (int i = 0; i < apInfo->dlpi_phnum; ++i) {
		const ElfW(Phdr)& rHeader = apInfo->dlpi_phdr[i];
		if (rHeader.p_type == PT_LOAD && (rHeader.p_flags & PF_X)) {
			const uintptr_t uiBegin = apInfo->dlpi_addr + rHeader.p_vaddr;
			gImage.uiCodeBegin = uiBegin < gImage.uiCodeBegin ? uiBegin : gImage.uiCodeBegin;
			gImage.uiCodeEnd = uiBegin + rHeader.p_memsz > gImage.uiCodeEnd ? uiBegin + rHeader.p_memsz : gImage.uiCodeEnd;
		}
	}
	gImage.bFound = true;
	return 1;
}

const Image& ProgramImage()
{
	if (!gImage.bFound) {
		dl_iterate_phdr(&OnModule, nullptr);
	}
	return gImage;
}

void OnFault(int aiSignal, siginfo_t* apInfo, void* apContext)
{
	if (apInfo->si_code > 0) {
		const greg_t* pRegisters = static_cast<const ucontext_t*>(apContext)->uc_mcontext.gregs;
		auto value = [&](int aiIndex) { return static_cast<uintptr_t>(pRegisters[aiIndex]); };
		const FaultRegister kRegisters[] = {{"rax", value(REG_RAX)}, {"rbx", value(REG_RBX)}, {"rcx", value(REG_RCX)},
			{"rdx", value(REG_RDX)}, {"rsi", value(REG_RSI)}, {"rdi", value(REG_RDI)}, {"rbp", value(REG_RBP)},
			{"r8", value(REG_R8)}, {"r9", value(REG_R9)}, {"r10", value(REG_R10)}, {"r11", value(REG_R11)},
			{"r12", value(REG_R12)}, {"r13", value(REG_R13)}, {"r14", value(REG_R14)}, {"r15", value(REG_R15)}};
		// the stack's top is known once the thread has allocated (StackTop's cache); else only the first words
		const uintptr_t uiSp = value(REG_RSP);
		DescribeFault(reinterpret_cast<uintptr_t>(apInfo->si_addr), (value(REG_ERR) & kErrorWrite) != 0,
			value(REG_RIP), uiSp, tuiStackTop ? tuiStackTop : uiSp + 16 * sizeof(uintptr_t), kRegisters,
			sizeof(kRegisters) / sizeof(kRegisters[0]));
	}
	if (gPrevious.sa_flags & SA_SIGINFO) {
		gPrevious.sa_sigaction(aiSignal, apInfo, apContext);
	} else if (gPrevious.sa_handler != SIG_DFL && gPrevious.sa_handler != SIG_IGN) {
		gPrevious.sa_handler(aiSignal);
	} else {
		// the fault repeats at its instruction once this returns, and kills the process
		struct sigaction kDefault = {};
		kDefault.sa_handler = SIG_DFL;
		sigaction(aiSignal, &kDefault, nullptr);
	}
}

// _aligned_malloc's form: the raw block and the size asked for right before the aligned one.
struct AlignedHeader {
	void* pRaw;
	size_t uiSize;
};

}  // namespace

size_t PageSize()
{
	return static_cast<size_t>(sysconf(_SC_PAGESIZE));
}

char* Reserve(size_t aSize)
{
	void* p = mmap(nullptr, aSize, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
	return p == MAP_FAILED ? nullptr : static_cast<char*>(p);
}

bool Commit(void* apPages, size_t aSize)
{
	return mprotect(apPages, aSize, PROT_READ | PROT_WRITE) == 0;
}

void Decommit(void* apPages, size_t aSize)
{
	madvise(apPages, aSize, MADV_DONTNEED);
	mprotect(apPages, aSize, PROT_NONE);
}

void* AllocateTable(size_t aSize)
{
	void* p = mmap(nullptr, aSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
	return p == MAP_FAILED ? nullptr : p;
}

uint32_t ThreadId()
{
	return static_cast<uint32_t>(syscall(SYS_gettid));
}

uintptr_t StackTop()
{
	if (!tuiStackTop) {
		pthread_attr_t kAttributes;
		void* pStack = nullptr;
		size_t uiSize = 0;
		if (pthread_getattr_np(pthread_self(), &kAttributes) == 0) {
			pthread_attr_getstack(&kAttributes, &pStack, &uiSize);
			pthread_attr_destroy(&kAttributes);
		}
		tuiStackTop = reinterpret_cast<uintptr_t>(pStack) + uiSize;
	}
	return tuiStackTop;
}

void CodeRange(const uint8_t*& arBegin, const uint8_t*& arEnd)
{
	const Image& rImage = ProgramImage();
	arBegin = reinterpret_cast<const uint8_t*>(rImage.uiCodeBegin);
	arEnd = reinterpret_cast<const uint8_t*>(rImage.uiCodeEnd);
}

uintptr_t ImageBase()
{
	return ProgramImage().uiBase;
}

const char* ImagePath()
{
	if (!gcImagePath[0]) {
		const ssize_t iLength = readlink("/proc/self/exe", gcImagePath, sizeof(gcImagePath) - 1);
		gcImagePath[iLength > 0 ? iLength : 0] = '\0';
	}
	return gcImagePath;
}

void OpenLog()
{
	giLog = open("fnvd_debugheap.log", O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
}

void WriteLog(const char* apText, size_t aSize)
{
	if (giLog >= 0 && write(giLog, apText, aSize) < 0) {
		giLog = -1;
	}
	[[maybe_unused]] const ssize_t iWritten = write(STDERR_FILENO, apText, aSize);
}

size_t ForeignSize(void* apMem)
{
	return malloc_usable_size(apMem);
}

[[noreturn]] void Stop()
{
	abort();
}

void InstallFaultHook()
{
	struct sigaction kAction = {};
	kAction.sa_sigaction = &OnFault;
	kAction.sa_flags = SA_SIGINFO | SA_ONSTACK;
	sigemptyset(&kAction.sa_mask);
	sigaction(SIGSEGV, &kAction, &gPrevious);
}

void* AlignedAllocate(size_t aSize, size_t aAlign)
{
	if (aAlign < alignof(AlignedHeader)) {
		aAlign = alignof(AlignedHeader);
	}
	char* pRaw = static_cast<char*>(malloc(sizeof(AlignedHeader) + aAlign - 1 + aSize));
	if (!pRaw) {
		return nullptr;
	}
	char* pUser = reinterpret_cast<char*>(
		(reinterpret_cast<uintptr_t>(pRaw) + sizeof(AlignedHeader) + aAlign - 1) & ~static_cast<uintptr_t>(aAlign - 1));
	AlignedHeader* pHeader = reinterpret_cast<AlignedHeader*>(pUser) - 1;
	pHeader->pRaw = pRaw;
	pHeader->uiSize = aSize;
	return pUser;
}

void AlignedFree(void* apMem)
{
	if (apMem) {
		free((static_cast<AlignedHeader*>(apMem) - 1)->pRaw);
	}
}

void* AlignedReallocate(void* apMem, size_t aSize, size_t aAlign)
{
	if (!apMem) {
		return AlignedAllocate(aSize, aAlign);
	}
	if (!aSize) {
		AlignedFree(apMem);
		return nullptr;
	}
	void* pNew = AlignedAllocate(aSize, aAlign);
	if (pNew) {
		const size_t uiOld = (static_cast<AlignedHeader*>(apMem) - 1)->uiSize;
		memcpy(pNew, apMem, uiOld < aSize ? uiOld : aSize);
		AlignedFree(apMem);
	}
	return pNew;
}

size_t AlignedSize(void* apMem, size_t)
{
	return (static_cast<AlignedHeader*>(apMem) - 1)->uiSize;
}

}  // namespace fnvd::debugheap::platform
