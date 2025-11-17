#pragma once

#include <pch.h>
#include <Utils/ProcessInfo.h>

using namespace winrt;

namespace winrt::StarlightGUI::implementation {
	class TaskUtils {
	public:
		static void EnsurePrivileges();

		static bool Task_TerminateProcess(StarlightGUI::ProcessInfo pi);

		static bool Task_EndTask(StarlightGUI::ProcessInfo pi);

		static bool Task_TerminateThread(StarlightGUI::ProcessInfo pi);

		static bool Task_TerminateProcessForce(StarlightGUI::ProcessInfo pi);

		static bool Task_EnableProcessPerformanceMode(StarlightGUI::ProcessInfo pi);

		static SIZE_T Task_GetProcessWorkingSet(HANDLE hProc);
		
		static void TaskUtils::FetchProcessCpuUsage(std::map<DWORD, hstring>& processCpuTable);

		static bool Task_CopyToClipboard(std::wstring str);

		static bool Task_OpenFolderAndSelectFile(std::wstring filePath);

		static bool Task_OpenFileProperties(std::wstring filePath);
	private:
		static BOOL CALLBACK EndTaskByWindow(HWND hwnd, LPARAM lparam);

		static bool EnableDebugPrivilege();
	};
}