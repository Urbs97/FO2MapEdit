# Creating and Exporting Worldmap Tiles & Masks for the Fallout 2 Engine

A step-by-step guide for using FO2MapEdit to create worldmap tile art and collision masks, then export them into a modded Fallout 2 installation.

---

## Table of Contents

1. [Background: How Worldmap Tiles Work](#1-background-how-worldmap-tiles-work)
2. [Preparing Your Source Image](#2-preparing-your-source-image)
3. [Exporting Worldmap Tiles (FRM) and Masks (MSK)](#3-exporting-worldmap-tiles-frm-and-masks-msk)
4. [Working with Worldmap Projects (.wmap)](#4-working-with-worldmap-projects-wmap)
5. [File Format Reference](#5-file-format-reference)

---

## 1. Background: How Worldmap Tiles Work

The Fallout 2 overworld travel map is composed of rectangular tiles with fixed dimensions hardcoded in the engine:

| Tile Type | Dimensions | Format | Purpose |
|-----------|-----------|--------|---------|
| Worldmap tile | 350 x 300 px | `.FRM` (8-bit indexed) | Background art for the overworld travel map |
| Mask tile | 350 x 300 px | `.MSK` (1-bit packed) | Collision/blocking overlay for worldmap tiles |

All FRM image files use the **Fallout palette**, a fixed 256-color lookup table. Any source artwork must be converted (palettized) to fit within these 256 colors before it can be used in-game.

### Worldmap File Locations

```
Fallout 2/
  data/
    art/
      intrface/             <-- worldmap FRM tiles go here
    data/                   <-- MSK mask files go here
```

---

## 2. Preparing Your Source Image

Your source image dimensions should be multiples of **350 x 300** pixels. FO2MapEdit will split the image into a grid of 350x300 tiles automatically.

| Source Image Size | Tiles Produced |
|-------------------|----------------|
| 350 x 300 | 1 tile |
| 700 x 300 | 2 tiles (1 row, 2 columns) |
| 700 x 600 | 4 tiles (2 rows, 2 columns) |
| 1050 x 900 | 9 tiles (3 rows, 3 columns) |

### Palette Considerations

- Fallout 2 images are **8-bit indexed** (256 colors max).
- FO2MapEdit palettizes your image using the built-in Fallout palette (`resources/palette/fo_color.pal`).
- The palettization uses **Euclidean distance color matching** with optional **Floyd-Steinberg dithering** to find the closest palette color for each pixel.
- **Palette index 0** is reserved for transparency.
- For best results, author your artwork with the Fallout palette in mind, or at least use a limited color range that maps well to it.

---

## 3. Exporting Worldmap Tiles (FRM) and Masks (MSK)

### Step 1: Create a New Worldmap Project

Go to **File > "New Worldmap Project"** and select your source image. FO2MapEdit accepts standard image formats (PNG, BMP, JPG). The image dimensions must be exact multiples of 350x300 pixels, if they aren't, an error is shown.  

A dialog appears showing the image size, the calculated tile grid (e.g., "Grid: 4 x 5 tiles"), and a **base name** field (max 6 characters, default: `WRLDMP`). Click **OK** to create the project. The image is automatically palettized to the Fallout 256-color palette during creation.

The project opens in a preview window with the **"Export Worldmap Tiles"** and **"Export Town-Map Tiles"** buttons available in the toolbar.

### Step 2: Enable Editing and Paint a Mask (Optional)

MSK files define **collision/blocking data** for the worldmap. Each pixel in the mask is a single bit: **1 = blocked** (solid), **0 = passable**.

To create or edit a mask:

1. Click **"Enable Editing"** in the toolbar. This enters edit mode and automatically creates an MSK layer for the worldmap project.
2. In the **Layers** panel, select the **Mask** layer to switch to mask editing. Use the **V** button next to the layer name to toggle mask overlay visibility.
3. Use the built-in **paint tools** to define blocked regions. The mask displays as a two-color (black and white) overlay on top of the map image.
4. Select the **Map** layer to switch back to editing the FRM image. Mask edits are committed automatically when switching layers.
5. Click **"Disable Editing"** to return to preview mode. Your mask edits are preserved.

### Step 3: Export Worldmap FRM Tiles

1. Click **"Export Worldmap Tiles"** in the toolbar. This opens the tile export dialog.
2. Choose a selection mode:
   - **All Tiles**: exports every tile in the grid (default).
   - **Single Tile**: click on the tile grid to select one tile.
3. If an MSK mask layer exists, the dialog shows an **"Also export MSK tiles"** checkbox. If the mask layer already contains data, this checkbox is checked by default.
4. The **base name** controls the tile filenames. The tool appends a 2-digit number automatically:
   ```
   WRLDMP00.FRM, WRLDMP01.FRM, WRLDMP02.FRM, ...
   WRLDMP00.MSK, WRLDMP01.MSK, WRLDMP02.MSK, ...   (if checkbox is checked)
   ```
   The total filename length is limited to 8 characters (6 for the base name + 2 digits). For worldmap projects, the base name is currently **read-only** (set when the project was created).
5. Export the tiles:

   You must have your Fallout 2 path set via **File > "Set Fallout2.exe Path"**, the **"Export Worldmap Tiles"** button is disabled until the path is configured. Clicking the button exports tiles directly to the correct game subdirectories:
   - FRM tiles go to `{game_path}/data/art/intrface/`
   - MSK tiles go to `{game_path}/data/data/`

   Existing files are silently overwritten, and no folder picker is shown. The directories are created automatically if they don't exist.

After a successful export, a **`worldmap.txt`** file is generated alongside the MSK tiles at `{game_path}/data/data/worldmap.txt`. The generated file contains a minimal working configuration with placeholder encounter data that you can customize later.  

To save your project for later editing, use **File > Save Project** (Ctrl+S) to create or update a `.wmap` project file. See [Section 4](#4-reopening-an-exported-worldmap-wmap) for details.  

Each exported FRM tile is a complete FRM file containing:
- A 62-byte FRM header (version 4, 1 frame per orientation)
- A 12-byte frame descriptor (350x300 dimensions)
- 105,000 bytes of 8-bit indexed pixel data

MSK files are raw binary with no header, just packed bits, line by line:
- Each line is `ceil(350 / 8) = 44 bytes` (352 bits, with 2 padding bits per line)
- 300 lines per tile = **13,200 bytes** total per MSK file

---

## 4. Working with Worldmap Projects (.wmap)

The `.wmap` project file is a self-contained binary file that stores the full stitched worldmap image (and optional MSK mask data) so you can continue editing without needing the original source image or the exported tile files.

### Saving a .wmap Project

Use **File > Save Project** (Ctrl+S) to save your current worldmap as a `.wmap` file. If the project has been saved before, it overwrites the existing file. If it hasn't been saved yet, a file dialog prompts you to choose a location.

### Loading a .wmap File

Open the `.wmap` file the same way you would open any other image:
- **Drag and drop** the `.wmap` file into the application window, or
- Use the **File** menu and select the `.wmap` file (it appears in the file type filter).

FO2MapEdit will read the embedded pixel data from the project file and reconstruct the full-resolution worldmap image. If the project includes MSK mask data, that is loaded as well.

The result is identical to having the original source image loaded, the **"Export Worldmap Tiles"** button is immediately available, and you can enter edit mode and re-export. If the project includes MSK data, the mask layer is carried into edit mode automatically, you can switch to the Mask layer and paint on top of the existing mask data without starting from scratch.

### Round-Trip Workflow

A typical iterative workflow looks like:

1. Load a source image, palettize, paint a mask, export worldmap tiles.
2. Save the project via **File > Save Project** (Ctrl+S).
3. Test in-game.
4. Reopen the `.wmap` file to make edits.
5. Modify tile art or mask data as needed.
6. Re-export tiles and save the project again.

---

## 5. File Format Reference

### FRM File (Worldmap Tile)

All multi-byte values are **big-endian**.

```
FRM Header (62 bytes):
  Offset  Size  Field
  0x0000  4     Version              (always 4 for tiles)
  0x0004  2     FPS                  (1 for tiles)
  0x0006  2     Action Frame         (0 for tiles)
  0x0008  2     Frames Per Orient    (1 for tiles)
  0x000A  12    Shift Orient X[6]    (all 0 for tiles)
  0x0016  12    Shift Orient Y[6]    (all 0 for tiles)
  0x0022  24    Frame 0 Offset[6]    (first entry used)
  0x003A  4     Frame Area           (pixel data size + frame header)

FRM Frame (12 bytes):
  0x003E  2     Frame Width          (350)
  0x0040  2     Frame Height         (300)
  0x0042  4     Frame Size           (105,000)
  0x0046  2     Shift Offset X       (0)
  0x0048  2     Shift Offset Y       (0)

Pixel Data:
  0x004A  W*H   8-bit palette indices (row-major, top-to-bottom)
```

**Total file size:** 62 + 12 + 105,000 = **105,074 bytes**

### MSK File

Raw binary, no header. Bits are packed MSB-first (bitmask starts at 128, shifts right).

```
For a 350x300 tile:
  Bytes per line: ceil(350 / 8) = 44
  Total lines:    300
  File size:      44 * 300 = 13,200 bytes

Bit packing per line:
  Bit 7 (0x80) = first pixel
  Bit 6 (0x40) = second pixel
  ...
  Bit 0 (0x01) = eighth pixel
  (next byte for pixels 9-16, etc.)
  Last byte of each line may have unused trailing bits.

Values:
  1 = blocked/solid
  0 = passable/open
```

### Worldmap Project File (.wmap)

Binary format (version 2). The file embeds the full stitched worldmap pixel data so the project is self-contained.

```
Header (44 bytes):
  Offset  Size  Field
  0x0000  4     Magic            "WMAP" (4 ASCII bytes)
  0x0004  4     Version          2
  0x0008  8     Base Name        null-padded (max 7 characters + null)
  0x0010  4     Tiles X          number of tile columns
  0x0014  4     Tiles Y          number of tile rows
  0x0018  4     Flags            bit 0: has_msk (1 if MSK data is present)
  0x001C  4     FRM Offset       byte offset of FRM pixel data (always 44)
  0x0020  4     FRM Size         FRM pixel data size in bytes
  0x0024  4     MSK Offset       byte offset of MSK pixel data (0 if none)
  0x0028  4     MSK Size         MSK pixel data size in bytes (0 if none)

FRM Pixel Data (at FRM Offset):
  tiles_x * 350 * tiles_y * 300 bytes of 8-bit palette indices
  (full stitched worldmap image, row-major)

MSK Pixel Data (at MSK Offset, if has_msk):
  Same dimensions as FRM data, 1 byte per pixel
  (full stitched mask image)
```

Tile dimensions are always 350x300 (hardcoded in the Fallout 2 engine) and are not stored in the file.
