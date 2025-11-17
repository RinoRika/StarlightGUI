#pragma once

#include <iostream>
#include <string>
#include <windows.h>
#include <winternl.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <cctype>
#include <algorithm>
#include <Utils/Utils.h>
#include <Utils/KernelBase.h>

typedef struct _PACKAGE_SUICIDE { intptr_t adExitProcess; }PACKAGE_SUICIDE, * PPACKAGE_SUICIDE;
typedef HANDLE(WINAPI* P_WinStationOpenServerW)(_In_ PWSTR serverName);
typedef BOOLEAN(WINAPI* P_WinStationTerminateProcess)(_In_opt_ HANDLE hServer, _In_ ULONG pid, _In_ ULONG exitCode);
typedef NTSTATUS(NTAPI* P_NtTerminateProcess)(HANDLE, NTSTATUS);
typedef BOOL(WINAPI* P_EndTask)(HWND hwnd, BOOL fShutdown, BOOL fForce);
typedef NTSTATUS(NTAPI* P_NtQueryInformationProcess)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);

P_EndTask EndTaskAddr{ nullptr };
P_WinStationOpenServerW fpWinStationOpenServerW = NULL;
P_WinStationTerminateProcess fpWinStationTerminateProcess = NULL;
static Panel t_parent{ nullptr };
static XamlRoot t_root{ nullptr };

using namespace winrt::StarlightGUI::implementation;

bool isStringNumber(const std::string& str) {
	if (str.empty()) return false;
	for (char c : str) {
		if (!std::isdigit(static_cast<unsigned char>(c))) {
			return false;
		}
	}
	return true;
}

DWORD GetProcessPIDByName(std::string procName) {
	DWORD pid = 0;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (hSnapshot != INVALID_HANDLE_VALUE) {
		PROCESSENTRY32W pe32;
		pe32.dwSize = sizeof(PROCESSENTRY32W);
		std::transform(procName.begin(), procName.end(), procName.begin(), ::tolower);

		if (Process32FirstW(hSnapshot, &pe32)) {
			do {
				std::wstring procExeFileName = pe32.szExeFile;
				std::transform(procExeFileName.begin(), procExeFileName.end(), procExeFileName.begin(), ::tolower);
				std::wstring wideProcessName(procName.begin(), procName.end());

				if (wideProcessName == procExeFileName) {
					pid = pe32.th32ProcessID;
					break;
				}
			} while (Process32NextW(hSnapshot, &pe32));
		}

		CloseHandle(hSnapshot);
	}

	return pid;
}

int GetPID(std::string input) {
	size_t start = input.find_first_not_of(" \t\n\r");
	size_t end = input.find_last_not_of(" \t\n\r");
	if (start != std::string::npos && end != std::string::npos) {
		input = input.substr(start, end - start + 1);
	}

	if (input.empty()) {
		return 0;
	}

	// Check if input is a number
	if (isStringNumber(input)) {
		int result = 0;
		sscanf_s(input.c_str(), "%d", &result);
		return result;
	}
	else {
		// Get Process PID
		DWORD pid = GetProcessPIDByName(input);

		if (pid != 0) {
			return (int)pid;
		}
	}

	return 0;
}

HANDLE GetProcessByPID(int pid) {
	// Open process
	HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (!hProc || hProc == NULL) {
		CreateInfoBarAndDisplay(L"Terminator", L"进程句柄未知或无效！", InfoBarSeverity::Error, t_root, t_parent);
		CreateInfoBarAndDisplay(L"Terminator", L"将进程提升至更高权限可能会有所帮助！", InfoBarSeverity::Warning, t_root, t_parent);
		return 0;
	}
	return hProc;
}

DWORD WINAPI RemoteThread_ExitProcess(LPVOID lpParam) {
	PPACKAGE_SUICIDE package = (PPACKAGE_SUICIDE)lpParam;
	typedef VOID(__stdcall* T_ExitProcess)(UINT);

	// Get address and exit 
	T_ExitProcess _ExitProcess = (T_ExitProcess)package->adExitProcess;
	_ExitProcess(0);

	return 0;
}

