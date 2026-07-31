# Lua APIs

The application exposes `filesystem` and `logger` tables to every loaded Lua
script.

## FileSystemAPI

```lua
filesystem.create_directory("data")
filesystem.write_file("data/settings.txt", "enabled=true\n")
filesystem.append_file("data/settings.txt", "volume=80\n")

local contents = filesystem.read_file("data/settings.txt")
local bytes = filesystem.file_size("data/settings.txt")

for _, path in ipairs(filesystem.list("data", false)) do
    logger.debug("Found " .. path)
end
```

Available functions:

- `exists(path)`
- `is_file(path)`
- `is_directory(path)`
- `read_file(path)`
- `write_file(path, contents)`
- `append_file(path, contents)`
- `create_directory(path)`
- `remove(path, recursive?)`
- `copy(source, destination, overwrite?)`
- `move(source, destination, overwrite?)`
- `list(path, recursive?)`
- `file_size(path)`
- `current_directory()`
- `absolute_path(path)`
- `join(left, right)`
- `file_name(path)`
- `extension(path)`
- `parent_path(path)`

`recursive` defaults to `false`. `overwrite` defaults to `true`. Failed
operations raise a Lua error that can be handled with `pcall`.

## LoggerAPI

```lua
logger.info("Script started")
logger.warn("Using default configuration")
logger.error("Unable to connect")

logger.set_level("debug")
logger.set_file("logs/my-script.log")
logger.enable_file(true)

logger.log("critical", "Unexpected application state")
```

Available functions:

- `trace(message)`
- `debug(message)`
- `info(message)`
- `warn(message)` / `warning(message)`
- `error(message)`
- `critical(message)`
- `log(level, message)`
- `set_level(level)` / `get_level()`
- `set_file(path)` / `get_file()`
- `enable_file(enabled)` / `is_file_enabled()`
- `clear_file()`

Messages are timestamped and written to the menu's Script Output panel and the
Windows debugger. File logging is enabled by default at
`logs/application.log`.

## ImGuiAPI

Execute a script once to register a named render callback. The application then
invokes that callback inside the **Lua UI** panel in the existing menu:

```lua
local enabled = false
local amount = 0.5

imgui.set_render_callback("my_menu_panel", function()
    imgui.text("This panel is rendered inside the menu.")

    local changed
    changed, enabled = imgui.checkbox("Enabled", enabled)
    changed, amount = imgui.slider_float("Amount", amount, 0.0, 1.0)

    if imgui.button("Save") then
        logger.info("Save clicked")
    end
end)
```

Render callback functions:

- `set_render_callback(name, function)`
- `remove_render_callback(name)`
- `clear_render_callbacks()`

Callbacks registered while a managed script is executing belong to that script.
Stopping or reloading a script removes only its callbacks; callbacks from other
scripts remain active.

Callbacks render inline by default and should use widgets/layout functions
directly. Standalone windows remain available for scripts that explicitly need
them:

- `begin_window(name, open?, flags?)` / `end_window()`
- `window(name, function, flags?)`
- `begin_child(name, width?, height?, border?, flags?)` / `end_child()`
- `child(name, function, width?, height?, border?, flags?)`

Widgets include `text`, `text_wrapped`, `text_colored`, `bullet_text`,
`button`, `small_button`, `checkbox`, `radio_button`, `slider_float`,
`slider_int`, `input_text`, `input_int`, `color_edit4`, and `combo`.

Layout and styling include `separator`, `same_line`, `spacing`, `new_line`,
`dummy`, `indent`, `unindent`, window/cursor position helpers, tooltips,
IDs, and style color/variable stacks. Constants are available under:

- `imgui.window_flags`
- `imgui.cond`
- `imgui.col`
- `imgui.style_var`
- `imgui.input_text_flags`

When using `begin_window`, `begin_child`, tooltip, ID, or style-stack
functions, always call the matching end/pop function. The scoped `window()` and
`child()` helpers automatically close their ImGui scope if the Lua callback
raises an error.

## Source layout

Application C++ code is organized under:

- `src/api` — FileSystemAPI, LoggerAPI, and ImGuiAPI
- `src/lua` — Lua state, modules, commands, and binding registration
- `src/render` — DirectX rendering helpers
- `src/main.cpp` — application entry point and menu
