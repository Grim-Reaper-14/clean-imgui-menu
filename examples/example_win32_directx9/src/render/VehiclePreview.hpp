#pragma once

#include <d3d9.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>

struct VehiclePreviewTexture
{
    IDirect3DTexture9* texture = nullptr;
    unsigned int width = 0;
    unsigned int height = 0;
    std::filesystem::path sourcePath;

    explicit operator bool() const noexcept
    {
        return texture != nullptr;
    }
};

class VehiclePreviewCache
{
public:
    VehiclePreviewCache() = default;
    ~VehiclePreviewCache();

    VehiclePreviewCache(const VehiclePreviewCache&) = delete;
    VehiclePreviewCache& operator=(const VehiclePreviewCache&) = delete;

    void Initialize(IDirect3DDevice9* device, std::filesystem::path rootDirectory);
    const VehiclePreviewTexture* Get(std::string_view modelName, std::uint32_t modelHash);
    void Refresh();
    void Shutdown();

    const std::filesystem::path& GetRootDirectory() const noexcept;
    const std::string& GetLastError() const noexcept;

    static std::uint32_t Joaat(std::string_view value) noexcept;
    static std::string FormatHash(std::uint32_t value, bool withPrefix = false);

private:
    struct CacheEntry
    {
        VehiclePreviewTexture preview;
        bool attempted = false;
    };

    std::string MakeCacheKey(std::string_view modelName, std::uint32_t modelHash) const;
    std::filesystem::path FindPreviewPath(std::string_view modelName, std::uint32_t modelHash) const;
    bool LoadTexture(const std::filesystem::path& path, VehiclePreviewTexture& output);
    static std::string NormalizeModelName(std::string_view value);

    IDirect3DDevice9* device_ = nullptr;
    std::filesystem::path rootDirectory_;
    std::unordered_map<std::string, CacheEntry> cache_;
    std::string lastError_;
};
