#pragma once

#include <array>
#include <string>

struct MenuSettings
{
    bool checkbox0 = false;
    bool checkbox1 = false;
    int sliderInteger = 50;
    float sliderFloat = 0.0f;
    int aimbotMode = 0;
    std::array<char, 64> inputText{};
    int aimbotKey = 0;
    int aimbotKeyMode = 1;
};

class ConfigManager
{
public:
    static bool Save(const std::string& path,
                     const MenuSettings& settings,
                     const float backgroundColor[4],
                     const float accentColor[4],
                     std::string& error);

    static bool Load(const std::string& path,
                     MenuSettings& settings,
                     float backgroundColor[4],
                     float accentColor[4],
                     std::string& error);
};
