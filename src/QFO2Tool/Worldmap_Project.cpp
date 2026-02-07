#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "Worldmap_Project.h"
#include "Image2Texture.h"
#include "display_FRM_OpenGL.h"
#include "ImGui_Warning.h"


bool write_wmap_file(const char* base_path, const char* save_name,
                     int tiles_x, int tiles_y, bool has_msk)
{
    char wmap_path[MAX_PATH];
    snprintf(wmap_path, MAX_PATH, "%s/%s.wmap", base_path, save_name);

    FILE* file = fopen(wmap_path, "w");
    if (!file) {
        printf("Error: write_wmap_file(), unable to open %s for writing: L%d\n", wmap_path, __LINE__);
        return false;
    }

    fprintf(file, "; QFO2Tool Worldmap Project\n");
    fprintf(file, "version=1\n");
    fprintf(file, "base_name=%s\n", save_name);
    fprintf(file, "tiles_x=%d\n", tiles_x);
    fprintf(file, "tiles_y=%d\n", tiles_y);
    fprintf(file, "has_msk=%d\n", has_msk ? 1 : 0);

    fclose(file);
    return true;
}


bool parse_wmap_file(const char* wmap_path, wmap_info* info)
{
    FILE* file = fopen(wmap_path, "r");
    if (!file) {
        set_popup_warning(
            "[ERROR] parse_wmap_file()\n\n"
            "Unable to open .wmap file."
        );
        printf("Error: parse_wmap_file(), Can't open file: %s : L%d\n", wmap_path, __LINE__);
        return false;
    }

    memset(info, 0, sizeof(wmap_info));

    // extract directory from wmap_path
    strncpy(info->directory, wmap_path, MAX_PATH);
    char* last_slash = strrchr(info->directory, '/');
#ifdef QFO2_WINDOWS
    char* last_bslash = strrchr(info->directory, '\\');
    if (last_bslash > last_slash) last_slash = last_bslash;
#endif
    if (last_slash) {
        *last_slash = '\0';
    }

    char line[MAX_PATH];
    while (fgets(line, sizeof(line), file))
    {
        // skip comments and blank lines
        if (line[0] == ';' || line[0] == '\n' || line[0] == '\r') {
            continue;
        }

        // strip trailing newline
        char* nl = strchr(line, '\n');
        if (nl) *nl = '\0';
        nl = strchr(line, '\r');
        if (nl) *nl = '\0';

        char* eq = strchr(line, '=');
        if (!eq) continue;

        *eq = '\0';
        char* key = line;
        char* val = eq + 1;

        if (strcmp(key, "version") == 0) {
            info->version = atoi(val);
        }
        else if (strcmp(key, "base_name") == 0) {
            strncpy(info->base_name, val, sizeof(info->base_name) - 1);
            info->base_name[sizeof(info->base_name) - 1] = '\0';
        }
        else if (strcmp(key, "tiles_x") == 0) {
            info->tiles_x = atoi(val);
        }
        else if (strcmp(key, "tiles_y") == 0) {
            info->tiles_y = atoi(val);
        }
        else if (strcmp(key, "has_msk") == 0) {
            info->has_msk = atoi(val) != 0;
        }
    }

    fclose(file);

    if (info->tiles_x < 1 || info->tiles_y < 1) {
        set_popup_warning(
            "[ERROR] parse_wmap_file()\n\n"
            "Invalid tile dimensions in .wmap file."
        );
        printf("Error: parse_wmap_file(), invalid tiles_x=%d tiles_y=%d : L%d\n",
               info->tiles_x, info->tiles_y, __LINE__);
        return false;
    }

    return true;
}


