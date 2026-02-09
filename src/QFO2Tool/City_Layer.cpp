#include "City_Layer.h"

#include "Image2Texture.h"
#include "Maps_Txt.h"
#include "display_FRM_OpenGL.h"
#include "imgui.h"
#include "platform_io.h"
#include "txt_parse_helpers.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// --- CITY.TXT Parser ---

static CitySize parse_city_size(const char* val) {
    while (*val == ' ' || *val == '\t') {
        val++;
    }
    if (strncasecmp(val, "Large", 5) == 0) {
        return CitySize::LARGE;
    }
    if (strncasecmp(val, "Medium", 6) == 0) {
        return CitySize::MEDIUM;
    }
    return CitySize::SMALL;
}

city_layer_data* parse_city_txt(const char* path) {
    char* txt = io_load_txt_file(path);
    if (txt == nullptr) {
        printf("Error: parse_city_txt(), unable to load: %s\n", path);
        return nullptr;
    }

    city_layer_data* data = (city_layer_data*)calloc(1, sizeof(city_layer_data));
    if (data == nullptr) {
        free(txt);
        return nullptr;
    }
    data->selected_area = -1;

    city_area* current = nullptr;
    int current_idx = -1;

    char* line = txt;
    while (line != nullptr && *line != '\0') {
        char* eol = strchr(line, '\n');
        int line_len = (eol != nullptr) ? (int)(eol - line) : (int)strlen(line);

        // Make null-terminated copy
        char buf[512];
        int copy_len = (line_len < (int)sizeof(buf) - 1) ? line_len : (int)sizeof(buf) - 1;
        memcpy(buf, line, copy_len);
        buf[copy_len] = '\0';
        trim_trailing(buf);

        // Skip comments and empty lines
        if (buf[0] == ';' || buf[0] == '\0') {
            line = (eol != nullptr) ? eol + 1 : nullptr;
            continue;
        }

        // Check for [Area NN] section header
        int area_num = -1;
        if (sscanf(buf, "[Area %d]", &area_num) == 1) {
            if (data->area_count >= MAX_CITY_AREAS) {
                printf("Warning: parse_city_txt(), max areas reached (%d)\n", MAX_CITY_AREAS);
                line = (eol != nullptr) ? eol + 1 : nullptr;
                continue;
            }
            current_idx = data->area_count;
            current = &data->areas[current_idx];
            memset(current, 0, sizeof(city_area));
            current->townmap_art_idx = -1;
            current->townmap_label_art_idx = -1;
            data->area_count++;
            line = (eol != nullptr) ? eol + 1 : nullptr;
            continue;
        }

        if (current == nullptr) {
            line = (eol != nullptr) ? eol + 1 : nullptr;
            continue;
        }

        // Strip inline comments (;)
        char* comment = strchr(buf, ';');
        if (comment != nullptr) {
            *comment = '\0';
            trim_trailing(buf);
        }

        // Parse key=value
        char* eq = strchr(buf, '=');
        if (eq == nullptr) {
            line = (eol != nullptr) ? eol + 1 : nullptr;
            continue;
        }

        *eq = '\0';
        char* key = buf;
        char* val = eq + 1;
        // Trim key
        trim_trailing(key);
        // Skip leading whitespace on key
        while (*key == ' ' || *key == '\t') {
            key++;
        }

        if (strcasecmp(key, "area_name") == 0) {
            while (*val == ' ' || *val == '\t') {
                val++;
            }
            strncpy(current->area_name, val, CITY_NAME_LEN - 1);
            current->area_name[CITY_NAME_LEN - 1] = '\0';
        } else if (strcasecmp(key, "world_pos") == 0) {
            int wx = 0;
            int wy = 0;
            if (sscanf(val, "%d,%d", &wx, &wy) == 2) {
                current->world_x = (int16_t)wx;
                current->world_y = (int16_t)wy;
            }
        } else if (strcasecmp(key, "start_state") == 0) {
            current->start_state = parse_on_off(val);
        } else if (strcasecmp(key, "lock_state") == 0) {
            current->lock_state = parse_on_off(val);
        } else if (strcasecmp(key, "size") == 0) {
            current->size = parse_city_size(val);
        } else if (strcasecmp(key, "townmap_art_idx") == 0) {
            current->townmap_art_idx = (int16_t)atoi(val);
        } else if (strcasecmp(key, "townmap_label_art_idx") == 0) {
            current->townmap_label_art_idx = (int16_t)atoi(val);
        } else if (str_starts_with(key, "entrance_")) {
            int ent_num = atoi(key + 9);
            if (ent_num >= 0 && ent_num < MAX_ENTRANCES) {
                city_entrance* ent = &current->entrances[ent_num];
                ent->elevation = -1;
                ent->tile_num = -1;
                if (ent_num >= current->entrance_count) {
                    current->entrance_count = ent_num + 1;
                }

                // Format: On/Off,X,Y,MapName,elevation,tile,orientation
                // Some entries have spaces after commas
                char state_str[8] = {};
                int ex = -1;
                int ey = -1;
                char map_buf[ENTRANCE_NAME_LEN] = {};
                int elev = -1;
                int tile = -1;
                int orient = 0;

                // Parse state
                char* p = val;
                while (*p == ' ' || *p == '\t') {
                    p++;
                }

                // Read state token
                char* comma = strchr(p, ',');
                if (comma != nullptr) {
                    int slen = (int)(comma - p);
                    slen = std::min(slen, 7);
                    memcpy(state_str, p, slen);
                    state_str[slen] = '\0';
                    trim_trailing(state_str);
                    ent->enabled = parse_on_off(state_str);
                    p = comma + 1;
                }

                // Read X
                if (sscanf(p, "%d", &ex) == 1) {
                    ent->x = (int16_t)ex;
                }
                comma = strchr(p, ',');
                if (comma != nullptr) {
                    p = comma + 1;
                }

                // Read Y
                if (sscanf(p, "%d", &ey) == 1) {
                    ent->y = (int16_t)ey;
                }
                comma = strchr(p, ',');
                if (comma != nullptr) {
                    p = comma + 1;
                }

                // Read map name (everything up to next comma)
                while (*p == ' ') {
                    p++;
                }
                char* next_comma = strchr(p, ',');
                if (next_comma != nullptr) {
                    int mlen = (int)(next_comma - p);
                    if (mlen >= ENTRANCE_NAME_LEN) {
                        mlen = ENTRANCE_NAME_LEN - 1;
                    }
                    memcpy(map_buf, p, mlen);
                    map_buf[mlen] = '\0';
                    trim_trailing(map_buf);
                    p = next_comma + 1;
                } else {
                    // No more commas — rest is map name
                    strncpy(map_buf, p, ENTRANCE_NAME_LEN - 1);
                    map_buf[ENTRANCE_NAME_LEN - 1] = '\0';
                    trim_trailing(map_buf);
                    p = nullptr;
                }
                strncpy(ent->map_name, map_buf, ENTRANCE_NAME_LEN - 1);
                ent->map_name[ENTRANCE_NAME_LEN - 1] = '\0';

                // Read elevation
                if (p != nullptr) {
                    sscanf(p, "%d", &elev);
                    ent->elevation = (int16_t)elev;
                    comma = strchr(p, ',');
                    if (comma != nullptr) {
                        p = comma + 1;
                    } else {
                        p = nullptr;
                    }
                }

                // Read tile_num
                if (p != nullptr) {
                    sscanf(p, "%d", &tile);
                    ent->tile_num = (int16_t)tile;
                    comma = strchr(p, ',');
                    if (comma != nullptr) {
                        p = comma + 1;
                    } else {
                        p = nullptr;
                    }
                }

                // Read orientation
                if (p != nullptr) {
                    sscanf(p, "%d", &orient);
                    ent->orientation = (int16_t)orient;
                }
            }
        }

        line = (eol != nullptr) ? eol + 1 : nullptr;
    }

    free(txt);
    printf("parse_city_txt(): parsed %d areas from %s\n", data->area_count, path);
    return data;
}