bool IsProcessAlive(int pid) {
	HANDLE hProc = GetProcessByPID(pid);
	if (hProc == NULL) {
		DWORD err = GetLastError();
		if (err == ERROR_INVALID_PARAMETER) {
			CloseHandle(hProc);
			return false;
		}
		CloseHandle(hProc);
		return true;
	}
	CloseHandle(hProc);
	return true;
}

BOOL WinstaKillProcess(PWSTR hServer, DWORD dwPid, ULONG exitCode) {
	BOOLEAN r = (unsigned)0;
	HANDLE hServerSign;
	if (!wcscmp(hServer, L"(null)")) {
		hServerSign = NULL;
	}
	else {
		hServerSign = fpWinStationOpenServerW(hServer);
	}

	r = fpWinStationTerminateProcess(hServerSign, dwPid, exitCode);

	if (r == (unsigned)0) {
		return FALSE;
	}

	return TRUE;
}

bool PatchThread64(DWORD pid) {
	static auto exitAddr = (DWORDLONG)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "ExitProcess");

	HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, pid);
	SuspendThread(hThread);

	CONTEXT context;
	context.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;

	// Get thread context
	BOOL r = GetThreadContext(hThread, &context);
	if (!r) {
		CreateInfoBarAndDisplay(L"Terminator", L"无法获得线程上下文", InfoBarSeverity::Error, t_root, t_parent);
		ResumeThread(hThread);
		CloseHandle(hThread);
		return false;
	}

	context.Rcx = 0U;
	context.Rip = exitAddr;
	// Set thread context to exit thread
	CreateInfoBarAndDisplay(L"Terminator", L"退出线程中...", InfoBarSeverity::Informational, t_root, t_parent);
	r = SetThreadContext(hThread, &context);
	if (!r) {
		CreateInfoBarAndDisplay(L"Terminator", L"无法传达命令", InfoBarSeverity::Error, t_root, t_parent);
		ResumeThread(hThread);
		CloseHandle(hThread);
		return false;
	}

	ResumeThread(hThread);
	CloseHandle(hThread);
	return true;
}

bool EnablePrivilege(LPCTSTR privilege) {
	HANDLE hToken;
	TOKEN_PRIVILEGES tkp{};

	if (!OpenProcessToken(GetCurrentProcess(),
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
		return false;
	}

	LookupPrivilegeValueW(NULL, privilege, &tkp.Privileges[0].Luid);

	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	BOOL result = AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, NULL, 0);
	CloseHandle(hToken);

	return result != FALSE;
}

