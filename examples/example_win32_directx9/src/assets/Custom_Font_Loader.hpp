#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

struct ImFont;

class CustomFontLoader final
{
public:
    explicit CustomFontLoader(std::filesystem::path directory);

    CustomFontLoader(const CustomFontLoader&) = delete;
    CustomFontLoader& operator=(const CustomFontLoader&) = delete;

    void SetDirectory(std::filesystem::path directory);
    const std::filesystem::path& GetDirectory() const noexcept;

    bool Refresh(std::string& error);
    const std::vector<std::filesystem::path>& GetFonts() const noexcept;

    void SetDefaultFont(ImFont* font) noexcept;
    void QueueFont(const std::filesystem::path& file, float sizePixels);
    void QueueDefault() noexcept;

    bool HasPendingChange() const noexcept;
    bool ApplyPending(std::string& status);

    const std::filesystem::path& GetActiveFont() const noexcept;
    float GetActiveSize() const noexcept;

private:
    enum class PendingAction
    {
        None,
        Font,
        Default
    };

    static bool IsSupported(const std::filesystem::path& file);
    static std::string Lowercase(std::string value);
    static std::string MakeCacheKey(const std::filesystem::path& file,
                                    float sizePixels);

    std::filesystem::path m_directory;
    std::vector<std::filesystem::path> m_fonts;
    std::unordered_map<std::string, ImFont*> m_cache;

    ImFont* m_defaultFont = nullptr;
    std::filesystem::path m_activeFont;
    float m_activeSize = 0.0f;

    PendingAction m_pendingAction = PendingAction::None;
    std::filesystem::path m_pendingFont;
    float m_pendingSize = 22.0f;
};
