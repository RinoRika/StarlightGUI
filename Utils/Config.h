#pragma once

#include <MainWindow.g.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <string>
#include <locale>
#include <codecvt>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace winrt::StarlightGUI::implementation {
    template<typename T>
    static auto ReadConfig(std::string key, T defaultValue) {
        try
        {
            // 获取用户文件夹路径
            char* value = NULL;
            size_t len = 0;

            _dupenv_s(&value, &len, "USERPROFILE");

            auto userFolder = fs::path(value);
            auto configFilePath = userFolder / "StarlightGUI.json";

            if (fs::exists(configFilePath))
            {
                std::ifstream configFile(configFilePath);
                json configData;
                configFile >> configData;

                if (configData.contains(key))
                {
                    return configData[key];
                }
            }
        }
        catch (...)
        {
            SaveConfig(key, defaultValue);
            return ReadConfig(key, defaultValue);
        }
        
        SaveConfig(key, defaultValue);
        return ReadConfig(key, defaultValue);
    }

    template<typename T>
    static void SaveConfig(std::string key, T s_value) {
        try
        {
            // 获取用户文件夹路径
            char* value = NULL;
            size_t len = 0;

            _dupenv_s(&value, &len, "USERPROFILE");

            auto userFolder = fs::path(value);
            auto configFilePath = userFolder / "StarlightGUI.json";
            json configData;

            if (fs::exists(configFilePath))
            {
                std::ifstream configFile(configFilePath);
                configFile >> configData;
            }

            configData[key] = s_value;

            std::ofstream configFile(configFilePath);
            configFile << configData.dump(4);
        }
        catch (...)
        {
        }
    }
}