// --- CITY.TXT Writer ---

static const char* city_size_str(CitySize size) {
    switch (size) {
        case CitySize::LARGE:
            return "Large";
        case CitySize::MEDIUM:
            return "Medium";
        case CitySize::SMALL:
            return "Small";
    }
    return "Small";
}

bool write_city_txt(const char* path, city_layer_data* data) {
    if (path == nullptr || data == nullptr) {
        return false;
    }

    char txt_path[MAX_PATH];
    snprintf(txt_path, MAX_PATH, "%s/city.txt", path);

    FILE* file = fopen(txt_path, "w");
    if (file == nullptr) {
        printf("Error: write_city_txt(), unable to open %s for writing: L%d\n", txt_path, __LINE__);
        return false;
    }

    fprintf(file, "; Generated by FO2MapEdit\n\n");

    for (int i = 0; i < data->area_count; i++) {
        city_area* area = &data->areas[i];

        fprintf(file, "[Area %02d]\n", i);
        fprintf(file, "area_name=%s\n", area->area_name);
        fprintf(file, "world_pos=%d,%d\n", area->world_x, area->world_y);
        fprintf(file, "start_state=%s\n", area->start_state ? "On" : "Off");
        fprintf(file, "lock_state=%s\n", area->lock_state ? "On" : "Off");
        fprintf(file, "size=%s\n", city_size_str(area->size));
        fprintf(file, "townmap_art_idx=%d\n", area->townmap_art_idx);
        fprintf(file, "townmap_label_art_idx=%d\n", area->townmap_label_art_idx);

        for (int j = 0; j < area->entrance_count; j++) {
            city_entrance* ent = &area->entrances[j];
            fprintf(file, "entrance_%d=%s,%d,%d,%s,%d,%d,%d\n", j, ent->enabled ? "On" : "Off",
                    ent->x, ent->y, ent->map_name, ent->elevation, ent->tile_num, ent->orientation);
        }

        fprintf(file, "\n");
    }

    fclose(file);
    printf("write_city_txt(): wrote %d areas to %s\n", data->area_count, txt_path);
    return true;
}

