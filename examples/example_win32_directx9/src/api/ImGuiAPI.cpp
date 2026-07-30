#include "ImGuiAPI.hpp"

#include "Lua_Manager.hpp"
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace
{
    ImGuiWindowFlags WindowFlags(sol::optional<int> flags)
    {
        return static_cast<ImGuiWindowFlags>(flags.value_or(ImGuiWindowFlags_None));
    }

    ImGuiCond Condition(sol::optional<int> condition, ImGuiCond fallback)
    {
        return static_cast<ImGuiCond>(condition.value_or(fallback));
    }

    void RunProtected(const sol::protected_function& callback)
    {
        sol::protected_function_result result = callback();
        if (!result.valid())
        {
            const sol::error error = result;
            throw std::runtime_error(error.what());
        }
    }
}

void ImGuiAPI::Register(sol::state& lua)
{
    sol::table imgui = lua.create_named_table("imgui");

    // Persistent, named callbacks. Registering the same name replaces the
    // previous callback, preventing duplicates when a script is re-executed.
    imgui.set_function("set_render_callback",
        [](const std::string& name, sol::protected_function callback)
        {
            LuaManager::Instance().SetRenderCallback(name, std::move(callback));
        });
    imgui.set_function("remove_render_callback",
        [](const std::string& name)
        {
            return LuaManager::Instance().RemoveRenderCallback(name);
        });
    imgui.set_function("clear_render_callbacks",
        []()
        {
            LuaManager::Instance().ClearRenderCallbacks();
        });

    // Window helpers.
    imgui.set_function("begin_window",
        [](const std::string& name, sol::optional<bool> open,
           sol::optional<int> flags)
        {
            bool isOpen = open.value_or(true);
            const bool visible = open
                ? ImGui::Begin(name.c_str(), &isOpen, WindowFlags(flags))
                : ImGui::Begin(name.c_str(), nullptr, WindowFlags(flags));
            return std::make_tuple(visible, isOpen);
        });
    imgui.set_function("end_window", []() { ImGui::End(); });

    // Scoped window helper: End() still runs if the Lua callback fails.
    imgui.set_function("window",
        [](const std::string& name, sol::protected_function callback,
           sol::optional<int> flags)
        {
            const bool visible =
                ImGui::Begin(name.c_str(), nullptr, WindowFlags(flags));
            try
            {
                if (visible)
                    RunProtected(callback);
            }
            catch (...)
            {
                ImGui::End();
                throw;
            }
            ImGui::End();
            return visible;
        });

    imgui.set_function("begin_child",
        [](const std::string& name, sol::optional<float> width,
           sol::optional<float> height, sol::optional<bool> border,
           sol::optional<int> flags)
        {
            return ImGui::BeginChild(
                name.c_str(),
                ImVec2(width.value_or(0.0f), height.value_or(0.0f)),
                border.value_or(false),
                WindowFlags(flags));
        });
    imgui.set_function("end_child", []() { ImGui::EndChild(); });

    imgui.set_function("child",
        [](const std::string& name, sol::protected_function callback,
           sol::optional<float> width, sol::optional<float> height,
           sol::optional<bool> border, sol::optional<int> flags)
        {
            const bool visible = ImGui::BeginChild(
                name.c_str(),
                ImVec2(width.value_or(0.0f), height.value_or(0.0f)),
                border.value_or(false),
                WindowFlags(flags));
            try
            {
                if (visible)
                    RunProtected(callback);
            }
            catch (...)
            {
                ImGui::EndChild();
                throw;
            }
            ImGui::EndChild();
            return visible;
        });

    // Text and basic widgets.
    imgui.set_function("text",
        [](const std::string& value) { ImGui::TextUnformatted(value.c_str()); });
    imgui.set_function("text_wrapped",
        [](const std::string& value)
        {
            ImGui::PushTextWrapPos(0.0f);
            ImGui::TextUnformatted(value.c_str());
            ImGui::PopTextWrapPos();
        });
    imgui.set_function("text_colored",
        [](float red, float green, float blue, float alpha,
           const std::string& value)
        {
            ImGui::TextColored(
                ImVec4(red, green, blue, alpha), "%s", value.c_str());
        });
    imgui.set_function("bullet_text",
        [](const std::string& value) { ImGui::BulletText("%s", value.c_str()); });
    imgui.set_function("button",
        [](const std::string& label, sol::optional<float> width,
           sol::optional<float> height)
        {
            return ImGui::Button(
                label.c_str(),
                ImVec2(width.value_or(0.0f), height.value_or(0.0f)));
        });
    imgui.set_function("small_button",
        [](const std::string& label) { return ImGui::SmallButton(label.c_str()); });
    imgui.set_function("checkbox",
        [](const std::string& label, bool value)
        {
            const bool changed = ImGui::Checkbox(label.c_str(), &value);
            return std::make_tuple(changed, value);
        });
    imgui.set_function("radio_button",
        [](const std::string& label, bool active)
        {
            return ImGui::RadioButton(label.c_str(), active);
        });
    imgui.set_function("slider_float",
        [](const std::string& label, float value, float minimum, float maximum,
           sol::optional<std::string> format)
        {
            const std::string displayFormat = format.value_or("%.3f");
            const bool changed = ImGui::SliderFloat(
                label.c_str(), &value, minimum, maximum, displayFormat.c_str());
            return std::make_tuple(changed, value);
        });
    imgui.set_function("slider_int",
        [](const std::string& label, int value, int minimum, int maximum,
           sol::optional<std::string> format)
        {
            const std::string displayFormat = format.value_or("%d");
            const bool changed = ImGui::SliderInt(
                label.c_str(), &value, minimum, maximum, displayFormat.c_str());
            return std::make_tuple(changed, value);
        });
    imgui.set_function("input_text",
        [](const std::string& label, std::string value,
           sol::optional<int> flags)
        {
            const bool changed = ImGui::InputText(
                label.c_str(), &value,
                static_cast<ImGuiInputTextFlags>(
                    flags.value_or(ImGuiInputTextFlags_None)));
            return std::make_tuple(changed, value);
        });
    imgui.set_function("input_int",
        [](const std::string& label, int value, sol::optional<int> step,
           sol::optional<int> fastStep)
        {
            const bool changed = ImGui::InputInt(
                label.c_str(), &value, step.value_or(1), fastStep.value_or(100));
            return std::make_tuple(changed, value);
        });
    imgui.set_function("color_edit4",
        [](const std::string& label, float red, float green, float blue,
           float alpha, sol::optional<int> flags)
        {
            float color[4] = { red, green, blue, alpha };
            const bool changed = ImGui::ColorEdit4(
                label.c_str(), color,
                static_cast<ImGuiColorEditFlags>(
                    flags.value_or(ImGuiColorEditFlags_None)));
            return std::make_tuple(
                changed, color[0], color[1], color[2], color[3]);
        });
    imgui.set_function("combo",
        [](const std::string& label, int currentItem, sol::table luaItems)
        {
            std::vector<std::string> items;
            const std::size_t itemCount = luaItems.size();
            items.reserve(itemCount);
            for (std::size_t index = 1; index <= itemCount; ++index)
                items.push_back(luaItems.get_or<std::string>(index, ""));

            if (items.empty())
                return std::make_tuple(false, 0);

            int selected = std::clamp(currentItem - 1, 0,
                                      static_cast<int>(items.size()) - 1);
            bool changed = false;
            if (ImGui::BeginCombo(label.c_str(), items[selected].c_str()))
            {
                for (int index = 0; index < static_cast<int>(items.size()); ++index)
                {
                    const bool isSelected = selected == index;
                    if (ImGui::Selectable(items[index].c_str(), isSelected))
                    {
                        selected = index;
                        changed = true;
                    }
                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            return std::make_tuple(changed, selected + 1);
        });

    // Layout and position helpers.
    imgui.set_function("separator", []() { ImGui::Separator(); });
    imgui.set_function("same_line",
        [](sol::optional<float> offset, sol::optional<float> spacing)
        {
            ImGui::SameLine(offset.value_or(0.0f), spacing.value_or(-1.0f));
        });
    imgui.set_function("spacing", []() { ImGui::Spacing(); });
    imgui.set_function("new_line", []() { ImGui::NewLine(); });
    imgui.set_function("dummy",
        [](float width, float height) { ImGui::Dummy(ImVec2(width, height)); });
    imgui.set_function("indent",
        [](sol::optional<float> width) { ImGui::Indent(width.value_or(0.0f)); });
    imgui.set_function("unindent",
        [](sol::optional<float> width) { ImGui::Unindent(width.value_or(0.0f)); });
    imgui.set_function("set_next_window_pos",
        [](float x, float y, sol::optional<int> condition,
           sol::optional<float> pivotX, sol::optional<float> pivotY)
        {
            ImGui::SetNextWindowPos(
                ImVec2(x, y), Condition(condition, ImGuiCond_Once),
                ImVec2(pivotX.value_or(0.0f), pivotY.value_or(0.0f)));
        });
    imgui.set_function("set_next_window_size",
        [](float width, float height, sol::optional<int> condition)
        {
            ImGui::SetNextWindowSize(
                ImVec2(width, height), Condition(condition, ImGuiCond_Once));
        });
    imgui.set_function("set_cursor_pos",
        [](float x, float y) { ImGui::SetCursorPos(ImVec2(x, y)); });
    imgui.set_function("get_cursor_pos", []()
    {
        const ImVec2 position = ImGui::GetCursorPos();
        return std::make_tuple(position.x, position.y);
    });
    imgui.set_function("get_content_region_avail", []()
    {
        const ImVec2 size = ImGui::GetContentRegionAvail();
        return std::make_tuple(size.x, size.y);
    });
    imgui.set_function("get_window_pos", []()
    {
        const ImVec2 position = ImGui::GetWindowPos();
        return std::make_tuple(position.x, position.y);
    });
    imgui.set_function("get_window_size", []()
    {
        const ImVec2 size = ImGui::GetWindowSize();
        return std::make_tuple(size.x, size.y);
    });

    // Item queries and tooltips.
    imgui.set_function("is_item_hovered", []() { return ImGui::IsItemHovered(); });
    imgui.set_function("is_item_clicked",
        [](sol::optional<int> button)
        {
            return ImGui::IsItemClicked(
                static_cast<ImGuiMouseButton>(
                    button.value_or(ImGuiMouseButton_Left)));
        });
    imgui.set_function("set_tooltip",
        [](const std::string& value) { ImGui::SetTooltip("%s", value.c_str()); });
    imgui.set_function("begin_tooltip", []() { ImGui::BeginTooltip(); });
    imgui.set_function("end_tooltip", []() { ImGui::EndTooltip(); });

    // IDs and styles.
    imgui.set_function("push_id",
        [](const std::string& value) { ImGui::PushID(value.c_str()); });
    imgui.set_function("pop_id", []() { ImGui::PopID(); });
    imgui.set_function("push_style_color",
        [](int colorIndex, float red, float green, float blue, float alpha)
        {
            ImGui::PushStyleColor(
                static_cast<ImGuiCol>(colorIndex),
                ImVec4(red, green, blue, alpha));
        });
    imgui.set_function("pop_style_color",
        [](sol::optional<int> count)
        {
            ImGui::PopStyleColor(count.value_or(1));
        });
    imgui.set_function("push_style_var_float",
        [](int styleIndex, float value)
        {
            ImGui::PushStyleVar(static_cast<ImGuiStyleVar>(styleIndex), value);
        });
    imgui.set_function("push_style_var_vec2",
        [](int styleIndex, float x, float y)
        {
            ImGui::PushStyleVar(
                static_cast<ImGuiStyleVar>(styleIndex), ImVec2(x, y));
        });
    imgui.set_function("pop_style_var",
        [](sol::optional<int> count)
        {
            ImGui::PopStyleVar(count.value_or(1));
        });

    // Lua-readable constants.
    sol::table windowFlags = lua.create_table();
    imgui["window_flags"] = windowFlags;
    windowFlags["none"] = ImGuiWindowFlags_None;
    windowFlags["no_title_bar"] = ImGuiWindowFlags_NoTitleBar;
    windowFlags["no_resize"] = ImGuiWindowFlags_NoResize;
    windowFlags["no_move"] = ImGuiWindowFlags_NoMove;
    windowFlags["no_scrollbar"] = ImGuiWindowFlags_NoScrollbar;
    windowFlags["no_collapse"] = ImGuiWindowFlags_NoCollapse;
    windowFlags["always_auto_resize"] = ImGuiWindowFlags_AlwaysAutoResize;
    windowFlags["no_background"] = ImGuiWindowFlags_NoBackground;
    windowFlags["no_saved_settings"] = ImGuiWindowFlags_NoSavedSettings;
    windowFlags["menu_bar"] = ImGuiWindowFlags_MenuBar;

    sol::table conditions = lua.create_table();
    imgui["cond"] = conditions;
    conditions["always"] = ImGuiCond_Always;
    conditions["once"] = ImGuiCond_Once;
    conditions["first_use_ever"] = ImGuiCond_FirstUseEver;
    conditions["appearing"] = ImGuiCond_Appearing;

    sol::table colors = lua.create_table();
    imgui["col"] = colors;
    colors["text"] = ImGuiCol_Text;
    colors["window_bg"] = ImGuiCol_WindowBg;
    colors["child_bg"] = ImGuiCol_ChildBg;
    colors["popup_bg"] = ImGuiCol_PopupBg;
    colors["border"] = ImGuiCol_Border;
    colors["frame_bg"] = ImGuiCol_FrameBg;
    colors["frame_bg_hovered"] = ImGuiCol_FrameBgHovered;
    colors["frame_bg_active"] = ImGuiCol_FrameBgActive;
    colors["title_bg"] = ImGuiCol_TitleBg;
    colors["title_bg_active"] = ImGuiCol_TitleBgActive;
    colors["check_mark"] = ImGuiCol_CheckMark;
    colors["slider_grab"] = ImGuiCol_SliderGrab;
    colors["slider_grab_active"] = ImGuiCol_SliderGrabActive;
    colors["button"] = ImGuiCol_Button;
    colors["button_hovered"] = ImGuiCol_ButtonHovered;
    colors["button_active"] = ImGuiCol_ButtonActive;
    colors["header"] = ImGuiCol_Header;
    colors["header_hovered"] = ImGuiCol_HeaderHovered;
    colors["header_active"] = ImGuiCol_HeaderActive;

    sol::table styleVariables = lua.create_table();
    imgui["style_var"] = styleVariables;
    styleVariables["alpha"] = ImGuiStyleVar_Alpha;
    styleVariables["window_padding"] = ImGuiStyleVar_WindowPadding;
    styleVariables["window_rounding"] = ImGuiStyleVar_WindowRounding;
    styleVariables["window_border_size"] = ImGuiStyleVar_WindowBorderSize;
    styleVariables["window_min_size"] = ImGuiStyleVar_WindowMinSize;
    styleVariables["window_title_align"] = ImGuiStyleVar_WindowTitleAlign;
    styleVariables["child_rounding"] = ImGuiStyleVar_ChildRounding;
    styleVariables["child_border_size"] = ImGuiStyleVar_ChildBorderSize;
    styleVariables["popup_rounding"] = ImGuiStyleVar_PopupRounding;
    styleVariables["popup_border_size"] = ImGuiStyleVar_PopupBorderSize;
    styleVariables["frame_padding"] = ImGuiStyleVar_FramePadding;
    styleVariables["frame_rounding"] = ImGuiStyleVar_FrameRounding;
    styleVariables["frame_border_size"] = ImGuiStyleVar_FrameBorderSize;
    styleVariables["item_spacing"] = ImGuiStyleVar_ItemSpacing;
    styleVariables["item_inner_spacing"] = ImGuiStyleVar_ItemInnerSpacing;
    styleVariables["indent_spacing"] = ImGuiStyleVar_IndentSpacing;
    styleVariables["scrollbar_size"] = ImGuiStyleVar_ScrollbarSize;
    styleVariables["scrollbar_rounding"] = ImGuiStyleVar_ScrollbarRounding;
    styleVariables["grab_min_size"] = ImGuiStyleVar_GrabMinSize;
    styleVariables["grab_rounding"] = ImGuiStyleVar_GrabRounding;

    sol::table inputTextFlags = lua.create_table();
    imgui["input_text_flags"] = inputTextFlags;
    inputTextFlags["none"] = ImGuiInputTextFlags_None;
    inputTextFlags["enter_returns_true"] = ImGuiInputTextFlags_EnterReturnsTrue;
    inputTextFlags["read_only"] = ImGuiInputTextFlags_ReadOnly;
    inputTextFlags["password"] = ImGuiInputTextFlags_Password;
}

void ImGuiAPI::Unregister(sol::state& lua)
{
    lua["imgui"] = sol::nil;
}
