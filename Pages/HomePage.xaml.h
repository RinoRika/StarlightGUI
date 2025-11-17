#pragma once

#include "HomePage.g.h"
#include <winrt/impl/Microsoft.UI.Xaml.Media.Imaging.0.h>

namespace winrt::StarlightGUI::implementation
{
    struct HomePage : public HomePageT<HomePage>
    {
        HomePage();

        void SetGreetingText();
        winrt::fire_and_forget SetUserProfile();
        winrt::fire_and_forget FetchHitokoto();
        winrt::fire_and_forget LoadDriverPath();
        void SetupClock();
        void OnClockTick(IInspectable const&, IInspectable const&);
        void UpdateClock();

        winrt::Microsoft::UI::Dispatching::DispatcherQueueTimer m_clockTimer{ nullptr };

        inline static winrt::hstring greeting;
        inline static winrt::hstring username;
        inline static winrt::hstring hitokoto;
        inline static winrt::Microsoft::UI::Xaml::Media::Imaging::BitmapImage avatar{ nullptr };
    };
}
namespace winrt::StarlightGUI::factory_implementation
{
    struct HomePage : public HomePageT<HomePage, implementation::HomePage>
    {
    };
}
