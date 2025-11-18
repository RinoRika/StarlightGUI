#pragma once

#include <windows.h>

/*
* 传递进程PID和所需访问权限以打开进程句柄
* 内核函数: ZwOpenProcess()
*/
typedef struct _ZWOPENPROCESS_INPUT { // 输入结构体
	DWORD pid;              // PID
	DWORD desiredAccess;    // 访问权限
} ZWOPENPROCESS_INPUT, * PZWOPENPROCESS_INPUT;

typedef struct _ZWOPENPROCESS_OUTPUT { // 输出结构体
	HANDLE hProcess;       // 进程句柄
	NTSTATUS status;       // 状态码
} PZWOPENPROCESS_OUTPUT, * PZWOPENPROCESS_OUTPUT;
// 内核态打开的句柄是全局的，我们可以使用这个句柄，但是不能直接CloseHandle，因为这个方法只会关闭用户态进程的句柄，所以需要其他办法
// 或许是ZwDuplicateObject()？把句柄复制到用户态进程中，然后关闭内核态的句柄

/*
* 如果我们不能做到上面这一点，我们也需要一个额外的内核关闭句柄的方法
* 内核函数: ZwClose()
*/
typedef struct _ZWCLOSE_INPUT { // 输入结构体
	HANDLE hObject;        // 进程句柄
} ZWCLOSE_INPUT, * PZWCLOSE_INPUT;
// 不需要输出

/*
* 一个完整的结构体，传输TaskPage中需要的所有信息
* 我们需要这些内容：进程名称，PID，内存使用，CPU使用，描述，图标
* 在用户态，我们的思路是：CreateToolhelp32Snapshot -> Process32First/Next -> OpenProcess -> QueryFullProcessImageNameW(可执行文件路径) ->
  K32QueryWorkingSet(这个可以查私有工作集，更准)/K32GetProcessMemoryInfo -> GetFileVersionInfo(描述) -> 查询PROCESSENTRY32W(PID和名称) ->
  NtQuerySystemInformation(CPU使用, 查KernelTime) -> SHGetFileInfo(图标)
* 并不是所有功能都需要在内核态实现，我们假设是这样
*/
typedef struct _PROCESS_INFO {
	WCHAR processName[MAX_PATH];	// 进程名称
	WCHAR executablePath[MAX_PATH]; // 可执行文件路径
	DWORD pid;						// 进程ID
	SIZE_T workingSetSize;			// 内存使用
	LARGE_INTEGER userTime;			// CPU使用，用户时间
	LARGE_INTEGER kernelTime;		// CPU使用，内核时间
	WCHAR description[256];			// 进程描述
} PROCESS_INFO, * PPROCESS_INFO;

typedef struct _PROCESS_INFO_OUTPUT {
	PROCESS_INFO processInfos[1];	// 进程信息数组，长度在内核态动态分配
	DWORD processCount;				// 进程数量
} PROCESS_INFO_OUTPUT, * PPROCESS_INFO_OUTPUT;

/*
* 终止进程
* 内核函数: ZwTerminateProcess()
*/
typedef struct _ZWTERMINATEPROCESS_INPUT { // 输入结构体
	DWORD pid;              // 进程ID
} ZWTERMINATEPROCESS_INPUT, * PZWTERMINATEPROCESS_INPUT;
// 不需要输出