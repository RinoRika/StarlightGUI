#include "pch.h"
#include "TaskUtils.h"
#include "Utils.h"
#include "TlHelp32.h"
#include "KernelBase.h"
#include "shellapi.h"
#include "winternl.h"
#include "Psapi.h"

typedef struct MY_SYSTEM_PROCESS_INFORMATION {
	ULONG NextEntryOffset;
	ULONG NumberOfThreads;
	LARGE_INTEGER WorkingSetPrivateSize;
	ULONG HardFaultCount;
	ULONG NumberOfThreadsHighWatermark;
	ULONGLONG CycleTime;
	LARGE_INTEGER CreateTime;
	LARGE_INTEGER UserTime;
	LARGE_INTEGER KernelTime;
	UNICODE_STRING ImageName;
	KPRIORITY BasePriority;
	HANDLE UniqueProcessId;
	HANDLE InheritedFromUniqueProcessId;
	ULONG HandleCount;
	ULONG SessionId;
	ULONG_PTR UniqueProcessKey;
	SIZE_T PeakVirtualSize;
	SIZE_T VirtualSize;
	ULONG PageFaultCount;
	SIZE_T PeakWorkingSetSize;
	SIZE_T WorkingSetSize;
	SIZE_T QuotaPeakPagedPoolUsage;
	SIZE_T QuotaPagedPoolUsage;
	SIZE_T QuotaPeakNonPagedPoolUsage;
	SIZE_T QuotaNonPagedPoolUsage;
	SIZE_T PagefileUsage;
	SIZE_T PeakPagefileUsage;
	SIZE_T PrivatePageCount;
	LARGE_INTEGER ReadOperationCount;
	LARGE_INTEGER WriteOperationCount;
	LARGE_INTEGER OtherOperationCount;
	LARGE_INTEGER ReadTransferCount;
	LARGE_INTEGER WriteTransferCount;
	LARGE_INTEGER OtherTransferCount;
} MY_SYSTEM_PROCESS_INFORMATION, * MY_PSYSTEM_PROCESS_INFORMATION;
typedef BOOL(WINAPI* P_EndTask)(HWND hwnd, BOOL fShutdown, BOOL fForce);
typedef NTSTATUS(NTAPI* P_NtQuerySystemInformation)(_SYSTEM_INFORMATION_CLASS SystemInformationClass, PVOID SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength);


P_EndTask _EndTask{ nullptr };
P_NtQuerySystemInformation _NtQuerySystemInformation{ nullptr };

