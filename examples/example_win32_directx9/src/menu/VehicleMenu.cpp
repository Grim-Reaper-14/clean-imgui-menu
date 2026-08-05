#include "VehicleMenu.hpp"

#include "imgui.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <utility>

namespace
{
    constexpr std::array<VehicleCatalogEntry, 42> kVehicleCatalog = {{
        {"Adder", "adder", "Super"},
        {"Autarch", "autarch", "Super"},
        {"Banshee 900R", "banshee2", "Super"},
        {"Bullet", "bullet", "Super"},
        {"Cheetah", "cheetah", "Super"},
        {"Cyclone", "cyclone", "Super"},
        {"Emerus", "emerus", "Super"},
        {"Entity XF", "entityxf", "Super"},
        {"Entity XXR", "entity2", "Super"},
        {"Itali GTB", "italigtb", "Super"},
        {"Krieger", "krieger", "Super"},
        {"Nero", "nero", "Super"},
        {"Osiris", "osiris", "Super"},
        {"Reaper", "reaper", "Super"},
        {"T20", "t20", "Super"},
        {"Tempesta", "tempesta", "Super"},
        {"Tezeract", "tezeract", "Super"},
        {"Turismo R", "turismor", "Super"},
        {"Tyrus", "tyrus", "Super"},
        {"Vagner", "vagner", "Super"},
        {"Visione", "visione", "Super"},
        {"Zentorno", "zentorno", "Super"},
        {"Buffalo STX", "buffalo4", "Sports"},
        {"Comet S2", "comet6", "Sports"},
        {"Elegy Retro Custom", "elegy", "Sports"},
        {"Feltzer", "feltzer2", "Sports"},
        {"Jester RR", "jester4", "Sports"},
        {"Kuruma", "kuruma", "Sports"},
        {"Pariah", "pariah", "Sports"},
        {"Sultan RS", "sultanrs", "Super"},
        {"Dominator ASP", "dominator7", "Muscle"},
        {"Duke O'Death", "dukes2", "Muscle"},
        {"Gauntlet Hellfire", "gauntlet4", "Muscle"},
        {"Insurgent Pick-Up Custom", "insurgent3", "Off-Road"},
        {"Nightshark", "nightshark", "Off-Road"},
        {"Oppressor Mk II", "oppressor2", "Motorcycle"},
        {"Sanchez", "sanchez", "Motorcycle"},
        {"Akula", "akula", "Helicopter"},
        {"Buzzard", "buzzard2", "Helicopter"},
        {"Hydra", "hydra", "Plane"},
        {"Lazer", "lazer", "Plane"},
        {"Kosatka", "kosatka", "Boat"}
    }};

    std::string ToLower(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character)
        {
            return static_cast<char>(std::tolower(character));
        });
        return value;
    }
}

void VehicleMenu::Initialize(
    IDirect3DDevice9* device,
    const std::filesystem::path& previewDirectory)
{
    previews_.Initialize(device, previewDirectory);
    std::snprintf(customModel_.data(), customModel_.size(), "%s", "adder");
}

