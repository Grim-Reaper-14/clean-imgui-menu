# Runtime asset folders

The DirectX 9 example creates and scans these folders beside the built
executable.

## Fonts

Place TrueType or OpenType files in:

    Fonts\

Supported extensions are .ttf and .otf. Open the Config tab, refresh the font
list, and select a font. A newly selected font is applied before the next ImGui
frame so the font atlas and DirectX 9 texture are rebuilt safely. The embedded
Segoe UI font remains available as the fallback.

## Background images

Place background files in:

    Images\

Supported extensions are .png, .jpg, .jpeg, .bmp, .tga, and .dds. Select a
background from the Config tab to apply it live. Choosing Built-in returns to
the embedded background.

## Sidebar icons

Place icon files in:

    Images\Icons\

The loader matches file stems, so the extension may be any supported image
format. Use these names:

- legitbot
- ragebot
- antiaim
- visuals
- misc
- playerlist
- skins
- lua
- config

For example, Images\Icons\visuals.png replaces the embedded Visuals glyph.
Missing or invalid icon files automatically fall back to the existing embedded
icon font.
