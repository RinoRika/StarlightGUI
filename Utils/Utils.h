#pragma once

#include <pch.h>
#include <Utils/CppUtils.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>
#include <winrt/Microsoft.UI.Xaml.Markup.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Media.Animation.h>
#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>
#include <winrt/Windows.Storage.Streams.h>

using namespace winrt;
using namespace Windows::Foundation;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Microsoft::UI::Xaml::Media::Animation;

static Windows::UI::Color& infobarColor = Windows::UI::Color();

namespace winrt::StarlightGUI::implementation {
    inline auto CreateFontIcon(hstring glyph) {
        FontIcon fontIcon;
        fontIcon.Glyph(glyph);
        fontIcon.FontFamily(FontFamily(L"Segoe Fluent Icons"));
        fontIcon.FontSize(16);

        return fontIcon;
    }

    inline auto CreateInfoBar(hstring title, hstring message, InfoBarSeverity severity, XamlRoot xamlRoot) {
        InfoBar infobar;

        infobar.Title(title);
        infobar.Message(message);
        infobar.Severity(severity);
        infobar.XamlRoot(xamlRoot);
        infobar.HorizontalAlignment(HorizontalAlignment::Right);
        infobar.VerticalAlignment(VerticalAlignment::Top);
        infobar.IsOpen(true);

        if (infobarColor.A == 0) {
            auto themeResources = Application::Current().Resources();
            auto chromeMediumColor = themeResources.TryLookup(box_value(L"SystemChromeMediumColor"));
            if (chromeMediumColor) {
                infobarColor = unbox_value<Windows::UI::Color>(chromeMediumColor);
            }
        }

        SolidColorBrush bg;
        bg.Color(infobarColor);
        bg.Opacity(0.8);
        infobar.Background(bg);

        return infobar;
    }

    inline void DisplayInfoBar(InfoBar infobar, Panel parent) {
        if (!infobar || !parent) return;

        // Entrance animation
        EdgeUIThemeTransition transition;
        TransitionCollection transitions;
        transitions.Append(transition);
        infobar.Transitions(transitions);

        // Add and display
        parent.Children().Append(infobar);
        infobar.IsOpen(true);

        // Auto close timer
        auto timer = DispatcherTimer();
        timer.Interval(std::chrono::seconds(3));
        timer.Tick([infobar, parent, timer](auto&&, auto&&) {
            // Run fade out animation first
            Storyboard storyboard;
            auto fadeOutAnimation = FadeOutThemeAnimation();
            Storyboard::SetTarget(fadeOutAnimation, infobar);
            storyboard.Children().Append(fadeOutAnimation.as<Timeline>());
            storyboard.Begin();

            // Then close and remove from parent
            auto timer2 = DispatcherTimer();
            timer2.Interval(std::chrono::milliseconds(300));
            timer2.Tick([infobar, parent, timer2](auto&&, auto&&) {
                infobar.IsOpen(false);
                uint32_t index;
                if (parent.Children().IndexOf(infobar, index)) {
                    parent.Children().RemoveAt(index);
                }
                timer2.Stop();
                });
            timer2.Start();

            timer.Stop();
            });
        timer.Start();

        // Manual close animation
        infobar.Closing([infobar, parent, timer](auto&&, auto&&) {
            infobar.IsOpen(true);
            timer.Stop();

            // Run fade out animation first
            Storyboard storyboard;
            auto fadeOutAnimation = FadeOutThemeAnimation();
            Storyboard::SetTarget(fadeOutAnimation, infobar);
            storyboard.Children().Append(fadeOutAnimation.as<Timeline>());
            storyboard.Begin();

            auto timer2 = DispatcherTimer();
            timer2.Interval(std::chrono::milliseconds(300));
            timer2.Tick([infobar, parent, timer2](auto&&, auto&&) {
                infobar.IsOpen(false);
                uint32_t index;
                if (parent.Children().IndexOf(infobar, index)) {
                    parent.Children().RemoveAt(index);
                }
                timer2.Stop();
                });
            timer2.Start();
            });
    }

    inline void CreateInfoBarAndDisplay(hstring title, hstring message, InfoBarSeverity severity, XamlRoot xamlRoot, Panel parent) {
        DisplayInfoBar(CreateInfoBar(title, message, severity, xamlRoot), parent);
    }

    inline auto CreateContentDialog(hstring title, hstring content, hstring closeMessage, XamlRoot xamlRoot) {
        ContentDialog dialog;

        dialog.Title(winrt::box_value(title));
        dialog.Content(winrt::box_value(content));
        dialog.CloseButtonText(closeMessage);
        dialog.XamlRoot(xamlRoot);

        return dialog;
    }

    inline auto GetContentDialogSuccessTemplate() {
        return XamlReader::Load(LR"(
        <DataTemplate xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation">
            <StackPanel Orientation="Horizontal" Spacing="8">
                <FontIcon Glyph="&#xec61;" FontSize="30" FontFamily="Segoe Fluent Icons" Foreground="Green" Margin="0,5,0,0"/>
                <TextBlock Text="{Binding}" VerticalAlignment="Center"/>
            </StackPanel>
        </DataTemplate>
    )").as<DataTemplate>();
    }

    inline auto GetContentDialogErrorTemplate() {
        return XamlReader::Load(LR"(
        <DataTemplate xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation">
            <StackPanel Orientation="Horizontal" Spacing="8">
                <FontIcon Glyph="&#xeb90;" FontSize="30" FontFamily="Segoe Fluent Icons" Foreground="OrangeRed" Margin="0,5,0,0"/>
                <TextBlock Text="{Binding}" VerticalAlignment="Center"/>
            </StackPanel>
        </DataTemplate>
    )").as<DataTemplate>();
    }

    inline auto GetContentDialogInfoTemplate() {
        return XamlReader::Load(LR"(
        <DataTemplate xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation">
            <StackPanel Orientation="Horizontal" Spacing="8">
                <FontIcon Glyph="&#xf167;" FontSize="30" FontFamily="Segoe Fluent Icons" Foreground="LightBlue" Margin="0,5,0,0"/>
                <TextBlock Text="{Binding}" VerticalAlignment="Center"/>
            </StackPanel>
        </DataTemplate>
    )").as<DataTemplate>();
    }

    inline auto GetTemplate(hstring xaml) {
        return XamlReader::Load(xaml).as<DataTemplate>();
    }

    inline winrt::Microsoft::UI::Xaml::Media::Imaging::BitmapImage ConvertHIconToImageSource(HICON hIcon)
    {
        if (hIcon == nullptr)
        {
            return nullptr;
        }

        auto stream = winrt::Windows::Storage::Streams::InMemoryRandomAccessStream();
        ICONINFO iconInfo;
        if (GetIconInfo(hIcon, &iconInfo)) {
            HDC hdc = GetDC(NULL);

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

            // 获取图像位图数据
            GetDIBits(hdc, iconInfo.hbmColor, 0, bmp.bmHeight, buffer.data(), reinterpret_cast<BITMAPINFO*>(&bmiHeader), DIB_RGB_COLORS);

            // 将数据写入内存流
            Windows::Storage::Streams::DataWriter writer;
            writer.WriteBytes(buffer);
            stream.WriteAsync(writer.DetachBuffer()).get();

            // Load stream to bitmap
            BitmapImage bitmapImage;
            stream.Seek(0);
            bitmapImage.SetSource(stream);

            ReleaseDC(NULL, hdc);
            DeleteObject(iconInfo.hbmColor);
            DeleteObject(iconInfo.hbmMask);

            return bitmapImage;
        }

        return nullptr;
    }
}