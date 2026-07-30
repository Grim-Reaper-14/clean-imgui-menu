#include "ConfigManager.hpp"

#include "api/FileSystemAPI.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace
{
    std::string EscapeJson(const char* value)
    {
        std::string escaped;
        for (const unsigned char character : std::string(value))
        {
            switch (character)
            {
            case '"':  escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\b': escaped += "\\b"; break;
            case '\f': escaped += "\\f"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default:
                if (character < 0x20)
                {
                    std::ostringstream code;
                    code << "\\u" << std::hex << std::setw(4)
                         << std::setfill('0') << static_cast<int>(character);
                    escaped += code.str();
                }
                else
                {
                    escaped += static_cast<char>(character);
                }
                break;
            }
        }
        return escaped;
    }

    std::size_t FindValue(const std::string& json, const char* key)
    {
        const std::string quotedKey = std::string("\"") + key + "\"";
        const std::size_t keyPosition = json.find(quotedKey);
        if (keyPosition == std::string::npos)
            throw std::runtime_error("Missing setting: " + std::string(key));

        std::size_t position = json.find(':', keyPosition + quotedKey.size());
        if (position == std::string::npos)
            throw std::runtime_error("Invalid setting: " + std::string(key));

        ++position;
        while (position < json.size() &&
               std::isspace(static_cast<unsigned char>(json[position])))
        {
            ++position;
        }
        return position;
    }

    bool ReadBoolean(const std::string& json, const char* key)
    {
        const std::size_t position = FindValue(json, key);
        if (json.compare(position, 4, "true") == 0)
            return true;
        if (json.compare(position, 5, "false") == 0)
            return false;
        throw std::runtime_error("Expected true or false for: " + std::string(key));
    }

    double ReadNumber(const std::string& json, const char* key)
    {
        const std::size_t position = FindValue(json, key);
        char* end = nullptr;
        const double value = std::strtod(json.c_str() + position, &end);
        if (end == json.c_str() + position)
            throw std::runtime_error("Expected a number for: " + std::string(key));
        return value;
    }

    std::array<float, 4> ReadColor(const std::string& json, const char* key)
    {
        std::size_t position = FindValue(json, key);
        if (position >= json.size() || json[position] != '[')
            throw std::runtime_error("Expected a color array for: " + std::string(key));

        ++position;
        std::array<float, 4> color{};
        for (std::size_t index = 0; index < color.size(); ++index)
        {
            while (position < json.size() &&
                   std::isspace(static_cast<unsigned char>(json[position])))
            {
                ++position;
            }

            char* end = nullptr;
            color[index] = std::strtof(json.c_str() + position, &end);
            if (end == json.c_str() + position)
                throw std::runtime_error("Invalid color value for: " + std::string(key));

            position = static_cast<std::size_t>(end - json.c_str());
            while (position < json.size() &&
                   std::isspace(static_cast<unsigned char>(json[position])))
            {
                ++position;
            }

            const char expected = index + 1 < color.size() ? ',' : ']';
            if (position >= json.size() || json[position] != expected)
                throw std::runtime_error("Invalid color array for: " + std::string(key));
            ++position;
        }
        return color;
    }

    std::string ReadString(const std::string& json, const char* key)
    {
        std::size_t position = FindValue(json, key);
        if (position >= json.size() || json[position] != '"')
            throw std::runtime_error("Expected text for: " + std::string(key));

        ++position;
        std::string value;
        while (position < json.size())
        {
            const char character = json[position++];
            if (character == '"')
                return value;

            if (character != '\\')
            {
                value += character;
                continue;
            }

            if (position >= json.size())
                break;

            const char escaped = json[position++];
            switch (escaped)
            {
            case '"':  value += '"'; break;
            case '\\': value += '\\'; break;
            case '/':  value += '/'; break;
            case 'b':  value += '\b'; break;
            case 'f':  value += '\f'; break;
            case 'n':  value += '\n'; break;
            case 'r':  value += '\r'; break;
            case 't':  value += '\t'; break;
            default:
                throw std::runtime_error("Unsupported text escape for: " +
                                         std::string(key));
            }
        }

        throw std::runtime_error("Unterminated text for: " + std::string(key));
    }

    float ClampColor(float value)
    {
        return std::clamp(value, 0.0f, 1.0f);
    }
}

bool ConfigManager::Save(const std::string& path,
                         const MenuSettings& settings,
                         const float backgroundColor[4],
                         const float accentColor[4],
                         std::string& error)
{
    try
    {
        std::ostringstream json;
        json << std::fixed << std::setprecision(3)
             << "{\n"
             << "  \"version\": 1,\n"
             << "  \"checkbox_0\": " << (settings.checkbox0 ? "true" : "false") << ",\n"
             << "  \"checkbox_1\": " << (settings.checkbox1 ? "true" : "false") << ",\n"
             << "  \"slider_integer\": " << settings.sliderInteger << ",\n"
             << "  \"slider_float\": " << settings.sliderFloat << ",\n"
             << "  \"background_color\": ["
             << backgroundColor[0] << ", " << backgroundColor[1] << ", "
             << backgroundColor[2] << ", " << backgroundColor[3] << "],\n"
             << "  \"accent_color\": ["
             << accentColor[0] << ", " << accentColor[1] << ", "
             << accentColor[2] << ", " << accentColor[3] << "],\n"
             << "  \"aimbot_mode\": " << settings.aimbotMode << ",\n"
             << "  \"input_text\": \"" << EscapeJson(settings.inputText.data()) << "\",\n"
             << "  \"aimbot_key\": " << settings.aimbotKey << ",\n"
             << "  \"aimbot_key_mode\": " << settings.aimbotKeyMode << "\n"
             << "}\n";

        FileSystemAPI::WriteFile(path, json.str());
        error.clear();
        return true;
    }
    catch (const std::exception& exception)
    {
        error = exception.what();
        return false;
    }
}

bool ConfigManager::Load(const std::string& path,
                         MenuSettings& settings,
                         float backgroundColor[4],
                         float accentColor[4],
                         std::string& error)
{
    try
    {
        const std::string json = FileSystemAPI::ReadFile(path);
        MenuSettings loaded;
        loaded.checkbox0 = ReadBoolean(json, "checkbox_0");
        loaded.checkbox1 = ReadBoolean(json, "checkbox_1");
        loaded.sliderInteger = std::clamp(
            static_cast<int>(ReadNumber(json, "slider_integer")), 1, 400);
        loaded.sliderFloat = std::clamp(
            static_cast<float>(ReadNumber(json, "slider_float")), 0.0f, 5.0f);
        loaded.aimbotMode = std::clamp(
            static_cast<int>(ReadNumber(json, "aimbot_mode")), 0, 1);
        loaded.aimbotKey = static_cast<int>(ReadNumber(json, "aimbot_key"));
        loaded.aimbotKeyMode = static_cast<int>(
            ReadNumber(json, "aimbot_key_mode"));

        const std::string inputText = ReadString(json, "input_text");
        strncpy_s(loaded.inputText.data(), loaded.inputText.size(),
                  inputText.c_str(), _TRUNCATE);

        const std::array<float, 4> loadedBackground =
            ReadColor(json, "background_color");
        const std::array<float, 4> loadedAccent =
            ReadColor(json, "accent_color");

        settings = loaded;
        for (std::size_t index = 0; index < 4; ++index)
        {
            backgroundColor[index] = ClampColor(loadedBackground[index]);
            accentColor[index] = ClampColor(loadedAccent[index]);
        }

        error.clear();
        return true;
    }
    catch (const std::exception& exception)
    {
        error = exception.what();
        return false;
    }
}
