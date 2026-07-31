#include "Custom_Font_Loader.hpp"

#include "imgui.h"
#include "imgui_impl_dx9.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <system_error>
#include <utility>

CustomFontLoader::CustomFontLoader(std::filesystem::path directory)
    : m_directory(std::move(directory))
{
}

void CustomFontLoader::SetDirectory(std::filesystem::path directory)
{
    m_directory = std::move(directory);
    m_fonts.clear();
}

const std::filesystem::path&
CustomFontLoader::GetDirectory() const noexcept
{
    return m_directory;
}

bool CustomFontLoader::Refresh(std::string& error)
{
    m_fonts.clear();

    if (m_directory.empty())
    {
        error = "Font directory is empty.";
        return false;
    }

    std::error_code filesystemError;
    std::filesystem::create_directories(m_directory, filesystemError);
    if (filesystemError)
    {
        error = "Unable to create font directory: " +
            filesystemError.message();
        return false;
    }

    std::filesystem::directory_iterator iterator(m_directory, filesystemError);
    const std::filesystem::directory_iterator end;
    while (!filesystemError && iterator != end)
    {
        std::error_code typeError;
        if (iterator->is_regular_file(typeError) && !typeError &&
            IsSupported(iterator->path()))
        {
            m_fonts.push_back(iterator->path());
        }

        iterator.increment(filesystemError);
    }

    if (filesystemError)
    {
        error = "Unable to scan font directory: " +
            filesystemError.message();
        m_fonts.clear();
        return false;
    }

    std::sort(m_fonts.begin(), m_fonts.end(),
        [](const std::filesystem::path& left,
           const std::filesystem::path& right)
        {
            return Lowercase(left.filename().string()) <
                   Lowercase(right.filename().string());
        });

    error.clear();
    return true;
}

const std::vector<std::filesystem::path>&
CustomFontLoader::GetFonts() const noexcept
{
    return m_fonts;
}

void CustomFontLoader::SetDefaultFont(ImFont* font) noexcept
{
    m_defaultFont = font;
}

void CustomFontLoader::QueueFont(const std::filesystem::path& file,
                                 float sizePixels)
{
    m_pendingFont = file;
    m_pendingSize = std::clamp(sizePixels, 8.0f, 72.0f);
    m_pendingAction = PendingAction::Font;
}

void CustomFontLoader::QueueDefault() noexcept
{
    m_pendingAction = PendingAction::Default;
}

bool CustomFontLoader::HasPendingChange() const noexcept
{
    return m_pendingAction != PendingAction::None;
}

bool CustomFontLoader::ApplyPending(std::string& status)
{
    const PendingAction action = m_pendingAction;
    m_pendingAction = PendingAction::None;

    if (action == PendingAction::None)
        return true;

    ImGuiIO& io = ImGui::GetIO();

    if (action == PendingAction::Default)
    {
        if (!m_defaultFont)
        {
            status = "Embedded default font is unavailable.";
            return false;
        }

        io.FontDefault = m_defaultFont;
        m_activeFont.clear();
        m_activeSize = 0.0f;
        status = "Applied the embedded default font.";
        return true;
    }

    std::error_code filesystemError;
    if (!std::filesystem::is_regular_file(m_pendingFont, filesystemError) ||
        filesystemError)
    {
        status = "Font file does not exist: " + m_pendingFont.string();
        return false;
    }

    if (!IsSupported(m_pendingFont))
    {
        status = "Unsupported font format: " +
            m_pendingFont.extension().string();
        return false;
    }

    const std::string cacheKey = MakeCacheKey(m_pendingFont, m_pendingSize);
    ImFont* font = nullptr;

    const auto cached = m_cache.find(cacheKey);
    if (cached != m_cache.end())
    {
        font = cached->second;
    }
    else
    {
        ImFontConfig configuration;
        configuration.OversampleH = 3;
        configuration.OversampleV = 2;
        configuration.PixelSnapH = false;

        const std::string fileName = m_pendingFont.string();
        font = io.Fonts->AddFontFromFileTTF(
            fileName.c_str(),
            m_pendingSize,
            &configuration,
            io.Fonts->GetGlyphRangesCyrillic());

        if (!font)
        {
            status = "Unable to load font: " +
                m_pendingFont.filename().string();
            return false;
        }

        if (!io.Fonts->Build())
        {
            status = "Unable to rebuild the ImGui font atlas.";
            return false;
        }

        ImGui_ImplDX9_InvalidateDeviceObjects();
        if (!ImGui_ImplDX9_CreateDeviceObjects())
        {
            status = "Unable to recreate the DirectX 9 font texture.";
            return false;
        }

        m_cache.emplace(cacheKey, font);
    }

    io.FontDefault = font;
    m_activeFont = m_pendingFont;
    m_activeSize = m_pendingSize;

    std::ostringstream message;
    message << "Applied " << m_activeFont.filename().string()
            << " at " << std::fixed << std::setprecision(0)
            << m_activeSize << " px.";
    status = message.str();
    return true;
}

const std::filesystem::path&
CustomFontLoader::GetActiveFont() const noexcept
{
    return m_activeFont;
}

float CustomFontLoader::GetActiveSize() const noexcept
{
    return m_activeSize;
}

bool CustomFontLoader::IsSupported(const std::filesystem::path& file)
{
    const std::string extension = Lowercase(file.extension().string());
    return extension == ".ttf" || extension == ".otf";
}

std::string CustomFontLoader::Lowercase(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
        [](const unsigned char character)
        {
            return static_cast<char>(std::tolower(character));
        });
    return value;
}

std::string CustomFontLoader::MakeCacheKey(
    const std::filesystem::path& file,
    float sizePixels)
{
    std::ostringstream key;
    key << Lowercase(file.lexically_normal().string()) << '#'
        << std::fixed << std::setprecision(2) << sizePixels;
    return key.str();
}
