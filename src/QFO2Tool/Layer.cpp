#include "Layer.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

int find_overlay(OverlayLayer* overlays, int count, LayerType type) {
    for (int i = 0; i < count; i++) {
        if (overlays[i].type == type) {
            return i;
        }
    }
    return -1;
}

int add_overlay(OverlayLayer* overlays, int* count, LayerType type, LayerBlend blend,
                const char* name, float r, float g, float b, float a) {
    if (*count >= MAX_OVERLAY_LAYERS) {
        printf("[ERROR] add_overlay(): overlay array full (%d/%d)\n", *count, MAX_OVERLAY_LAYERS);
        return -1;
    }
    int idx = *count;
    OverlayLayer* layer = &overlays[idx];
    *layer = OverlayLayer{};
    layer->type = type;
    layer->blend = blend;
    layer->name = name;
    layer->color[0] = r;
    layer->color[1] = g;
    layer->color[2] = b;
    layer->color[3] = a;
    (*count)++;
    return idx;
}

void init_layer_edit_surface(OverlayLayer* layer) {
    if (layer == nullptr || layer->srfc == nullptr) {
        return;
    }
    int w = layer->srfc->w;
    int h = layer->srfc->h;

    if (layer->edit_srfc == nullptr) {
        layer->edit_srfc = (Surface*)calloc(1, sizeof(Surface));
        if (layer->edit_srfc == nullptr) {
            printf("[ERROR] init_layer_edit_surface(): alloc failed\n");
            return;
        }
        layer->edit_srfc->pxls = (uint8_t*)calloc(1, static_cast<size_t>(w) * h);
        if (layer->edit_srfc->pxls == nullptr) {
            printf("[ERROR] init_layer_edit_surface(): pixel alloc failed\n");
            free(layer->edit_srfc);
            layer->edit_srfc = nullptr;
            return;
        }
        layer->edit_srfc->w = static_cast<uint16_t>(w);
        layer->edit_srfc->h = static_cast<uint16_t>(h);
        layer->edit_srfc->pitch = w;
        layer->edit_srfc->channels = 1;
    }

    memcpy(layer->edit_srfc->pxls, layer->srfc->pxls, static_cast<size_t>(w) * h);
}

void commit_layer_edits(OverlayLayer* layer) {
    if (layer == nullptr) {
        return;
    }
    if (layer->edit_srfc == nullptr || layer->edit_srfc->pxls == nullptr) {
        return;
    }
    if (layer->srfc == nullptr) {
        return;
    }
    memcpy(layer->srfc->pxls, layer->edit_srfc->pxls,
           static_cast<size_t>(layer->edit_srfc->w) * layer->edit_srfc->h);
}

void cleanup_layer_edit_surface(OverlayLayer* layer) {
    if (layer == nullptr) {
        return;
    }
    if (layer->edit_srfc != nullptr) {
        free(layer->edit_srfc->pxls);
        layer->edit_srfc->pxls = nullptr;
        free(layer->edit_srfc);
        layer->edit_srfc = nullptr;
    }
}

void clear_overlay(OverlayLayer* layer) {
    if (layer == nullptr) {
        return;
    }
    cleanup_layer_edit_surface(layer);
    if (layer->source_data != nullptr) {
        free(layer->source_data);
        layer->source_data = nullptr;
        layer->source_data_size = 0;
    }
    if (layer->srfc != nullptr) {
        FreeSurface(layer->srfc);
        layer->srfc = nullptr;
    }
    if (layer->texture != 0) {
        glDeleteTextures(1, &layer->texture);
        layer->texture = 0;
    }
    *layer = OverlayLayer{};
}

void clear_all_overlays(OverlayLayer* overlays, int* count) {
    for (int i = 0; i < *count; i++) {
        clear_overlay(&overlays[i]);
    }
    *count = 0;
}
