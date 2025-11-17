#include "pch.h"
#include "KernelBase.h"

namespace winrt::StarlightGUI::implementation {
	bool DriverUtils::LoadKernelDriver(LPCWSTR kernelPath, std::wstring& dbgMsg) {
		SC_HANDLE hSCM, hService;

		hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if (!hSCM) {
			dbgMsg = L"无法打开SCManager";
			return false;
		}

		hService = OpenService(hSCM, L"StarlightGUI Kernel Driver", SERVICE_ALL_ACCESS);
		if (hService) {
			// Start the service if it"s not running
			SERVICE_STATUS serviceStatus;
			if (!QueryServiceStatus(hService, &serviceStatus)) {
				CloseServiceHandle(hService);
				CloseServiceHandle(hSCM);
				dbgMsg = L"无法查询服务信息";
				return false;
			}

			if (serviceStatus.dwCurrentState == SERVICE_STOPPED) {
				if (!StartService(hService, 0, nullptr)) {
					CloseServiceHandle(hService);
					CloseServiceHandle(hSCM);
					dbgMsg = L"无法启动服务";
					return false;
				}
			}

			CloseServiceHandle(hService);
			CloseServiceHandle(hSCM);
			return true;
		}
		else {
			// Create the service
			hService = CreateService(hSCM, L"StarlightGUI Kernel Driver", L"StarlightGUI Kernel Driver", SERVICE_ALL_ACCESS,
				SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START,
				SERVICE_ERROR_IGNORE, kernelPath, NULL, NULL, NULL,
				NULL, NULL);

			if (!hService) {
				CloseServiceHandle(hSCM);
				dbgMsg = L"无法创建服务";
				return false;
			}

			// Start the service
			if (!StartService(hService, 0, nullptr)) {
				CloseServiceHandle(hService);
				CloseServiceHandle(hSCM);
				dbgMsg = L"无法启动服务";
				return false;
			}

			CloseServiceHandle(hService);
			CloseServiceHandle(hSCM);
			return true;
		}
		return false;
	}

	bool DriverUtils::LoadDriver(LPCWSTR kernelPath, LPCWSTR fileName, std::wstring& dbgMsg) {
		SC_HANDLE hSCM, hService;

		hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if (!hSCM) {
			dbgMsg = L"无法打开SCManager";
			return false;
		}

		hService = OpenService(hSCM, fileName, SERVICE_ALL_ACCESS);
		if (hService) {
			// Start the service if it"s not running
			SERVICE_STATUS serviceStatus;
			if (!QueryServiceStatus(hService, &serviceStatus)) {
				CloseServiceHandle(hService);
				CloseServiceHandle(hSCM);
				dbgMsg = L"无法查询服务信息";
				return false;
			}

			if (serviceStatus.dwCurrentState == SERVICE_STOPPED) {
				if (!StartService(hService, 0, nullptr)) {
					CloseServiceHandle(hService);
					CloseServiceHandle(hSCM);
					dbgMsg = L"无法启动服务";
					return false;
				}
			}

			CloseServiceHandle(hService);
			CloseServiceHandle(hSCM);
			return true;
		}
		else {
			// Create the service
			hService = CreateService(hSCM, fileName, fileName, SERVICE_ALL_ACCESS,
				SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START,
				SERVICE_ERROR_IGNORE, kernelPath, NULL, NULL, NULL,
				NULL, NULL);

			if (!hService) {
				CloseServiceHandle(hSCM);
				dbgMsg = L"无法创建服务";
				return false;
			}

			// Start the service
			if (!StartService(hService, 0, nullptr)) {
				CloseServiceHandle(hService);
				CloseServiceHandle(hSCM);
				dbgMsg = L"无法启动服务";
				return false;
			}

			CloseServiceHandle(hService);
			CloseServiceHandle(hSCM);
			return true;
		}
		return false;
	}
}