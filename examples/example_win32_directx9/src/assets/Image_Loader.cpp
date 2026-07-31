#include "Image_Loader.hpp"

#include <d3d9.h>
#include <dxsdk-d3dx/d3dx9.h>
#pragma comment(lib, "d3dx9.lib")

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <system_error>
#include <utility>

ImageLoader::ImageLoader(IDirect3DDevice9* device,
                         std::filesystem::path directory)
    : m_device(device), m_directory(std::move(directory))
{
}

ImageLoader::~ImageLoader()
{
    Clear();
}

void ImageLoader::SetDirectory(std::filesystem::path directory)
{
    m_directory = std::move(directory);
    m_images.clear();
}

const std::filesystem::path& ImageLoader::GetDirectory() const noexcept
{
    return m_directory;
}

bool ImageLoader::Refresh(std::string& error)
{
    m_images.clear();

    if (m_directory.empty())
    {
        error = "Image directory is empty.";
        return false;
    }

    std::error_code filesystemError;
    std::filesystem::create_directories(m_directory, filesystemError);
    if (filesystemError)
    {
        error = "Unable to create image directory: " +
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
            m_images.push_back(iterator->path());
        }

        iterator.increment(filesystemError);
    }

    if (filesystemError)
    {
        error = "Unable to scan image directory: " +
            filesystemError.message();
        m_images.clear();
        return false;
    }

    std::sort(m_images.begin(), m_images.end(),
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
ImageLoader::GetImages() const noexcept
{
    return m_images;
}

bool ImageLoader::Load(const std::string& slot,
                       const std::filesystem::path& file,
                       std::string& error)
{
    if (!m_device)
    {
        error = "DirectX 9 device is not available.";
        return false;
    }

    if (slot.empty())
    {
        error = "Image slot cannot be empty.";
        return false;
    }

    std::error_code filesystemError;
    if (!std::filesystem::is_regular_file(file, filesystemError) ||
        filesystemError)
    {
        error = "Image file does not exist: " + file.string();
        return false;
    }

    if (!IsSupported(file))
    {
        error = "Unsupported image format: " + file.extension().string();
        return false;
    }

    D3DXIMAGE_INFO imageInfo = {};
    IDirect3DTexture9* texture = nullptr;
    const HRESULT result = D3DXCreateTextureFromFileExW(
        m_device,
        file.c_str(),
        D3DX_DEFAULT,
        D3DX_DEFAULT,
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

    if (FAILED(result) || !texture)
    {
        std::ostringstream message;
        message << "Unable to load image '" << file.filename().string()
                << "' (HRESULT 0x" << std::hex << std::uppercase
                << static_cast<unsigned long>(result) << ").";
        error = message.str();
        return false;
    }

    Unload(slot);

    TextureInfo loaded;
    loaded.texture = texture;
    loaded.source = file;
    loaded.width = imageInfo.Width;
    loaded.height = imageInfo.Height;
    m_textures.emplace(slot, std::move(loaded));

    error.clear();
    return true;
}

bool ImageLoader::LoadByStem(const std::string& slot,
                             const std::string& fileStem,
                             std::string& error)
{
    const std::string expected = Lowercase(fileStem);
    const auto found = std::find_if(
        m_images.begin(), m_images.end(),
        [&expected](const std::filesystem::path& file)
        {
            return Lowercase(file.stem().string()) == expected;
        });

    if (found == m_images.end())
    {
        error = "No image named '" + fileStem + "' was found in " +
            m_directory.string() + ".";
        return false;
    }

    return Load(slot, *found, error);
}

IDirect3DTexture9*
ImageLoader::GetTexture(const std::string& slot) const noexcept
{
    const TextureInfo* info = GetTextureInfo(slot);
    return info ? info->texture : nullptr;
}

const ImageLoader::TextureInfo*
ImageLoader::GetTextureInfo(const std::string& slot) const noexcept
{
    const auto found = m_textures.find(slot);
    return found == m_textures.end() ? nullptr : &found->second;
}

void ImageLoader::Unload(const std::string& slot) noexcept
{
    const auto found = m_textures.find(slot);
    if (found == m_textures.end())
        return;

    if (found->second.texture)
        found->second.texture->Release();

    m_textures.erase(found);
}

void ImageLoader::Clear() noexcept
{
    for (auto& texture : m_textures)
    {
        if (texture.second.texture)
            texture.second.texture->Release();
    }

    m_textures.clear();
}

bool ImageLoader::IsSupported(const std::filesystem::path& file)
{
    const std::string extension = Lowercase(file.extension().string());
    return extension == ".png" || extension == ".jpg" ||
           extension == ".jpeg" || extension == ".bmp" ||
           extension == ".tga" || extension == ".dds";
}

std::string ImageLoader::Lowercase(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
        [](const unsigned char character)
        {
            return static_cast<char>(std::tolower(character));
        });
    return value;
}