// --- Marker Rendering ---

static int city_radius(CitySize size) {
    switch (size) {
        case CitySize::LARGE:
            return 7;
        case CitySize::MEDIUM:
            return 5;
        case CitySize::SMALL:
            return 3;
    }
    return 3;
}

static void draw_filled_circle(Surface* srfc, int cx, int cy, int radius, uint8_t value) {
    int r2 = radius * radius;
    for (int dy = -radius; dy <= radius; dy++) {
        int py = cy + dy;
        if (py < 0 || py >= srfc->h) {
            continue;
        }
        for (int dx = -radius; dx <= radius; dx++) {
            int px = cx + dx;
            if (px < 0 || px >= srfc->w) {
                continue;
            }
            if ((dx * dx) + (dy * dy) <= r2) {
                srfc->pxls[(py * srfc->w) + px] = value;
            }
        }
    }
}

static void draw_ring(Surface* srfc, int cx, int cy, int radius, uint8_t value) {
    int r2_outer = (radius + 1) * (radius + 1);
    int r2_inner = (radius - 1) * (radius - 1);
    for (int dy = -(radius + 1); dy <= (radius + 1); dy++) {
        int py = cy + dy;
        if (py < 0 || py >= srfc->h) {
            continue;
        }
        for (int dx = -(radius + 1); dx <= (radius + 1); dx++) {
            int px = cx + dx;
            if (px < 0 || px >= srfc->w) {
                continue;
            }
            int d2 = (dx * dx) + (dy * dy);
            if (d2 <= r2_outer && d2 >= r2_inner) {
                srfc->pxls[(py * srfc->w) + px] = value;
            }
        }
    }
}

void render_city_markers(city_layer_data* data, Surface* srfc) {
    if (data == nullptr || srfc == nullptr || srfc->pxls == nullptr) {
        return;
    }

    // Clear surface
    memset(srfc->pxls, 0, (size_t)srfc->w * srfc->h);

    for (int i = 0; i < data->area_count; i++) {
        city_area* area = &data->areas[i];
        int r = city_radius(area->size);
        int cx = area->world_x;
        int cy = area->world_y;

        if (i == data->selected_area) {
            // Selected: dimmer fill + ring for differentiation
            draw_filled_circle(srfc, cx, cy, r, 128);
            draw_ring(srfc, cx, cy, r + 2, 255);
        } else {
            // Normal: full intensity
            draw_filled_circle(srfc, cx, cy, r, 255);
        }
    }
}

// --- Serialization ---

// Fixed-size binary format:
// [4B] area_count, [4B] selected_area
// Per area (×MAX_CITY_AREAS):
//   [48B name][2B x][2B y][1B start][1B lock][1B size][2B art][2B label_art][4B ent_count]
//   Per entrance (×MAX_ENTRANCES):
//     [1B enabled][2B x][2B y][48B name][2B elev][2B tile][2B orient]

static constexpr int ENTRANCE_SERIAL_SIZE = 1 + 2 + 2 + ENTRANCE_NAME_LEN + 2 + 2 + 2;
static constexpr int AREA_SERIAL_SIZE =
    CITY_NAME_LEN + 2 + 2 + 1 + 1 + 1 + 2 + 2 + 4 + (MAX_ENTRANCES * ENTRANCE_SERIAL_SIZE);
static constexpr int CITY_DATA_SERIAL_SIZE = 4 + 4 + (MAX_CITY_AREAS * AREA_SERIAL_SIZE);

