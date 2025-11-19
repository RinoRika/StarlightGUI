#include "pch.h"
#include "SettingsPage.xaml.h"
#include "Utils/Config.h"
#if __has_include("SettingsPage.g.cpp")
#include "SettingsPage.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;

namespace winrt::StarlightGUI::implementation
{
    static std::string background_type;
    static bool dangerous_confirm;
    static bool elevator_full_privileges;
    static bool bypass_signature;

    SettingsPage::SettingsPage()
    {
        InitializeComponent();

        // idk why it cant understand the fucking boolean
        background_type = ReadConfig("background_type", "Static");
        dangerous_confirm = ReadConfig("dangerous_confirm", true);
        elevator_full_privileges = ReadConfig("elevator_full_privileges", true);
        bypass_signature = ReadConfig("bypass_signature", false);

        if (background_type == "Mica") {
            BackgroundComboBox().SelectedIndex(1);
        }
        else if (background_type == "Acrylic") {
            BackgroundComboBox().SelectedIndex(2);
        }
        else {
            BackgroundComboBox().SelectedIndex(0);
        }

        DangerousConfirmButton().IsOn(dangerous_confirm);
        FullPrivilegesButton().IsOn(elevator_full_privileges);
        BypassSignatureButton().IsOn(bypass_signature);
    }

    void SettingsPage::BackgroundComboBox_SelectionChanged(IInspectable const& sender, SelectionChangedEventArgs const& e)
    {
        if (BackgroundComboBox().SelectedIndex() == 0)
        {
            background_type = "Static";
        }
        else if (BackgroundComboBox().SelectedIndex() == 1)
        {
            background_type = "Mica";
        }
        else if (BackgroundComboBox().SelectedIndex() == 2)
        {
            background_type = "Acrylic";
        }
        SaveConfig("background_type", background_type);
    }

    void SettingsPage::DangerousConfirmButton_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        dangerous_confirm = DangerousConfirmButton().IsOn();
        SaveConfig("dangerous_confirm", dangerous_confirm);
    }

    void SettingsPage::FullPrivilegesButton_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        elevator_full_privileges = FullPrivilegesButton().IsOn();
        SaveConfig("elevator_full_privileges", elevator_full_privileges);
    }

    void SettingsPage::BypassSignatureButton_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        bypass_signature = BypassSignatureButton().IsOn();
        SaveConfig("bypass_signature", bypass_signature);
    }
}
