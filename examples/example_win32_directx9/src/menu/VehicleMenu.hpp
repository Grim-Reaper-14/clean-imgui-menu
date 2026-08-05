#pragma once

#include "imgui.h"
#include "render/VehiclePreview.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>

struct VehicleCatalogEntry
{
    const char* displayName;
    const char* modelName;
    const char* category;
};

class VehicleMenu
{
public:
    using SpawnHandler = std::function<void(std::uint32_t)>;

    void Initialize(IDirect3DDevice9* device, const std::filesystem::path& previewDirectory);
    void Render(float animationOffset);
    void Shutdown();

    void SetSpawnHandler(SpawnHandler handler);
    bool ConsumeSpawnRequest(std::uint32_t& modelHash);

private:
    const VehicleCatalogEntry& GetSelectedEntry() const;
    std::string GetSelectedModelName() const;
    std::string GetSelectedDisplayName() const;
    std::uint32_t GetSelectedHash() const;
    void DrawPreview(const VehiclePreviewTexture* preview, const ImVec2& size) const;
    void DrawFallbackPreview(const ImVec2& size, const std::string& displayName) const;
    bool MatchesSearch(const VehicleCatalogEntry& entry) const;

    VehiclePreviewCache previews_;
    SpawnHandler spawnHandler_;
    std::array<char, 64> search_{};
    std::array<char, 64> customModel_{};
    int selectedIndex_ = 0;
    bool customSelected_ = false;
    bool spawnRequested_ = false;
    std::uint32_t requestedHash_ = 0;
    std::string status_ = "Select a vehicle to preview it.";
};