uint8_t* serialize_city_data(city_layer_data* data, int* out_size) {
    if (data == nullptr || out_size == nullptr) {
        return nullptr;
    }

    *out_size = CITY_DATA_SERIAL_SIZE;
    uint8_t* buf = (uint8_t*)calloc(1, CITY_DATA_SERIAL_SIZE);
    if (buf == nullptr) {
        return nullptr;
    }

    uint8_t* p = buf;

    // Header
    memcpy(p, &data->area_count, 4);
    p += 4;
    memcpy(p, &data->selected_area, 4);
    p += 4;

    for (auto& i : data->areas) {
        city_area* area = &i;
        memcpy(p, area->area_name, CITY_NAME_LEN);
        p += CITY_NAME_LEN;
        memcpy(p, &area->world_x, 2);
        p += 2;
        memcpy(p, &area->world_y, 2);
        p += 2;
        *p++ = area->start_state ? 1 : 0;
        *p++ = area->lock_state ? 1 : 0;
        *p++ = (uint8_t)area->size;
        memcpy(p, &area->townmap_art_idx, 2);
        p += 2;
        memcpy(p, &area->townmap_label_art_idx, 2);
        p += 2;
        memcpy(p, &area->entrance_count, 4);
        p += 4;

        for (auto& entrance : area->entrances) {
            city_entrance* ent = &entrance;
            *p++ = ent->enabled ? 1 : 0;
            memcpy(p, &ent->x, 2);
            p += 2;
            memcpy(p, &ent->y, 2);
            p += 2;
            memcpy(p, ent->map_name, ENTRANCE_NAME_LEN);
            p += ENTRANCE_NAME_LEN;
            memcpy(p, &ent->elevation, 2);
            p += 2;
            memcpy(p, &ent->tile_num, 2);
            p += 2;
            memcpy(p, &ent->orientation, 2);
            p += 2;
        }
    }

    return buf;
}

city_layer_data* deserialize_city_data(const uint8_t* buf, int size) {
    if (buf == nullptr || size < CITY_DATA_SERIAL_SIZE) {
        printf("Warning: deserialize_city_data(), buffer too small (%d < %d)\n", size,
               CITY_DATA_SERIAL_SIZE);
        return nullptr;
    }

    city_layer_data* data = (city_layer_data*)calloc(1, sizeof(city_layer_data));
    if (data == nullptr) {
        return nullptr;
    }

    const uint8_t* p = buf;

    memcpy(&data->area_count, p, 4);
    p += 4;
    memcpy(&data->selected_area, p, 4);
    p += 4;

    data->area_count = std::min(data->area_count, MAX_CITY_AREAS);

    for (auto& i : data->areas) {
        city_area* area = &i;
        memcpy(area->area_name, p, CITY_NAME_LEN);
        p += CITY_NAME_LEN;
        area->area_name[CITY_NAME_LEN - 1] = '\0';
        memcpy(&area->world_x, p, 2);
        p += 2;
        memcpy(&area->world_y, p, 2);
        p += 2;
        area->start_state = (*p++ != 0);
        area->lock_state = (*p++ != 0);
        area->size = (CitySize)*p++;
        memcpy(&area->townmap_art_idx, p, 2);
        p += 2;
        memcpy(&area->townmap_label_art_idx, p, 2);
        p += 2;
        memcpy(&area->entrance_count, p, 4);
        p += 4;

        area->entrance_count = std::min(area->entrance_count, MAX_ENTRANCES);

        for (auto& entrance : area->entrances) {
            city_entrance* ent = &entrance;
            ent->enabled = (*p++ != 0);
            memcpy(&ent->x, p, 2);
            p += 2;
            memcpy(&ent->y, p, 2);
            p += 2;
            memcpy(ent->map_name, p, ENTRANCE_NAME_LEN);
            p += ENTRANCE_NAME_LEN;
            ent->map_name[ENTRANCE_NAME_LEN - 1] = '\0';
            memcpy(&ent->elevation, p, 2);
            p += 2;
            memcpy(&ent->tile_num, p, 2);
            p += 2;
            memcpy(&ent->orientation, p, 2);
            p += 2;
        }
    }

    return data;
}

// --- Overlay Management ---

int create_city_overlay(image_data* img, city_layer_data* data) {
    if (img == nullptr || data == nullptr) {
        return -1;
    }

    int idx = add_overlay(img->overlay, &img->overlay_count, LayerType::CITY, LayerBlend::COLOR_MIX,
                          "City", 0.0F, 0.8F, 0.0F, 0.6F);
    if (idx < 0) {
        return -1;
    }

    OverlayLayer* layer = &img->overlay[idx];
    layer->interactive = true;
    layer->editable = false;
    layer->source_data = data;
    layer->source_data_size = sizeof(city_layer_data);

    // Create 8-bit surface at worldmap dimensions
    Surface* srfc = Create_8Bit_Surface(img->width, img->height, nullptr);
    if (srfc == nullptr) {
        printf("Error: create_city_overlay(), surface alloc failed\n");
        img->overlay_count--;
        *layer = OverlayLayer{};
        return -1;
    }

    layer->srfc = srfc;
    render_city_markers(data, srfc);
    layer->texture = init_texture(srfc, srfc->w, srfc->h, img_type::MSK);
    SURFACE_to_texture(srfc, layer->texture, srfc->w, srfc->h, 1);

    printf("create_city_overlay(): created city overlay with %d areas\n", data->area_count);
    return idx;
}

