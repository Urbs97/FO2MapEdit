// If you feel like dropping a dollar on me :)
// https://www.subscribestar.com/quantumapprentice

#include "Edit_Image.h"

#include "Load_Files.h"
#include "Zoom_Pan.h"
#include "display_FRM_OpenGL.h"
#include "imgui_internal.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

void surface_paint(variables* My_Variables, Surface* edit_srfc, float x, float y);

// displays edit_data->render_texture in window
// size and position stored in edit_data
// uv_min/uv_max/tint all stored in My_Variables
// TODO: check this against image_render()
// returns total position of image in desktop
ImVec2 display_img_ImGUI(variables* My_Variables, image_data* edit_data) {
    ImVec2 uv_min = My_Variables->uv_min; // (0.0f,0.0f)
    ImVec2 uv_max = My_Variables->uv_max; // (1.0f,1.0f)
    ImVec4 tint = My_Variables->tint_col;
    // shortcuts
    int dir = edit_data->display_orient_num;
    float scale = edit_data->scale;
    int width = edit_data->ANM_bounding_box[dir].x2 - edit_data->ANM_bounding_box[dir].x1;
    int height = edit_data->ANM_bounding_box[dir].y2 - edit_data->ANM_bounding_box[dir].y1;
    ImVec2 size = ImVec2((float)(width * scale), (float)(height * scale));

    ImVec2 img_pos = top_corner(edit_data->offset);

    // image I'm trying to pan with
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    window->DrawList->AddImage((ImTextureID)(uintptr_t)edit_data->render_texture, img_pos,
                               bottom_corner(size, img_pos), uv_min, uv_max,
                               ImGui::GetColorU32(tint));

    return img_pos;
}

// helper: recomposite the edit image after undo/cancel
static void recomposite(shader_info* shaders, image_data* edit_data, ANM_Dir* edit_struct,
                        Surface* edit_srfc, GLuint texture, int dir, int num, uint64_t time_ms) {
    SURFACE_to_texture(edit_srfc, texture, edit_srfc->w, edit_srfc->h, 1);
    if (edit_data->ANM_dir[dir].frame_data) {
        animate_SURFACE_to_sub_texture(edit_data, edit_struct[dir].frame_data[num], time_ms);
    }
    if (edit_data->MSK_srfc) {
        draw_PAL_to_framebuffer(shaders->FO_pal, shaders->render_PAL_shader,
                                &shaders->giant_triangle, edit_data);
    } else {
        draw_texture_to_framebuffer(shaders->FO_pal, shaders->render_FRM_shader,
                                    &shaders->giant_triangle, edit_data->framebuffer,
                                    edit_data->FRM_texture, edit_data->width, edit_data->height);
    }
}

