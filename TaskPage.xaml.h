#pragma once

#include "TaskPage.g.h"
#include <map>
#include <TlHelp32.h>

namespace winrt::StarlightGUI::implementation
{
    struct TaskPage : TaskPageT<TaskPage>
    {
        TaskPage();

        ~TaskPage();

        void RefreshProcessListButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void ProcessListView_RightTapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e);
        void OnNavigatedFrom(winrt::Microsoft::UI::Xaml::Navigation::NavigationEventArgs const& e);

        void ColumnHeader_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void ApplySort(bool& isAscending, const std::string& column);

        winrt::fire_and_forget ProcessSearchBox_TextChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        winrt::Windows::Foundation::IAsyncOperation<bool> ApplyFilter(const winrt::StarlightGUI::ProcessInfo& process, hstring& query);

        // 进程列表相关方法
        winrt::Windows::Foundation::IAsyncAction LoadProcessList();
        winrt::Windows::Foundation::IAsyncAction GetProcessInfoAsync(const PROCESSENTRY32W& pe32, std::vector<winrt::StarlightGUI::ProcessInfo>& processes);
        winrt::Windows::Foundation::IAsyncAction GetProcessIconAsync(const winrt::StarlightGUI::ProcessInfo& process);

        winrt::hstring FormatMemorySize(uint64_t bytes);

        winrt::Windows::Foundation::Collections::IObservableVector<winrt::StarlightGUI::ProcessInfo> m_processList{
            winrt::single_threaded_observable_vector<winrt::StarlightGUI::ProcessInfo>()
        };

        winrt::Windows::Foundation::Collections::IObservableVector<winrt::StarlightGUI::ProcessInfo> m_processList_unsorted{
            winrt::single_threaded_observable_vector<winrt::StarlightGUI::ProcessInfo>()
        };

        bool m_isLoadingProcesses{ false };
        winrt::Microsoft::UI::Xaml::DispatcherTimer m_refreshTimer;

        const int refreshInterval = 5;

        inline static bool TaskPage::m_isNameAscending = true;
        inline static bool TaskPage::m_isCpuAscending = true;
        inline static bool TaskPage::m_isMemoryAscending = true;
        inline static bool TaskPage::m_isIdAscending = true;
        inline static bool currentSortingOption;
        inline static std::string currentSortingType;

        template <typename T>
        T FindParent(winrt::Microsoft::UI::Xaml::DependencyObject const& child);
    };
}

namespace winrt::StarlightGUI::factory_implementation
{
    struct TaskPage : TaskPageT<TaskPage, implementation::TaskPage>
    {
    };
}
