local enabled = false
local speed = 0.5
local player_name = "Player"
local mode = 1

imgui.set_render_callback("imgui_api_example", function()
    imgui.text_colored(0.75, 0.15, 0.15, 1.0, "Lua is drawing inside the menu")
    imgui.separator()

    local changed
    changed, enabled = imgui.checkbox("Enabled", enabled)
    changed, speed = imgui.slider_float("Speed", speed, 0.0, 1.0, "%.2f")
    changed, player_name = imgui.input_text("Name", player_name)
    changed, mode = imgui.combo("Mode", mode, { "Legit", "Rage", "Custom" })

    imgui.spacing()
    if imgui.button("Write settings", 140, 28) then
        local contents =
            "enabled=" .. tostring(enabled) .. "\n" ..
            "speed=" .. tostring(speed) .. "\n" ..
            "name=" .. player_name .. "\n" ..
            "mode=" .. tostring(mode) .. "\n"

        filesystem.write_file("data/lua_imgui_settings.txt", contents)
        logger.info("Saved Lua ImGui settings")
    end

    imgui.same_line()
    if imgui.button("Unload UI", 100, 28) then
        imgui.remove_render_callback("imgui_api_example")
        logger.debug("Lua ImGui example unloaded")
    end
end)

logger.info("Inline Lua ImGui example registered")