bool EnableDebugPrivilege() {
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

// [1] TerminateProcess
bool TerminateProcessNormally(int pid) {
	HANDLE hProc = GetProcessByPID(pid);

	if (!hProc) {
		return false;
	}

	// Terminate Process
	std::wstring content = L"运行TerminateProcess, 目标PID: " + std::to_wstring(pid);
	CreateInfoBarAndDisplay(L"Terminator", content.c_str(), InfoBarSeverity::Informational, t_root, t_parent);
	BOOL result = TerminateProcess(hProc, 0);

	CloseHandle(hProc);

	return (bool)result;
}

// [2] PostMessage
BOOL CALLBACK TerminateProcessByWindow(HWND hwnd, LPARAM lparam) {
	DWORD* procPid = (DWORD*)lparam;
	DWORD windowPid;

	// Get thread ID
	GetWindowThreadProcessId(hwnd, &windowPid);

	if (windowPid == *procPid) {
		std::wstring content = L"向窗口发送WM消息, 目标PID: " + std::to_wstring(windowPid);
		CreateInfoBarAndDisplay(L"Terminator", content.c_str(), InfoBarSeverity::Informational, t_root, t_parent);
		// Post window messages to let the window exit itself
		PostMessage(hwnd, WM_CLOSE, 0, 0);
		PostMessage(hwnd, WM_DESTROY, 0, 0);
		PostMessage(hwnd, WM_NCDESTROY, 0, 0);
		PostMessage(hwnd, WM_QUIT, 0, 0);
		PostMessage(hwnd, WM_COMMAND | SC_CLOSE, 0, 0);
		PostMessage(hwnd, WM_ENDSESSION, 0, 0);
	}
	if (GetLastError() != 0 && GetLastError() != 1400 && GetLastError() != 6) {
		CreateInfoBarAndDisplay(L"Terminator", L"出现了一些问题, 程序可能未按预期运行！", InfoBarSeverity::Warning, t_root, t_parent);
	}
	return TRUE;
}

// [3] NtTerminateProcess
bool TerminateProcessNT(int pid) {
	HANDLE hProc = GetProcessByPID(pid);

	if (!hProc) {
		return false;
	}

	CreateInfoBarAndDisplay(L"Terminator", L"正在获取NtTerminateProcess的地址...", InfoBarSeverity::Informational, t_root, t_parent);

	auto ntdll = GetModuleHandleW(L"ntdll.dll");
	if (ntdll) {
		P_NtTerminateProcess NtTerminateProcess = (P_NtTerminateProcess)GetProcAddress(ntdll, "NtTerminateProcess");
		if (NtTerminateProcess) {
			std::wstring content = L"运行NtTerminateProcess, 目标PID: " + std::to_wstring(pid);
			CreateInfoBarAndDisplay(L"Terminator", content.c_str(), InfoBarSeverity::Informational, t_root, t_parent);

			// Terminate process
			NTSTATUS result = NtTerminateProcess(hProc, 0);
			CloseHandle(hProc);

			return result >= 0;
		}
	}

	return false;
}

// [4] TerminateThread
bool TerminateProcessByThread(int pid) {
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, (DWORD)pid);

	if (hSnapshot && hSnapshot != INVALID_HANDLE_VALUE) {
		CreateInfoBarAndDisplay(L"Terminator", L"正在寻找线程...", InfoBarSeverity::Informational, t_root, t_parent);
		THREADENTRY32 threadEntry = { sizeof(threadEntry) };
		BOOL finder = Thread32First(hSnapshot, &threadEntry);

		for (; finder; finder = Thread32Next(hSnapshot, &threadEntry)) {
			// Find process thread
			if (threadEntry.th32OwnerProcessID == (DWORD)pid) {
				std::wstring content = L"结束线程, 目标PID: " + std::to_wstring(threadEntry.th32ThreadID);
				CreateInfoBarAndDisplay(L"Terminator", content.c_str(), InfoBarSeverity::Informational, t_root, t_parent);
				HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, threadEntry.th32ThreadID);

				if (hThread) {
					// Terminate process thread
					BOOL result = TerminateThread(hThread, 0);
					if (!result) {
						std::wstring errorMsg = L"无法结束进程, 目标PID: " + std::to_wstring(threadEntry.th32ThreadID);
						CreateInfoBarAndDisplay(L"Terminator", errorMsg.c_str(), InfoBarSeverity::Error, t_root, t_parent);
					}
				}

				CloseHandle(hThread);
				CloseHandle(hSnapshot);

				if (!IsProcessAlive(pid)) break;
			}
		}
	}
	CloseHandle(hSnapshot);
	return false;
}

// [5] TerminateJobObject
bool TerminateProcessByJob(int pid) {
	HANDLE hProc = GetProcessByPID(pid);

	if (!hProc) {
		return false;
	}

	HANDLE hJob = CreateJobObject(NULL, NULL);

	// Assign process to job in order to terminate the process with job together
	CreateInfoBarAndDisplay(L"Terminator", L"正在分配任务...", InfoBarSeverity::Informational, t_root, t_parent);
	if (!AssignProcessToJobObject(hJob, hProc)) {
		CreateInfoBarAndDisplay(L"Terminator", L"无法分配任务", InfoBarSeverity::Error, t_root, t_parent);
		CloseHandle(hProc);
		CloseHandle(hJob);
		return false;
	}

	// Terminate the process with job together
	CreateInfoBarAndDisplay(L"Terminator", L"正在退出任务...", InfoBarSeverity::Informational, t_root, t_parent);
	BOOL result = TerminateJobObject(hJob, 0);
	CloseHandle(hProc);
	CloseHandle(hJob);

	return (bool)result;
}