Surface* load_stitch_FRM_tiles(wmap_info* info, Palette* pal)
{
    int full_w = info->tiles_x * WMAP_TILE_W;
    int full_h = info->tiles_y * WMAP_TILE_H;

    Surface* full = Create_8Bit_Surface(full_w, full_h, pal);
    if (!full) {
        set_popup_warning(
            "[ERROR] load_stitch_FRM_tiles()\n\n"
            "Unable to allocate surface for stitched worldmap."
        );
        printf("Error: load_stitch_FRM_tiles(), alloc failed for %dx%d : L%d\n",
               full_w, full_h, __LINE__);
        return NULL;
    }

    int header_skip = sizeof(FRM_Header) + sizeof(FRM_Frame);

    int tile_num = 0;
    for (int y = 0; y < info->tiles_y; y++)
    {
        for (int x = 0; x < info->tiles_x; x++)
        {
            char tile_path[MAX_PATH];
            snprintf(tile_path, MAX_PATH, "%s/%s%02d.FRM",
                     info->directory, info->base_name, tile_num);

            int file_size = 0;
            uint8_t* buffer = load_entire_file(tile_path, &file_size);
            if (!buffer) {
                // skip missing tiles (surface is zero-filled)
                tile_num++;
                continue;
            }

            if (file_size < header_skip) {
                free(buffer);
                tile_num++;
                continue;
            }

            uint8_t* tile_pxls = buffer + header_skip;
            int dst_offset = (y * full_w * WMAP_TILE_H) + (x * WMAP_TILE_W);

            for (int row = 0; row < WMAP_TILE_H; row++) {
                memcpy(&full->pxls[dst_offset + row * full_w],
                       &tile_pxls[row * WMAP_TILE_W], WMAP_TILE_W);
            }

            free(buffer);
            tile_num++;
        }
    }

    return full;
}


Surface* load_stitch_MSK_tiles(wmap_info* info)
{
    int full_w = info->tiles_x * WMAP_TILE_W;
    int full_h = info->tiles_y * WMAP_TILE_H;

    Surface* full = Create_8Bit_Surface(full_w, full_h, nullptr);
    if (!full) {
        set_popup_warning(
            "[ERROR] load_stitch_MSK_tiles()\n\n"
            "Unable to allocate surface for stitched MSK."
        );
        printf("Error: load_stitch_MSK_tiles(), alloc failed for %dx%d : L%d\n",
               full_w, full_h, __LINE__);
        return NULL;
    }

    int tile_num = 0;
    for (int y = 0; y < info->tiles_y; y++)
    {
        for (int x = 0; x < info->tiles_x; x++)
        {
            char tile_path[MAX_PATH];
            snprintf(tile_path, MAX_PATH, "%s/%s%02d.MSK",
                     info->directory, info->base_name, tile_num);

            int msk_w = WMAP_TILE_W;
            int msk_h = WMAP_TILE_H;
            int buffsize = (msk_w + 7) / 8 * msk_h;

            FILE* fp = fopen(tile_path, "rb");
            if (!fp) {
                // skip missing tiles
                tile_num++;
                continue;
            }

            uint8_t* msk_buffer = (uint8_t*)malloc(buffsize);
            if (!msk_buffer) {
                fclose(fp);
                tile_num++;
                continue;
            }
            fread(msk_buffer, buffsize, 1, fp);
            fclose(fp);

            // unpack bits to 8-bit pixels (same logic as MSK_Convert.cpp)
            uint8_t tile_pxls[WMAP_TILE_W * WMAP_TILE_H] = {};
            uint8_t bitmask  = 128;
            uint8_t* bin_ptr = msk_buffer;

            for (int pxl_y = 0; pxl_y < msk_h; pxl_y++) {
                for (int pxl_x = 0; pxl_x < msk_w; pxl_x++) {
                    uint8_t buff = *bin_ptr;
                    bool mask_1_or_0 = (buff & bitmask);
                    if (mask_1_or_0) {
                        tile_pxls[pxl_y * msk_w + pxl_x] = 1;
                    }
                    bitmask >>= 1;

                    if (bitmask == 0) {
                        ++bin_ptr;
                        bitmask = 128;
                    }
                }
                if (bitmask < 128) {
                    ++bin_ptr;
                    bitmask = 128;
                }
            }
            free(msk_buffer);

            // copy into full surface
            int dst_offset = (y * full_w * WMAP_TILE_H) + (x * WMAP_TILE_W);
            for (int row = 0; row < WMAP_TILE_H; row++) {
                memcpy(&full->pxls[dst_offset + row * full_w],
                       &tile_pxls[row * WMAP_TILE_W], WMAP_TILE_W);
            }

            tile_num++;
        }
    }

    return full;
}


