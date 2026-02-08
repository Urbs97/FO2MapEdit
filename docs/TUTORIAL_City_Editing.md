# Editing and Exporting City Data (CITY.TXT) in FO2MapEdit

A step-by-step guide to importing, viewing, editing, and exporting city placement data using FO2MapEdit.

---

## Table of Contents

1. [Background: How CITY.TXT Works](#1-background-how-citytxt-works)
2. [Importing City Data](#2-importing-city-data)
3. [Viewing Cities on the Worldmap](#3-viewing-cities-on-the-worldmap)
4. [Editing City Properties](#4-editing-city-properties)
5. [Exporting CITY.TXT](#5-exporting-citytxt)

---

## 1. Background: How CITY.TXT Works

Fallout 2 uses a file called `CITY.TXT` (located at `data/data/city.txt` relative to the game root) to define all cities and locations that appear on the overworld travel map. Each city entry specifies:

- **Name** — the display name of the location
- **World position** — pixel coordinates on the worldmap
- **Size** — Small, Medium, or Large (controls the area on the map)
- **Start state** — whether the city is visible at game start (On/Off)
- **Lock state** — whether to save the location of this city and mark it on the map. If On, the location is not saved between sessions. If Off (the default), the location is saved normally. Not required in the file; defaults to Off if omitted
- **Town map art indices** — references to the art used in the town map screen
- **Entrances** — up to 10 named entry points into the city, each linking to a specific map, elevation, and tile

FO2MapEdit can import this file from an existing Fallout 2 installation, display the cities as an interactive overlay on the worldmap, let you edit the core city properties, and export a modified `CITY.TXT` back out.

---

## 2. Importing City Data

City data is loaded automatically when you import a worldmap from a Fallout 2 installation.

### Steps

1. Go to **File > Import Worldmap from FO2**.
2. Select either the **Fallout 2 game root** folder or its **`data/`** subfolder.
3. FO2MapEdit parses `worldmap.txt` for tile data and also looks for `data/city.txt` in the same data folder.
4. If `city.txt` is found and contains valid city entries, a **City** overlay layer is created on the worldmap automatically.

If `city.txt` is not found or is empty, the worldmap imports normally without a city layer. You can still work with tiles and masks as usual.

### What gets loaded

The parser reads all `[Area NN]` sections from the file. For each city area it loads the name, world position, size, start/lock states, town map art indices, and all entrance definitions. Comments (lines starting with `;`) and inline comments are handled correctly.

City data is also saved into `.wmap` project files, so the next time you open the project the city layer is restored automatically without needing the original `city.txt`.

---

## 3. Viewing Cities on the Worldmap

After importing, cities appear as **green markers** on the worldmap. The marker size reflects the city's configured size:

| City Size | Marker Radius |
|-----------|---------------|
| Small     | 3 pixels      |
| Medium    | 5 pixels      |
| Large     | 7 pixels      |

The city overlay is drawn as a semi-transparent green layer on top of the map image. You can toggle its visibility using the **V** button next to the City layer in the **Layers** panel.

### Interacting with markers

- **Hover** over a marker to see a green highlight circle around it.
- **Left-click** a marker to select the city. The selected city shows a yellow highlight circle and the **City Info** panel opens.
- **Left-click** the selected city again to deselect it.
- Only one city can be selected at a time.

---

## 4. Editing City Properties

When a city is selected and the City layer is active, you can edit its properties in the **City Info** panel.

### Activating edit mode

1. Click **"Enable Editing"** in the toolbar if you haven't already.
2. Select the **City** layer in the **Layers** panel
3. Click a city marker on the map. The **City Info** window opens with editable fields.

### Editable fields

| Field | Control | Description |
|-------|---------|-------------|
| **City Name** | Text input | The display name of the location (max 48 characters) |
| **Position X** | Integer input | Horizontal pixel position on the worldmap |
| **Position Y** | Integer input | Vertical pixel position on the worldmap |
| **Size** | Dropdown | Small, Medium, or Large — controls the marker size |
| **Start** | Checkbox | Whether the city is visible at game start |
| **Lock** | Checkbox | Whether to save the city location. On = location is not saved between sessions; Off = location is saved normally (default) |

Edits take effect immediately — the city marker updates its position and size on the map in real-time as you change values.

### Read-only fields

The following properties are displayed but not editable in the current version:

- **Entrance list** — shows each entrance's enabled status, map name, position, elevation, tile number, and orientation
- **Town map art indices** — the art index values for the town map screen

### Unsaved changes

When you modify any city property, the project is marked as having unsaved changes. Save the project via **File > Save Project** (Ctrl+S) to persist your edits to the `.wmap` file.

---

## 5. Exporting CITY.TXT

Modified city data can be exported as a Fallout 2-compatible `CITY.TXT` file alongside your worldmap tiles.

### Exporting with a Fallout 2 game path set

1. Click **"Export Worldmap"** button in the toolbar.
2. If city data exists in the project, an **"Also export CITY.TXT"** checkbox appears in the export dialog.
3. Check the box and proceed with the export.
4. The `CITY.TXT` file is written to `{game_path}/data/data/` — the same location the Fallout 2 engine reads it from.

