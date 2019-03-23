#include "precompiled.h"

uintptr_t gClBase;
unsigned int gClSize;

extern "C" __declspec(dllexport) int __stdcall RIB_Main(int, int, int, int, int)
{
	return 1;
}

unsigned int GetModuleSize(uintptr_t base)
{
	if (!base)
		return 0;

	auto dosheader = (PIMAGE_DOS_HEADER)base;
	auto ntheader = (PIMAGE_NT_HEADERS)((int)dosheader + dosheader->e_lfanew);
	return ntheader->OptionalHeader.SizeOfImage;
}

bool ImplementScrollTextUpFix()
{
	// Pattern: 7E 03 88 0A
	// Patch:   7E 03 (jle) -> 76 03 (jbe)

	for (auto p = gClBase; p < gClBase + gClSize - 1; p++)
	{
		if (*(long *)p == 0x0A88037E)
		{
			DWORD protect;

			VirtualProtect((LPVOID)p, 1, PAGE_EXECUTE_READWRITE, &protect);
			*(unsigned char *)p = 0x76;
			VirtualProtect((LPVOID)p, 1, protect, &protect);

			return true;
		}
	}

	return false;
}

DWORD CALLBACK ModuleEntry(LPVOID)
{
	for (;;)
	{
		gClBase = (uintptr_t)(GetModuleHandleA("client.dll"));

		if (gClBase)
		{
			gClSize = GetModuleSize(gClBase);
			break;
		}

		Sleep(250);
	}

	if (!ImplementScrollTextUpFix())
	{
		MessageBoxA(HWND_DESKTOP, __FUNCTION__ ": Could not implement ScrollTextUp fix.", "Fatal Error", MB_ICONWARNING | MB_SYSTEMMODAL);
		ExitProcess(EXIT_SUCCESS);
	}

	return TRUE;
}

BOOL WINAPI DllMain(_In_ HINSTANCE hinstDLL, _In_ DWORD fdwReason, _In_ LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		CreateMutexA(NULL, TRUE, "__ScrollTextUpFix");
		if (GetLastError() != ERROR_ALREADY_EXISTS)
		{
			CreateThread(NULL, 0, ModuleEntry, NULL, 0, NULL);
		}
	}

	return TRUE;
}