#pragma once

#include "pch.h"

namespace winrt::StarlightGUI::implementation {
	class KernelInstance {
	public:
		static BOOL ZwTerminateProcess0(DWORD pid);

		static DWORD ReadMemoryPrimitive(DWORD Size, DWORD64 Address);

		static void WriteMemoryPrimitive(DWORD Size, DWORD64 Address, DWORD Value);

		static WORD ReadMemoryWORD(DWORD64 Address);

		static DWORD ReadMemoryDWORD(DWORD64 Address);

		static DWORD64 ReadMemoryDWORD64(DWORD64 Address);

		static void WriteMemoryDWORD64(DWORD64 Address, DWORD64 Value);
	};

	class KernelBase {
	private:
		static DWORD64 GetCIBaseAddress();
		static ULONG64 HackCI();

	public:
		static bool HackCIOption(DWORD64 value);
		static DWORD GetCIOption();
	};

	class DriverUtils {

	public:
		static bool LoadKernelDriver(LPCWSTR kernelPath, std::wstring& dbgMsg);

		static bool LoadDriver(LPCWSTR kernelPath, LPCWSTR fileName, std::wstring& dbgMsg);
	};
}