#include "Show_Preview_Window.h"

#include "../App_State.h"
#include "../Image2Texture.h"
#include "../Load_Animation.h"
#include "../Stroke_State.h"
#include "../file_types/File_Type_Registry.h"
#include "../rendering/display_FRM_OpenGL.h"
#include "../worldmap/City_Layer.h"
#include "../worldmap/Maps_Txt.h"
#include "Edit_Image.h"
#include "ImGui_Warning.h"
#include "Preview_Image.h"
#include "Preview_Tiles.h"
#include "Zoom_Pan.h"
#include "imgui.h"

#include <cstring>
#include <string>

// Commit edit surfaces to edit_data and copy to img_data, then save the
// project. Each window now owns its own edit_struct.
static void commit_and_save_edits(LF* F_Prop) {
    commit_map_edits(F_Prop->edit_struct, &F_Prop->edit_data);
    commit_all_overlay_edits(&F_Prop->img_data);
    if ((F_Prop->edit_data.ANM_dir != nullptr) && (F_Prop->img_data.ANM_dir != nullptr)) {
        for (int d = 0; d < 6; d++) {
            int nf = F_Prop->edit_data.ANM_dir[d].num_frames;
            for (int f = 0; f < nf; f++) {
                Surface* src = F_Prop->edit_data.ANM_dir[d].frame_data[f];
                Surface* dst = F_Prop->img_data.ANM_dir[d].frame_data[f];
                if ((src == nullptr) || (dst == nullptr)) {
                    continue;
                }
                memcpy(dst->pxls, src->pxls, static_cast<size_t>(src->w) * src->h);
            }
        }
    }
    const FileTypeEntry* entry = F_Prop->file_type;
    if (entry != nullptr && (entry->flags & FileTypeFlags::HAS_QUICKSAVE)) {
        if (entry->quick_save(F_Prop, &usr_info)) {
            F_Prop->dirty = false;
        }
    }
}