// TODO: maybe pass the dithering choice through?
void Edit_Image(variables* My_Variables, ImVec2 img_pos, image_data* edit_data,
                ANM_Dir* edit_struct, Surface* edit_MSK_srfc, bool edit_MSK, bool Palette_Update,
                uint8_t* Color_Pick, StrokeState* stroke_state) {
    shader_info* shaders = &My_Variables->shaders;

    // handle zoom and panning for the image
    // skip right-click panning during active stroke (right-click cancels instead)
    if (!stroke_state->stroke_active) {
        zoom_pan(edit_data, My_Variables->new_mouse_pos, My_Variables->mouse_delta);
    } else {
        // still allow scroll-wheel zoom during stroke
        float mouse_wheel = ImGui::GetIO().MouseWheel;
        if (mouse_wheel > 0 && ImGui::GetIO().KeyCtrl && ImGui::IsWindowHovered()) {
            zoom(1.05, My_Variables->new_mouse_pos, edit_data);
        } else if (mouse_wheel < 0 && ImGui::GetIO().KeyCtrl && ImGui::IsWindowHovered()) {
            zoom(0.95, My_Variables->new_mouse_pos, edit_data);
        }
        ImVec2 size =
            ImVec2(edit_data->width * edit_data->scale, edit_data->height * edit_data->scale);
        viewport_boundary(edit_data, size);
    }

    // handle frame display by orientation and number
    int num = edit_data->display_frame_num;
    int dir = edit_data->display_orient_num;
    ANM_Dir* anm_dir = edit_data->ANM_dir;

    // TODO: maybe set display_frame_num to 0 for every edit image?
    if (num > anm_dir[dir].num_frames) {
        num = anm_dir[dir].num_frames - 1;
    }

    Surface* edit_srfc;
    if (edit_MSK == false) {
        edit_srfc = edit_struct[dir].frame_data[num];
    } else {
        edit_srfc = edit_MSK_srfc;
    }

    if (!edit_data->ANM_dir) {
        ImGui::Text("No ANM_dir");
        return;
    }
    if (edit_data->ANM_dir[dir].frame_data == NULL && !edit_data->MSK_srfc) {
        ImGui::Text("No frame_data");
        return;
    }

    // Determine which surface and texture we're editing
    Surface* srfc_ptr = edit_srfc;
    GLuint texture = edit_data->FRM_texture;
    if (edit_MSK) {
        srfc_ptr = edit_MSK_srfc;
        texture = edit_data->MSK_texture;
    }

    // --- Compute brush cursor position (always when window is hovered) ---
    stroke_state->cursor_visible = false;

    // Coordinate calculation (shared by cursor preview and painting)
    ImVec2 mouse_pos = My_Variables->new_mouse_pos;
    int x_offset;
    int y_offset;
    if (edit_MSK) {
        x_offset = 0;
        y_offset = 0;
    } else {
        x_offset = edit_data->ANM_dir[dir].frame_box[num].x1 - edit_data->ANM_bounding_box[dir].x1;
        y_offset = edit_data->ANM_dir[dir].frame_box[num].y1 - edit_data->ANM_bounding_box[dir].y1;
    }

    float scale = edit_data->scale;
    ImVec2 img_offset = {
        (mouse_pos.x - img_pos.x) / scale,
        (mouse_pos.y - img_pos.y) / scale,
    };

    ImVec2 sub_image_offset = {
        (img_offset.x - x_offset),
        (img_offset.y - y_offset),
    };

    float x, y;
    if (edit_MSK) {
        x = img_offset.x;
        y = img_offset.y;
    } else {
        x = sub_image_offset.x;
        y = sub_image_offset.y;
    }

    if (My_Variables->pixel_perfect) {
        x = floorf(x);
        y = floorf(y);
    }

    bool cursor_in_bounds = (0 <= x && x < edit_srfc->w) && (0 <= y && y < edit_srfc->h);

    // Update brush cursor when hovering over the image
    if (ImGui::IsWindowHovered() && cursor_in_bounds) {
        float brush_w = My_Variables->pixel_perfect ? 1.0f : My_Variables->brush_size.x;
        float brush_h = My_Variables->pixel_perfect ? 1.0f : My_Variables->brush_size.y;

        // Clamp brush size to surface
        if (brush_w > edit_srfc->w)
            brush_w = edit_srfc->w;
        if (brush_h > edit_srfc->h)
            brush_h = edit_srfc->h;

        float brush_x0, brush_y0;
        if (My_Variables->pixel_perfect) {
            // Pixel perfect: snapped coordinate is the top-left of the pixel cell
            brush_x0 = x;
            brush_y0 = y;
        } else {
            // Normal mode: center brush on cursor with edge clamping
            float bx = x;
            float by = y;
            if ((bx + brush_w / 2) > edit_srfc->w)
                bx = edit_srfc->w - brush_w / 2;
            if ((bx - brush_w / 2) < 0)
                bx = brush_w / 2;
            if ((by + brush_h / 2) > edit_srfc->h)
                by = edit_srfc->h - brush_h / 2;
            if ((by - brush_h / 2) < 0)
                by = brush_h / 2;
            brush_x0 = bx - brush_w / 2;
            brush_y0 = by - brush_h / 2;
        }

        // Convert from image space back to screen space
        // image_x = (screen_x - img_pos.x)/scale - x_offset  (for non-MSK)
        // screen_x = (image_x + x_offset) * scale + img_pos.x
        float off_x = edit_MSK ? 0 : (float)x_offset;
        float off_y = edit_MSK ? 0 : (float)y_offset;

        stroke_state->cursor_min.x = (brush_x0 + off_x) * scale + img_pos.x;
        stroke_state->cursor_min.y = (brush_y0 + off_y) * scale + img_pos.y;
        stroke_state->cursor_max.x = (brush_x0 + brush_w + off_x) * scale + img_pos.x;
        stroke_state->cursor_max.y = (brush_y0 + brush_h + off_y) * scale + img_pos.y;
        stroke_state->cursor_visible = true;
    }

    // --- Handle stroke cancel (right-click or Escape during active stroke) ---
    if (stroke_state->stroke_active) {
        bool cancel = false;
        if (ImGui::GetIO().MouseClicked[1]) {
            cancel = true;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            cancel = true;
        }
        if (cancel) {
            stroke_cancel(stroke_state);
            recomposite(shaders, edit_data, edit_struct, srfc_ptr, texture, dir, num,
                        My_Variables->CurrentTime_ms);
            return;
        }
    }

    // --- Handle Ctrl+Z undo or menu undo (only when no active stroke) ---
    bool undo_trigger =
        ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z) && ImGui::IsWindowFocused();
    if (My_Variables->undo_requested) {
        undo_trigger = true;
        My_Variables->undo_requested = false;
    }
    if (!stroke_state->stroke_active && undo_trigger) {
        if (stroke_undo(stroke_state)) {
            recomposite(shaders, edit_data, edit_struct, srfc_ptr, texture, dir, num,
                        My_Variables->CurrentTime_ms);
            return;
        }
    }

    // --- Handle Ctrl+Y redo or menu redo (only when no active stroke) ---
    bool redo_trigger =
        ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y) && ImGui::IsWindowFocused();
    if (My_Variables->redo_requested) {
        redo_trigger = true;
        My_Variables->redo_requested = false;
    }
    if (!stroke_state->stroke_active && redo_trigger) {
        if (stroke_redo(stroke_state)) {
            recomposite(shaders, edit_data, edit_struct, srfc_ptr, texture, dir, num,
                        My_Variables->CurrentTime_ms);
            return;
        }
    }

    // --- Stroke lifecycle + painting ---
    bool image_edited = false;

    // On mouse button press: begin stroke
    if (ImGui::GetIO().MouseClicked[0] && ImGui::IsWindowHovered() && cursor_in_bounds) {
        stroke_begin(stroke_state, srfc_ptr);
    }

    // While mouse held and stroke is active: paint
    if (ImGui::GetIO().MouseDown[0] && ImGui::IsWindowHovered() && stroke_state->stroke_active) {
        if (cursor_in_bounds) {
            image_edited = true;
            surface_paint(My_Variables, srfc_ptr, x, y);
            SURFACE_to_texture(srfc_ptr, texture, srfc_ptr->w, srfc_ptr->h, 1);
        }
    }

    // On mouse release: commit stroke
    if (ImGui::GetIO().MouseReleased[0] && stroke_state->stroke_active) {
        stroke_commit(stroke_state);
    }

    // Converts unpalettized image to texture for display
    if (Palette_Update || image_edited) {
        if (edit_data->ANM_dir[dir].frame_data) {
            animate_SURFACE_to_sub_texture(edit_data, edit_struct[dir].frame_data[num],
                                           My_Variables->CurrentTime_ms);
        }

        if (edit_data->MSK_srfc) {
            draw_PAL_to_framebuffer(shaders->FO_pal, shaders->render_PAL_shader,
                                    &shaders->giant_triangle, edit_data);
        } else {
            draw_texture_to_framebuffer(shaders->FO_pal, shaders->render_FRM_shader,
                                        &shaders->giant_triangle, edit_data->framebuffer,
                                        edit_data->FRM_texture, edit_data->width,
                                        edit_data->height);
        }
    }
}