namespace winrt::StarlightGUI::implementation {
	void TaskUtils::EnsurePrivileges() {
		TaskUtils::EnableDebugPrivilege();
	}
	/*
	* 结束进程
	*/
	bool TaskUtils::Task_TerminateProcess(StarlightGUI::ProcessInfo pi) {
		int pid = pi.Id();
		if (pid != 0) {
			HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pid);

			if (hProc) {
				BOOL result = TerminateProcess(hProc, 0);
				CloseHandle(hProc);

				return result;
			}
		}
		return false;
	}
	/*
	* 结束任务
	*/
	bool TaskUtils::Task_EndTask(StarlightGUI::ProcessInfo pi) {
		int pid = pi.Id();
		if (pid != 0) {
			return EnumWindows(EndTaskByWindow, (LPARAM)&pid);
		}
		return false;
	}

	/*
	* 结束线程
	*/
	bool TaskUtils::Task_TerminateThread(StarlightGUI::ProcessInfo pi) {
		int pid = pi.Id();
		if (pid != 0) {
			HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, pid);
			if (hSnapshot && hSnapshot != INVALID_HANDLE_VALUE) {
				THREADENTRY32 threadEntry = { sizeof(threadEntry) };
				BOOL finder = Thread32First(hSnapshot, &threadEntry);

				for (; finder; finder = Thread32Next(hSnapshot, &threadEntry)) {
					if (threadEntry.th32OwnerProcessID == pid) {
						HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, threadEntry.th32ThreadID);
						BOOL result = FALSE;

						if (hThread) {
							result = TerminateThread(hThread, 0);
						}

						CloseHandle(hThread);
						CloseHandle(hSnapshot);
						return result;
					}
				}
			}
			CloseHandle(hSnapshot);
		}
		return false;
	}

	/*
	* 强制结束进程
	*/
	bool TaskUtils::Task_TerminateProcessForce(StarlightGUI::ProcessInfo pi) {
		int pid = pi.Id();
		if (pid != 0) {
			return KernelInstance::ZwTerminateProcess0(pid);
		}
		return false;
	}

	/*
	* 开启进程效能模式
	*/
	bool TaskUtils::Task_EnableProcessPerformanceMode(StarlightGUI::ProcessInfo pi) {
		int pid = pi.Id();
		if (pid != 0) {
			HANDLE hProc = OpenProcess(PROCESS_SET_INFORMATION, FALSE, pid);

			if (hProc) {
				PROCESS_POWER_THROTTLING_STATE throttling;
				ZeroMemory(&throttling, sizeof(throttling));

				throttling.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
				throttling.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;
				throttling.StateMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;

				BOOL result = SetProcessInformation(hProc, ProcessPowerThrottling, &throttling, sizeof(throttling));
				CloseHandle(hProc);

				return result;
			}
		}
		return false;
	}

	/*
	* 获取进程私有工作集
	*/
	SIZE_T TaskUtils::Task_GetProcessWorkingSet(HANDLE hProc) {
		PSAPI_WORKING_SET_INFORMATION workSetInfo{};
		PBYTE pByte = NULL;
		PSAPI_WORKING_SET_BLOCK* pWorkSetBlock = workSetInfo.WorkingSetInfo;

		if (!K32QueryWorkingSet(hProc, &workSetInfo, sizeof(workSetInfo))) {
			if (GetLastError() == ERROR_BAD_LENGTH)
			{
				DWORD realSize = sizeof(workSetInfo.NumberOfEntries)
					+ workSetInfo.NumberOfEntries * sizeof(PSAPI_WORKING_SET_BLOCK);
				try
				{
					pByte = new BYTE[realSize];
					memset(pByte, 0, realSize);
					pWorkSetBlock = (PSAPI_WORKING_SET_BLOCK*)(pByte + sizeof(workSetInfo.NumberOfEntries));
					if (!K32QueryWorkingSet(hProc, pByte, realSize))
					{
						delete[] pByte;
						return 0;
					}
				}
				catch (char* e)
				{
					return 0;
				}
			}
			else {
				return 0;
			}
		}
		PERFORMANCE_INFORMATION performanceInfo{};
		if (!K32GetPerformanceInfo(&performanceInfo, sizeof(performanceInfo))) return 0;
		SIZE_T pageSize = performanceInfo.PageSize;
		SIZE_T privateWorkingSet = 0;
		for (ULONG_PTR i = 0; i < workSetInfo.NumberOfEntries; ++i)
		{
			if (!pWorkSetBlock[i].Shared) // Remove shared pages
				privateWorkingSet += pageSize;
		}
		if (pByte)
			delete[] pByte;
		return privateWorkingSet;
	}

	/*
	* 获取进程CPU占用
	*/
	void TaskUtils::FetchProcessCpuUsage(std::map<DWORD, hstring>& processCpuTable) {
		HMODULE hNtdll = LoadLibrary(L"ntdll.dll");

		if (!_NtQuerySystemInformation) _NtQuerySystemInformation = (P_NtQuerySystemInformation)GetProcAddress(hNtdll, "NtQuerySystemInformation");

		ULONG len = 0;
		std::vector<BYTE> buffer(0x10000);
		NTSTATUS status = _NtQuerySystemInformation(SystemProcessInformation, buffer.data(), len, &len);

		if (NT_ERROR(status)) {
			buffer.resize(len);
			status = _NtQuerySystemInformation(SystemProcessInformation, buffer.data(), len, &len);
		}

		if (!NT_SUCCESS(status)) return;

		// Loop through the processes and find the one with the specified PID
		MY_PSYSTEM_PROCESS_INFORMATION spi = (MY_PSYSTEM_PROCESS_INFORMATION)buffer.data();
		while (spi) {
			LONGLONG cpuUsage = spi->KernelTime.QuadPart;
			if (cpuUsage > 0) {
				processCpuTable[(DWORD)(ULONG_PTR)spi->UniqueProcessId] = to_hstring(std::round(cpuUsage / 1000000000.0) / 10.0) + L"%";
			}
			else processCpuTable[(DWORD)(ULONG_PTR)spi->UniqueProcessId] = L"0%";
			if (spi->NextEntryOffset == 0)
				break;
			spi = (MY_PSYSTEM_PROCESS_INFORMATION)((BYTE*)spi + spi->NextEntryOffset);
		}

		return;
	}

	/*
	* 复制至剪贴板
	*/
	bool TaskUtils::Task_CopyToClipboard(std::wstring str) {
		if (str.empty()) {
			return false;
		}

		if (!OpenClipboard(nullptr)) {
			return false;
		}

		if (!EmptyClipboard()) {
			CloseClipboard();
			return false;
		}

		size_t sizeInBytes = (str.size() + 1) * sizeof(wchar_t);
		HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, sizeInBytes);
		if (!hGlobal) {
			CloseClipboard();
			return false;
		}

		void* pGlobal = GlobalLock(hGlobal);
		if (!pGlobal) {
			GlobalFree(hGlobal);
			CloseClipboard();
			return false;
		}
		memcpy(pGlobal, str.c_str(), sizeInBytes);
		GlobalUnlock(hGlobal);

		if (!SetClipboardData(CF_UNICODETEXT, hGlobal)) {
			GlobalFree(hGlobal);
			CloseClipboard();
			return false;
		}

		CloseClipboard();
		return true;
	}
	/*
	* 打开文件所在位置
	*/
	bool TaskUtils::Task_OpenFolderAndSelectFile(std::wstring filePath) {
		PIDLIST_ABSOLUTE pidl = nullptr;

		HRESULT hr = SHParseDisplayName(filePath.c_str(), nullptr, &pidl, 0, nullptr);
		if (FAILED(hr) || !pidl) {
			return false;
			return false;
		}

		hr = SHOpenFolderAndSelectItems(pidl, 0, nullptr, 0);
		CoTaskMemFree(pidl);

		if (FAILED(hr)) {
			return false;
		}
		return true;
	}

	/*
	* 打开文件属性
	*/
	bool TaskUtils::Task_OpenFileProperties(std::wstring filePath) {
		SHELLEXECUTEINFOW sei = { 0 };
		sei.cbSize = sizeof(sei);
		sei.fMask = SEE_MASK_INVOKEIDLIST;
		sei.hwnd = NULL;
		sei.lpVerb = L"properties";
		sei.lpFile = filePath.c_str();
		sei.nShow = SW_SHOW;

		return ShellExecuteExW(&sei) != FALSE;
	}

	// =====================================================
	// Private --- Starting here
	// =====================================================
	BOOL CALLBACK TaskUtils::EndTaskByWindow(HWND hwnd, LPARAM lparam) {
		DWORD* procPid = (DWORD*)lparam;
		DWORD windowPid;

		if (_EndTask == nullptr) {
			_EndTask = (P_EndTask)GetProcAddress(GetModuleHandleW(L"user32.dll"), "EndTask");
		}

		// Get thread ID
		GetWindowThreadProcessId(hwnd, &windowPid);

		if (windowPid == *procPid) {
			_EndTask(hwnd, FALSE, TRUE);
		}
		return TRUE;
	}

	bool TaskUtils::EnableDebugPrivilege() {
		HANDLE hToken;
		TOKEN_PRIVILEGES tkp;

		if (!OpenProcessToken(GetCurrentProcess(),
			TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
			return false;
		}

		LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tkp.Privileges[0].Luid);

		tkp.PrivilegeCount = 1;
		tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		BOOL result = AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, NULL, 0);
		CloseHandle(hToken);

		return result != FALSE;
	}
}