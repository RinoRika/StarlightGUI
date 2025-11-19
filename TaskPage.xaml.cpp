#include "pch.h"
#include "TaskPage.xaml.h"
#if __has_include("TaskPage.g.cpp")
#include "TaskPage.g.cpp"
#endif

#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Foundation.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <sstream>
#include <iomanip>
#include <future>
#include <mutex>
#include <shellapi.h>
#include <Utils/TaskUtils.h>
#include <Utils/Utils.h>
#include <unordered_set>

using namespace winrt;
using namespace Microsoft::UI::Text;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Windows::Storage::Streams;
using namespace Windows::Graphics::Imaging;
using namespace Windows::System;

namespace winrt::StarlightGUI::implementation
{
    const static uint64_t KB = 1024;
    const static uint64_t MB = KB * 1024;
    const static uint64_t GB = MB * 1024;
    static std::map<hstring, std::optional<winrt::Microsoft::UI::Xaml::Media::ImageSource>> iconCache;
    static std::map<hstring, hstring> descriptionCache;
    static HDC hdc{ nullptr };
    static std::unordered_set<int> filteredPids;
    static std::vector<winrt::StarlightGUI::ProcessInfo> fullRecordedProcesses;
    static std::mutex safelock;

    TaskPage::TaskPage() {
        InitializeComponent();

        ProcessListView().ItemsSource(m_processList);
        ProcessListView().ItemContainerTransitions().Clear();
        ProcessListView().ItemContainerTransitions().Append(EntranceThemeTransition());
        TaskUtils::EnsurePrivileges();

        hdc = GetDC(NULL);

        this->Loaded([this](auto&&, auto&&) {
            LoadProcessList();

            m_refreshTimer.Interval(std::chrono::seconds(refreshInterval));
            m_refreshTimer.Tick([this](auto&&, auto&&) {
                LoadProcessList();
                });
            m_refreshTimer.Start();
            });
    }

    void TaskPage::ProcessListView_RightTapped(IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e)
    {
        auto listView = sender.as<ListView>();

        if (auto fe = e.OriginalSource().try_as<FrameworkElement>())
        {
            auto container = FindParent<ListViewItem>(fe);
            if (container)
            {
                listView.SelectedItem(container.Content());
            }
        }

        if (!listView.SelectedItem()) return;

        auto item = listView.SelectedItem().as<winrt::StarlightGUI::ProcessInfo>();

        MenuFlyout menuFlyout;

        // 选项1
        MenuFlyoutItem item1;
        item1.Icon(CreateFontIcon(L"\ue711"));
        item1.Text(L"结束进程");
        item1.Click([this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::Task_TerminateProcess(item)) {
                CreateInfoBarAndDisplay(L"成功", L"成功结束进程: " + item.Name() + L" (" + to_hstring(item.Id()) + L")", InfoBarSeverity::Success, XamlRoot(), InfoBarPanel());
                LoadProcessList();
            }
            else CreateInfoBarAndDisplay(L"失败", L"无法结束进程: " + item.Name() + L" (" + to_hstring(item.Id()) + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, XamlRoot(), InfoBarPanel());
            co_return;
            });

