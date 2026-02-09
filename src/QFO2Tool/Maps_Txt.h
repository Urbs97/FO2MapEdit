#pragma once

#include <cstdint>

constexpr int MAX_MAP_ENTRIES = 200;
constexpr int MAP_NAME_LEN = 48;
constexpr int MAX_AMBIENT_SFX = 10;
constexpr int MAX_RANDOM_STARTS = 10;

struct ambient_sfx_entry {
    char name[32];
    int weight; // percentage 0-100
};

struct random_start_point {
    int elevation;
    int tile_num;
};

struct map_entry {
    int map_number; // from [Map NNN]
    char lookup_name[MAP_NAME_LEN];
    char map_name[MAP_NAME_LEN];
    char music[MAP_NAME_LEN];

    int ambient_sfx_count;
    ambient_sfx_entry ambient_sfx[MAX_AMBIENT_SFX];

    bool saved;            // Yes/No
    bool dead_bodies_age;  // Yes/No, default true
    bool can_rest_here[3]; // per elevation, default true
    bool pipboy_active;    // Yes/No, default true
    bool state_on;         // state=On flag

    int random_start_count;
    random_start_point random_starts[MAX_RANDOM_STARTS];
};

struct maps_txt_data {
    int map_count;
    map_entry maps[MAX_MAP_ENTRIES];
};

// Parse a MAPS.TXT file. Returns nullptr on file load failure. Caller must free().
maps_txt_data* parse_maps_txt(const char* path);

// Parse MAPS.TXT from an in-memory string. Caller must free().
// The input buffer is modified during parsing (nulls inserted).
maps_txt_data* parse_maps_txt_from_buffer(char* txt);

// Write maps.txt to the given directory. Returns true on success.
bool write_maps_txt(const char* path, maps_txt_data* data);

// Serialize maps data for .wmap save. Caller must free() returned buffer.
uint8_t* serialize_maps_data(maps_txt_data* data, int* out_size);

// Deserialize maps data from .wmap load. Caller must free() returned pointer.
maps_txt_data* deserialize_maps_data(const uint8_t* buf, int size);