void refresh_city_overlay(OverlayLayer* layer) {
    if (layer == nullptr || layer->type != LayerType::CITY) {
        return;
    }
    if (layer->srfc == nullptr || layer->source_data == nullptr) {
        return;
    }

    city_layer_data* data = (city_layer_data*)layer->source_data;
    render_city_markers(data, layer->srfc);
    SURFACE_to_texture(layer->srfc, layer->texture, layer->srfc->w, layer->srfc->h, 1);
}

// --- Interactive Editing ---

static bool add_city(city_layer_data* data, int16_t x, int16_t y, CitySize size) {
    if (data->area_count >= MAX_CITY_AREAS) {
        return false;
    }

    city_area* area = &data->areas[data->area_count];
    *area = city_area{};
    snprintf(area->area_name, CITY_NAME_LEN, "New City %d", data->area_count);
    area->world_x = x;
    area->world_y = y;
    area->size = size;
    area->start_state = true;
    area->lock_state = false;
    area->townmap_art_idx = 0;
    area->townmap_label_art_idx = 0;
    area->entrance_count = 0;

    data->area_count++;
    return true;
}

static bool remove_city(city_layer_data* data, int index) {
    if (index < 0 || index >= data->area_count) {
        return false;
    }

    for (int i = index; i < data->area_count - 1; i++) {
        data->areas[i] = data->areas[i + 1];
    }
    data->area_count--;
    data->areas[data->area_count] = city_area{};

    if (data->selected_area == index) {
        data->selected_area = -1;
    } else if (data->selected_area > index) {
        data->selected_area--;
    }
    return true;
}

bool Edit_City_Layer(variables* vars, ImVec2 img_pos, image_data* img_data, image_data* edit_data,
                     int layer_idx) {
    if (vars == nullptr || img_data == nullptr) {
        return false;
    }
    if (layer_idx < 0 || layer_idx >= img_data->overlay_count) {
        return false;
    }

    OverlayLayer* layer = &img_data->overlay[layer_idx];
    if (layer->source_data == nullptr) {
        return false;
    }

    city_layer_data* data = (city_layer_data*)layer->source_data;

    // Convert mouse position to map pixel coordinates
    ImVec2 mouse = ImGui::GetMousePos();
    float scale = edit_data->scale;
    float map_x = (mouse.x - img_pos.x) / scale;
    float map_y = (mouse.y - img_pos.y) / scale;

    // Hit test cities
    int hover_idx = -1;
    for (int i = 0; i < data->area_count; i++) {
        city_area* area = &data->areas[i];
        float dx = map_x - (float)area->world_x;
        float dy = map_y - (float)area->world_y;
        float r = (float)city_radius(area->size) + 2.0F; // slight tolerance
        if ((dx * dx) + (dy * dy) <= r * r) {
            hover_idx = i;
            break;
        }
    }

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Cursor visualization for placement tools
    bool is_placement = (data->active_brush == CityBrush::PLACE_SMALL ||
                         data->active_brush == CityBrush::PLACE_MEDIUM ||
                         data->active_brush == CityBrush::PLACE_LARGE);
    if (is_placement && ImGui::IsWindowHovered()) {
        CitySize brush_size = CitySize::SMALL;
        if (data->active_brush == CityBrush::PLACE_MEDIUM) {
            brush_size = CitySize::MEDIUM;
        } else if (data->active_brush == CityBrush::PLACE_LARGE) {
            brush_size = CitySize::LARGE;
        }
        float r = (float)city_radius(brush_size) * scale;
        float cx = img_pos.x + (map_x * scale);
        float cy = img_pos.y + (map_y * scale);
        draw_list->AddCircle(ImVec2(cx, cy), r, IM_COL32(0, 255, 0, 100), 24, 1.5F);
    }

    // Draw hover highlight
    if (hover_idx >= 0) {
        city_area* area = &data->areas[hover_idx];
        float cx = img_pos.x + ((float)area->world_x * scale);
        float cy = img_pos.y + ((float)area->world_y * scale);
        float r = ((float)city_radius(area->size) + 3.0F) * scale;
        // Red highlight for eraser, green otherwise
        ImU32 hover_color = (data->active_brush == CityBrush::ERASER) ? IM_COL32(255, 60, 60, 200)
                                                                      : IM_COL32(0, 255, 0, 180);
        draw_list->AddCircle(ImVec2(cx, cy), r, hover_color, 24, 2.0F);
    }

    // Draw selection highlight
    if (data->selected_area >= 0 && data->selected_area < data->area_count) {
        city_area* area = &data->areas[data->selected_area];
        float cx = img_pos.x + ((float)area->world_x * scale);
        float cy = img_pos.y + ((float)area->world_y * scale);
        float r = ((float)city_radius(area->size) + 4.0F) * scale;
        draw_list->AddCircle(ImVec2(cx, cy), r, IM_COL32(255, 255, 0, 220), 24, 2.5F);
    }

    // Left-click handler
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered()) {
        switch (data->active_brush) {
            case CityBrush::SELECT: {
                int prev_selected = data->selected_area;
                if (hover_idx >= 0 && hover_idx != data->selected_area) {
                    data->selected_area = hover_idx;
                } else {
                    data->selected_area = -1;
                }
                if (data->selected_area != prev_selected) {
                    refresh_city_overlay(layer);
                }
                break;
            }
            case CityBrush::PLACE_SMALL:
            case CityBrush::PLACE_MEDIUM:
            case CityBrush::PLACE_LARGE: {
                CitySize size = CitySize::SMALL;
                if (data->active_brush == CityBrush::PLACE_MEDIUM) {
                    size = CitySize::MEDIUM;
                } else if (data->active_brush == CityBrush::PLACE_LARGE) {
                    size = CitySize::LARGE;
                }
                if (add_city(data, (int16_t)map_x, (int16_t)map_y, size)) {
                    data->selected_area = data->area_count - 1;
                    data->active_brush = CityBrush::SELECT;
                    refresh_city_overlay(layer);
                    return true;
                }
                break;
            }
            case CityBrush::ERASER: {
                if (hover_idx >= 0) {
                    remove_city(data, hover_idx);
                    refresh_city_overlay(layer);
                    return true;
                }
                break;
            }
        }
    }

    return false;
}