// [6] EndTask
BOOL CALLBACK TerminateProcessForce(HWND hwnd, LPARAM lparam) {
	DWORD* procPid = (DWORD*)lparam;
	DWORD windowPid;

	if (EndTaskAddr == nullptr) {
		CreateInfoBarAndDisplay(L"Terminator", L"正在获取EndTask的地址...", InfoBarSeverity::Informational, t_root, t_parent);
		EndTaskAddr = (P_EndTask)GetProcAddress(GetModuleHandleW(L"user32.dll"), "EndTask");
	}

	// Get thread ID
	GetWindowThreadProcessId(hwnd, &windowPid);

	if (windowPid == *procPid) {
		std::wstring content = L"对线程进行EndTask, 目标PID: " + std::to_wstring(windowPid);
		CreateInfoBarAndDisplay(L"Terminator", content.c_str(), InfoBarSeverity::Informational, t_root, t_parent);
		// End task
		BOOL status = EndTaskAddr(hwnd, FALSE, TRUE);
		if (!status) {
			std::wstring errorMsg = L"无法结束线程, 目标PID: " + std::to_wstring(windowPid);
			CreateInfoBarAndDisplay(L"Terminator", errorMsg.c_str(), InfoBarSeverity::Error, t_root, t_parent);
		}
	}
	if (GetLastError() != 0 && GetLastError() != 1400 && GetLastError() != 6) {
		CreateInfoBarAndDisplay(L"Terminator", L"出现了一些问题, 程序可能未按预期运行！", InfoBarSeverity::Warning, t_root, t_parent);
	}
	return TRUE;
}

// [7] Inject >> WriteProcessMemory, ExitProcess
bool TerminateProcessByInjection(int pid) {
	HANDLE hProc = GetProcessByPID(pid);

	if (!hProc) {
		return false;
	}

	PACKAGE_SUICIDE package{ 0 };

	// Get process address dynamicly
	CreateInfoBarAndDisplay(L"Terminator", L"正在获取ExitProcess的地址...", InfoBarSeverity::Informational, t_root, t_parent);
	package.adExitProcess = (intptr_t)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "ExitProcess");

	// Allocate virtual memory to access process
	CreateInfoBarAndDisplay(L"Terminator", L"正在尝试 VirtualAllocEx() 1st...", InfoBarSeverity::Informational, t_root, t_parent);
	LPVOID lpData = VirtualAllocEx(hProc, NULL, sizeof package, MEM_COMMIT, PAGE_READWRITE);
	if (!lpData) {
		CreateInfoBarAndDisplay(L"Terminator", L"VirtualAllocEx() 1st 失败", InfoBarSeverity::Error, t_root, t_parent);
		return false;
	}

	// Write read memory to process
	CreateInfoBarAndDisplay(L"Terminator", L"正在尝试 WriteProcessMemory() 1st...", InfoBarSeverity::Informational, t_root, t_parent);
	SIZE_T idk = 0;
	BOOL wpm = WriteProcessMemory(hProc, lpData, &package, sizeof package, &idk);
	if (!wpm) {
		CreateInfoBarAndDisplay(L"Terminator", L"WriteProcessMemory() 1st 失败", InfoBarSeverity::Error, t_root, t_parent);
		return false;
	}

	// Allocate virtual memory to execute command
	CreateInfoBarAndDisplay(L"Terminator", L"正在尝试 VirtualAllocEx() 2nd...", InfoBarSeverity::Informational, t_root, t_parent);
	LPVOID lpCode = VirtualAllocEx(hProc, NULL, 0x1000, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (!lpCode) {
		CreateInfoBarAndDisplay(L"Terminator", L"VirtualAllocEx() 2nd 失败", InfoBarSeverity::Error, t_root, t_parent);
		return false;
	}

	// Write exit memory to process
	CreateInfoBarAndDisplay(L"Terminator", L"正在尝试 WriteProcessMemory() 2nd...", InfoBarSeverity::Informational, t_root, t_parent);
	wpm = WriteProcessMemory(hProc, lpCode, (LPVOID)&RemoteThread_ExitProcess, 0x1000, &idk);
	if (!wpm) {
		CreateInfoBarAndDisplay(L"Terminator", L"WriteProcessMemory() 2nd 失败", InfoBarSeverity::Error, t_root, t_parent);
		return false;
	}

	// Start remote thread
	CreateInfoBarAndDisplay(L"Terminator", L"正在创建线程...", InfoBarSeverity::Informational, t_root, t_parent);
	HANDLE hThread = CreateRemoteThread(hProc, NULL, 0, (LPTHREAD_START_ROUTINE)lpCode, lpData, 0, NULL);
	if (!hThread) {
		CreateInfoBarAndDisplay(L"Terminator", L"无法创建线程", InfoBarSeverity::Error, t_root, t_parent);
		return false;
	}

	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hProc);
	CloseHandle(hThread);

	return true;
}

