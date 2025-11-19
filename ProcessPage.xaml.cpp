#pragma once
#include "pch.h"
#include "ProcessPage.xaml.h"
#if __has_include("ProcessPage.g.cpp")
#include "ProcessPage.g.cpp"
#endif

#include <Utils/Terminator.h>
#include <Utils/Elevator.h>
#include <Utils/Utils.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Pickers.h>
#include <winrt/Microsoft.UI.h>
#include <winrt/Microsoft.UI.Text.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>
#include <winrt/Microsoft.UI.Interop.h>
#include <Utils/Config.h>
#include <vector>
#include <memory>
#include <sstream>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Text;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Composition::SystemBackdrops;
using namespace Windows::Foundation;
using namespace Windows::ApplicationModel;
using namespace Windows::Storage;
using namespace Windows::Storage::Pickers;


namespace winrt::StarlightGUI::implementation
{
    ProcessPage::ProcessPage() {
        InitializeComponent();
    }

    winrt::fire_and_forget ProcessPage::TerminatorButton_Clicked(IInspectable const& sender, RoutedEventArgs const& e)
    {
        hstring processName;
        int methodIndex = TerminatorComboBox().SelectedIndex() + 1; // Add 1 because our methoded start at 1

        TerminatorEditBox().TextDocument().GetText(TextGetOptions::NoHidden, processName);

        std::wstring content;
        InfoBar infobar{ nullptr };

        if (!processName.empty() && methodIndex > 0) {
            bool confirmed = true;
            if (methodIndex == 10 && ReadConfig("dangerous_confirm", true)) {
                ContentDialog dialog = CreateContentDialog(L"警告", L"该方法需要加载驱动并获取内核级权限，可能导致系统出现问题，确定继续吗？\n必须以管理员身份运行！", L"关闭", XamlRoot());
                dialog.TitleTemplate(GetContentDialogInfoTemplate());
                dialog.IsPrimaryButtonEnabled(true);
                dialog.IsSecondaryButtonEnabled(true);
                dialog.PrimaryButtonText(L"确定");
                dialog.SecondaryButtonText(L"取消");
                dialog.DefaultButton(ContentDialogButton::Primary);
                auto result = co_await dialog.ShowAsync();

                if (result == ContentDialogResult::Secondary || result == ContentDialogResult::None) {
                    confirmed = false;
                }
            }
            else if (methodIndex > 10 && ReadConfig("dangerous_confirm", true)) {
                ContentDialog dialog = CreateContentDialog(L"警告", L"该方法使用了不稳定的方法或漏洞，可能导致系统出现问题，确定继续吗？\n以非管理员身份运行可能会导致Starlight GUI崩溃！", L"关闭", XamlRoot());
                dialog.TitleTemplate(GetContentDialogInfoTemplate());
                dialog.IsPrimaryButtonEnabled(true);
                dialog.IsSecondaryButtonEnabled(true);
                dialog.PrimaryButtonText(L"确定");
                dialog.SecondaryButtonText(L"取消");
                dialog.DefaultButton(ContentDialogButton::Primary);
                auto result = co_await dialog.ShowAsync();

                if (result == ContentDialogResult::Secondary || result == ContentDialogResult::None) {
                    confirmed = false;
                }
            }

            if (!confirmed) {
                co_return;
            }

            std::wstring wideProcessName = std::wstring_view(processName.c_str()).data();

            if (wideProcessName.find(L".exe") == std::wstring::npos) {
                wideProcessName += L".exe";
            }


            // Process input to pid
            int pid = GetPID(winrt::to_string(wideProcessName));

            if (methodIndex == 10) {
                if (IsRunningAsAdmin()) {

                    EnableDebugPrivilege();
                    if (TerminateProcessByKernel(pid, content)) {
                        infobar = CreateInfoBar(L"成功", content.c_str(), InfoBarSeverity::Success, XamlRoot());
                    }
                    else {
                        if (GetLastError() == 2148204812 || GetLastError() == 577) {
                            content = L"驱动证书验证失败，请关闭签名验证！";
                        }
                        else {
                            content += L" (" + std::to_wstring(GetLastError()) + L")";
                        }
                        infobar = CreateInfoBar(L"失败", content.c_str(), InfoBarSeverity::Error, XamlRoot());
                    }
                }
                else {
                    infobar = CreateInfoBar(L"警告", L"请以管理员身份运行！", InfoBarSeverity::Warning, XamlRoot());
                }
            }
            else {
                if (pid != 0) {
                    if (DoTerminateProcess(methodIndex, pid, XamlRoot(), InfoBarPanel())) {
                        content = L"成功终止了PID为" + std::to_wstring(pid) + L"的进程！";
                        infobar = CreateInfoBar(L"成功", content.c_str(), InfoBarSeverity::Success, XamlRoot());
                    }
                    else {
                        content = L"无法终止PID为" + std::to_wstring(pid) + L"的进程！";
                        infobar = CreateInfoBar(L"失败", content.c_str(), InfoBarSeverity::Error, XamlRoot());
                    }
                }
                else {
                    infobar = CreateInfoBar(L"错误", L"进程名/PID不存在！", InfoBarSeverity::Error, XamlRoot());
                }
            }
        }
        else {
            infobar = CreateInfoBar(L"错误", L"进程名/PID为空！", InfoBarSeverity::Error, XamlRoot());
        }

        DisplayInfoBar(infobar, InfoBarPanel());

        co_return;
    }

