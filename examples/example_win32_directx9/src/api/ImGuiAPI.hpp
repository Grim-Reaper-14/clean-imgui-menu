#pragma once

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

// Registers a Lua-friendly subset of Dear ImGui. The binding includes named
// per-frame render callbacks so Lua windows remain visible after script setup.
class ImGuiAPI
{
public:
    static void Register(sol::state& lua);
    static void Unregister(sol::state& lua);
};