// TODO: store image/editing info in the window itself
void Show_Preview_Window(struct variables* My_Variables, LF* F_Prop, int counter) {
    shader_info* shaders = &My_Variables->shaders;
    image_data* img_data = &F_Prop->img_data;

    // Per-window edit state
    ANM_Dir(&edit_struct)[6] = F_Prop->edit_struct;
    StrokeState& stroke_state = F_Prop->stroke_state;

    std::string a = F_Prop->c_name;
    char b[3];
    sprintf(b, "%02d", counter);
    std::string name = a + "###preview" + b;

    bool was_open = F_Prop->file_open_window;

    if (ImGui::Begin(name.c_str(), (&F_Prop->file_open_window), 0)) {
        // Track focus at window level, not just Worldmap tab
        if (ImGui::IsWindowFocused()) {
            My_Variables->window_number_focus = counter;
            My_Variables->edit_image_focused = F_Prop->editing_enabled;
        }

        bool use_tabs = (F_Prop->wmap != nullptr);
        bool show_map_content = true;

        // Lookup maps data early so it's available for both the trailing button and the Maps tab
        maps_txt_data* maps_data = nullptr;
        if (use_tabs) {
            int maps_idx = find_overlay(F_Prop->img_data.overlay, F_Prop->img_data.overlay_count,
                                        LayerType::MAPS);
            if (maps_idx >= 0) {
                maps_data = (maps_txt_data*)F_Prop->img_data.overlay[maps_idx].source_data;
            }
        }

        // Lookup city layer for the Cities tab
        OverlayLayer* city_layer_tab = nullptr;
        if (use_tabs) {
            int city_idx = find_overlay(F_Prop->img_data.overlay, F_Prop->img_data.overlay_count,
                                        LayerType::CITY);
            if (city_idx >= 0) {
                city_layer_tab = &F_Prop->img_data.overlay[city_idx];
            }
        }

        if (use_tabs) {
            if (ImGui::BeginTabBar("##wmap_tabs")) {
                // Trailing Edit/Done button — styled as plain clickable text with icon
                {
                    bool is_editing = F_Prop->editing_enabled;
                    ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0, 0, 0, 0));
                    ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0, 0, 0, 0));
                    ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0, 0, 0, 0));
                    char label[32];
                    if (is_editing) {
                        snprintf(label, sizeof(label), "\xef\x80\x8c Done%s",
                                 F_Prop->dirty ? " *" : "");
                    } else {
                        snprintf(label, sizeof(label), "\xef\x81\x80 Edit%s",
                                 F_Prop->dirty ? " *" : "");
                    }
                    if (ImGui::TabItemButton(label, ImGuiTabItemFlags_Trailing |
                                                        ImGuiTabItemFlags_NoTooltip)) {
                        F_Prop->wmap_edit_toggled = true;
                    }
                    ImGui::PopStyleColor(3);
                }
                show_map_content = ImGui::BeginTabItem("Worldmap");
            } else {
                show_map_content = false;
            }
        }

        if (show_map_content) {
            const FileTypeEntry* ft_entry = F_Prop->file_type;
            bool has_image = (ft_entry != nullptr && (ft_entry->flags & FileTypeFlags::HAS_IMAGE));
            if (has_image) {
                ImGui::Checkbox("Show Frame Stats", &F_Prop->show_stats);

                ImGui::PushItemWidth(100);
                float* zoom_scale =
                    F_Prop->editing_enabled ? &F_Prop->edit_data.scale : &img_data->scale;
                ImGui::DragFloat("##Zoom", zoom_scale, 0.1F, 0.0F, 10.0F, "Zoom: %%%.2fx", 0);
                ImGui::PopItemWidth();
            }

            // --- Contextual toolbar for this preview window ---
            {
                const FileTypeEntry* entry = F_Prop->file_type;
                if (entry != nullptr && (entry->flags & FileTypeFlags::HAS_TOOLBAR)) {
                    DrawContext ctx = {My_Variables, F_Prop, &usr_info, counter};
                    entry->toolbar(&ctx);
                }
                ImGui::Separator();
            }

            // TODO: show image name for each frame for new animations
            //       this would require attaching the name to each surface
            ImGui::Text("%s", F_Prop->c_name);

            // Show overlay visibility toggles in preview mode (above the clipped map region)
            if (!F_Prop->editing_enabled && img_data->overlay_count > 0 &&
                (F_Prop->wmap != nullptr)) {
                ImGui::Text("Layers");
                for (int oi = 0; oi < img_data->overlay_count; oi++) {
                    OverlayLayer* layer = &img_data->overlay[oi];
                    if (layer->srfc == nullptr) {
                        continue;
                    }
                    ImGui::SameLine();
                    ImGui::PushID(100 + oi);
                    if (ImGui::SmallButton(layer->visible ? "V" : "-")) {
                        layer->visible = !layer->visible;
                        if (layer->visible) {
                            SURFACE_to_texture(layer->srfc, layer->texture, layer->srfc->w,
                                               layer->srfc->h, 1);
                        } else {
                            int w = layer->srfc->w;
                            int h = layer->srfc->h;
                            Surface blank_srfc = {};
                            blank_srfc.pxls = (uint8_t*)calloc(1, static_cast<size_t>(w) * h);
                            blank_srfc.w = static_cast<uint16_t>(w);
                            blank_srfc.h = static_cast<uint16_t>(h);
                            blank_srfc.pitch = w;
                            blank_srfc.channels = 1;
                            SURFACE_to_texture(&blank_srfc, layer->texture, w, h, 1);
                            free(blank_srfc.pxls);
                        }
                    }
                    ImGui::SameLine();
                    ImGui::Text("%s", (layer->name != nullptr) ? layer->name : "Overlay");
                    ImGui::PopID();
                }
            }

            ImVec2 map_child_size = ImGui::GetContentRegionAvail();
            ImGui::BeginChild("##map_viewport", map_child_size, ImGuiChildFlags_None,
                              ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

            if (F_Prop->editing_enabled) {
                // --- Edit mode ---
                image_data* edit_data = &F_Prop->edit_data;

                if (edit_data->ANM_dir == nullptr) {
                    ImGui::Text("No FRM_dir");
                } else if (edit_data->ANM_dir[edit_data->display_orient_num].frame_data ==
                           nullptr) {
                    ImGui::Text("No frame_data");
                } else {
                    // Initialize edit structures on demand
                    if (edit_struct[0].frame_data == nullptr) {
                        init_edit_struct_ANM(edit_struct, edit_data, My_Variables->FO_Palette);
                    }
                    // Initialize overlay edit surfaces on demand
                    for (int oi = 0; oi < F_Prop->img_data.overlay_count; oi++) {
                        if ((F_Prop->img_data.overlay[oi].srfc != nullptr) &&
                            (F_Prop->img_data.overlay[oi].edit_srfc == nullptr)) {
                            init_layer_edit_surface(&F_Prop->img_data.overlay[oi]);
                        }
                    }

                    if (F_Prop->show_stats) {
                        show_image_stats_FRM_SURFACE(&F_Prop->edit_data, My_Variables->Font);
                    }

                    ImVec2 img_pos = display_img_ImGUI(My_Variables, edit_data);

                    bool is_interactive =
                        (F_Prop->active_layer >= 0 &&
                         F_Prop->active_layer < F_Prop->img_data.overlay_count &&
                         F_Prop->img_data.overlay[F_Prop->active_layer].interactive);
                    if (is_interactive) {
                        // Interactive layers don't use Edit_Image, so handle zoom/pan here
                        zoom_pan(edit_data, My_Variables->new_mouse_pos, My_Variables->mouse_delta);
                    }
                    if (is_interactive &&
                        F_Prop->img_data.overlay[F_Prop->active_layer].type == LayerType::CITY) {
                        if (Edit_City_Layer(My_Variables, img_pos, &F_Prop->img_data, edit_data,
                                            F_Prop->active_layer)) {
                            F_Prop->dirty = true;
                        }
                        // Recomposite so overlay texture changes (position, size) show on the map
                        shader_info* shaders = &My_Variables->shaders;
                        draw_PAL_to_framebuffer(
                            shaders->FO_pal, shaders->render_PAL_shader, &shaders->giant_triangle,
                            edit_data, F_Prop->img_data.overlay, F_Prop->img_data.overlay_count);
                    } else {
                        Edit_Image(My_Variables, img_pos, &F_Prop->edit_data, &F_Prop->img_data,
                                   edit_struct, F_Prop->active_layer, My_Variables->Palette_Update,
                                   &My_Variables->Color_Pick, &stroke_state);
                        draw_brush_cursor(&stroke_state);
                        if (!stroke_state.undo_stack.empty()) {
                            F_Prop->dirty = true;
                        }
                    }

                    draw_frame_boundary(edit_data, img_pos, F_Prop->active_layer);
                    if (My_Variables->pixel_perfect) {
                        draw_pixel_grid(edit_data, img_pos, F_Prop->active_layer);
                    }

                    Gui_Video_Controls(&F_Prop->edit_data, F_Prop->edit_data.type);
                }
            } else {
                // --- Preview mode ---
                const FileTypeEntry* entry = F_Prop->file_type;
                if (entry != nullptr && (entry->flags & FileTypeFlags::HAS_PREVIEW)) {
                    DrawContext ctx = {My_Variables, F_Prop, &usr_info, counter};
                    entry->preview(&ctx);
                }
            }

            ImGui::EndChild();

            if (use_tabs) {
                ImGui::EndTabItem();
            }
        } // end show_map_content

        // Handle wmap Edit/Done toggle (runs every frame, regardless of active tab)
        if (F_Prop->wmap != nullptr && F_Prop->wmap_edit_toggled) {
            F_Prop->wmap_edit_toggled = false;
            if (!F_Prop->editing_enabled) {
                // Initialize edit surfaces if needed (same as Export Worldmap path)
                if (F_Prop->edit_data.ANM_dir == nullptr) {
                    prep_image_SURFACE(F_Prop, My_Variables->FO_Palette,
                                       My_Variables->color_match_algo, nullptr, false);
                }
                F_Prop->editing_enabled = true;

                // Auto-create MSK overlay for worldmap projects
                image_data* img = &F_Prop->img_data;
                if (find_overlay(img->overlay, img->overlay_count, LayerType::MSK) < 0) {
                    int idx = add_overlay(img->overlay, &img->overlay_count, LayerType::MSK,
                                          LayerBlend::WHITE_MIX, "Mask", 1.0F, 1.0F, 1.0F, 0.5F);
                    if (idx >= 0) {
                        img->overlay[idx].srfc =
                            Create_8Bit_Surface(img->width, img->height, nullptr);
                        img->overlay[idx].texture =
                            init_texture(img->overlay[idx].srfc, img->overlay[idx].srfc->w,
                                         img->overlay[idx].srfc->h, img_type::MSK);
                    }
                }

                // Auto-switch to city layer if a city is selected
                {
                    int ci = find_overlay(img->overlay, img->overlay_count, LayerType::CITY);
                    if (ci >= 0) {
                        city_layer_data* cd = (city_layer_data*)img->overlay[ci].source_data;
                        if (cd != nullptr && cd->selected_area >= 0) {
                            F_Prop->active_layer = ci;
                        }
                    }
                }
            } else {
                commit_map_edits(edit_struct, &F_Prop->edit_data);
                commit_all_overlay_edits(&F_Prop->img_data);

                // Copy map edits to img_data for preview
                if ((F_Prop->edit_data.ANM_dir != nullptr) &&
                    (F_Prop->img_data.ANM_dir != nullptr)) {
                    for (int d = 0; d < 6; d++) {
                        int nf = F_Prop->edit_data.ANM_dir[d].num_frames;
                        for (int f = 0; f < nf; f++) {
                            Surface* src = F_Prop->edit_data.ANM_dir[d].frame_data[f];
                            Surface* dst = F_Prop->img_data.ANM_dir[d].frame_data[f];
                            if ((src == nullptr) || (dst == nullptr)) {
                                continue;
                            }
                            memcpy(dst->pxls, src->pxls, static_cast<size_t>(src->w) * src->h);
                        }
                    }
                }

                // Sync zoom/pan from edit back to preview
                F_Prop->img_data.scale = F_Prop->edit_data.scale;
                F_Prop->img_data.offset = F_Prop->edit_data.offset;

                F_Prop->editing_enabled = false;
                F_Prop->active_layer = -1;
                My_Variables->edit_image_focused = false;
            }
        }

        if (use_tabs) {
            if (ImGui::BeginTabItem("Cities")) {
                if (draw_cities_info_panel(city_layer_tab, F_Prop->editing_enabled, maps_data)) {
                    F_Prop->dirty = true;
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Maps")) {
                if (draw_maps_info_panel(maps_data, F_Prop->editing_enabled)) {
                    F_Prop->dirty = true;
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    show_popup_warnings();

    // Intercept tab close when there are unsaved edits
    if (was_open && !F_Prop->file_open_window && F_Prop->editing_enabled && F_Prop->dirty) {
        F_Prop->file_open_window = true; // keep window alive
        F_Prop->show_close_confirm = true;
    }

    // Tab close confirmation popup
    char close_popup_id[48];
    snprintf(close_popup_id, sizeof(close_popup_id), "Unsaved Changes##close%02d", counter);

    if (F_Prop->show_close_confirm) {
        ImGui::OpenPopup(close_popup_id);
        F_Prop->show_close_confirm = false;
    }
    if (ImGui::BeginPopupModal(close_popup_id, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("You have unsaved edits.");
        ImGui::Separator();
        if (ImGui::Button("Save & Close")) {
            commit_and_save_edits(F_Prop);
            F_Prop->editing_enabled = false;
            F_Prop->active_layer = -1;
            F_Prop->file_open_window = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Close without saving")) {
            F_Prop->editing_enabled = false;
            F_Prop->active_layer = -1;
            F_Prop->file_open_window = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::End();

    // Commit edits and save before cleanup frees the statics
    if (F_Prop->pending_commit_and_save && !F_Prop->editing_enabled) {
        commit_and_save_edits(F_Prop);
        F_Prop->pending_commit_and_save = false;
    }

    // Cleanup when editing is disabled for this window
    if (!F_Prop->editing_enabled && edit_struct[0].frame_data != nullptr) {
        stroke_state_cleanup(&stroke_state);
        // Cleanup overlay edit surfaces
        for (int oi = 0; oi < F_Prop->img_data.overlay_count; oi++) {
            cleanup_layer_edit_surface(&F_Prop->img_data.overlay[oi]);
        }
        for (int i = 0; i < 6; i++) {
            // Free individual Surface objects before freeing the pointer array
            if (edit_struct[i].frame_data != nullptr) {
                image_data* ed = &F_Prop->edit_data;
                int num_frames = ((ed->ANM_dir != nullptr) && ed->ANM_dir[i].num_frames > 0)
                                     ? ed->ANM_dir[i].num_frames
                                     : 0;
                for (int f = 0; f < num_frames; f++) {
                    if (edit_struct[i].frame_data[f] != nullptr) {
                        FreeSurface(edit_struct[i].frame_data[f]);
                        edit_struct[i].frame_data[f] = nullptr;
                    }
                }
            }
            free(static_cast<void*>(edit_struct[i].frame_data));
            edit_struct[i].frame_data = nullptr;
        }
    }

    // Preview tiles from red boxes
    if (F_Prop->preview_tiles_window) {
        Preview_Tiles_Window(My_Variables, F_Prop, counter);
    }
    // Preview full image
    if (F_Prop->show_image_render) {
        Show_Image_Render(My_Variables, F_Prop, &usr_info, counter);
    }

    if (!F_Prop->file_open_window) {
        if (F_Prop->ssl_text != nullptr) {
            free(F_Prop->ssl_text);
            F_Prop->ssl_text = nullptr;
            F_Prop->ssl_text_len = 0;
        }
        // TODO: free img_data?
    }
}

void Preview_Tiles_Window(variables* My_Variables, LF* F_Prop, int counter) {
    std::string image_name = F_Prop->c_name;
    image_data* edit_data = &F_Prop->edit_data;
    char window_id[3];
    sprintf(window_id, "%02d", counter);
    std::string name = image_name + " Preview...###render" + window_id;

    if (edit_data->type != img_type::TILE) {
        edit_data->type = img_type::TILE;
    }

    // shortcuts
    if (ImGui::Begin(name.c_str(), &F_Prop->preview_tiles_window, 0)) {

        ImGui::PushItemWidth(100);
        ImGui::DragFloat("##Zoom", &edit_data->scale, 0.1F, 0.0F, 10.0F, "Zoom: %%%.2fx", 0);
        ImGui::PopItemWidth();

        if (ImGui::IsWindowFocused()) {
            My_Variables->window_number_focus = counter;
            My_Variables->tile_window_focused = true;
            My_Variables->render_wind_focused = false;
        }

        prev_TMAP_tiles_SURFACE(&usr_info, My_Variables, edit_data);
    }
    ImGui::End();
}

void Show_Image_Render(variables* My_Variables, LF* F_Prop, struct user_info* usr_info,
                       int counter) {
    image_data* edit_data = &F_Prop->edit_data;
    char b[3];
    sprintf(b, "%02d", counter);
    std::string a = F_Prop->c_name;
    std::string name = a + "Render Window...###render" + b;

    if (ImGui::Begin(name.c_str(), &F_Prop->show_image_render, 0)) {
        if (ImGui::IsWindowFocused()) {
            My_Variables->window_number_focus = counter;
            My_Variables->render_wind_focused = true;
            My_Variables->tile_window_focused = false;
        }
        ImGui::PushItemWidth(100);
        ImGui::DragFloat("##Zoom", &edit_data->scale, 0.1F, 0.0F, 10.0F, "Zoom: %%%.2fx", 0);
        ImGui::PopItemWidth();
        ImGui::Checkbox("Show Frame Stats", &F_Prop->show_stats);

        preview_FRM_SURFACE(My_Variables, edit_data,
                            (F_Prop->show_stats || usr_info->show_image_stats));

        Gui_Video_Controls(edit_data, edit_data->type);
    }
    ImGui::End();
}