// --- Info Panel ---

static void tip(const char* desc) {
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    ImGui::SetItemTooltip("%s", desc);
}

// Shared helper: draws all per-city fields and entrances.
// Caller must wrap in PushID/PopID for widget ID uniqueness.
static bool draw_city_area_content(city_area* area, OverlayLayer* layer, bool editing,
                                   maps_txt_data* maps) {
    bool modified = false;

    if (editing) {
        ImGui::Text("City Name:");
        tip("Name of the location (area_name).");
        if (ImGui::InputText("##city_name", area->area_name, CITY_NAME_LEN)) {
            modified = true;
        }

        int x_int = area->world_x;
        int y_int = area->world_y;
        ImGui::Text("Position:");
        tip("Pixel position on the world map (world_pos).");
        ImGui::SetNextItemWidth(100);
        if (ImGui::InputInt("##city_x", &x_int)) {
            area->world_x = (int16_t)x_int;
            modified = true;
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        if (ImGui::InputInt("##city_y", &y_int)) {
            area->world_y = (int16_t)y_int;
            modified = true;
        }

        int size_int = (int)area->size;
        ImGui::Text("Size:");
        tip("Size of the circle drawn on the world map\n(Small, Medium, or Large).");
        ImGui::SetNextItemWidth(100);
        if (ImGui::Combo("##city_size", &size_int, "Small\0Medium\0Large\0")) {
            area->size = (CitySize)size_int;
            modified = true;
        }

        if (ImGui::Checkbox("Start", &area->start_state)) {
            modified = true;
        }
        ImGui::SetItemTooltip(
            "Whether this city is visible at\nthe start of the game (start_state).");
        ImGui::SameLine();
        if (ImGui::Checkbox("Lock", &area->lock_state)) {
            modified = true;
        }
        ImGui::SetItemTooltip("When On, this location is not saved\nto the map file (lock_state).");

        if (modified) {
            refresh_city_overlay(layer);
        }
    } else {
        ImGui::Text("City: %s", area->area_name);
        tip("Name of the location (area_name).");
        ImGui::Text("Position: %d, %d", area->world_x, area->world_y);
        tip("Pixel position on the world map (world_pos).");

        const char* size_str = "Small";
        if (area->size == CitySize::MEDIUM) {
            size_str = "Medium";
        } else if (area->size == CitySize::LARGE) {
            size_str = "Large";
        }
        ImGui::Text("Size: %s", size_str);
        tip("Size of the circle drawn on the world map\n(Small, Medium, or Large).");

        ImGui::Text("Start: %s  Lock: %s", area->start_state ? "On" : "Off",
                    area->lock_state ? "On" : "Off");
        tip("Start: visible at game start.\nLock: when On, not saved to map file.");
    }

    if (area->entrance_count > 0) {
        ImGui::Separator();
        ImGui::Text("Entrances:");
        for (int j = 0; j < area->entrance_count; j++) {
            city_entrance* ent = &area->entrances[j];
            ImGui::PushID(j);

            char ent_label[256];
            snprintf(ent_label, sizeof(ent_label), "%d: %s %s (%d,%d)###ent", j,
                     ent->enabled ? "[On]" : "[Off]", ent->map_name, ent->x, ent->y);

            bool ent_open = ImGui::TreeNodeEx(ent_label, ImGuiTreeNodeFlags_None);
            ImGui::SetItemTooltip("Entrance %d: %s\nMap: %s\nTown map position: %d, %d", j,
                                  ent->enabled ? "On" : "Off", ent->map_name, ent->x, ent->y);
            if (ent_open) {
                if (editing) {
                    int elev_int = ent->elevation;
                    int tile_int = ent->tile_num;
                    int orient_int = ent->orientation;
                    ImGui::SetNextItemWidth(120);
                    if (ImGui::InputInt("Elevation##ent_elev", &elev_int)) {
                        ent->elevation = (int16_t)elev_int;
                        modified = true;
                    }
                    ImGui::SetItemTooltip("Map elevation the player spawns on (0-2).");
                    ImGui::SetNextItemWidth(120);
                    if (ImGui::InputInt("Tile##ent_tile", &tile_int)) {
                        ent->tile_num = (int16_t)tile_int;
                        modified = true;
                    }
                    ImGui::SetItemTooltip("Hex tile number where the player spawns.");
                    ImGui::SetNextItemWidth(120);
                    if (ImGui::InputInt("Orientation##ent_orient", &orient_int)) {
                        ent->orientation = (int16_t)orient_int;
                        modified = true;
                    }
                    ImGui::SetItemTooltip("Direction the player faces on spawn (0-5).");
                } else {
                    ImGui::Text("Elevation: %d", ent->elevation);
                    tip("Map elevation the player spawns on (0-2).");
                    ImGui::Text("Tile: %d", ent->tile_num);
                    tip("Hex tile number where the player spawns.");
                    ImGui::Text("Orientation: %d", ent->orientation);
                    tip("Direction the player faces on spawn (0-5).");
                }

                if (maps == nullptr) {
                    ImGui::TextDisabled("MAPS.TXT not loaded");
                } else {
                    if (editing) {
                        static char filter_buf[64] = {};
                        const char* preview = ent->map_name[0] != '\0' ? ent->map_name : "(none)";
                        ImGui::Text("Map:");
                        tip("The lookup_name in MAPS.TXT that this\nentrance links to.");
                        if (ImGui::BeginCombo("##map_combo", preview)) {
                            ImGui::SetNextItemWidth(-1);
                            ImGui::InputTextWithHint("##filter", "Filter...", filter_buf,
                                                     sizeof(filter_buf));
                            if (ImGui::IsWindowAppearing()) {
                                ImGui::SetKeyboardFocusHere(-1);
                            }

                            ImGui::Separator();
                            for (int m = 0; m < maps->map_count; m++) {
                                const char* name = maps->maps[m].lookup_name;
                                if (filter_buf[0] != '\0' &&
                                    !str_contains_nocase(name, filter_buf)) {
                                    continue;
                                }
                                bool is_selected = (strcasecmp(name, ent->map_name) == 0);
                                char item_label[128];
                                snprintf(item_label, sizeof(item_label), "[%03d] %s",
                                         maps->maps[m].map_number, name);
                                if (ImGui::Selectable(item_label, is_selected)) {
                                    strncpy(ent->map_name, name, ENTRANCE_NAME_LEN - 1);
                                    ent->map_name[ENTRANCE_NAME_LEN - 1] = '\0';
                                    filter_buf[0] = '\0';
                                    modified = true;
                                }
                                if (is_selected) {
                                    ImGui::SetItemDefaultFocus();
                                }
                            }
                            ImGui::EndCombo();
                        } else {
                            filter_buf[0] = '\0';
                        }
                    }

                    map_entry* me = find_map_by_lookup_name(maps, ent->map_name);
                    if (me == nullptr) {
                        ImGui::TextColored(ImVec4(1.0F, 0.6F, 0.2F, 1.0F),
                                           "No matching map found in MAPS.TXT");
                    } else {
                        ImGui::Separator();
                        ImGui::Text("Map %03d: %s", me->map_number, me->map_name);
                        tip("Map file in master.dat/maps/ (map_name).");

                        if (me->music[0] != '\0') {
                            ImGui::Text("Music: %s", me->music);
                            tip("Background music track, without .ACM extension.");
                        }

                        ImGui::Text("Saved: %s", me->saved ? "Yes" : "No");
                        tip("Yes for cities (state persists between visits),\n"
                            "No for random encounters (map resets).");

                        if (!me->dead_bodies_age) {
                            ImGui::Text("Dead bodies age: No");
                            tip("Whether corpses on this map are\nremoved over time.");
                        }

                        if (!me->can_rest_here[0] || !me->can_rest_here[1] ||
                            !me->can_rest_here[2]) {
                            ImGui::Text("Can rest: %s, %s, %s", me->can_rest_here[0] ? "Yes" : "No",
                                        me->can_rest_here[1] ? "Yes" : "No",
                                        me->can_rest_here[2] ? "Yes" : "No");
                            tip("Whether the player can rest on this map,\n"
                                "per elevation level (0, 1, 2).");
                        }

                        if (!me->pipboy_active) {
                            ImGui::Text("Pipboy active: No");
                            tip("Whether the Pip-Boy is accessible\non this map.");
                        }

                        if (me->state_on) {
                            ImGui::Text("State: On");
                            tip("When On, the map is accessible from the\n"
                                "city without having visited it first.");
                        }

                        if (me->ambient_sfx_count > 0) {
                            ImGui::Text("Ambient SFX:");
                            tip("Background sound effects played on this map,\n"
                                "with percentage weight for frequency.");
                            for (int s = 0; s < me->ambient_sfx_count; s++) {
                                ImGui::Text("  %s (%d%%)", me->ambient_sfx[s].name,
                                            me->ambient_sfx[s].weight);
                            }
                        }

                        if (me->random_start_count > 0) {
                            ImGui::Text("Random starts:");
                            tip("Possible spawn points for random encounters\n"
                                "(elevation and hex tile).");
                            for (int r = 0; r < me->random_start_count; r++) {
                                ImGui::Text("  elev:%d tile:%d", me->random_starts[r].elevation,
                                            me->random_starts[r].tile_num);
                            }
                        }
                    }
                }
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
    }

    return modified;
}

bool draw_city_info_panel(OverlayLayer* layer, bool editing, maps_txt_data* maps) {
    if (layer == nullptr || layer->type != LayerType::CITY || layer->source_data == nullptr) {
        return false;
    }

    city_layer_data* data = (city_layer_data*)layer->source_data;
    if (data->selected_area < 0 || data->selected_area >= data->area_count) {
        ImGui::Text("Click a city marker to select");
        return false;
    }

    city_area* area = &data->areas[data->selected_area];
    ImGui::PushID(data->selected_area);
    bool modified = draw_city_area_content(area, layer, editing, maps);
    ImGui::PopID();
    return modified;
}

bool draw_cities_info_panel(OverlayLayer* layer, bool editing, maps_txt_data* maps) {
    if (layer == nullptr || layer->type != LayerType::CITY || layer->source_data == nullptr) {
        ImGui::TextDisabled("No city data loaded");
        return false;
    }

    city_layer_data* data = (city_layer_data*)layer->source_data;
    bool modified = false;

    ImGui::Text("Cities: %d entries", data->area_count);
    ImGui::Separator();

    static char city_filter[128] = "";
    ImGui::InputTextWithHint("##city_filter", "Search cities...", city_filter, sizeof(city_filter));

    for (int i = 0; i < data->area_count; i++) {
        city_area* area = &data->areas[i];
        if (city_filter[0] != '\0' && !str_contains_nocase(area->area_name, city_filter)) {
            continue;
        }
        ImGui::PushID(i);

        char node_label[128];
        snprintf(node_label, sizeof(node_label), "[%02d] %s", i, area->area_name);

        bool node_open = ImGui::TreeNodeEx(node_label, ImGuiTreeNodeFlags_None);
        if (!node_open) {
            const char* size_str = "Small";
            if (area->size == CitySize::LARGE) {
                size_str = "Large";
            } else if (area->size == CitySize::MEDIUM) {
                size_str = "Medium";
            }
            ImGui::SetItemTooltip("Pos: %d, %d  Size: %s\nStart: %s  Lock: %s", area->world_x,
                                  area->world_y, size_str, area->start_state ? "On" : "Off",
                                  area->lock_state ? "On" : "Off");
        }

        if (node_open) {
            if (draw_city_area_content(area, layer, editing, maps)) {
                modified = true;
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    return modified;
}
