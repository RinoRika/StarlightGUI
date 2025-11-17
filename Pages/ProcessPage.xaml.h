#pragma once

#include "ProcessPage.g.h"
#include <vector>
#include <winrt/Windows.Foundation.Collections.h>
#include <Utils/ProcessInfo.h>

namespace winrt::StarlightGUI::implementation
{
    struct ProcessPage : ProcessPageT<ProcessPage>
    {
        ProcessPage();

        winrt::fire_and_forget TerminatorButton_Clicked(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        winrt::fire_and_forget ElevatorButton_Clicked(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        winrt::fire_and_forget DriverLoaderLoadButton_Clicked(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        inline static bool confirmedOnce = false;
    };
}

namespace winrt::StarlightGUI::factory_implementation
{
    struct ProcessPage : ProcessPageT<ProcessPage, implementation::ProcessPage>
    {
    };
}
