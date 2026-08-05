# Vehicle preview images

Place vehicle preview images in this folder. The menu loads them lazily when a vehicle is selected and caches the DirectX 9 texture.

Supported formats: `.png`, `.jpg`, `.jpeg`, `.bmp`, `.tga`, and `.dds`.

Name each file using either the GTA model name or its uppercase JOAAT hash:

- `adder.png`
- `B779A091.png`
- `0xB779A091.png`

Model-name files are the easiest to maintain. Images keep their aspect ratio inside the preview panel. Missing files use the built-in vehicle silhouette, so the menu stays usable while the image library is being populated.