void VehicleMenu::Render(float animationOffset)
{
    ImGui::BeginChild("VehicleCatalog", ImVec2(339.0f, 523.0f), true);
    ImGui::TextUnformatted("Vehicle Catalog");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint("##vehicle_search", "Search vehicles...", search_.data(), search_.size());

    ImGui::Spacing();
    ImGui::BeginChild("VehicleList", ImVec2(-1.0f, 350.0f), true);
    for (int index = 0; index < static_cast<int>(kVehicleCatalog.size()); ++index)
    {
        const VehicleCatalogEntry& entry = kVehicleCatalog[static_cast<std::size_t>(index)];
        if (!MatchesSearch(entry))
            continue;

        char label[128] = {};
        std::snprintf(label, sizeof(label), "%s  [%s]", entry.displayName, entry.category);
        if (ImGui::Selectable(label, !customSelected_ && selectedIndex_ == index))
        {
            selectedIndex_ = index;
            customSelected_ = false;
            status_ = std::string("Selected ") + entry.displayName + ".";
        }
    }
    ImGui::EndChild();

    ImGui::Spacing();
    ImGui::TextUnformatted("Custom model");
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::InputText(
            "##custom_vehicle_model",
            customModel_.data(),
            customModel_.size(),
            ImGuiInputTextFlags_EnterReturnsTrue))
    {
        customSelected_ = customModel_[0] != '\0';
        status_ = customSelected_
            ? "Custom model selected."
            : "Enter a vehicle model name.";
    }

    if (ImGui::Button("Use custom model", ImVec2(-1.0f, 30.0f)))
    {
        customSelected_ = customModel_[0] != '\0';
        status_ = customSelected_
            ? "Custom model selected."
            : "Enter a vehicle model name.";
    }

    ImGui::TextDisabled("Preview folder: %s", previews_.GetRootDirectory().string().c_str());
    ImGui::EndChild();

    ImGui::SetCursorPos(ImVec2(555.0f, 88.0f - animationOffset));
    ImGui::BeginChild("VehiclePreview", ImVec2(339.0f, 300.0f), true);
    const std::string modelName = GetSelectedModelName();
    const std::string displayName = GetSelectedDisplayName();
    const std::uint32_t modelHash = GetSelectedHash();

    ImGui::TextUnformatted("Vehicle Preview");
    ImGui::Separator();
    ImGui::Spacing();

    const VehiclePreviewTexture* preview = previews_.Get(modelName, modelHash);
    DrawPreview(preview, ImVec2(307.0f, 172.0f));

    ImGui::Spacing();
    ImGui::Text("%s", displayName.c_str());
    ImGui::TextDisabled("Model: %s", modelName.c_str());
    ImGui::TextDisabled("Hash: %s", VehiclePreviewCache::FormatHash(modelHash, true).c_str());
    ImGui::EndChild();

    ImGui::SetCursorPos(ImVec2(555.0f, 400.0f - animationOffset));
    ImGui::BeginChild("VehicleActions", ImVec2(339.0f, 211.0f), true);
    ImGui::TextUnformatted("Actions");
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Spawn selected", ImVec2(-1.0f, 34.0f)))
    {
        requestedHash_ = modelHash;
        spawnRequested_ = true;
        status_ = "Spawn request queued for the BigBaseV2 backend.";
        if (spawnHandler_)
        {
            spawnHandler_(modelHash);
            spawnRequested_ = false;
            status_ = "Spawn request sent to the backend.";
        }
    }

    if (ImGui::Button("Refresh preview images", ImVec2(-1.0f, 30.0f)))
    {
        previews_.Refresh();
        status_ = "Vehicle preview cache refreshed.";
    }

    ImGui::Spacing();
    ImGui::TextWrapped("%s", status_.c_str());
    if (!previews_.GetLastError().empty())
        ImGui::TextColored(ImVec4(0.9f, 0.35f, 0.35f, 1.0f), "%s", previews_.GetLastError().c_str());
    ImGui::EndChild();
}

void VehicleMenu::Shutdown()
{
    previews_.Shutdown();
    spawnHandler_ = {};
    spawnRequested_ = false;
    requestedHash_ = 0;
}

void VehicleMenu::SetSpawnHandler(SpawnHandler handler)
{
    spawnHandler_ = std::move(handler);
}

bool VehicleMenu::ConsumeSpawnRequest(std::uint32_t& modelHash)
{
    if (!spawnRequested_)
        return false;

    modelHash = requestedHash_;
    spawnRequested_ = false;
    return true;
}

const VehicleCatalogEntry& VehicleMenu::GetSelectedEntry() const
{
    const std::size_t safeIndex = std::min<std::size_t>(
        static_cast<std::size_t>(std::max(selectedIndex_, 0)),
        kVehicleCatalog.size() - 1);
    return kVehicleCatalog[safeIndex];
}

std::string VehicleMenu::GetSelectedModelName() const
{
    if (customSelected_ && customModel_[0] != '\0')
        return customModel_.data();

    return GetSelectedEntry().modelName;
}

