#pragma once

#include "Layer.h"
#include "load_FRM_OpenGL.h"

#include <cstdint>

constexpr int MAX_ENTRANCES = 10;
constexpr int MAX_CITY_AREAS = 64;
constexpr int CITY_NAME_LEN = 48;
constexpr int ENTRANCE_NAME_LEN = 48;

enum class CitySize : uint8_t { SMALL = 0, MEDIUM = 1, LARGE = 2 };

enum class CityBrush : uint8_t {
    SELECT = 0,       // Click to select/deselect (existing behavior)
    PLACE_SMALL = 1,  // Click to place a small city
    PLACE_MEDIUM = 2, // Click to place a medium city
    PLACE_LARGE = 3,  // Click to place a large city
    ERASER = 4,       // Click on a city to remove it
};

struct city_entrance {
    bool enabled;
    int16_t x, y; // town map position
    char map_name[ENTRANCE_NAME_LEN];
    int16_t elevation;
    int16_t tile_num;
    int16_t orientation;
};

struct city_area {
    char area_name[CITY_NAME_LEN];
    int16_t world_x, world_y; // pixel coords on worldmap
    bool start_state;
    bool lock_state;
    CitySize size;
    int16_t townmap_art_idx;
    int16_t townmap_label_art_idx;
    int entrance_count;
    city_entrance entrances[MAX_ENTRANCES];
};

struct city_layer_data {
    int area_count;
    int selected_area;      // -1 = none
    CityBrush active_brush; // UI-only state, not serialized
    city_area areas[MAX_CITY_AREAS];
};

// Parse Fallout 2 CITY.TXT file. Caller owns returned pointer (free with free()).
city_layer_data* parse_city_txt(const char* path);

// Write city data to a Fallout 2 CITY.TXT file. Returns true on success.
bool write_city_txt(const char* path, city_layer_data* data);

// Draw city markers onto an 8-bit surface.
void render_city_markers(city_layer_data* data, Surface* srfc);

// Serialize city data for .wmap save. Caller must free returned buffer.
uint8_t* serialize_city_data(city_layer_data* data, int* out_size);

// Deserialize city data from .wmap load. Caller owns returned pointer.
city_layer_data* deserialize_city_data(const uint8_t* buf, int size);

// Add a CITY overlay slot to img_data. Returns overlay index or -1.
int create_city_overlay(image_data* img, city_layer_data* data);

// Re-render city markers after edits (e.g. selection change).
void refresh_city_overlay(OverlayLayer* layer);

struct variables;
struct maps_txt_data;

// Interactive city editing handler (mouse hit-test, selection, placement, erasure).
// Returns true if the city data was modified (placement or erasure).
bool Edit_City_Layer(variables* vars, ImVec2 img_pos, image_data* img_data, image_data* edit_data,
                     int layer_idx);

// ImGui panel showing selected city info. Returns true if any field was modified.
bool draw_city_info_panel(OverlayLayer* layer, bool editing, maps_txt_data* maps = nullptr);

// ImGui panel listing ALL cities as expandable tree nodes. Returns true if any field was modified.
bool draw_cities_info_panel(OverlayLayer* layer, bool editing, maps_txt_data* maps = nullptr);