void draw_brush_cursor(StrokeState* stroke_state) {
    if (!stroke_state->cursor_visible) {
        return;
    }
    ImDrawList* draw_list = ImGui::GetForegroundDrawList();
    draw_list->AddRect(stroke_state->cursor_min, stroke_state->cursor_max, IM_COL32(0, 0, 0, 255),
                       0.0f, 0, 2.0f);
    draw_list->AddRect(stroke_state->cursor_min, stroke_state->cursor_max,
                       IM_COL32(255, 255, 255, 255), 0.0f, 0, 1.0f);
}

// paint surfaces for both MSK and FRM items (possibly also PAL? or other 32bit
// surfaces?)
// TODO: repack all the x&y variables into vectors of appropriate type
// (int/float)
void surface_paint(variables* My_Variables, Surface* dst, float x, float y) {
    int color_pick = My_Variables->Color_Pick;
    float brush_w = My_Variables->pixel_perfect ? 1.0f : My_Variables->brush_size.x;
    float brush_h = My_Variables->pixel_perfect ? 1.0f : My_Variables->brush_size.y;
    int w = dst->w;
    int h = dst->h;

    if (My_Variables->pixel_perfect) {
        // Pixel perfect: snapped coordinate is the top-left of the pixel cell
        x = floorf(x);
        y = floorf(y);
        if (x < 0)
            x = 0;
        if (y < 0)
            y = 0;
        if (x >= w)
            x = w - 1;
        if (y >= h)
            y = h - 1;
    } else {
        // clamp brush size to within surface
        if (brush_w > w) {
            brush_w = w;
        }
        if (brush_h > h) {
            brush_h = h;
        }
        // clamp brush position to edge
        if ((x + brush_w / 2) > w) {
            x = w - brush_w / 2;
        }
        if ((x - brush_w / 2) < 0) {
            x = brush_w / 2;
        }
        if ((y + brush_h / 2) > h) {
            y = h - brush_h / 2;
        }
        if ((y - brush_h / 2) < 0) {
            y = brush_h / 2;
        }

        // further clamp the brush to prevent overflow
        // TODO: this is a lazy implementation that doesn't allow
        //       the brush to shrink in size as it goes over the edge
        //       or rather, to clip the brush so it doesn't paint off the edge
        x -= brush_w / 2;
        y -= brush_h / 2;
    }

    Rect dst_rect = {(int)x, (int)y, (int)brush_w, (int)brush_h};
    PaintSurface(dst, dst_rect, color_pick);
}

