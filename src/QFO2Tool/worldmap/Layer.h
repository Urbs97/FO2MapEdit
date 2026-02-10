#pragma once

#include "../formats/MiniSDL.h"

#include <cstdint>
#include <glad/glad.h>

constexpr int MAX_OVERLAY_LAYERS = 8;

enum class LayerType : int8_t {
    NONE = -1,
    MSK = 0,       // walk mask (1-bit, paint-based, WHITE_MIX)
    CITY = 1,      // city placement (structured, interactive, COLOR_MIX green)
    TERRAIN = 2,   // biome types (multi-valued, paint-based, COLOR_MIX per-value)
    ENCOUNTER = 3, // encounter zones (multi-valued, paint-based, COLOR_MIX per-value)
    MAPS = 4,      // MAPS.TXT data (source_data only, no visual surface)
};

enum class LayerBlend : int8_t {
    WHITE_MIX = 0,  // mix(color, white, 0.5) -- current MSK behavior
    COLOR_MIX = 1,  // mix(color, layer_color, alpha) -- single tint color
    RGBA_BLEND = 2, // alpha-blend pre-rendered RGBA overlay -- for multi-valued layers
};

struct OverlayLayer {
    LayerType type = LayerType::NONE;
    LayerBlend blend = LayerBlend::WHITE_MIX;
    bool visible = true;
    bool editable = true;       // can user paint on this layer?
    bool interactive = false;   // does this layer use structured editing (city)?
    const char* name = nullptr; // "Mask", "City", "Terrain", "Encounters"

    // Pixel surface for shader compositing (same dimensions as map)
    Surface* srfc = nullptr;
    GLuint texture = 0;

    // Type-specific source data (e.g., city_layer_data* for CITY)
    void* source_data = nullptr;
    int source_data_size = 0;

    // Fixed blend color for this layer type (RGBA)
    float color[4] = {1.0F, 1.0F, 1.0F, 0.5F};

    // Edit working buffer (allocated when editing is enabled)
    Surface* edit_srfc = nullptr;
};

// Helper to find a layer by type. Returns index or -1 if not found.
int find_overlay(OverlayLayer* overlays, int count, LayerType type);

// Add a new overlay layer. Returns the index, or -1 if full.
int add_overlay(OverlayLayer* overlays, int* count, LayerType type, LayerBlend blend,
                const char* name, float r, float g, float b, float a);

// Initialize a layer's edit surface (copy from committed surface).
void init_layer_edit_surface(OverlayLayer* layer);

// Commit edits: copy edit_srfc back to srfc.
void commit_layer_edits(OverlayLayer* layer);

// Free the edit surface.
void cleanup_layer_edit_surface(OverlayLayer* layer);

// Free all resources for a single overlay layer.
void clear_overlay(OverlayLayer* layer);

// Free all overlay layers in the array.
void clear_all_overlays(OverlayLayer* overlays, int* count);