std::string VehicleMenu::GetSelectedDisplayName() const
{
    if (customSelected_ && customModel_[0] != '\0')
        return std::string("Custom: ") + customModel_.data();

    return GetSelectedEntry().displayName;
}

std::uint32_t VehicleMenu::GetSelectedHash() const
{
    return VehiclePreviewCache::Joaat(GetSelectedModelName());
}

void VehicleMenu::DrawPreview(
    const VehiclePreviewTexture* preview,
    const ImVec2& size) const
{
    if (preview == nullptr || !*preview)
    {
        DrawFallbackPreview(size, GetSelectedDisplayName());
        return;
    }

    const float sourceWidth = static_cast<float>(preview->width);
    const float sourceHeight = static_cast<float>(preview->height);
    if (sourceWidth <= 0.0f || sourceHeight <= 0.0f)
    {
        DrawFallbackPreview(size, GetSelectedDisplayName());
        return;
    }

    const float scale = std::min(size.x / sourceWidth, size.y / sourceHeight);
    const ImVec2 imageSize(sourceWidth * scale, sourceHeight * scale);
    const float horizontalPadding = (size.x - imageSize.x) * 0.5f;
    const float verticalPadding = (size.y - imageSize.y) * 0.5f;

    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(origin, ImVec2(origin.x + size.x, origin.y + size.y), ImColor(12, 12, 14, 220), 8.0f);

    ImGui::SetCursorScreenPos(ImVec2(origin.x + horizontalPadding, origin.y + verticalPadding));
    ImGui::Image(preview->texture, imageSize);
    ImGui::SetCursorScreenPos(ImVec2(origin.x, origin.y + size.y));
}

void VehicleMenu::DrawFallbackPreview(
    const ImVec2& size,
    const std::string& displayName) const
{
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec2 end(origin.x + size.x, origin.y + size.y);

    drawList->AddRectFilled(origin, end, ImColor(12, 12, 14, 235), 8.0f);
    drawList->AddRect(origin, end, ImColor(65, 65, 72, 255), 8.0f, 0, 1.0f);

    const ImU32 accent = ImColor(190, 38, 38, 220);
    const float centerX = origin.x + size.x * 0.5f;
    const float baseline = origin.y + size.y * 0.58f;
    drawList->AddRectFilled(
        ImVec2(centerX - 92.0f, baseline - 24.0f),
        ImVec2(centerX + 92.0f, baseline + 18.0f),
        accent,
        16.0f);
    drawList->AddQuadFilled(
        ImVec2(centerX - 55.0f, baseline - 24.0f),
        ImVec2(centerX - 28.0f, baseline - 52.0f),
        ImVec2(centerX + 45.0f, baseline - 52.0f),
        ImVec2(centerX + 72.0f, baseline - 24.0f),
        accent);
    drawList->AddCircleFilled(ImVec2(centerX - 58.0f, baseline + 20.0f), 15.0f, ImColor(20, 20, 22, 255));
    drawList->AddCircleFilled(ImVec2(centerX + 58.0f, baseline + 20.0f), 15.0f, ImColor(20, 20, 22, 255));

    const char* message = "No preview image found";
    const ImVec2 textSize = ImGui::CalcTextSize(message);
    drawList->AddText(
        ImVec2(centerX - textSize.x * 0.5f, origin.y + 18.0f),
        ImColor(190, 190, 195, 255),
        message);

    const ImVec2 nameSize = ImGui::CalcTextSize(displayName.c_str());
    drawList->AddText(
        ImVec2(centerX - nameSize.x * 0.5f, end.y - 28.0f),
        ImColor(150, 150, 155, 255),
        displayName.c_str());

    ImGui::Dummy(size);
}

bool VehicleMenu::MatchesSearch(const VehicleCatalogEntry& entry) const
{
    if (search_[0] == '\0')
        return true;

    const std::string needle = ToLower(search_.data());
    return ToLower(entry.displayName).find(needle) != std::string::npos ||
           ToLower(entry.modelName).find(needle) != std::string::npos ||
           ToLower(entry.category).find(needle) != std::string::npos;
}