        // 选项2
        MenuFlyoutSubItem item2;
        item2.Icon(CreateFontIcon(L"\ue8f0"));
        item2.Text(L"结束进程 (更多)");
        MenuFlyoutItem item2_sub1;
        item2_sub1.Icon(CreateFontIcon(L"\ue733"));
        item2_sub1.Text(L"结束任务");
        item2_sub1.Click([this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::Task_EndTask(item)) {
                CreateInfoBarAndDisplay(L"成功", L"成功结束进程: " + item.Name() + L" (" + to_hstring(item.Id()) + L")", InfoBarSeverity::Success, XamlRoot(), InfoBarPanel());
                LoadProcessList();
            }
            else CreateInfoBarAndDisplay(L"失败", L"无法结束进程: " + item.Name() + L" (" + to_hstring(item.Id()) + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, XamlRoot(), InfoBarPanel());
            co_return;
            });
        item2.Items().Append(item2_sub1);
        MenuFlyoutItem item2_sub2;
        item2_sub2.Icon(CreateFontIcon(L"\ue71f"));
        item2_sub2.Text(L"结束线程");
        item2_sub2.Click([this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::Task_TerminateThread(item)) {
                CreateInfoBarAndDisplay(L"成功", L"成功结束进程: " + item.Name() + L" (" + to_hstring(item.Id()) + L")", InfoBarSeverity::Success, XamlRoot(), InfoBarPanel());
                LoadProcessList();
            }
            else CreateInfoBarAndDisplay(L"失败", L"无法结束进程: " + item.Name() + L" (" + to_hstring(item.Id()) + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, XamlRoot(), InfoBarPanel());
            co_return;
            });
        item2.Items().Append(item2_sub2);

        // 选项3
        MenuFlyoutItem item3;
        item3.Icon(CreateFontIcon(L"\ue945"));
        item3.Text(L"强制结束进程");
        item3.Click([this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::Task_TerminateProcessForce(item)) {
                CreateInfoBarAndDisplay(L"成功", L"成功强制结束进程: " + item.Name() + L" (" + to_hstring(item.Id()) + L")", InfoBarSeverity::Success, XamlRoot(), InfoBarPanel());
                LoadProcessList();
            }
            else CreateInfoBarAndDisplay(L"失败", L"无法强制结束进程: " + item.Name() + L" (" + to_hstring(item.Id()) + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, XamlRoot(), InfoBarPanel());
            co_return;
            });

        // 分割线1
        MenuFlyoutSeparator separator1;

        // 选项4
        MenuFlyoutItem item4;
        item4.Icon(CreateFontIcon(L"\uf1e8"));
        item4.Text(L"效率模式");
        item4.Click([this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::Task_EnableProcessPerformanceMode(item)) {
                CreateInfoBarAndDisplay(L"成功", L"成功为进程开启效率模式: " + item.Name() + L" (" + to_hstring(item.Id()) + L")", InfoBarSeverity::Success, XamlRoot(), InfoBarPanel());
                LoadProcessList();
            }
            else CreateInfoBarAndDisplay(L"失败", L"无法为进程开启效率模式: " + item.Name() + L" (" + to_hstring(item.Id()) + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, XamlRoot(), InfoBarPanel());
            co_return;
            });

        // 分割线2
        MenuFlyoutSeparator separator2;

        // 选项5
        MenuFlyoutSubItem item5;
        item5.Icon(CreateFontIcon(L"\ue8c8"));
        item5.Text(L"复制信息");
        MenuFlyoutItem item5_sub1;
        item5_sub1.Icon(CreateFontIcon(L"\ue8ac"));
        item5_sub1.Text(L"名称");
        item5_sub1.Click([this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::Task_CopyToClipboard(item.Name().c_str())) {
                CreateInfoBarAndDisplay(L"成功", L"已复制内容至剪贴板", InfoBarSeverity::Success, XamlRoot(), InfoBarPanel());
            }
            else CreateInfoBarAndDisplay(L"失败", L"无法复制内容至剪贴板, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, XamlRoot(), InfoBarPanel());
            co_return;
            });
        item5.Items().Append(item5_sub1);
        MenuFlyoutItem item5_sub2;
        item5_sub2.Icon(CreateFontIcon(L"\ue943"));
        item5_sub2.Text(L"PID");
        item5_sub2.Click([this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::Task_CopyToClipboard(std::to_wstring(item.Id()))) {
                CreateInfoBarAndDisplay(L"成功", L"已复制内容至剪贴板", InfoBarSeverity::Success, XamlRoot(), InfoBarPanel());
            }
            else CreateInfoBarAndDisplay(L"失败", L"无法复制内容至剪贴板, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, XamlRoot(), InfoBarPanel());
            co_return;
            });
        item5.Items().Append(item5_sub2);
        MenuFlyoutItem item5_sub3;
        item5_sub3.Icon(CreateFontIcon(L"\uec6c"));
        item5_sub3.Text(L"文件路径");
        item5_sub3.Click([this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::Task_CopyToClipboard(item.ExecutablePath().c_str())) {
                CreateInfoBarAndDisplay(L"成功", L"已复制内容至剪贴板", InfoBarSeverity::Success, XamlRoot(), InfoBarPanel());
            }
            else CreateInfoBarAndDisplay(L"失败", L"无法复制内容至剪贴板, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, XamlRoot(), InfoBarPanel());
            co_return;
            });
        item5.Items().Append(item5_sub3);

        // 选项5
        MenuFlyoutItem item6;
        item6.Icon(CreateFontIcon(L"\uec50"));
        item6.Text(L"打开文件所在位置");
        item6.Click([this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::Task_OpenFolderAndSelectFile(item.ExecutablePath().c_str())) {
                CreateInfoBarAndDisplay(L"成功", L"已打开文件夹", InfoBarSeverity::Success, XamlRoot(), InfoBarPanel());
            }
            else CreateInfoBarAndDisplay(L"失败", L"无法打开文件夹, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, XamlRoot(), InfoBarPanel());
            co_return;
            });

        // 选项6
        MenuFlyoutItem item7;
        item7.Icon(CreateFontIcon(L"\ue8ec"));
        item7.Text(L"属性");
        item7.Click([this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::Task_OpenFileProperties(item.ExecutablePath().c_str())) {
                CreateInfoBarAndDisplay(L"成功", L"已打开文件属性", InfoBarSeverity::Success, XamlRoot(), InfoBarPanel());
            }
            else CreateInfoBarAndDisplay(L"失败", L"无法打开文件属性, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, XamlRoot(), InfoBarPanel());
            co_return;
            });

        menuFlyout.Items().Append(item1);
        menuFlyout.Items().Append(item2);
        menuFlyout.Items().Append(item3);
        menuFlyout.Items().Append(separator1);
        menuFlyout.Items().Append(item4);
        menuFlyout.Items().Append(separator2);
        menuFlyout.Items().Append(item5);
        menuFlyout.Items().Append(item6);
        menuFlyout.Items().Append(item7);

        menuFlyout.ShowAt(listView, e.GetPosition(listView));
    }

    winrt::Windows::Foundation::IAsyncAction TaskPage::LoadProcessList()
    {
        if (m_isLoadingProcesses) {
            co_return;
        }

        m_isLoadingProcesses = true;

        auto start = std::chrono::high_resolution_clock::now();

        auto lifetime = get_strong();
        int selectedItem = -1;
        if (ProcessListView().SelectedItem()) selectedItem = ProcessListView().SelectedItem().as<winrt::StarlightGUI::ProcessInfo>().Id();

        co_await winrt::resume_background();

        std::vector<winrt::StarlightGUI::ProcessInfo> processes;
        std::map<DWORD, hstring> processCpuTable;
        processes.reserve(100);

        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            co_await wil::resume_foreground(DispatcherQueue());
            CreateInfoBarAndDisplay(L"错误", L"无法获取进程快照", InfoBarSeverity::Error, XamlRoot(), InfoBarPanel());
            co_return;
        }

        PROCESSENTRY32W pe32{};
        pe32.dwSize = sizeof(PROCESSENTRY32W);

        if (Process32FirstW(hSnapshot, &pe32)) {
            do {
				co_await GetProcessInfoAsync(pe32, processes);
            } while (Process32NextW(hSnapshot, &pe32));
        }

        CloseHandle(hSnapshot);

		// 异步加载CPU使用率
        co_await TaskUtils::FetchProcessCpuUsage(processCpuTable);

        co_await wil::resume_foreground(DispatcherQueue());

        m_processList.Clear();
        m_processList_unsorted.Clear();

		// 恢复筛选
        winrt::hstring query;
        ProcessSearchBox().Document().GetText(TextGetOptions::NoHidden, query);

        // 当不搜索时载入完整列表
        if (query.empty()) {
			fullRecordedProcesses = processes;
        }

        StarlightGUI::ProcessInfo& selectedProcess = winrt::make<winrt::StarlightGUI::implementation::ProcessInfo>();
        for (const auto& process : processes) {
            bool shouldRemove = query.empty() ? false : co_await ApplyFilter(process, query);

			if (shouldRemove) continue;

            m_processList.Append(process);
			// 从缓存加载图标，没有则获取
            co_await GetProcessIconAsync(process);

            if (processCpuTable.find((DWORD)process.Id()) == processCpuTable.end()) process.CpuUsage(L"未知");
            else process.CpuUsage(processCpuTable[(DWORD)process.Id()]);

            if (selectedItem == process.Id()) selectedProcess = process;
        }

        m_processList_unsorted = m_processList;

        // 恢复排序
        ApplySort(currentSortingOption, currentSortingType);

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

		// 更新进程数量文本
        std::wstringstream countText;
        countText << L"共 " << processes.size() << L" 个进程 (" << duration.count() << " ms)";
        ProcessCountText().Text(countText.str());
        processes.clear();

		// 恢复选中项
        uint32_t index;
        if (m_processList.IndexOf(selectedProcess, index)) {
            ProcessListView().SelectedIndex(index);
        }

        m_isLoadingProcesses = false;
    }

    winrt::Windows::Foundation::IAsyncAction TaskPage::GetProcessInfoAsync(const PROCESSENTRY32W& pe32, std::vector<winrt::StarlightGUI::ProcessInfo>& processes)
    {
		// 跳过筛选的PID，在搜索时性能更好
        std::lock_guard<std::mutex> lock(safelock);
        if (filteredPids.find(pe32.th32ProcessID) != filteredPids.end()) {
            co_return;
		}
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
        if (hProcess) {
            WCHAR processName[MAX_PATH] = L"";
            DWORD size = MAX_PATH;

            if (QueryFullProcessImageNameW(hProcess, 0, processName, &size)) {
                uint64_t memoryUsage = TaskUtils::Task_GetProcessWorkingSet(hProcess);
                if (memoryUsage == 0) {
                    PROCESS_MEMORY_COUNTERS pmc{};
                    if (K32GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
                        memoryUsage = pmc.WorkingSetSize;
                    }
                }

				// 缓存描述信息
                if (descriptionCache.find(processName) == descriptionCache.end()) {
                    DWORD dummy;
                    DWORD versionInfoSize = GetFileVersionInfoSizeW(processName, &dummy);
                    std::wstring description = L"";

                    if (versionInfoSize > 0) {
                        std::vector<BYTE> versionInfo(versionInfoSize);
                        if (GetFileVersionInfoW(processName, 0, versionInfoSize, versionInfo.data())) {
                            VS_FIXEDFILEINFO* fileInfo;
                            UINT fileInfoSize;

                            if (VerQueryValueW(versionInfo.data(), L"\\", (LPVOID*)&fileInfo, &fileInfoSize)) {
                                if (fileInfo->dwFileFlagsMask & VS_FF_INFOINFERRED) {
                                }
                            }

                            wchar_t* fileDescription = nullptr;
                            UINT descriptionSize;
                            if (VerQueryValueW(versionInfo.data(), L"\\StringFileInfo\\040904b0\\FileDescription", (LPVOID*)&fileDescription, &descriptionSize) && fileDescription) {
                                description = fileDescription;
                            }
                        }
                    }

                    if (description.empty()) {
                        description = L"应用程序";
                    }

					descriptionCache[processName] = description;
				}

                auto processInfo = winrt::make<winrt::StarlightGUI::implementation::ProcessInfo>();
                processInfo.Id((int32_t)pe32.th32ProcessID);
                processInfo.Name(pe32.szExeFile);
                processInfo.Description(descriptionCache[processName]);
                processInfo.MemoryUsage(FormatMemorySize(memoryUsage));
                processInfo.MemoryUsageByte(memoryUsage);
                processInfo.ExecutablePath(processName);
                processInfo.Icon(nullptr); // 先设置成null，后面再加载

                processes.push_back(processInfo);
            }

            CloseHandle(hProcess);
        }
        else {
            // 因为一些原因导致无法获取句柄，那么我们以后就不用搜索它了，加入缓存列表
            filteredPids.insert(pe32.th32ProcessID);
        }

        co_return;
    }

    winrt::Windows::Foundation::IAsyncAction TaskPage::GetProcessIconAsync(const winrt::StarlightGUI::ProcessInfo& process) {
        if (iconCache.find(process.ExecutablePath()) == iconCache.end()) {
            SHFILEINFO shfi;
            if (SHGetFileInfoW(process.ExecutablePath().c_str(), 0, &shfi, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_SMALLICON)) {
                auto stream = winrt::Windows::Storage::Streams::InMemoryRandomAccessStream();
                ICONINFO iconInfo;
                if (GetIconInfo(shfi.hIcon, &iconInfo)) {
                    BITMAP bmp;
                    GetObject(iconInfo.hbmColor, sizeof(bmp), &bmp);
                    BITMAPINFOHEADER bmiHeader = { 0 };
                    bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                    bmiHeader.biWidth = bmp.bmWidth;
                    bmiHeader.biHeight = bmp.bmHeight;
                    bmiHeader.biPlanes = 1;
                    bmiHeader.biBitCount = 32;
                    bmiHeader.biCompression = BI_RGB;

                    int dataSize = bmp.bmWidthBytes * bmp.bmHeight;
                    std::vector<BYTE> buffer(dataSize);

                    GetDIBits(hdc, iconInfo.hbmColor, 0, bmp.bmHeight, buffer.data(), reinterpret_cast<BITMAPINFO*>(&bmiHeader), DIB_RGB_COLORS);

                    winrt::Microsoft::UI::Xaml::Media::Imaging::WriteableBitmap writeableBitmap(bmp.bmWidth, bmp.bmHeight);

                    // 将数据写入 WriteableBitmap
                    uint8_t* data = writeableBitmap.PixelBuffer().data();
                    int rowSize = bmp.bmWidth * 4;
                    for (int i = 0; i < bmp.bmHeight; ++i) {
                        int srcOffset = i * rowSize;
                        int dstOffset = (bmp.bmHeight - 1 - i) * rowSize;
                        std::memcpy(data + dstOffset, buffer.data() + srcOffset, rowSize);
                    }

                    DeleteObject(iconInfo.hbmColor);
                    DeleteObject(iconInfo.hbmMask);
                    DestroyIcon(shfi.hIcon);

                    // 将图标缓存到 map 中
                    iconCache[process.ExecutablePath()] = writeableBitmap.as<winrt::Microsoft::UI::Xaml::Media::ImageSource>();
                }
            }
        }
        process.Icon(iconCache[process.ExecutablePath()].value());

        co_return;
    }

    winrt::hstring TaskPage::FormatMemorySize(uint64_t bytes)
    {
        std::wstringstream ss;
        ss << std::fixed << std::setprecision(1);

        if (bytes >= GB) {
            ss << (static_cast<double>(bytes) / GB) << " GB";
        }
        else if (bytes >= MB) {
            ss << (static_cast<double>(bytes) / MB) << " MB";
        }
        else if (bytes >= KB) {
            ss << (static_cast<double>(bytes) / KB) << " KB";
        }
        else {
            ss << bytes << " B";
        }

        return to_hstring(ss.str().c_str());
    }

    void TaskPage::ColumnHeader_Click(IInspectable const& sender, RoutedEventArgs const& e)
    {
        Button clickedButton = sender.as<Button>();
        winrt::hstring columnName = clickedButton.Tag().as<winrt::hstring>();

        // Reset all first
        if (columnName == L"Name")
        {
            ApplySort(m_isNameAscending, "Name");
        }
        else if (columnName == L"CpuUsage")
        {
            ApplySort(m_isCpuAscending, "CpuUsage");
        }
        else if (columnName == L"MemoryUsage")
        {
            ApplySort(m_isMemoryAscending, "MemoryUsage");
        }
        else if (columnName == L"Id")
        {
            ApplySort(m_isIdAscending, "Id");
        }
    }

    // 排序切换
    void TaskPage::ApplySort(bool& isAscending, const std::string& column)
    {
        NameHeaderButton().Content(box_value(L"进程"));
        CpuHeaderButton().Content(box_value(L"CPU"));
        MemoryHeaderButton().Content(box_value(L"内存"));
        IdHeaderButton().Content(box_value(L"PID"));

        std::vector<winrt::StarlightGUI::ProcessInfo> sortedProcesses;

        for (auto& process : m_processList) {
            sortedProcesses.push_back(process);
        }

        if (column == "Name") {
            if (isAscending) {
                NameHeaderButton().Content(box_value(L"进程 ↓"));
                std::sort(sortedProcesses.begin(), sortedProcesses.end(), [](auto a, auto b) {
                    std::wstring aName = a.Name().c_str();
                    std::wstring bName = b.Name().c_str();
                    std::transform(aName.begin(), aName.end(), aName.begin(), ::towlower);
                    std::transform(bName.begin(), bName.end(), bName.begin(), ::towlower);

                    return aName < bName;
                    });
                
            }
            else {
                NameHeaderButton().Content(box_value(L"进程 ↑"));
                std::sort(sortedProcesses.begin(), sortedProcesses.end(), [](auto a, auto b) {
                    std::wstring aName = a.Name().c_str();
                    std::wstring bName = b.Name().c_str();
                    std::transform(aName.begin(), aName.end(), aName.begin(), ::towlower);
                    std::transform(bName.begin(), bName.end(), bName.begin(), ::towlower);

                    return aName > bName;
                    });
            }
        }
        else if (column == "CpuUsage") {
            if (isAscending) {
                CpuHeaderButton().Content(box_value(L"CPU ↓"));
                std::sort(sortedProcesses.begin(), sortedProcesses.end(), [](auto a, auto b) {
                    return std::stod(a.CpuUsage().c_str()) < std::stod(b.CpuUsage().c_str());
                    });
            }
            else {
                CpuHeaderButton().Content(box_value(L"CPU ↑"));
                std::sort(sortedProcesses.begin(), sortedProcesses.end(), [](auto a, auto b) {
                    return std::stod(a.CpuUsage().c_str()) > std::stod(b.CpuUsage().c_str());
                    });
            }
        }
        else if (column == "MemoryUsage") {
            if (isAscending) {
                MemoryHeaderButton().Content(box_value(L"内存 ↓"));
                std::sort(sortedProcesses.begin(), sortedProcesses.end(), [](auto a, auto b) {
                    return a.MemoryUsageByte() < b.MemoryUsageByte();
                    });
            }
            else {
                MemoryHeaderButton().Content(box_value(L"内存 ↑"));
                std::sort(sortedProcesses.begin(), sortedProcesses.end(), [](auto a, auto b) {
                    return a.MemoryUsageByte() > b.MemoryUsageByte();
                    });
            }
        }
        else if (column == "Id") {
            if (isAscending) {
                IdHeaderButton().Content(box_value(L"PID ↓"));
                std::sort(sortedProcesses.begin(), sortedProcesses.end(), [](auto a, auto b) {
                    return a.Id() < b.Id();
                    });
            }
            else {
                IdHeaderButton().Content(box_value(L"PID ↑"));
                std::sort(sortedProcesses.begin(), sortedProcesses.end(), [](auto a, auto b) {
                    return a.Id() > b.Id();
                    });
            }
        }

        m_processList.Clear();
        for (auto& process : sortedProcesses) {
            m_processList.Append(process);
        }

        isAscending = !isAscending;
        currentSortingOption = !isAscending;
        currentSortingType = column;
    }

    winrt::fire_and_forget TaskPage::ProcessSearchBox_TextChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
		// 每次搜索都清空之前缓存的过滤结果
        std::lock_guard<std::mutex> lock(safelock);
        filteredPids.clear();

        winrt::hstring query;
        ProcessSearchBox().Document().GetText(TextGetOptions::NoHidden, query);

        /*
        * 我们的思路是这样的：
		*  - 在LoadProcessList()中，如果搜索框为空，则缓存一次完整的进程列表
		*  - 搜索时，先遍历这个完整的进程列表，记录需要过滤掉的进程PID
		*  - 由于在遍历进程时，我们会先检查一遍filteredPids，所以这会直接跳过进程的所有处理
		*  - 原先的逻辑是每次搜索都重新获取进程列表，然后再筛选一遍得到需要添加到ListView的进程，这意味着我们仍然会去处理进程，即使它最终会被过滤掉
		*  - 这样可以大幅提升搜索性能，并且我们会在添加进程时再进程一次过滤，确保新增的进程也会被正常过滤
		*  - 唯一的缺点是，如果正好原先有个进程退出，然后新启动了一个进程，这个新进程的PID正好是之前被过滤掉的进程的PID，那么这个新进程就会被错误地过滤掉
		*  - 但这种情况发生的概率极低，而且影响也不大，所以可以接受，我们还有个刷新按钮可以手动刷新进程列表
        */
        if (!query.empty()) {
            for (const auto& process : fullRecordedProcesses) {
				co_await ApplyFilter(process, query);
            }
        }

        // 加载列表，因为我们不是排序而是直接删除项
        LoadProcessList();
    }

    winrt::Windows::Foundation::IAsyncOperation<bool> TaskPage::ApplyFilter(const winrt::StarlightGUI::ProcessInfo& process, hstring& query) {
        std::wstring name = process.Name().c_str();
        std::wstring queryWStr = query.c_str();

        // 不比较大小写
        std::transform(name.begin(), name.end(), name.begin(), ::towlower);
        std::transform(queryWStr.begin(), queryWStr.end(), queryWStr.begin(), ::towlower);

        uint32_t index;
        bool result = name.find(queryWStr) == std::wstring::npos;
        if (result) {
			// 缓存过滤的PID，这样下次可以直接跳过
			filteredPids.insert(process.Id());
        }

        co_return result;
    }


    void TaskPage::RefreshProcessListButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
		// 手动刷新时清空过滤列表，确保获取最新的进程列表
		filteredPids.clear();
        LoadProcessList();
    }

    void TaskPage::OnNavigatedFrom(winrt::Microsoft::UI::Xaml::Navigation::NavigationEventArgs const& e)
    {
        m_refreshTimer.Stop();
        ReleaseDC(NULL, hdc);
    }

    TaskPage::~TaskPage() {
        m_refreshTimer.Stop();
        ReleaseDC(NULL, hdc);
    }

    template <typename T>
    T TaskPage::FindParent(DependencyObject const& child)
    {
        DependencyObject parent = VisualTreeHelper::GetParent(child);
        while (parent && !parent.try_as<T>())
        {
            parent = VisualTreeHelper::GetParent(parent);
        }
        return parent.try_as<T>();
    }
}