    winrt::fire_and_forget ProcessPage::ElevatorButton_Clicked(IInspectable const& sender, RoutedEventArgs const& e)
    {
        hstring processName;
        bool fullPrivileges = ReadConfig("elevator_full_privileges", true);

        ElevatorEditBox().TextDocument().GetText(TextGetOptions::NoHidden, processName);

        std::wstring content;
        InfoBar infobar{ nullptr };

        if (!processName.empty()) {
            std::wstring wideProcessName = std::wstring_view(processName.c_str()).data();

            if (wideProcessName.find(L"\"") != std::wstring::npos) {
                wideProcessName.erase(wideProcessName.end());
                wideProcessName.erase(wideProcessName.begin());
            }

            if (wideProcessName.find(L".exe") == std::wstring::npos) {
                wideProcessName += L".exe";
            }

            if (IsRunningAsAdmin()) {
                int result = CreateProcessElevated(wideProcessName, fullPrivileges, XamlRoot(), InfoBarPanel());

                if (result != 1) {
                    content = L"程序启动成功，PID: " + std::to_wstring(result);
                    infobar = CreateInfoBar(L"成功", content.c_str(), InfoBarSeverity::Success, XamlRoot());
                }
                else {
                    infobar = CreateInfoBar(L"失败", L"程序启动失败！", InfoBarSeverity::Error, XamlRoot());
                }
            }
            else {
                infobar = CreateInfoBar(L"警告", L"请以管理员身份运行！", InfoBarSeverity::Error, XamlRoot());
            }
        }
        else {
            infobar = CreateInfoBar(L"错误", L"进程名/PID为空！", InfoBarSeverity::Error, XamlRoot());
        }

        DisplayInfoBar(infobar, InfoBarPanel());

        co_return;
    }

    /*winrt::fire_and_forget ProcessPage::DriverLoaderExploreButton_Clicked(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        HWND hWnd = GetProcessFirstWindowHandle();

        FileOpenPicker picker = FileOpenPicker();

        picker.as<IInitializeWithWindow>()->Initialize(hWnd);
        picker.SuggestedStartLocation(PickerLocationId::ComputerFolder);
        picker.FileTypeFilter().Append(L".sys");

        auto file = co_await picker.PickSingleFileAsync();

        if (file != nullptr && file.IsAvailable()) {
            if (file.FileType() == L".sys") {
                DriverLoaderEditBox().TextDocument().SetText(TextSetOptions::None, file.Path());
                CreateInfoBarAndDisplay(L"成功", L"已导入文件路径！", InfoBarSeverity::Success, XamlRoot(), InfoBarPanel());
            }
            else {
                CreateInfoBarAndDisplay(L"错误", L"请导入.sys文件！", InfoBarSeverity::Warning, XamlRoot(), InfoBarPanel());
            }
        }
    } */

