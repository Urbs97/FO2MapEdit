# FO2MapEdit — Fallout 2 Map & Image Editor

A C++17 desktop GUI application for editing Fallout 2 image assets — FRM sprites, MSK mask tiles, overworld map tiles, and town map tiles. Built with Dear ImGui (docking branch), GLFW, OpenGL 3.3, and GLAD.

Forked from [msk2bmpGUI](https://github.com/QuantumApprentice/msk2bmpGUI) by QuantumApprentice, which was originally based on the [msk2bmp](https://github.com/temaperacl/msk2bmp) sources by temaperacl.

---

## Features

### Image Loading & Conversion
- Load standard image formats (PNG, BMP, JPG, GIF) and Fallout FRM/FR0-FR5 files via file dialog or drag & drop
- Load MSK mask files and `.wmap` worldmap project files
- Import an existing worldmap directly from a Fallout 2 installation (parses `worldmap.txt`, stitches FRM tiles and MSK masks into a single editable project)
- Palettize images to the Fallout 256-color palette using Euclidean distance color matching with optional Floyd-Steinberg dithering
- Real-time palette rendering via OpenGL fragment shaders
- Display multi-frame FRM animations in all 6 orientations
- Drag & drop a folder of images to load them as sequential animation frames, with automatic directional grouping (NE, E, SE, SW, W, NW subfolders)

### Worldmap Tile Export
- Split a source image into a grid of 350x300 FRM tiles for the Fallout 2 overworld map
- Export all tiles or a single selected tile
- Automatic `.wmap` project file creation for round-trip editing — reload exported tiles without the original source image
- **Note:** Only the default vanilla worldmap size of 1400x1500 (4x5 tiles) has been tested so far. Other sizes may work but are untested.

### Mask (MSK) Editing & Export
- Create and paint 1-bit collision masks on a dedicated MSK layer
- Export MSK mask tiles alongside FRM tiles

### Game Integration
- Set a Fallout 2 game path for one-click export of FRM and MSK files to the correct subdirectories
- Export a minimal `WORLDMAP.TXT` template for registering worldmap tiles

### Paint Tools
- Paint on palettized images using colors from the Fallout palette
- Fill-rect tool with configurable brush size

---

## Tutorials

Detailed step-by-step guides are included in this repository:

- [**Importing & Loading Files**](docs/TUTORIAL_Import_Project.md) — loading images, opening `.wmap` projects, importing a worldmap from a Fallout 2 installation, drag & drop animation loading
- [**Worldmap Tiles & Masks**](docs/TUTORIAL_Map_and_Mask_Export.md) — exporting overworld FRM/MSK tiles, `.wmap` project files, file format reference

---

## Build

### Dependencies (vendored)

- [Dear ImGui 1.90.8](https://github.com/ocornut/imgui) (docking branch)
- [GLFW 3.4](https://www.glfw.org/)
- [GLAD](https://glad.dav1d.de/) (OpenGL loader)
- [stb_image](https://github.com/nothings/stb)
- [ImFileDialog](https://github.com/dfranx/ImFileDialog)
- [tinyfiledialogs](https://sourceforge.net/projects/tinyfiledialogs/)

### Linux (CMake)

```bash
cmake -S . -B build
cmake --build build
```

The executable is `build/FO2MapEdit`. Resources are copied to `build/resources/` automatically.

You may need to uninstall `libtbb-dev` if it conflicts with the CMake build.

### Linux (Shell script — unity build)

```bash
./build_linux.sh                  # debug build
./build_linux.sh release          # optimized build (-O3)
./build_linux.sh release test     # build + run tests
./build_linux.sh coverage test    # build with coverage + run tests
```

### Windows

Open `msk2bmpGUI.sln` in Visual Studio and build from the IDE.

---

## License

This project is licensed under the [GNU General Public License v3.0](LICENSE). Portions of this software were originally licensed under the MIT License.

Licenses for vendored dependencies are in the [Licenses](Licenses/) folder.