// [8] DebugSetProcessKillOnExit
bool TerminateProcessByDebugger(int pid) {
	std::wstring content = L"正在附加调试器, 目标PID: " + std::to_wstring(pid);
	CreateInfoBarAndDisplay(L"Terminator", content.c_str(), InfoBarSeverity::Informational, t_root, t_parent);
	if (!DebugActiveProcess((DWORD)pid)) {
		CreateInfoBarAndDisplay(L"Terminator", L"无法附加调试器", InfoBarSeverity::Error, t_root, t_parent);
		return false;
	}

	CreateInfoBarAndDisplay(L"Terminator", L"尝试退出...", InfoBarSeverity::Informational, t_root, t_parent);
	if (!DebugSetProcessKillOnExit(TRUE)) {
		CreateInfoBarAndDisplay(L"Terminator", L"无法退出程序或设置命令", InfoBarSeverity::Error, t_root, t_parent);
		return false;
	}

	return true;
}

// [9] WinStationTerminateProcess
bool TerminateProcessWinsta(int pid) {
	PWSTR hServer = new WCHAR[45];

	HMODULE hWinsta = LoadLibraryW(L"winsta.dll");
	if (!hWinsta) {
		CreateInfoBarAndDisplay(L"Terminator", L"无法加载winsta.dll！", InfoBarSeverity::Error, t_root, t_parent);
		return false;
	}

	// Get process addresses
	fpWinStationOpenServerW = (P_WinStationOpenServerW)GetProcAddress(hWinsta, "WinStationOpenServerW");
	if (fpWinStationOpenServerW == NULL) {
		CreateInfoBarAndDisplay(L"Terminator", L"无法加载winsta.dll的相关函数！", InfoBarSeverity::Error, t_root, t_parent);
		FreeLibrary(hWinsta);
		return false;
	}

	fpWinStationTerminateProcess = (P_WinStationTerminateProcess)GetProcAddress(hWinsta, "WinStationTerminateProcess");
	if (fpWinStationTerminateProcess == NULL) {
		CreateInfoBarAndDisplay(L"Terminator", L"无法加载winsta.dll的相关函数！", InfoBarSeverity::Error, t_root, t_parent);
		FreeLibrary(hWinsta);
		return false;
	}

	wcscpy_s(hServer, 45 * sizeof(WCHAR), L"(null)");

	// Kill process
	std::wstring killMsg = L"运行WinStationTerminateProcess, 目标PID: " + std::to_wstring(pid);
	CreateInfoBarAndDisplay(L"Terminator", killMsg.c_str(), InfoBarSeverity::Informational, t_root, t_parent);
	if (!WinstaKillProcess(hServer, (DWORD)pid, 0)) {
		FreeLibrary(hWinsta);
		return false;
	}

	FreeLibrary(hWinsta);
	return true;
}

// [10] PatchThread >> ExitProcess
bool TerminateProcessByPatch(int pid) {
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, (DWORD)pid);
	HANDLE hProc = GetProcessByPID(pid);

	if (!hProc) {
		return false;
	}

	if (hSnapshot != INVALID_HANDLE_VALUE) {
		CreateInfoBarAndDisplay(L"Terminator", L"正在寻找线程...", InfoBarSeverity::Informational, t_root, t_parent);
		THREADENTRY32 threadEntry = { sizeof(threadEntry) };
		BOOL finder = Thread32First(hSnapshot, &threadEntry);

		for (; finder; finder = Thread32Next(hSnapshot, &threadEntry)) {
			// Find process thread
			if (threadEntry.th32OwnerProcessID == (DWORD)pid) {
				std::wstring content = L"正在修补线程, 目标PID: " + std::to_wstring(threadEntry.th32ThreadID);
				CreateInfoBarAndDisplay(L"Terminator", content.c_str(), InfoBarSeverity::Informational, t_root, t_parent);

				bool result = PatchThread64(threadEntry.th32ThreadID);

				if (!result) {
					std::wstring errorMsg = L"无法修补线程, 目标PID: " + std::to_wstring(threadEntry.th32ThreadID);
					CreateInfoBarAndDisplay(L"Terminator", errorMsg.c_str(), InfoBarSeverity::Error, t_root, t_parent);
				}

				if (!IsProcessAlive(pid)) break;
			}
		}
	}

	CloseHandle(hSnapshot);
	CloseHandle(hProc);
	return true;
}

