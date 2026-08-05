#include "VehiclePreview.hpp"

#include <dxsdk-d3dx/d3dx9.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <system_error>
#include <utility>

namespace
{
    constexpr std::array<const wchar_t*, 6> kPreviewExtensions = {
        L".png", L".jpg", L".jpeg", L".bmp", L".tga", L".dds"
    };

    void ReleasePreview(VehiclePreviewTexture& preview) noexcept
    {
        if (preview.texture != nullptr)
        {
            preview.texture->Release();
            preview.texture = nullptr;
        }

        preview.width = 0;
        preview.height = 0;
        preview.sourcePath.clear();
    }
}

VehiclePreviewCache::~VehiclePreviewCache()
{
    Shutdown();
}

void VehiclePreviewCache::Initialize(
    IDirect3DDevice9* device,
    std::filesystem::path rootDirectory)
{
    Shutdown();
    device_ = device;
    rootDirectory_ = std::move(rootDirectory);

    std::error_code error;
    std::filesystem::create_directories(rootDirectory_, error);
    if (error)
        lastError_ = "Could not create vehicle preview folder: " + error.message();
}

const VehiclePreviewTexture* VehiclePreviewCache::Get(
    std::string_view modelName,
    std::uint32_t modelHash)
{
    if (device_ == nullptr)
    {
        lastError_ = "Vehicle preview cache is not initialized.";
        return nullptr;
    }

    const std::string key = MakeCacheKey(modelName, modelHash);
    CacheEntry& entry = cache_[key];
    if (entry.attempted)
        return entry.preview ? &entry.preview : nullptr;

    entry.attempted = true;
    const std::filesystem::path previewPath = FindPreviewPath(modelName, modelHash);
    if (previewPath.empty())
    {
        lastError_.clear();
        return nullptr;
    }

    if (!LoadTexture(previewPath, entry.preview))
        return nullptr;

    lastError_.clear();
    return &entry.preview;
}

void VehiclePreviewCache::Refresh()
{
    for (auto& pair : cache_)
        ReleasePreview(pair.second.preview);

    cache_.clear();
    lastError_.clear();
}

void VehiclePreviewCache::Shutdown()
{
    Refresh();
    device_ = nullptr;
    rootDirectory_.clear();
}

const std::filesystem::path& VehiclePreviewCache::GetRootDirectory() const noexcept
{
    return rootDirectory_;
}

const std::string& VehiclePreviewCache::GetLastError() const noexcept
{
    return lastError_;
}

std::uint32_t VehiclePreviewCache::Joaat(std::string_view value) noexcept
{
    std::uint32_t hash = 0;
    for (const unsigned char character : value)
    {
        hash += static_cast<unsigned char>(std::tolower(character));
        hash += hash << 10;
        hash ^= hash >> 6;
    }

    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;
    return hash;
}

std::string VehiclePreviewCache::FormatHash(std::uint32_t value, bool withPrefix)
{
    std::ostringstream stream;
    if (withPrefix)
        stream << "0x";

    stream << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << value;
    return stream.str();
}

std::string VehiclePreviewCache::MakeCacheKey(
    std::string_view modelName,
    std::uint32_t modelHash) const
{
    return NormalizeModelName(modelName) + ":" + FormatHash(modelHash);
}

std::filesystem::path VehiclePreviewCache::FindPreviewPath(
    std::string_view modelName,
    std::uint32_t modelHash) const
{
    if (rootDirectory_.empty())
        return {};

    const std::string normalizedName = NormalizeModelName(modelName);
    const std::array<std::string, 4> stems = {
        normalizedName,
        FormatHash(modelHash),
        FormatHash(modelHash, true),
        std::string(modelName)
    };

    std::error_code error;
    for (const std::string& stem : stems)
    {
        if (stem.empty())
            continue;

        for (const wchar_t* extension : kPreviewExtensions)
        {
            std::filesystem::path candidate = rootDirectory_ / std::filesystem::path(stem);
            candidate.replace_extension(extension);
            if (std::filesystem::is_regular_file(candidate, error) && !error)
                return candidate;

            error.clear();
        }
    }

    return {};
}

bool VehiclePreviewCache::LoadTexture(
    const std::filesystem::path& path,
    VehiclePreviewTexture& output)
{
    const std::wstring widePath = path.wstring();
    D3DXIMAGE_INFO imageInfo = {};
    HRESULT result = D3DXGetImageInfoFromFileW(widePath.c_str(), &imageInfo);
    if (FAILED(result))
    {
        lastError_ = "Could not read vehicle preview: " + path.string();
        return false;
    }

    IDirect3DTexture9* texture = nullptr;
    result = D3DXCreateTextureFromFileExW(
        device_,
        widePath.c_str(),
        D3DX_DEFAULT_NONPOW2,
        D3DX_DEFAULT_NONPOW2,
        D3DX_DEFAULT,
        0,
        D3DFMT_UNKNOWN,
        D3DPOOL_MANAGED,
        D3DX_DEFAULT,
        D3DX_DEFAULT,
        0,
        &imageInfo,
        nullptr,
        &texture);

    if (FAILED(result) || texture == nullptr)
    {
        lastError_ = "Could not create vehicle preview texture: " + path.string();
        return false;
    }

    output.texture = texture;
    output.width = imageInfo.Width;
    output.height = imageInfo.Height;
    output.sourcePath = path;
    return true;
}

std::string VehiclePreviewCache::NormalizeModelName(std::string_view value)
{
    std::string normalized;
    normalized.reserve(value.size());
    for (const unsigned char character : value)
    {
        if (std::isalnum(character) || character == '_' || character == '-')
            normalized.push_back(static_cast<char>(std::tolower(character)));
    }

    return normalized;
}
