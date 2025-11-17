#pragma once
#include <iostream>
#include <string>
#include <random>
#include <limits>

namespace winrt::StarlightGUI::implementation {
    static std::wstring GenerateRandomString(size_t length) {
        const std::wstring charset =
            L"0123456789"
            L"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            L"abcdefghijklmnopqrstuvwxyz";

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, static_cast<int>(charset.size() - 1));

        std::wstring result;
        result.reserve(length);

        for (size_t i = 0; i < length; ++i) {
            result += charset[dist(gen)];
        }
        return result;
    }
}