bool load_wmap_project(const char* wmap_path, LF* F_Prop,
                       image_data* img_data, shader_info* shaders)
{
    wmap_info info;
    if (!parse_wmap_file(wmap_path, &info)) {
        return false;
    }

    // stitch FRM tiles
    Surface* stitched = load_stitch_FRM_tiles(&info, shaders->FO_pal);
    if (!stitched) {
        return false;
    }

    // allocate ANM_dir[6]
    img_data->ANM_dir = (ANM_Dir*)malloc(sizeof(ANM_Dir) * 6);
    if (!img_data->ANM_dir) {
        set_popup_warning(
            "[ERROR] load_wmap_project()\n\n"
            "Unable to allocate memory for ANM_dir."
        );
        printf("Error: load_wmap_project(), ANM_dir alloc failed : L%d\n", __LINE__);
        FreeSurface(stitched);
        return false;
    }
    new (img_data->ANM_dir) ANM_Dir[6];

    // allocate frame_data[0]
    img_data->ANM_dir[0].frame_data = (Surface**)malloc(sizeof(Surface*));
    if (!img_data->ANM_dir[0].frame_data) {
        set_popup_warning(
            "[ERROR] load_wmap_project()\n\n"
            "Unable to allocate memory for frame_data."
        );
        printf("Error: load_wmap_project(), frame_data alloc failed : L%d\n", __LINE__);
        FreeSurface(stitched);
        free(img_data->ANM_dir);
        img_data->ANM_dir = NULL;
        return false;
    }

    img_data->ANM_dir[0].frame_data[0] = stitched;
    img_data->ANM_dir[0].num_frames    = 1;
    img_data->width  = stitched->w;
    img_data->height = stitched->h;
    img_data->type   = FRM;
    img_data->display_orient_num = NE;
    img_data->display_frame_num  = 0;

    // allocate frame_box
    img_data->ANM_dir[0].frame_box = (rectangle*)calloc(1, sizeof(rectangle));
    if (!img_data->ANM_dir[0].frame_box) {
        set_popup_warning(
            "[ERROR] load_wmap_project()\n\n"
            "Unable to allocate memory for frame_box."
        );
        printf("Error: load_wmap_project(), frame_box alloc failed : L%d\n", __LINE__);
        FreeSurface(stitched);
        free(img_data->ANM_dir[0].frame_data);
        free(img_data->ANM_dir);
        img_data->ANM_dir = NULL;
        return false;
    }

    // set bounding boxes so preview_FRM_SURFACE / animate_SURFACE_to_sub_texture work
    img_data->ANM_dir[0].frame_box[0] = {0, 0, stitched->w, stitched->h};
    img_data->ANM_bounding_box[0]     = {0, 0, stitched->w, stitched->h};

    // create OpenGL texture + framebuffer
    img_data->FRM_texture = init_texture(
        stitched, stitched->w, stitched->h, FRM);

    framebuffer_init(
        &img_data->render_texture,
        &img_data->framebuffer,
        stitched->w,
        stitched->h);

    SURFACE_to_texture(
        stitched,
        img_data->FRM_texture,
        stitched->w, stitched->h, 1);

    draw_texture_to_framebuffer(
        shaders->FO_pal,
        shaders->render_FRM_shader,
        &shaders->giant_triangle,
        img_data->framebuffer,
        img_data->FRM_texture,
        stitched->w, stitched->h);

    // synthetic FRM_Header + FRM_Frame buffer so copy_it_all_ANM() works in edit mode
    int synth_size = sizeof(FRM_Header) + sizeof(FRM_Frame) + (stitched->w * stitched->h);
    uint8_t* synth_buf = (uint8_t*)calloc(1, synth_size);
    if (synth_buf) {
        FRM_Header* hdr    = (FRM_Header*)synth_buf;
        hdr->version           = 4;
        hdr->Frames_Per_Orient = 1;
        hdr->Frame_Area        = sizeof(FRM_Frame) + (stitched->w * stitched->h);

        FRM_Frame* frame   = (FRM_Frame*)(synth_buf + sizeof(FRM_Header));
        frame->Frame_Width  = stitched->w;
        frame->Frame_Height = stitched->h;
        frame->Frame_Size   = stitched->w * stitched->h;

        img_data->FRM_data = synth_buf;
        img_data->FRM_hdr  = hdr;
        img_data->FRM_size = synth_size;
    }

    // load MSK tiles if present
    if (info.has_msk) {
        Surface* msk_full = load_stitch_MSK_tiles(&info);
        if (msk_full) {
            img_data->MSK_srfc = msk_full;
            img_data->MSK_texture = init_texture(
                msk_full, msk_full->w, msk_full->h, MSK);
        }
    }

    F_Prop->palettized       = true;
    F_Prop->image_is_tileable = true;
    F_Prop->file_open_window = true;

    return true;
}
