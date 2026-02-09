#include "City_Layer.h"

#include "Image2Texture.h"
#include "display_FRM_OpenGL.h"
#include "imgui.h"
#include "platform_io.h"
#include "txt_parse_helpers.h"

#include <algorithm>
#include <cctype>
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

void Edit_City_Layer(variables* vars, ImVec2 img_pos, image_data* img_data, image_data* edit_data,
                     int layer_idx) {
    if (vars == nullptr || img_data == nullptr) {
        return;
    }
    if (layer_idx < 0 || layer_idx >= img_data->overlay_count) {
        return;
    }

    OverlayLayer* layer = &img_data->overlay[layer_idx];
    if (layer->source_data == nullptr) {
        return;
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

    // Draw hover highlight
    if (hover_idx >= 0) {
        city_area* area = &data->areas[hover_idx];
        float cx = img_pos.x + ((float)area->world_x * scale);
        float cy = img_pos.y + ((float)area->world_y * scale);
        float r = ((float)city_radius(area->size) + 3.0F) * scale;
        ImDrawList* draw_list = ImGui::GetForegroundDrawList();
        draw_list->AddCircle(ImVec2(cx, cy), r, IM_COL32(0, 255, 0, 180), 24, 2.0F);
    }

    // Draw selection highlight
    if (data->selected_area >= 0 && data->selected_area < data->area_count) {
        city_area* area = &data->areas[data->selected_area];
        float cx = img_pos.x + ((float)area->world_x * scale);
        float cy = img_pos.y + ((float)area->world_y * scale);
        float r = ((float)city_radius(area->size) + 4.0F) * scale;
        ImDrawList* draw_list = ImGui::GetForegroundDrawList();
        draw_list->AddCircle(ImVec2(cx, cy), r, IM_COL32(255, 255, 0, 220), 24, 2.5F);
    }

    // Left-click: select/deselect
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered()) {
        int prev_selected = data->selected_area;
        if (hover_idx >= 0 && hover_idx != data->selected_area) {
            data->selected_area = hover_idx;
        } else {
            data->selected_area = -1;
        }
        if (data->selected_area != prev_selected) {
            refresh_city_overlay(layer);
        }
    }
}

// --- Info Panel ---

bool draw_city_info_panel(OverlayLayer* layer, bool editing) {
    if (layer == nullptr || layer->type != LayerType::CITY || layer->source_data == nullptr) {
        return false;
    }

    city_layer_data* data = (city_layer_data*)layer->source_data;
    if (data->selected_area < 0 || data->selected_area >= data->area_count) {
        ImGui::Text("Click a city marker to select");
        return false;
    }

    city_area* area = &data->areas[data->selected_area];
    bool modified = false;

    if (editing) {
        ImGui::Text("City Name:");
        if (ImGui::InputText("##city_name", area->area_name, CITY_NAME_LEN)) {
            modified = true;
        }

        int x_int = area->world_x;
        int y_int = area->world_y;
        ImGui::Text("Position:");
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
        ImGui::SetNextItemWidth(100);
        if (ImGui::Combo("##city_size", &size_int, "Small\0Medium\0Large\0")) {
            area->size = (CitySize)size_int;
            modified = true;
        }

        if (ImGui::Checkbox("Start", &area->start_state)) {
            modified = true;
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Lock", &area->lock_state)) {
            modified = true;
        }

        if (modified) {
            refresh_city_overlay(layer);
        }
    } else {
        ImGui::Text("City: %s", area->area_name);
        ImGui::Text("Position: %d, %d", area->world_x, area->world_y);

        const char* size_str = "Small";
        if (area->size == CitySize::MEDIUM) {
            size_str = "Medium";
        } else if (area->size == CitySize::LARGE) {
            size_str = "Large";
        }
        ImGui::Text("Size: %s", size_str);

        ImGui::Text("Start: %s  Lock: %s", area->start_state ? "On" : "Off",
                    area->lock_state ? "On" : "Off");
    }

    if (area->entrance_count > 0) {
        ImGui::Separator();
        ImGui::Text("Entrances:");
        for (int i = 0; i < area->entrance_count; i++) {
            city_entrance* ent = &area->entrances[i];
            ImGui::Text("  %d: %s %s (%d,%d)", i, ent->enabled ? "[On]" : "[Off]", ent->map_name,
                        ent->x, ent->y);
        }
    }

    return modified;
}
