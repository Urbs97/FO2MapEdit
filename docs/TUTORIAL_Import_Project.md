# Importing & Loading Files in FO2MapEdit

A guide to the different ways you can load files and projects into FO2MapEdit.

---

## Table of Contents

1. [Supported File Types](#1-supported-file-types)
2. [Loading Individual Files](#2-loading-individual-files)
3. [Opening a Worldmap Project (.wmap)](#3-opening-a-worldmap-project-wmap)
4. [Creating a New Worldmap Project from an Image](#4-creating-a-new-worldmap-project-from-an-image)
5. [Importing a Worldmap from a Fallout 2 Installation](#5-importing-a-worldmap-from-a-fallout-2-installation)
6. [Loading Animation Sequences via Drag & Drop](#6-loading-animation-sequences-via-drag--drop)
7. [How File Loading Works Internally](#7-how-file-loading-works-internally)

---

## 1. Supported File Types

| Extension | Type | Description |
|-----------|------|-------------|
| `.FRM` | Fallout sprite | 8-bit indexed image with big-endian header, up to 6 orientations and multiple animation frames |
| `.FR0`-`.FR5` | FRM variant | Directional FRM files (one orientation per file) |
| `.MSK` | Mask tile | 1-bit packed collision mask (350x300 pixels) |
| `.WMAP` | Worldmap project | Self-contained binary project file storing stitched FRM pixel data and optional MSK mask data |
| `.PNG` | Image | Standard image, loaded as RGBA |
| `.BMP` | Image | Standard image, loaded as RGBA |
| `.JPG` / `.JPEG` | Image | Standard image, loaded as RGBA |
| `.GIF` | Image | Standard image, loaded as RGBA |

All file extensions are matched **case-insensitively** (e.g., `.FRM`, `.frm`, and `.Frm` are all accepted).

---

## 2. Loading Individual Files

There are three ways to load a single file into FO2MapEdit:

### File Dialog (Load File button)

1. Click the **"Load File"** button in the main window toolbar.
2. A file dialog opens with a filter showing all supported file types.
3. Select a file and click **Open**.

The file dialog remembers the last directory you loaded from.

### Drag & Drop

Drag one or more files from your file manager directly onto the FO2MapEdit application window. Each file opens in its own preview window.

If you drop a file that is already open, the existing window is focused instead of opening a duplicate.

### Command Line

Pass a file path as a command-line argument when launching the application:

```bash
./FO2MapEdit /full/path/to/file.frm
```

**Note:** On Linux, a full (absolute) path is currently required. Relative paths are not yet supported.

---

## 3. Opening a Worldmap Project (.wmap)

A `.wmap` file is a self-contained worldmap project that stores the full stitched worldmap image (and optional mask data) in a single binary file. Opening one restores the complete editing state without needing the original source image or exported tile files.

### How to open

- **File > Open Worldmap Project** (Ctrl+O) — opens a file dialog filtered to `.wmap` files.
- **Load File button** — `.wmap` appears in the file type filter alongside other formats.
- **Drag & drop** — drop a `.wmap` file onto the application window.

---

## 4. Creating a New Worldmap Project from an Image

Use **File > New Worldmap Project** to create a fresh worldmap project from a standard image file.

### Steps

1. Go to **File > New Worldmap Project**. A file dialog opens for selecting a source image (PNG, BMP, or JPG).
2. Select your image. The dimensions must be exact multiples of **350x300 pixels** (the Fallout 2 tile size). If not, an error is shown.
3. A dialog appears showing:
   - The image dimensions (e.g., "1400x1500 pixels")
   - The calculated tile grid (e.g., "Grid: 4 x 5 tiles")
   - A **project name** field (default: `WRLDMP`, max 6 characters)
4. Click **OK** to create the project. The source image is automatically **palettized** to the Fallout 256-color palette during creation.

The new project opens in a preview window with no MSK mask data. You can add a mask layer by entering edit mode and switching to the Mask layer (see the [Worldmap Tiles & Masks tutorial](TUTORIAL_Map_and_Mask_Export.md) for details).

---

## 5. Importing a Worldmap from a Fallout 2 Installation

This feature reads the worldmap tile data directly from an existing Fallout 2 game installation and reconstructs a full editable worldmap project from it. This is useful for modifying an existing worldmap or using it as a starting point.

### Prerequisites

You need a Fallout 2 installation with the worldmap files extracted to disk. In a vanilla Fallout 2 install, these files are packed inside **`master.dat`** and are not directly accessible on the filesystem. You will need to unpack `master.dat` first using a DAT extraction tool (e.g., `fo2dat` or similar) so the files are present as loose files in the `data/` directory tree.

The required files after extraction:

```
Fallout 2/
  data/
    worldmap.txt          <-- worldmap tile definitions
    *.msk                 <-- mask files (optional)
  art/
    intrface/
      intrface.lst        <-- master file list for art/intrface/
      wrldmp00.frm        <-- worldmap FRM tile files
      wrldmp01.frm
      ...
```

The exact filenames and casing don't matter on Linux — FO2MapEdit uses case-insensitive path resolution to handle ALL CAPS filenames that are common in extracted Fallout 2 archives.

### Steps

1. Go to **File > Import Worldmap from FO2**.
2. A folder picker dialog opens. Select either:
   - The **Fallout 2 game root** folder (the one containing the `data/` subfolder), or
   - The **`data/`** folder itself.

   FO2MapEdit auto-detects which one you selected by checking for the presence of `worldmap.txt`.
3. A confirmation dialog appears showing the detected data folder path and a **project name** field (default: `WRLDMP`). You can change the project name here.
4. Click **Import**.

### After import

The imported worldmap opens as a new project with no save path set. Use **File > Save Project** (Ctrl+S) to save it as a `.wmap` file for future editing.

If any MSK files were missing during import, a warning message tells you how many were skipped. The mask layer will be incomplete in those areas but can be painted manually in edit mode.

### Troubleshooting

| Problem | Cause | Solution |
|---------|-------|----------|
| "worldmap.txt not found" | Selected the wrong folder | Select the Fallout 2 root folder (containing `data/`) or the `data/` folder itself |
| "intrface.lst not found" | Missing or misplaced game data | Ensure `art/intrface/intrface.lst` exists in the data folder |
| "Missing FRM files" | FRM tiles referenced in worldmap.txt don't exist | Ensure all worldmap FRM files are present in `art/intrface/` |
| "FRM tile has unexpected dimensions" | A tile file is not 350x300 | The FRM file may be corrupted or not a worldmap tile |
| "N MSK mask files could not be found" | Some mask files are missing | Import succeeds but the mask layer is incomplete; paint missing areas manually |

---

## 6. Loading Animation Sequences via Drag & Drop

FO2MapEdit can load a group of image files as sequential animation frames for a single FRM sprite.

### Single-direction animation

1. Drag & drop a **folder** containing multiple image files (PNG, BMP, JPG) onto the application window.
2. A dialog asks: *"Is this a group of sequential animation frames?"*
3. Click **"Yep, everything in this folder is part of an animation."** to load all images as frames of a single animation.
   - Alternatively, click **"Nope, open all images up individually."** to open each image in its own window.

The images are sorted by filename and loaded as sequential frames in the NE orientation.

### Multi-direction animation

To load animation frames for multiple orientations at once, organize your files into subdirectories named after Fallout's 6 directions:

```
my_animation/
  NE/
    frame01.png
    frame02.png
    frame03.png
  E/
    frame01.png
    frame02.png
    frame03.png
  SE/
    ...
```

Drag the parent folder (`my_animation/`) onto the application window. FO2MapEdit will detect the directional subfolders and load each set of frames into the corresponding orientation slot.

Valid subdirectory names (case-insensitive): `NE`, `E`, `SE`, `SW`, `W`, `NW`.

---

## 7. How File Loading Works Internally

This section provides a technical overview of the loading pipeline for anyone interested in the code architecture.

### Entry points

All files enter the application through one of these entry points:

| Entry Point | Location | Trigger |
|-------------|----------|---------|
| `ImDialog_load_files()` | `Load_Files.cpp` | "Load File" button click |
| `dropped_files_callback()` | `msk2bmpGUI.cpp` | GLFW drag & drop callback |
| Command-line argument | `msk2bmpGUI.cpp` | Program startup |
| `NewWmapImageDialog` | `msk2bmpGUI.cpp` | File > New Worldmap Project |
| `OpenWmapDialog` | `msk2bmpGUI.cpp` | File > Open Worldmap Project |
| `ImportWmapFolderDialog` | `msk2bmpGUI.cpp` | File > Import Worldmap from FO2 |

### Central dispatcher

Most loading flows converge on `File_Type_Check()` in `Load_Files.cpp`, which:

1. Calls `prep_extension()` to extract the file extension and set up prev/next file navigation.
2. Routes to the appropriate loader based on extension:

| Extension | Loader | Result |
|-----------|--------|--------|
| FRM, FR0-FR5 | `load_FRM_OpenGL()` | Parses big-endian FRM header, loads animation frames |
| MSK | `Load_MSK_Tile_SURFACE()` | Loads 1-bit mask into an 8-bit surface |
| WMAP | `load_wmap_project()` | Reads binary project file, restores full project state |
| PNG/BMP/JPG/GIF | `Load_File_to_RGBA()` | Decodes image via stb_image |

3. Creates OpenGL textures and framebuffers for display.
4. Sets `F_Prop->file_open_window = true` to trigger the preview window.

### Worldmap-specific flows

The three worldmap menu actions bypass the generic `File_Type_Check()` dispatcher and use dedicated functions:

- **New Worldmap Project**: `Load_File_to_RGBA()` -> `PAL_Color_Convert()` -> `new_wmap_project()`
- **Open Worldmap Project**: Routes through `File_Type_Check()` -> `load_wmap_project()`
- **Import from FO2**: `import_wmap_from_fo2()` -> `parse_worldmap_txt()` + tile stitching -> `init_wmap_opengl()`

All three ultimately call `init_wmap_opengl()`, which sets up the OpenGL resources, creates a synthetic FRM header for edit mode compatibility, and populates the `wmap_info` metadata structure.

### Duplicate detection

Before opening a file, the application calls `find_open_file()` to check if the same file path is already open. If it is, the existing window is focused instead of creating a duplicate.

### Key data structures

- **`LF`** (`Load_Files.h`) — Per-file state: file paths, image data, edit data, window flags, and an optional `wmap_info*` pointer (non-null for worldmap projects).
- **`image_data`** (`load_FRM_OpenGL.h`) — Image payload: FRM headers, animation directions (`ANM_Dir[6]`), OpenGL textures, framebuffers.
- **`wmap_info`** (`Worldmap_Project.h`) — Worldmap project metadata: version, base name, tile grid dimensions, mask flag, save path.