// [11] MemoryZero >> WriteProcessMemory
bool TerminateProcessByMemoryZero(int pid) {
	HANDLE hProc = GetProcessByPID(pid);

	if (!hProc) {
		return false;
	}

	CreateInfoBarAndDisplay(L"Terminator", L"正在枚举程序的内存区域...", InfoBarSeverity::Informational, t_root, t_parent);

	MEMORY_BASIC_INFORMATION mbi;
	SIZE_T offset = 0;
	int regionsZeroed = 0;

	while (VirtualQueryEx(hProc, (LPCVOID)offset, &mbi, sizeof(mbi))) {
		// Skip protected area
		if (mbi.State == MEM_COMMIT &&
			mbi.Protect != PAGE_NOACCESS &&
			mbi.Protect != PAGE_GUARD) {

			SIZE_T regionSize = mbi.RegionSize;
			PVOID zeroBuffer = calloc(regionSize, 1);

			if (zeroBuffer) {
				SIZE_T bytesWritten = 0;
				// Write process memory to zero
				BOOL result = WriteProcessMemory(hProc, mbi.BaseAddress, zeroBuffer, regionSize, &bytesWritten);

				if (result && bytesWritten > 0) {
					regionsZeroed++;
					std::wstring memoryMsg = L"清空位于 " + std::to_wstring(reinterpret_cast<uintptr_t>(mbi.BaseAddress)) +
											L" 的内存, 大小: " + std::to_wstring(regionSize) + L" bytes";
					CreateInfoBarAndDisplay(L"Terminator", memoryMsg.c_str(), InfoBarSeverity::Informational, t_root, t_parent);
				}

				free(zeroBuffer);
			}
		}

		offset = (SIZE_T)mbi.BaseAddress + mbi.RegionSize;
		if (offset == 0) break;
	}

	std::wstring resultMsg = L"清空了 " + std::to_wstring(regionsZeroed) + L" 块内存区域!";
	CreateInfoBarAndDisplay(L"Terminator", resultMsg.c_str(), InfoBarSeverity::Informational, t_root, t_parent);
	CloseHandle(hProc);

	return regionsZeroed > 0;
}

bool DoTerminateProcess(int method, int pid, XamlRoot& xamlRoot, Panel panel) {
	t_root = xamlRoot;
	t_parent = panel;
	switch (method) {
	case 1:
		return TerminateProcessNormally(pid);
		break;
	case 2:
		return (bool)EnumWindows(TerminateProcessByWindow, (LPARAM)&pid);
		break;
	case 3:
		return TerminateProcessNT(pid);
		break;
	case 4:
		return TerminateProcessByThread(pid);
		break;
	case 5:
		return TerminateProcessByJob(pid);
		break;
	case 6:
		return (bool)EnumWindows(TerminateProcessForce, (LPARAM)&pid);
		break;
	case 7:
		return TerminateProcessByInjection(pid);
		break;
	case 8:
		return TerminateProcessByDebugger(pid);
		break;
	case 9:
		return TerminateProcessWinsta(pid);
		break;
	case 11:
		return TerminateProcessByPatch(pid);
		break;
	case 12:
		return TerminateProcessByMemoryZero(pid);
		break;
	default:
		CreateInfoBarAndDisplay(L"Terminator", L"未知方法!", InfoBarSeverity::Error, t_root, t_parent);
		return false;
		break;
	}
	return false;
}

bool TerminateProcessByKernel(int pid, std::wstring& content) {
	if (KernelInstance::ZwTerminateProcess0(pid)) {
		content = L"成功强制终止了PID为" + std::to_wstring(pid) + L"的进程！";
		return true;
	}
	content = L"无法终止PID为" + std::to_wstring(pid) + L"的进程！";
	return false;
}