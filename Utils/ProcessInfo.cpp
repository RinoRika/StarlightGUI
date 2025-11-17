#include "pch.h"
#include "ProcessInfo.h"
#if __has_include("ProcessInfo.g.cpp")
#include "ProcessInfo.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml::Media;

namespace winrt::StarlightGUI::implementation
{

    int32_t ProcessInfo::Id()
    {
        return m_id;
    }

    void ProcessInfo::Id(int32_t value)
    {
        m_id = value;
    }

    hstring ProcessInfo::Name()
    {
        return m_name;
    }

    void ProcessInfo::Name(hstring const& value)
    {
        m_name = value;
    }

    hstring ProcessInfo::Description()
    {
        return m_description;
    }

    void ProcessInfo::Description(hstring const& value)
    {
        m_description = value;
    }

    hstring ProcessInfo::MemoryUsage()
    {
        return m_memoryUsage;
    }

    void ProcessInfo::MemoryUsage(hstring const& value)
    {
        m_memoryUsage = value;
    }

    uint64_t ProcessInfo::MemoryUsageByte()
    {
        return m_memoryUsageByte;
    }

    void ProcessInfo::MemoryUsageByte(uint64_t const& value)
    {
        m_memoryUsageByte = value;
    }

    hstring ProcessInfo::CpuUsage()
    {
        return m_cpuUsage;
    }

    void ProcessInfo::CpuUsage(hstring const& value)
    {
        m_cpuUsage = value;
    }

    hstring ProcessInfo::ExecutablePath() {
        return m_executablePath;
    }

    void ProcessInfo::ExecutablePath(hstring const& value) {
        m_executablePath = value;
    }

    ImageSource ProcessInfo::Icon()
    {
        return m_icon;
    }

    void ProcessInfo::Icon(ImageSource const& value)
    {
        m_icon = value;
    }
}