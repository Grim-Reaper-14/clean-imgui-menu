#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

struct IDirect3DDevice9;
struct IDirect3DTexture9;

class ImageLoader final
{
public:
    struct TextureInfo
    {
        IDirect3DTexture9* texture = nullptr;
        std::filesystem::path source;
        unsigned int width = 0;
        unsigned int height = 0;
    };

    ImageLoader(IDirect3DDevice9* device, std::filesystem::path directory);
    ~ImageLoader();

    ImageLoader(const ImageLoader&) = delete;
    ImageLoader& operator=(const ImageLoader&) = delete;

    void SetDirectory(std::filesystem::path directory);
    const std::filesystem::path& GetDirectory() const noexcept;

    bool Refresh(std::string& error);
    const std::vector<std::filesystem::path>& GetImages() const noexcept;

    bool Load(const std::string& slot,
              const std::filesystem::path& file,
              std::string& error);
    bool LoadByStem(const std::string& slot,
                    const std::string& fileStem,
                    std::string& error);

    IDirect3DTexture9* GetTexture(const std::string& slot) const noexcept;
    const TextureInfo* GetTextureInfo(const std::string& slot) const noexcept;

    void Unload(const std::string& slot) noexcept;
    void Clear() noexcept;

private:
    static bool IsSupported(const std::filesystem::path& file);
    static std::string Lowercase(std::string value);

    IDirect3DDevice9* m_device = nullptr;
    std::filesystem::path m_directory;
    std::vector<std::filesystem::path> m_images;
    std::unordered_map<std::string, TextureInfo> m_textures;
};