    winrt::fire_and_forget ProcessPage::DriverLoaderLoadButton_Clicked(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e) {
        if (!IsRunningAsAdmin()) {
            CreateInfoBarAndDisplay(L"警告", L"请以管理员身份运行！", InfoBarSeverity::Warning, XamlRoot(), InfoBarPanel());
            co_return;
        }

        try {
            winrt::hstring filePath = L"";

            DriverLoaderEditBox().Document().GetText(TextGetOptions::NoHidden, filePath);

            if (filePath.empty()) {
                CreateInfoBarAndDisplay(L"错误", L"文件路径为空！", InfoBarSeverity::Error, XamlRoot(), InfoBarPanel());
                co_return;
            }

            auto file = co_await winrt::Windows::Storage::StorageFile::GetFileFromPathAsync(filePath);

            if (!file) {
                CreateInfoBarAndDisplay(L"错误", L"文件不存在: " + filePath, InfoBarSeverity::Error, XamlRoot(), InfoBarPanel());
                co_return;
            }

            if (file.FileType() != L".sys") {
                CreateInfoBarAndDisplay(L"错误", L"文件不是驱动文件(.sys)！" + filePath, InfoBarSeverity::Error, XamlRoot(), InfoBarPanel());
                co_return;
            }

            CreateInfoBarAndDisplay(L"信息", L"将要加载的驱动文件: " + file.Name(), InfoBarSeverity::Informational, XamlRoot(), InfoBarPanel());

            std::wstring content;
            InfoBar infobar;
            bool confirmed = true;
            bool bypass = ReadConfig("bypass_signature", false);

            if (bypass) {
                ContentDialog dialog = CreateContentDialog(L"警告", L"该方法需要加载驱动并获取内核级权限，可能导致系统出现问题，确定继续吗？\n必须以管理员身份运行！", L"关闭", XamlRoot());
                dialog.TitleTemplate(GetContentDialogInfoTemplate());
                dialog.IsPrimaryButtonEnabled(true);
                dialog.IsSecondaryButtonEnabled(true);
                dialog.PrimaryButtonText(L"确定");
                dialog.SecondaryButtonText(L"取消");
                dialog.DefaultButton(ContentDialogButton::Primary);
                auto result = co_await dialog.ShowAsync();

                if (result == ContentDialogResult::Secondary || result == ContentDialogResult::None) {
                    confirmed = false;
                }
            }

            if (confirmed) {
                bool result = true;

                CreateInfoBarAndDisplay(L"g_ciOptions", std::to_wstring(KernelBase::GetCIOption()).c_str(), InfoBarSeverity::Warning, XamlRoot(), InfoBarPanel());

                if (bypass) {
                    CreateInfoBarAndDisplay(L"警告", L"该操作可能随机导致蓝屏(PatchGuard)，若出现异常请再次尝试！", InfoBarSeverity::Warning, XamlRoot(), InfoBarPanel());
                    result = KernelBase::HackCIOption(0);
                    if (result) CreateInfoBarAndDisplay(L"信息", L"Set g_ciOptions = 0", InfoBarSeverity::Informational, XamlRoot(), InfoBarPanel());
                    CreateInfoBarAndDisplay(L"g_ciOptions", std::to_wstring(KernelBase::GetCIOption()).c_str(), InfoBarSeverity::Warning, XamlRoot(), InfoBarPanel());
                }

                if (result) result = DriverUtils::LoadDriver(filePath.c_str(), file.Name().c_str(), unused);

                if (bypass) {
                    result = KernelBase::HackCIOption(6);
                    if (result) CreateInfoBarAndDisplay(L"信息", L"Set g_ciOptions = 6", InfoBarSeverity::Informational, XamlRoot(), InfoBarPanel());
                    CreateInfoBarAndDisplay(L"g_ciOptions", std::to_wstring(KernelBase::GetCIOption()).c_str(), InfoBarSeverity::Warning, XamlRoot(), InfoBarPanel());
                }

                if (result) {
                    infobar = CreateInfoBar(L"成功", L"驱动加载成功！", InfoBarSeverity::Success, XamlRoot());
                }
                else {
                    content = L"驱动加载失败: " + content;
                    infobar = CreateInfoBar(L"失败", content.c_str(), InfoBarSeverity::Error, XamlRoot());
                }

                CreateInfoBarAndDisplay(L"g_ciOptions", std::to_wstring(KernelBase::GetCIOption()).c_str(), InfoBarSeverity::Warning, XamlRoot(), InfoBarPanel());

                DisplayInfoBar(infobar, InfoBarPanel());
            }
        }
        catch (winrt::hresult_error const& ex) {
            winrt::hstring errorMessage;

            switch (ex.code()) {
            case E_INVALIDARG:
                errorMessage = L"未知格式";
                break;
            case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):
                errorMessage = L"文件不存在";
                break;
            case HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND):
                errorMessage = L"路径不存在";
                break;
            case HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED):
                errorMessage = L"文件拒绝访问";
                break;
            default:
                errorMessage = winrt::to_hstring(ex.code());
                break;
            }

            CreateInfoBarAndDisplay(L"错误", L"执行任务时出错: " + errorMessage, InfoBarSeverity::Error, XamlRoot(), InfoBarPanel());
        }
    }
}
