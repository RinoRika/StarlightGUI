#include "pch.h"
#include <Utils/Utils.h>
#include <winternl.h>
#include <Psapi.h>
#include "KernelBase.h"

namespace winrt::StarlightGUI::implementation {
	DWORD64 KernelBase::GetCIBaseAddress() {
		LPVOID drivers[1024];
		DWORD cbNeeded;

		if (!EnumDeviceDrivers(drivers, sizeof(drivers), &cbNeeded)) {
			return 0;
		}

		int driverCount = cbNeeded / sizeof(drivers[0]);

		for (int i = 0; i < driverCount; i++) {
			CHAR driverName[MAX_PATH];
			if (GetDeviceDriverBaseNameA(drivers[i], driverName, sizeof(driverName))) {

				if (_stricmp(driverName, "ci.dll") == 0) {
					return (DWORD64)drivers[i];
				}
			}
		}

		return 0;
	}
	ULONG64 KernelBase::HackCI() {
		HMODULE hModule = LoadLibraryExA("C:\\Windows\\System32\\ci.dll", NULL, DONT_RESOLVE_DLL_REFERENCES);
		if (!hModule) return 0;

		// Offset for Windows 11 25H2
		ULONG64 g_ciOptions = GetCIBaseAddress() + 0x4E004;

		FreeLibrary(hModule);

		return g_ciOptions;
	}

	bool KernelBase::HackCIOption(DWORD64 value) {
		ULONG64 ciOptions = HackCI();

		KernelInstance::WriteMemoryDWORD64(ciOptions, value);

		return KernelInstance::ReadMemoryDWORD(ciOptions) == value;
	}

	DWORD KernelBase::GetCIOption() {
		ULONG64 ciOptions = HackCI();
		return KernelInstance::ReadMemoryDWORD(ciOptions);
	}
}