void brush_size_handler(variables* My_Variables) {
    ImGui::Checkbox("Pixel Perfect", &My_Variables->pixel_perfect);

    ImGui::BeginDisabled(My_Variables->pixel_perfect);
    ImGui::DragFloat("###width", &My_Variables->brush_size.x, 1.0f, 1.0f, FLT_MAX,
                     "Brush Width: %.0f pixels");
    ImGui::SameLine();
    ImGui::Checkbox("Link", &My_Variables->link_brush_sizes);

    if (My_Variables->link_brush_sizes) {
        My_Variables->brush_size.y = My_Variables->brush_size.x;
    }
    ImGui::DragFloat("###height", &My_Variables->brush_size.y, 1.0f, 1.0f, FLT_MAX,
                     "Brush Height: %.0f pixels");
    ImGui::EndDisabled();
}

void draw_frame_boundary(image_data* edit_data, ImVec2 img_pos, bool edit_MSK) {
    // Editing MSK layer: covers full canvas, nothing to dim
    if (edit_MSK)
        return;
    // MSK files opened directly: frame == canvas
    if (edit_data->type == MSK)
        return;

    int dir = edit_data->display_orient_num;
    int num = edit_data->display_frame_num;
    ANM_Dir* anm_dir = edit_data->ANM_dir;
    if (!anm_dir)
        return;
    if (!anm_dir[dir].frame_box)
        return;
    if (num < 0 || num >= anm_dir[dir].num_frames)
        return;

    rectangle* frame_box = anm_dir[dir].frame_box;
    rectangle* bbox = &edit_data->ANM_bounding_box[dir];

    int fx = frame_box[num].x1 - bbox->x1;
    int fy = frame_box[num].y1 - bbox->y1;
    int fw = frame_box[num].x2 - frame_box[num].x1;
    int fh = frame_box[num].y2 - frame_box[num].y1;
    int cw = bbox->x2 - bbox->x1;
    int ch = bbox->y2 - bbox->y1;

    // Frame fills entire bounding box — nothing to dim
    if (fx == 0 && fy == 0 && fw == cw && fh == ch)
        return;

    float scale = edit_data->scale;
    // Frame rect in screen space
    ImVec2 f_min = {img_pos.x + fx * scale, img_pos.y + fy * scale};
    ImVec2 f_max = {img_pos.x + (fx + fw) * scale, img_pos.y + (fy + fh) * scale};
    // Canvas rect in screen space
    ImVec2 c_min = img_pos;
    ImVec2 c_max = {img_pos.x + cw * scale, img_pos.y + ch * scale};

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImU32 dim_col = IM_COL32(0, 0, 0, 100);

    // Top strip
    draw_list->AddRectFilled({c_min.x, c_min.y}, {c_max.x, f_min.y}, dim_col);
    // Bottom strip
    draw_list->AddRectFilled({c_min.x, f_max.y}, {c_max.x, c_max.y}, dim_col);
    // Left strip (between top and bottom)
    draw_list->AddRectFilled({c_min.x, f_min.y}, {f_min.x, f_max.y}, dim_col);
    // Right strip (between top and bottom)
    draw_list->AddRectFilled({f_max.x, f_min.y}, {c_max.x, f_max.y}, dim_col);

    // Outline: black 2px outer, white 1px inner
    draw_list->AddRect(f_min, f_max, IM_COL32(0, 0, 0, 255), 0.0f, 0, 2.0f);
    draw_list->AddRect(f_min, f_max, IM_COL32(255, 255, 255, 255), 0.0f, 0, 1.0f);
}

