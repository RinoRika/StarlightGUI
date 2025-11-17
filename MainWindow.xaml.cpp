#include "pch.h"
#include "Utils/Config.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include <winrt/Microsoft.UI.h>
#include <winrt/Microsoft.UI.Windowing.h>
#include <winrt/Microsoft.UI.Dispatching.h>
#include <winrt/Microsoft.UI.Composition.SystemBackdrops.h>
#include <winrt/Windows.UI.h>
#include <Windows.h>
#include <winrt/Windows.UI.Xaml.Interop.h>
#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Microsoft::UI::Dispatching;
using namespace Microsoft::UI::Composition::SystemBackdrops;

namespace winrt::StarlightGUI::implementation
{
    // 静态成员变量，用于窗口过程
    static MainWindow* g_mainWindowInstance = nullptr;
    static std::string background_type;

    MainWindow::MainWindow()
    {
        InitializeComponent();

        // 设置自定义标题栏
        auto windowNative{ this->try_as<::IWindowNative>() };
        HWND hWnd{ 0 };
        windowNative->get_WindowHandle(&hWnd);

        SetWindowPos(hWnd, NULL, 0, 0, 1200, 800, SWP_NOMOVE | SWP_NOZORDER);

        // 扩展内容到标题栏
        this->ExtendsContentIntoTitleBar(true);
        this->SetTitleBar(AppTitleBar());

        // 设置背景
        LoadBackdrop();

        // 设置窗口子类化以捕获窗口状态变化
        g_mainWindowInstance = this;
        SetWindowSubclass(hWnd, &MainWindow::WindowProc, 0, 0);

        // 初始更新最大化按钮状态
        UpdateMaximizeButton();

        // 设置主页
        MainFrame().Navigate(xaml_typename<StarlightGUI::HomePage>());
        RootNavigation().SelectedItem(RootNavigation().MenuItems().GetAt(0));
    }

    MainWindow::~MainWindow()
    {
    }

    void MainWindow::InitializeComponent()
    {
        MainWindowT::InitializeComponent();
    }

    void MainWindow::LoadBackdrop()
    {
        background_type = ReadConfig("background_type", "Static");

        if (background_type == "Mica" && MicaController::IsSupported()) {
            micaBackdrop = winrt::Microsoft::UI::Xaml::Media::MicaBackdrop();
            micaBackdrop.Kind(MicaKind::BaseAlt);

            this->SystemBackdrop(micaBackdrop);
        }
        else if (background_type == "Acrylic" && DesktopAcrylicController::IsSupported()) {
            acrylicBackdrop = winrt::Microsoft::UI::Xaml::Media::DesktopAcrylicBackdrop();
            
            this->SystemBackdrop(acrylicBackdrop);
        }
        else
        {
            this->SystemBackdrop(nullptr);
        }
    }

    void MainWindow::MinimizeButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        // 最小化窗口
        auto hWnd = GetWindowHandle();
        ShowWindow(hWnd, SW_MINIMIZE);
    }

    void MainWindow::MaximizeButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        auto hWnd = GetWindowHandle();

        // 检查当前窗口状态
        WINDOWPLACEMENT wp;
        wp.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(hWnd, &wp);

        if (wp.showCmd == SW_SHOWMAXIMIZED)
        {
            // 如果已最大化，则恢复
            ShowWindow(hWnd, SW_RESTORE);
        }
        else
        {
            // 如果未最大化，则最大化
            ShowWindow(hWnd, SW_MAXIMIZE);
        }
    }

    void MainWindow::CloseButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        this->Close();
    }

    void MainWindow::UpdateMaximizeButton()
    {
        auto hWnd = GetWindowHandle();

        WINDOWPLACEMENT wp;
        wp.length = sizeof(WINDOWPLACEMENT);

        if (GetWindowPlacement(hWnd, &wp))
        {
            if (wp.showCmd == SW_SHOWMAXIMIZED)
            {
                MaximizeButtonContent().Text(L"\uE923"); // 恢复
            }
            else
            {
                MaximizeButtonContent().Text(L"\uE922"); // 最大化
            }
        }
    }

    void MainWindow::RootNavigation_ItemInvoked(Microsoft::UI::Xaml::Controls::NavigationView sender, Microsoft::UI::Xaml::Controls::NavigationViewItemInvokedEventArgs args)
    {
        if (args.IsSettingsInvoked())
        {
            MainFrame().Navigate(xaml_typename<StarlightGUI::SettingsPage>());
            return;
        }

        auto invokedItem = args.InvokedItem().try_as<winrt::hstring>();

        if (invokedItem == L"主页")
        {
            MainFrame().Navigate(xaml_typename<StarlightGUI::HomePage>());
            RootNavigation().SelectedItem(RootNavigation().MenuItems().GetAt(0));
        }
        else if (invokedItem == L"进程管理") {
            MainFrame().Navigate(xaml_typename<StarlightGUI::TaskPage>());
            RootNavigation().SelectedItem(RootNavigation().MenuItems().GetAt(1));
        }
        else if (invokedItem == L"系统管理") {
            MainFrame().Navigate(xaml_typename<StarlightGUI::ProcessPage>());
            RootNavigation().SelectedItem(RootNavigation().MenuItems().GetAt(2));
        }
    }

    HWND MainWindow::GetWindowHandle()
    {
        auto windowNative{ this->try_as<::IWindowNative>() };
        HWND hWnd{ 0 };
        windowNative->get_WindowHandle(&hWnd);
        return hWnd;
    }

    LRESULT CALLBACK MainWindow::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {
        switch (message)
        {
        case WM_WINDOWPOSCHANGED:
            if (g_mainWindowInstance)
            {
                g_mainWindowInstance->HandleWindowPosChanged();
            }
            break;
        case WM_NCDESTROY:
            RemoveWindowSubclass(hWnd, &MainWindow::WindowProc, uIdSubclass);
            break;
        }

        return DefSubclassProc(hWnd, message, wParam, lParam);
    }

    void MainWindow::HandleWindowPosChanged()
    {
        // 在UI线程上更新最大化按钮
        auto dispatcherQueue = this->DispatcherQueue();
        if (dispatcherQueue)
        {
            dispatcherQueue.TryEnqueue(DispatcherQueuePriority::Normal, [this]() {
                UpdateMaximizeButton();
                });
        }
    }
}