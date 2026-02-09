#include "Maps_Txt.h"

#include "platform_io.h"
#include "txt_parse_helpers.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

// Set default values for a map_entry (called right after memset to 0).
static void set_map_defaults(map_entry* m) {
    m->dead_bodies_age = true;
    m->can_rest_here[0] = true;
    m->can_rest_here[1] = true;
    m->can_rest_here[2] = true;
    m->pipboy_active = true;
}

// Parse comma-separated "name:weight" pairs into ambient_sfx array.
static void parse_ambient_sfx(const char* val, map_entry* m) {
    m->ambient_sfx_count = 0;
    while (*val != '\0' && m->ambient_sfx_count < MAX_AMBIENT_SFX) {
        // Skip whitespace
        while (*val == ' ' || *val == '\t') {
            val++;
        }
        if (*val == '\0') {
            break;
        }

        // Find colon separator
        const char* colon = strchr(val, ':');
        if (colon == nullptr) {
            break;
        }

        ambient_sfx_entry* sfx = &m->ambient_sfx[m->ambient_sfx_count];
        int name_len = (int)(colon - val);
        if (name_len >= (int)sizeof(sfx->name)) {
            name_len = (int)sizeof(sfx->name) - 1;
        }
        memcpy(sfx->name, val, name_len);
        sfx->name[name_len] = '\0';

        // Trim trailing whitespace from name
        int nlen = name_len;
        while (nlen > 0 && (sfx->name[nlen - 1] == ' ' || sfx->name[nlen - 1] == '\t')) {
            sfx->name[--nlen] = '\0';
        }

        sfx->weight = atoi(colon + 1);
        m->ambient_sfx_count++;

        // Advance past this entry
        const char* comma = strchr(colon, ',');
        if (comma != nullptr) {
            val = comma + 1;
        } else {
            break;
        }
    }
}

// Parse "can_rest_here" — either a single Yes/No or three comma-separated values.
static void parse_can_rest_here(const char* val, map_entry* m) {
    while (*val == ' ' || *val == '\t') {
        val++;
    }

    // Check for comma — if present, parse three values
    const char* c1 = strchr(val, ',');
    if (c1 != nullptr) {
        m->can_rest_here[0] = parse_yes_no(val);
        const char* c2 = strchr(c1 + 1, ',');
        if (c2 != nullptr) {
            m->can_rest_here[1] = parse_yes_no(c1 + 1);
            m->can_rest_here[2] = parse_yes_no(c2 + 1);
        } else {
            m->can_rest_here[1] = parse_yes_no(c1 + 1);
            m->can_rest_here[2] = m->can_rest_here[1];
        }
    } else {
        // Single value applies to all three
        bool v = parse_yes_no(val);
        m->can_rest_here[0] = v;
        m->can_rest_here[1] = v;
        m->can_rest_here[2] = v;
    }
}

// Parse "random_start_point_N" — format: "elev:X, tile_num:Y"
static void parse_random_start(const char* key, const char* val, map_entry* m) {
    // Extract N from "random_start_point_N"
    const char* underscore = key + strlen("random_start_point_");
    int idx = atoi(underscore);
    if (idx < 0 || idx >= MAX_RANDOM_STARTS) {
        return;
    }

    random_start_point* rsp = &m->random_starts[idx];

    // Parse "elev:X, tile_num:Y"
    const char* elev_colon = strstr(val, "elev:");
    if (elev_colon != nullptr) {
        rsp->elevation = atoi(elev_colon + 5);
    }

    const char* tile_colon = strstr(val, "tile_num:");
    if (tile_colon != nullptr) {
        rsp->tile_num = atoi(tile_colon + 9);
    }

    if (idx >= m->random_start_count) {
        m->random_start_count = idx + 1;
    }
}

// --- public API ---

std::unique_ptr<maps_txt_data> parse_maps_txt_from_buffer(char* txt) {
    auto data = std::make_unique<maps_txt_data>();

    if (txt == nullptr || *txt == '\0') {
        return data;
    }

    map_entry* current = nullptr;

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

        // Check for [Map NNN] section header
        int map_num = -1;
        if (sscanf(buf, "[Map %d]", &map_num) == 1) {
            if (data->map_count >= MAX_MAP_ENTRIES) {
                printf("Warning: parse_maps_txt(), max map entries reached (%d)\n",
                       MAX_MAP_ENTRIES);
                line = (eol != nullptr) ? eol + 1 : nullptr;
                continue;
            }
            current = &data->maps[data->map_count];
            memset(current, 0, sizeof(map_entry));
            set_map_defaults(current);
            current->map_number = map_num;
            data->map_count++;
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
        while (*key == ' ' || *key == '\t') {
            key++;
        }

        if (strcasecmp(key, "lookup_name") == 0) {
            while (*val == ' ' || *val == '\t') {
                val++;
            }
            strncpy(current->lookup_name, val, MAP_NAME_LEN - 1);
            current->lookup_name[MAP_NAME_LEN - 1] = '\0';
        } else if (strcasecmp(key, "map_name") == 0) {
            while (*val == ' ' || *val == '\t') {
                val++;
            }
            strncpy(current->map_name, val, MAP_NAME_LEN - 1);
            current->map_name[MAP_NAME_LEN - 1] = '\0';
        } else if (strcasecmp(key, "music") == 0) {
            while (*val == ' ' || *val == '\t') {
                val++;
            }
            strncpy(current->music, val, MAP_NAME_LEN - 1);
            current->music[MAP_NAME_LEN - 1] = '\0';
        } else if (strcasecmp(key, "ambient_sfx") == 0) {
            parse_ambient_sfx(val, current);
        } else if (strcasecmp(key, "saved") == 0) {
            current->saved = parse_yes_no(val);
        } else if (strcasecmp(key, "dead_bodies_age") == 0) {
            current->dead_bodies_age = parse_yes_no(val);
        } else if (strcasecmp(key, "can_rest_here") == 0) {
            parse_can_rest_here(val, current);
        } else if (strcasecmp(key, "pipboy_active") == 0) {
            current->pipboy_active = parse_yes_no(val);
        } else if (strcasecmp(key, "state") == 0) {
            while (*val == ' ' || *val == '\t') {
                val++;
            }
            current->state_on = (strncasecmp(val, "On", 2) == 0);
        } else if (str_starts_with(key, "random_start_point_")) {
            parse_random_start(key, val, current);
        }

        line = (eol != nullptr) ? eol + 1 : nullptr;
    }

    return data;
}

std::unique_ptr<maps_txt_data> parse_maps_txt(const char* path) {
    char* txt = io_load_txt_file(path);
    if (txt == nullptr) {
        printf("Error: parse_maps_txt(), unable to load: %s\n", path);
        return nullptr;
    }

    auto data = parse_maps_txt_from_buffer(txt);
    free(txt);

    if (data != nullptr) {
        printf("parse_maps_txt(): parsed %d maps from %s\n", data->map_count, path);
    }
    return data;
}