void draw_pixel_grid(image_data* edit_data, ImVec2 img_pos, bool edit_MSK) {
    float scale = edit_data->scale;
    if (scale < 4.0f)
        return;

    int dir = edit_data->display_orient_num;
    int img_w, img_h;
    if (edit_MSK || edit_data->type == MSK) {
        img_w = edit_data->width;
        img_h = edit_data->height;
    } else {
        img_w = edit_data->ANM_bounding_box[dir].x2 - edit_data->ANM_bounding_box[dir].x1;
        img_h = edit_data->ANM_bounding_box[dir].y2 - edit_data->ANM_bounding_box[dir].y1;
    }

    // Image rect in screen space
    ImVec2 img_min = img_pos;
    ImVec2 img_max = {img_pos.x + img_w * scale, img_pos.y + img_h * scale};

    // Clip to visible window region
    ImVec2 win_min = ImGui::GetWindowPos();
    ImVec2 win_size = ImGui::GetWindowSize();
    ImVec2 win_max = {win_min.x + win_size.x, win_min.y + win_size.y};

    ImVec2 vis_min = {fmaxf(img_min.x, win_min.x), fmaxf(img_min.y, win_min.y)};
    ImVec2 vis_max = {fminf(img_max.x, win_max.x), fminf(img_max.y, win_max.y)};

    if (vis_min.x >= vis_max.x || vis_min.y >= vis_max.y)
        return;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->PushClipRect(vis_min, vis_max, true);

    ImU32 grid_col = IM_COL32(128, 128, 128, 60);

    // First/last visible pixel indices
    int first_col = (int)floorf((vis_min.x - img_pos.x) / scale);
    int last_col = (int)ceilf((vis_max.x - img_pos.x) / scale);
    int first_row = (int)floorf((vis_min.y - img_pos.y) / scale);
    int last_row = (int)ceilf((vis_max.y - img_pos.y) / scale);

    if (first_col < 0)
        first_col = 0;
    if (last_col > img_w)
        last_col = img_w;
    if (first_row < 0)
        first_row = 0;
    if (last_row > img_h)
        last_row = img_h;

    // Vertical lines (pixel column boundaries)
    for (int col = first_col; col <= last_col; col++) {
        float sx = img_pos.x + col * scale;
        draw_list->AddLine({sx, vis_min.y}, {sx, vis_max.y}, grid_col);
    }

    // Horizontal lines (pixel row boundaries)
    for (int row = first_row; row <= last_row; row++) {
        float sy = img_pos.y + row * scale;
        draw_list->AddLine({vis_min.x, sy}, {vis_max.x, sy}, grid_col);
    }

    draw_list->PopClipRect();
}
